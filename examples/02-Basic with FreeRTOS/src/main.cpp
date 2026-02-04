/**************************************************************************/
/*!
    @file     main.cpp
    @author   Ivan Hermida - HermitX SLU
    @brief    ESP32-S3 + PN532 (SPI) card reader using FreeRTOS tasks.
              - No Wi-Fi / OTA
              - LED chaser idle
              - Card detected -> yellow ramp 2s
              - Play /success.mp3 + green LEDs
              - If no SD (or audio can't start) -> returns to idle automatically
*/
/**************************************************************************/

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <FastLED.h>
#include <Adafruit_PN532.h>
#include <Audio.h>
#include "config.h"

// ======================= GLOBAL OBJECTS ==========================
CRGB leds[NUM_LEDS];
Audio audio;

// SD uses its own SPI pins (from config.h)
SPIClass &SPI_SD = SPI;

// PN532 uses a separate SPI bus to avoid conflicts with SD/audio.
SPIClass SPI_NFC(HSPI);

// PN532 (SPI) on SPI_NFC
Adafruit_PN532 nfc(PN532_SS, &SPI_NFC);

// ======================= STATE ==========================
static volatile ReaderState state = STATE_IDLE;

// UID buffer
static uint8_t uid[7];
static uint8_t uidLength = 0;

// SD status
static bool sdOk = false;

// Audio 
const int targetVol = 21;

// Effect parameters
static constexpr uint16_t CHASER_INTERVAL_MS = 120;
static const CRGB CHASER_COLOR = CRGB::Blue;
static constexpr uint8_t CHASER_DIM = 22;

static int8_t chaserIndex = 0;
static int8_t chaserDir = 1;

// ======================= RTOS HANDLES ==========================
static TaskHandle_t taskLedHandle = nullptr;
static TaskHandle_t taskNfcHandle = nullptr;
static TaskHandle_t taskAudioHandle = nullptr;

// Task notifications (bit flags)
static constexpr uint32_t EVT_CARD_DETECTED = (1u << 0);
static constexpr uint32_t EVT_PLAY_SUCCESS = (1u << 1);
static constexpr uint32_t EVT_AUDIO_DONE = (1u << 2);

// For timing inside LED task
static TickType_t stateStartTick = 0;

// ======================= HELPERS ==========================
static inline TickType_t msToTicks(uint32_t ms) { return pdMS_TO_TICKS(ms); }

static void setAllLeds(const CRGB &c)
{
    fill_solid(leds, NUM_LEDS, c);
    FastLED.show();
}

static void changeState(ReaderState newState)
{
    state = newState;
    stateStartTick = xTaskGetTickCount();
}

static void resetToIdle()
{
    FastLED.setBrightness(MAX_BRIGHTNESS);
    setAllLeds(CRGB::Black);
    chaserIndex = 0;
    chaserDir = 1;
    changeState(STATE_IDLE);
}

static void errorState()
{
    FastLED.setBrightness(MAX_BRIGHTNESS);
    setAllLeds(CRGB::Red);
    changeState(STATE_ERROR);
}

static void transitionToSuccess()
{
    // Show green immediately
    FastLED.setBrightness(MAX_BRIGHTNESS);
    setAllLeds(CRGB::Green);

    // If no SD, do not block in SUCCESS waiting for audio that will never run
    if (!sdOk)
    {
        vTaskDelay(msToTicks(250)); // small visual confirmation
        resetToIdle();
        return;
    }

    // Ask audio task to start playback; LED task will wait for EVT_AUDIO_DONE
    xTaskNotify(taskAudioHandle, EVT_PLAY_SUCCESS, eSetBits);
    changeState(STATE_SUCCESS);
}

// ======================= LED EFFECTS (non-blocking) ==========================
static void runChaserStep()
{
    static TickType_t lastStepTick = 0;
    TickType_t now = xTaskGetTickCount();

    if ((now - lastStepTick) >= msToTicks(CHASER_INTERVAL_MS))
    {
        CRGB dimColor = CHASER_COLOR;
        dimColor.nscale8(CHASER_DIM);
        fill_solid(leds, NUM_LEDS, dimColor);

        leds[chaserIndex] = CHASER_COLOR;
        FastLED.show();

        chaserIndex += chaserDir;
        if (chaserIndex >= (NUM_LEDS - 1) || chaserIndex <= 0)
        {
            chaserDir = -chaserDir;
        }
        lastStepTick = now;
    }
}

static void runYellowRamp()
{
    const uint32_t RAMP_MS = 2000;

    TickType_t now = xTaskGetTickCount();
    uint32_t elapsedMs = (uint32_t)((now - stateStartTick) * portTICK_PERIOD_MS);

    if (elapsedMs <= RAMP_MS)
    {
        uint8_t b = map(elapsedMs, 0, RAMP_MS, MIN_BRIGHTNESS, MAX_BRIGHTNESS);
        FastLED.setBrightness(b);
        fill_solid(leds, NUM_LEDS, CRGB::Yellow);
        FastLED.show();
    }
    else
    {
        FastLED.setBrightness(MAX_BRIGHTNESS);
        transitionToSuccess();
    }
}

// ======================= TASKS ==========================
static void TaskLEDState(void *param)
{
    (void)param;
    changeState(STATE_IDLE);

    for (;;)
    {
        // Consume notifications (non-blocking)
        uint32_t notif = 0;
        xTaskNotifyWait(0, UINT32_MAX, &notif, 0);

        if (notif & EVT_CARD_DETECTED)
        {
            changeState(STATE_CARD_DETECTED);
        }
        if (notif & EVT_AUDIO_DONE)
        {
            resetToIdle();
        }

        // State machine
        switch (state)
        {
        case STATE_IDLE:
            runChaserStep();
            break;

        case STATE_CARD_DETECTED:
            runYellowRamp();
            break;

        case STATE_SUCCESS:
            // Wait for EVT_AUDIO_DONE (or auto return if audio couldn't start)
            break;

        case STATE_ERROR:
            break;
        }

        vTaskDelay(msToTicks(10)); // 100 Hz
    }
}

static void TaskNFC(void *param)
{
    (void)param;

    for (;;)
    {
        if (state == STATE_IDLE)
        {
            uint8_t success = nfc.readPassiveTargetID(
                PN532_MIFARE_ISO14443A, uid, &uidLength, 50);

            if (success)
            {
                Serial.print(F("[CARD] UID length="));
                Serial.println(uidLength);

                xTaskNotify(taskLedHandle, EVT_CARD_DETECTED, eSetBits);
                vTaskDelay(msToTicks(300));
            }
            else
            {
                vTaskDelay(msToTicks(40));
            }
        }
        else
        {
            vTaskDelay(msToTicks(80));
        }
    }
}

static void TaskAudio(void *param)
{
    (void)param;

    bool wasRunning = false;

    for (;;)
    {
        if (audio.isRunning())
        {
            audio.loop();
        }

        // Handle play requests
        uint32_t notif = 0;
        xTaskNotifyWait(0, UINT32_MAX, &notif, 0);

        if (notif & EVT_PLAY_SUCCESS)
        {

            audio.setVolume(0);
            vTaskDelay(pdMS_TO_TICKS(10));

            audio.connecttoFS(SD, "/success.mp3");

            vTaskDelay(pdMS_TO_TICKS(20));

            if (!audio.isRunning())
            {
                audio.setVolume(targetVol);
                xTaskNotify(taskLedHandle, EVT_AUDIO_DONE, eSetBits);
            }
            else
            {
                for (int v = 0; v <= targetVol; v++)
                {
                    audio.setVolume(v);
                    vTaskDelay(pdMS_TO_TICKS(5)); 
                }
            }
        }

        // Detect end of playback
        bool running = audio.isRunning();
        if (wasRunning && !running)
        {
            xTaskNotify(taskLedHandle, EVT_AUDIO_DONE, eSetBits);
        }
        wasRunning = running;

        vTaskDelay(msToTicks(2));
    }
}

// ======================= SETUP/LOOP ==========================
void setup()
{
    Serial.begin(115200);

    // ----- LEDs -----
    FastLED.addLeds<LED_TYPE, LED_PIN>(leds, NUM_LEDS);
    FastLED.setBrightness(MAX_BRIGHTNESS);
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    FastLED.show();

    // ----- SD -----
    SPI_SD.begin(SPI_SCK, SPI_MISO, SPI_MOSI, SD_CS);
    sdOk = SD.begin(SD_CS, SPI_SD, 25000000); // 25 MHz
    if (!sdOk)
    {
        Serial.println(F("[SD] Card mount failed"));
    }
    else
    {
        Serial.println(F("[SD] OK"));
    }

    // ----- Audio (I2S) -----
    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    audio.setVolume(targetVol);

    // ----- PN532 (separate SPI bus) -----
    SPI_NFC.begin(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);
    nfc.begin();

    uint32_t version = nfc.getFirmwareVersion();
    if (!version)
    {
        Serial.println(F("[PN532] Not found!"));
        errorState();
    }
    else
    {
        Serial.print(F("[PN532] Found PN5"));
        Serial.println((version >> 24) & 0xFF, HEX);
    }
    nfc.SAMConfig();

    // ----- Create tasks -----
    xTaskCreatePinnedToCore(TaskLEDState, "LEDState", 4096, nullptr, 2, &taskLedHandle, 0);
    xTaskCreatePinnedToCore(TaskNFC, "NFC", 4096, nullptr, 2, &taskNfcHandle, 0);
    xTaskCreatePinnedToCore(TaskAudio, "Audio", 8192, nullptr, 5, &taskAudioHandle, 1);
}

void loop()
{
    vTaskDelay(pdMS_TO_TICKS(1000));
}
