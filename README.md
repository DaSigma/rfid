# RFID Access Control System with Magnetic Lock

A comprehensive Arduino-based access control system using RC522 RFID reader, WS2812 LED indicator, buzzer feedback, and 12V magnetic lock control. The system supports multiple authorized cards with persistent EEPROM storage and includes master card functionality for system management.

## 🚀 Features

- **RFID Card Authentication**: Support for up to 20 authorized cards
- **12V Magnetic Lock Control**: Toggle lock state with authorized cards
- **Visual LED Feedback**: WS2812 RGB LED with color-coded status indicators
- **Audio Feedback**: Buzzer with distinct sounds for different actions
- **Persistent Storage**: Cards saved to EEPROM (survives power cycles)
- **Master Card Management**: Add/remove cards using master card
- **Card Removal Mode**: Double-tap master card to remove cards from system
- **Fail-Safe Design**: Lock releases on power loss for safety

## 📋 Hardware Requirements

### Core Components
- Arduino Uno WiFi Rev 2 (or compatible)
- RC522 RFID Reader Module
- WS2812 LED Strip (minimum 1 LED)
- 5V Relay Module (capable of switching 12V/2A+)
- 12V Magnetic Lock
- 12V/2A+ Power Supply
- 2-pin Buzzer (active)
- RFID Cards/Tags
- Jumper wires and breadboard

### Power Requirements
- **Arduino**: 5V (USB or external)
- **12V Power Supply**: 2A+ rating for magnetic lock
- **Common Ground**: Essential between Arduino and 12V supply

## 🔌 Wiring Diagram

### RC522 RFID Reader
| RC522 Pin | Arduino Pin | Description |
|-----------|-------------|-------------|
| SDA       | 8           | Slave Select |
| SCK       | ICSP-3      | Serial Clock |
| MOSI      | ICSP-4      | Master Out Slave In |
| MISO      | ICSP-1      | Master In Slave Out |
| IRQ       | -           | Not connected |
| GND       | GND         | Ground |
| RST       | 9           | Reset |
| 3.3V      | 3.3V        | Power |

### WS2812 LED Strip
| LED Pin | Arduino Pin | Description |
|---------|-------------|-------------|
| VCC     | 5V          | Power |
| GND     | GND         | Ground |
| DIN     | 6           | Data Input |

### Buzzer
| Buzzer Pin | Arduino Pin | Description |
|------------|-------------|-------------|
| Positive   | 7           | Signal |
| Negative   | GND         | Ground |

### Relay Module (12V Mag Lock Control)
| Relay Pin | Connection | Description |
|-----------|------------|-------------|
| VCC       | 5V         | Power |
| GND       | GND        | Ground |
| IN        | 5          | Control Signal |
| COM       | 12V+       | Common (12V Power Supply +) |
| NO        | Lock+      | Normally Open (Mag Lock +) |

### Power Supply Connections
| Component | Connection | Notes |
|-----------|------------|--------|
| 12V Supply + | Relay COM | Powers magnetic lock |
| 12V Supply - | Lock - AND Arduino GND | **Critical: Common ground required** |
| Arduino | USB/5V Jack | Separate 5V supply |

### ICSP Header Pinout (Arduino Uno WiFi Rev 2)
```
1: MISO    2: VCC
3: SCK     4: MOSI  
5: RESET   6: GND
```

## 💾 Software Setup

### Required Libraries
Install these libraries through Arduino IDE Library Manager:

```cpp
#include <SPI.h>          // Built-in (Arduino SPI)
#include <MFRC522.h>      // RC522 RFID Library
#include <Adafruit_NeoPixel.h>  // WS2812 LED Control
#include <EEPROM.h>       // Built-in (Arduino EEPROM)
```

### Library Installation
1. Open Arduino IDE
2. Go to **Sketch → Include Library → Manage Libraries**
3. Search and install:
   - "MFRC522" by GithubCommunity
   - "Adafruit NeoPixel" by Adafruit

### Master Card Configuration
Update the master card UID in the code:

```cpp
// Replace with your master card UID
byte masterCard[4] = {0x93, 0x52, 0xEA, 0x2F};
```

To find your card's UID:
1. Upload the code
2. Open Serial Monitor (9600 baud)
3. Scan any card - the UID will be displayed

## 🎯 System Operation

### LED Status Indicators
| Color | Status | Description |
|-------|--------|-------------|
| 🔵 Blue | System Ready | Lock engaged, waiting for card |
| 🟢 Green | Unlocked | Magnetic lock released |
| 🔴 Red | Access Denied | Unknown card or removal mode |
| 🟠 Orange | Master Card | Master card detected |
| ⚫ Off | Flashing | Card removal mode active |

### Audio Feedback
| Beep Pattern | Action | Description |
|--------------|--------|-------------|
| 2 Short Ascending | System Startup | System initialized |
| 2 Ascending Tones | Access Granted | Authorized card accepted |
| 3 Low Beeps | Access Denied | Unknown card rejected |
| 3 Ascending Tones | Master Card | Master card recognized |
| 4 Ascending Tones | Card Added | New card added to system |
| 4 Descending Tones | Card Removed | Card deleted from system |
| Alternating Pattern | Removal Mode | Card removal mode activated |
| 2 Ascending | Lock Released | Magnetic lock unlocked |
| 2 Descending | Lock Engaged | Magnetic lock locked |

### Card Management

#### Adding New Cards
1. Scan **master card** → System grants 5-second access window
2. Scan **new card** during window → Card added with confirmation
3. System returns to normal operation

#### Removing Cards (Double-Tap Method)
1. Scan **master card** twice quickly (within 3 seconds)
2. LED flashes red → Removal mode active (5 seconds)
3. Scan **card to remove** → Card deleted with confirmation
4. System automatically returns to normal operation

#### Lock Control (Authorized Users)
1. Scan **authorized card** → Lock state toggles immediately
2. **If locked** → Becomes unlocked (green LED)
3. **If unlocked** → Becomes locked (blue LED)
4. Lock maintains state until next authorized scan

## ⚙️ Configuration Options

### Adjustable Parameters
```cpp
#define MAX_CARDS       20      // Maximum stored cards (1-20)
#define ACCESS_TIMEOUT  5000    // Master card access window (ms)
#define LED_TIMEOUT     3000    // Status LED display duration (ms)
#define NUM_LEDS        1       // Number of WS2812 LEDs
```

### Pin Assignments (Customizable)
```cpp
#define RST_PIN         9       // RC522 Reset pin
#define SS_PIN          8       // RC522 Slave Select pin
#define LED_PIN         6       // WS2812 LED data pin
#define BUZZER_PIN      7       // Buzzer control pin
#define RELAY_PIN       5       // Relay control pin
```

## 🔧 Troubleshooting

### Common Issues

#### RFID Not Reading Cards
- **Check wiring**: Verify SPI connections to ICSP header
- **Power supply**: Ensure 3.3V connection to RC522
- **Card distance**: Hold card 1-3cm from reader
- **Serial output**: Check for "Firmware Version" message on startup

#### LED Not Working
- **Wiring**: Verify VCC (5V), GND, and data pin connections
- **Power**: WS2812 requires 5V, not 3.3V
- **Library**: Ensure Adafruit_NeoPixel is installed
- **LED count**: Verify `NUM_LEDS` matches your strip

#### Magnetic Lock Issues
- **No lock control**: Check relay wiring and 12V power supply
- **Lock doesn't engage**: Verify relay switches 12V properly
- **Intermittent operation**: Ensure common ground between Arduino and 12V supply
- **Lock stays on**: Check relay normally-open (NO) connection

#### Cards Not Saving
- **EEPROM corruption**: Clear EEPROM and restart system
- **Power loss**: Cards save immediately when added/removed
- **Max cards reached**: System holds maximum 20 cards

### Serial Monitor Debug
Enable Serial Monitor (9600 baud) to see:
- Card UIDs when scanned
- System status messages
- Add/remove confirmations
- Error messages and troubleshooting info

### Factory Reset
To clear all stored cards:
1. Call `clearAllCardsFromEEPROM()` function
2. Or manually clear EEPROM address 0

## 🔒 Security Considerations

### Security Features
- **Encrypted storage**: Card UIDs stored in EEPROM
- **Master card protection**: Required for system changes
- **Access logging**: All events logged to serial output
- **Fail-safe design**: Lock releases on power failure

### Security Best Practices
- **Physical security**: Protect Arduino and power supply
- **Master card**: Keep master card secure and separate
- **Regular audits**: Monitor serial logs for unauthorized attempts
- **Power backup**: Consider UPS for critical applications

### Limitations
- **Card cloning**: RFID cards can potentially be cloned
- **Physical bypass**: Relay/wiring can be physically compromised
- **Power dependency**: System requires constant power to maintain lock
- **Range**: RFID range limited to ~3cm

## 📊 Technical Specifications

### Performance
- **Card capacity**: 20 authorized cards maximum
- **Read time**: <500ms typical response
- **Memory usage**: ~1KB EEPROM for card storage
- **Power consumption**: ~200mA (Arduino + accessories)

### Operating Conditions
- **Temperature**: 0°C to 70°C (typical Arduino range)
- **Humidity**: 10% to 85% non-condensing
- **Power supply**: 5V for Arduino, 12V/2A+ for magnetic lock
- **RFID frequency**: 13.56MHz (ISO14443A)

### Compatibility
- **Arduino boards**: Uno, Nano, Pro Mini (with hardware SPI)
- **RFID cards**: ISO14443A (MIFARE Classic, NTAG, etc.)
- **Magnetic locks**: 12V solenoid/electromagnetic locks up to 2A

## 🤝 Contributing

### Reporting Issues
- Include complete error messages
- Provide wiring photos if hardware-related
- Specify Arduino model and library versions
- Include serial monitor output

### Feature Requests
- Describe use case and benefits
- Consider backward compatibility
- Provide implementation suggestions if possible

### Code Contributions
- Follow existing code style and commenting
- Test thoroughly with actual hardware
- Update documentation for new features
- Maintain security best practices

## 📄 License

This project is provided as-is for educational and personal use. Use at your own risk for security applications.

## 🆘 Support

For technical support and questions:
1. Check troubleshooting section above
2. Review serial monitor output for error messages
3. Verify wiring against provided diagrams
4. Test individual components separately

---

**⚠️ Safety Warning**: This system controls physical security devices. Test thoroughly before deployment and consider professional installation for critical applications. Always ensure fail-safe operation and backup access methods.