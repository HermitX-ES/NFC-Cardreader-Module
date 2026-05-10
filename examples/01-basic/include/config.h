#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ===================== PN532 (SPI) =====================
#define PN532_SCK      14
#define PN532_MOSI     13
#define PN532_MISO     12
#define PN532_SS       15
#define PN532_IRQ      21
#define PN532_RESET     3

// ===================== SD (SPI) =====================
#define SD_CS           5
#define SPI_MOSI       17
#define SPI_MISO        8
#define SPI_SCK        18

// ===================== I2S Audio =====================
#define I2S_DOUT       40
#define I2S_BCLK       41
#define I2S_LRC        42

// ===================== LED Strip =====================
#define LED_PIN        16
#define NUM_LEDS        8
#define LED_TYPE    NEOPIXEL
#define MIN_BRIGHTNESS  2
#define MAX_BRIGHTNESS 65

// ===================== Reader state =====================
typedef enum {
    STATE_IDLE,
    STATE_CARD_DETECTED,
    STATE_SUCCESS,
    STATE_ERROR
} ReaderState;

#endif // CONFIG_H
