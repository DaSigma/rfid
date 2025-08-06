# RFID Access Control System

This project is an Arduino-based RFID access control system using an RC522 RFID reader, a WS2812 LED for status indication, and a 12V magnetic lock for securing a door.

## Features

*   **RFID Access:** Grants or denies access based on RFID cards.
*   **Master Card:** A designated master card is used to add or remove other cards.
*   **LED Status Indicator:** A WS2812 LED provides visual feedback for different system states (e.g., access granted, access denied, ready).
*   **Magnetic Lock Control:** Controls a 12V magnetic lock via a relay module.
*   **EEPROM Storage:** Stores authorized card UIDs in the Arduino's EEPROM, so they are not lost when the power is turned off.
*   **Buzzer Feedback:** Provides audible feedback for various actions.

## Hardware Requirements

*   Arduino Uno WiFi Rev 2 (or compatible board)
*   MFRC522 RFID Reader
*   WS2812 LED (NeoPixel)
*   Buzzer (2-pin)
*   5V Relay Module
*   12V Magnetic Lock
*   12V Power Supply
*   Jumper Wires

## Wiring

| Component           | Pin/Connection              | Arduino Pin/Connection      |
| ------------------- | --------------------------- | --------------------------- |
| **RC522 RFID Reader** |                             |                             |
|                     | SDA                         | Pin 8                       |
|                     | SCK                         | ICSP Header Pin 3 (SCK)     |
|                     | MOSI                        | ICSP Header Pin 4 (MOSI)    |
|                     | MISO                        | ICSP Header Pin 1 (MISO)    |
|                     | RST                         | Pin 9                       |
|                     | 3.3V                        | 3.3V                        |
|                     | GND                         | GND                         |
| **WS2812 LED**      |                             |                             |
|                     | VCC                         | 5V                          |
|                     | GND                         | GND                         |
|                     | DIN                         | Pin 6                       |
| **Buzzer**          |                             |                             |
|                     | Positive (+)                | Pin 7                       |
|                     | Negative (-)                | GND                         |
| **Relay Module**    |                             |                             |
|                     | VCC                         | 5V                          |
|                     | GND                         | GND                         |
|                     | IN                          | Pin 5                       |
|                     | COM                         | 12V Power Supply Positive   |
|                     | NO                          | Magnetic Lock Positive      |
| **12V Power Supply**|                             |                             |
|                     | Positive                    | Relay COM Terminal          |
|                     | Negative                    | Magnetic Lock Negative & Arduino GND |

## How to Use

1.  **Master Card:** The system has a pre-defined master card (UID: `93 52 EA 2F`).
2.  **Adding Cards:**
    *   Scan the master card once. The LED will turn orange, indicating the system is in "add mode".
    *   Scan a new card to add it to the system. The LED will flash green.
3.  **Removing Cards:**
    *   Scan the master card twice in quick succession. The LED will flash red, indicating the system is in "removal mode".
    *   Scan a card to remove it from the system.
4.  **Access:**
    *   Scan an authorized card to toggle the magnetic lock. The LED will turn green when the lock is released and blue when it is engaged.

## Required Libraries

*   [SPI](https://www.arduino.cc/en/reference/spi)
*   [MFRC522](https://github.com/miguelbalboa/rfid)
*   [Adafruit NeoPixel](https://github.com/adafruit/Adafruit_NeoPixel)
*   [EEPROM](https://www.arduino.cc/en/reference/eeprom)
