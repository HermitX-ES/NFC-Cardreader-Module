#ifndef CONFIG_H
#define CONFIG_H

// Hardware pin definitions for PN532 NFC reader
#define PN532_SCK      14  // SPI clock pin
#define PN532_MOSI     13  // SPI MOSI pin
#define PN532_MISO     12  // SPI MISO pin
#define PN532_SS       15  // SPI chip select pin
#define PN532_IRQ      21  // Interrupt pin
#define PN532_RESET     3  // Reset pin

// Audio and SD card interface pins
#define SD_CS           5  // SD card chip select
#define SPI_MOSI       17  // SPI MOSI for SD card
#define SPI_MISO        8  // SPI MISO for SD card
#define SPI_SCK        18  // SPI clock for SD card

// I2S audio output pins
#define I2S_DOUT       40  // Data output
#define I2S_BCLK       41  // Bit clock
#define I2S_LRC        42  // Left/right clock

// LED strip configuration
#define LED_PIN        16  // Data pin for WS2812B LEDs
#define NUM_LEDS        8  // Total number of LEDs
#define LED_TYPE    NEOPIXEL
#define MIN_BRIGHTNESS  2  // Minimum brightness (0-255)
#define MAX_BRIGHTNESS 65  // Maximum brightness (0-255)

// Reader state enumeration
typedef enum {
    STATE_IDLE,
    STATE_CARD_DETECTED,
    STATE_SUCCESS,
    STATE_ERROR
} ReaderState;

// Core function declarations
void handleCardInterrupt();
void startListeningForCards();
void handleCardDetection();

void changeState(ReaderState newState);
void resetToIdle();
void transitionToSuccess();
void errorState();

// Audio callback functions
void audio_info(const char *info);
void audio_id3data(const char *info);
void audio_eof_mp3(const char *info);

// State handler functions
void idleStateHandler();
void cardDetectedHandler();
void successStateHandler();
void errorStateHandler();

#endif // CONFIG_H
