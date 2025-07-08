/**************************************************************************/
/*!
    @file     main.cpp
    @author   Ivan Hermida - HermitX SLU
    @brief    Stand-alone ESP32-S3 + PN532 (SPI) card reader.
              • No Wi-Fi / OTA
              • Bidirectional "chaser" effect with 8 LEDs: active LED in blue, inactive LEDs in dim blue
              • Detects card by polling (SPI does not support IRQ)
              • On detection → yellow LEDs with 2 s brightness ramp
              • Then plays /success.mp3 and shows green LEDs
              • After audio finishes → returns to chaser effect
*/
/**************************************************************************/

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <FastLED.h>
#include <Adafruit_PN532.h>
#include <Audio.h>
#include "config.h"   // Pin definitions and constants (see your .h)

// ---------- Global objects ----------
CRGB leds[NUM_LEDS];
Audio audio;
Adafruit_PN532 nfc(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS); // ► SPI ◄

// ---------- Global state ----------
static ReaderState state = STATE_IDLE;   // enum defined in config.h
static unsigned long stateStart = 0;

// UID buffer
static uint8_t uid[7];
static uint8_t uidLength;

// ---------- Effect parameters ----------
const uint16_t CHASER_INTERVAL = 120;   // ms between steps
const CRGB     CHASER_COLOR    = CRGB::Blue;
const uint8_t  CHASER_DIM      = 22;    // 0-255 brightness for inactive LEDs
static int8_t  chaserIndex     = 0;
static int8_t  chaserDir       = 1;     // 1 → forward, -1 → backward

// ---------- Forward declarations ----------
void runChaserEffect();
void changeState(ReaderState newState);
void resetToIdle();
void transitionToSuccess();
void errorState();
void cardPolling();
void yellowRamp();

// ---------- Audio callbacks (optional) ----------
void audio_info(const char *info) {}
void audio_id3data(const char *info) {}
void audio_eof_mp3(const char *info) {}

// ===================================================================
void setup() {
  Serial.begin(115200);

  // --- LED Strip setup
  FastLED.addLeds<LED_TYPE, LED_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness(MAX_BRIGHTNESS);
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();

  // --- SD card setup
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, SD_CS);
  if (!SD.begin(SD_CS)) {
    Serial.println(F("[SD] Card mount failed"));
  }

  // --- Audio (I2S) setup
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(15);                // 0-21

  // --- PN532 (SPI) setup
  nfc.begin();
  uint32_t version = nfc.getFirmwareVersion();
  if (!version) {
    Serial.println(F("[PN532] Not found!"));
    errorState();
  } else {
    Serial.print(F("[PN532] Found PN5"));
    Serial.println((version >> 24) & 0xFF, HEX);
  }
  nfc.SAMConfig();

  changeState(STATE_IDLE);
}

// ===================================================================
void loop() {
  // Keep audio running if active
  if (audio.isRunning()) audio.loop();

  switch (state) {
    case STATE_IDLE:
      runChaserEffect();
      cardPolling();
      break;

    case STATE_CARD_DETECTED:
      yellowRamp();  // blocking 2 s ramp, then transitions to SUCCESS
      break;

    case STATE_SUCCESS:
      if (!audio.isRunning()) resetToIdle();
      break;

    case STATE_ERROR:
      // optional red blinking
      break;
  }
}

// ===================================================================
// ------------------   STATE FUNCTIONS   ----------------------------

void changeState(ReaderState newState) {
  state      = newState;
  stateStart = millis();
}

void resetToIdle() {
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.setBrightness(MAX_BRIGHTNESS);
  FastLED.show();
  chaserIndex = 0;
  chaserDir   = 1;
  changeState(STATE_IDLE);
}

void transitionToSuccess() {
  fill_solid(leds, NUM_LEDS, CRGB::Green);
  FastLED.show();
  audio.connecttoFS(SD, "/success.mp3");
  changeState(STATE_SUCCESS);
}

void errorState() {
  fill_solid(leds, NUM_LEDS, CRGB::Red);
  FastLED.show();
  changeState(STATE_ERROR);
}

// ===================================================================
// ------------------   LED EFFECTS   -----------------------------

void runChaserEffect() {
  static unsigned long lastStep = 0;
  if (millis() - lastStep >= CHASER_INTERVAL) {
    // Dimmed color for inactive LEDs
    CRGB dimColor = CHASER_COLOR;
    dimColor.nscale8(CHASER_DIM);      // adjust brightness
    fill_solid(leds, NUM_LEDS, dimColor);

    // Active LED at full brightness
    leds[chaserIndex] = CHASER_COLOR;
    FastLED.show();

    // Advance index and bounce at ends
    chaserIndex += chaserDir;
    if (chaserIndex >= NUM_LEDS - 1 || chaserIndex <= 0) {
      chaserDir = -chaserDir;
    }
    lastStep = millis();
  }
}

void yellowRamp() {
  const uint16_t RAMP_MS = 2000;
  uint32_t t = millis() - stateStart;
  if (t <= RAMP_MS) {
    uint8_t b = map(t, 0, RAMP_MS, MIN_BRIGHTNESS, MAX_BRIGHTNESS);
    FastLED.setBrightness(b);
    fill_solid(leds, NUM_LEDS, CRGB::Yellow);
    FastLED.show();
  } else {
    FastLED.setBrightness(MAX_BRIGHTNESS);
    transitionToSuccess();
  }
}

// ===================================================================
// ------------------   NFC / CARDS   -----------------------------

void cardPolling() {
  uint8_t success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 50); // timeout 50 ms
  if (success) {
    Serial.print(F("[CARD] UID length=")); Serial.println(uidLength);
    changeState(STATE_CARD_DETECTED);
  }
}
