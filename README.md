# NFC Card Reader Module Examples

This repository hosts a collection of examples demonstrating how to use the **NFC Card Reader Module**. Whether you’re building an interactive installation, an access control system, or a smart business card, you’ll find ready-to-use sketches and detailed guides to get you started.

---

## 📦 Module Overview

The **NFC + Audio + LEDs + SD Card** development module combines:

* **ESP32‑S3** microcontroller (Wi‑Fi & BLE)
* **PN5321** NFC reader (ISO/IEC 14443 A/B, peer-to-peer, emulation)
* **MAX98357** I²S amplifier (3 W audio output)
* **8 × WS2812B** addressable RGB LEDs
* **MicroSD card slot** (up to 32 GB)

<img src="https://directus.hermitx.es/assets/6fc54b4f-3617-4675-8660-0a960521d1fc.png?width=800&height=600" alt="Module Dimensions" />

*Size and mounting holes are shown above.*

Find the latest price and order here: [Buy on Lectronz — €44.95](https://lectronz.com/products/nfc-card-reader-module)

---

## 🌟 Key Features

* **ESP32‑S3 SoC**: Real-time audio decoding, LED effects, NFC, and SD access.
* **NFC Reader (PN5321)**: Supports MIFARE Classic/Ultralight/DESFire, FeliCa, NTAG215, P2P, and card emulation.
* **Audio Output**: I²S amplifier to drive a 3 W speaker.
* **LED Effects**: Per-pixel RGB control for animations and status indicators.
* **Local Storage**: MicroSD slot for audio files, logs, and configurations.
* **Modular Design**: Detachable NFC antenna board and included NTAG215 card.

---

## 📋 Specifications

| Component     | Description                                |
| ------------- | ------------------------------------------ |
| **MCU**       | ESP32‑S3 (Wi‑Fi & BLE)                     |
| **NFC**       | PN5321 (ISO/IEC 14443 A/B, P2P, Emulation) |
| **Amplifier** | MAX98357 (I²S → 3 W speaker)               |
| **LEDs**      | 8× WS2812B addressable RGB LEDs            |
| **SD Slot**   | MicroSD (up to 32 GB)                      |
| **Included**  | NFC antenna board, speaker, NTAG215 card   |

---

## 📡 Pinout & Connections

### PN5321 (SPI)

| Signal | ESP32‑S3 GPIO |
| :----- | :------------ |
| MISO   | IO12          |
| MOSI   | IO13          |
| SCK    | IO14          |
| SS     | IO15          |
| IRQ    | IO21          |

### MAX98357 (I²S)

| Signal | ESP32‑S3 GPIO |
| :----- | :------------ |
| DOUT   | IO40          |
| BCLK   | IO41          |
| LRC    | IO42          |

### MicroSD Card (SPI)

| Signal | ESP32‑S3 GPIO |
| :----- | :------------ |
| MOSI   | IO17          |
| MISO   | IO8           |
| CLK    | IO18          |
| CS     | IO5           |

### WS2812B LEDs

| Signal | ESP32‑S3 GPIO |
| :----- | :------------ |
| Data   | IO16          |

---

## 🚀 Quick Start

1. **Power** the module via USB-C or 5 V on the onboard connector.
2. **Insert** a MicroSD card with a `success.mp3` file at the root.
3. **Upload** the `01-basic_card_reader` example from this repo.
4. **Tap** an NFC tag → observe the LED chase effect, yellow brightness ramp, and green success pattern with audio playback.

---

## 🛠️ Code Examples

| Example               | Description                              |
| --------------------- | ---------------------------------------- |
| **01-basic\_setup**   | Initialize NFC, LEDs, audio, and SD card |



---

## 🎨 Project Ideas

* **Interactive Museum Guide**: Tap tags to play exhibit narrations and light sequences.
* **Smart Business Card**: Play a company jingle and cycle brand-colored LEDs when tapped.
* **Access Control**: Green LED + unlock on authorized cards; red flash + alert on unauthorized.
* **Educational Kit**: Teach programming with NFC puzzles and LED quizzes.
* **Sound Art Installation**: Synchronize ambient sounds and lights across multiple modules.


