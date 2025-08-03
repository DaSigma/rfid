/*
 * RC522 RFID Access Control System with WS2812 LED for Arduino Uno WiFi Rev 2
 * Master Card: 93 52 EA 2F
 * 
 * Wiring connections:
 * RC522     Arduino Uno WiFi Rev 2
 * SDA  -->  Pin 8
 * SCK  -->  ICSP header pin 3 (SCK)
 * MOSI -->  ICSP header pin 4 (MOSI)
 * MISO -->  ICSP header pin 1 (MISO)
 * IRQ  -->  Not connected
 * GND  -->  GND
 * RST  -->  Pin 9
 * 3.3V -->  3.3V
 * 
 * WS2812 LED Strip:
 * VCC  -->  5V
 * GND  -->  GND
 * DIN  -->  Pin 6
 * 
 * Buzzer (2-pin):
 * Positive (+) --> Pin 7
 * Negative (-) --> GND
 * 
 * ICSP Header pinout (2x3 header):
 * 1: MISO    2: VCC
 * 3: SCK     4: MOSI  
 * 5: RESET   6: GND
 */

#include <SPI.h>
#include <MFRC522.h>
#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>

#define RST_PIN         9          // Reset pin
#define SS_PIN          8          // Slave Select pin
#define LED_PIN         6          // WS2812 LED strip data pin
#define BUZZER_PIN      7          // Buzzer pin
#define NUM_LEDS        1          // Number of LEDs (change if using more)
#define MAX_CARDS       20         // Maximum number of stored cards
#define ACCESS_TIMEOUT  5000       // 5 seconds in milliseconds
#define LED_TIMEOUT     3000       // 3 seconds to show status before returning to blue

// EEPROM addresses
#define EEPROM_CARD_COUNT_ADDR  0  // Address to store card count
#define EEPROM_CARDS_START_ADDR 1  // Starting address for card data (each card = 4 bytes)

// LED Colors
#define COLOR_OFF       0x000000   // Black (off)
#define COLOR_RED       0xFF0000   // Red (access denied)
#define COLOR_GREEN     0x00FF00   // Green (access granted)
#define COLOR_ORANGE    0xFF8000   // Orange (waiting for access)
#define COLOR_BLUE      0x0000FF   // Blue (system ready)

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// Master card UID: 93 52 EA 2F
byte masterCard[4] = {0x93, 0x52, 0xEA, 0x2F};

// Storage for allowed cards
byte allowedCards[MAX_CARDS][4];
int cardCount = 0;

// Access control variables
bool accessGranted = false;
bool removalMode = false;  // New: removal mode for deleting cards
unsigned long accessStartTime = 0;
bool waitingForMaster = false;

// LED control variables
unsigned long ledTimer = 0;
bool ledTimerActive = false;
uint32_t currentLEDState = COLOR_BLUE;
// Flashing LED variables for removal mode
unsigned long flashTimer = 0;
bool flashState = false;
#define FLASH_INTERVAL 300  // Flash every 300ms

void setup() {
  Serial.begin(9600);
  while (!Serial);
  SPI.begin();
  mfrc522.PCD_Init();
  
  // Initialize LED strip
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  
  // Initialize buzzer pin
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  
  // Load cards from EEPROM
  loadCardsFromEEPROM();
  
  delay(4);
  
  // System startup sequence
  setLEDColor(COLOR_BLUE);
  systemStartupBeep();
  
  Serial.println(F("================================================="));
  Serial.println(F("    RFID Access Control System Initialized"));
  Serial.println(F("================================================="));
  Serial.print(F("Master Card: "));
  printHex(masterCard, 4);
  Serial.println();
  Serial.print(F("Loaded "));
  Serial.print(cardCount);
  Serial.println(F(" authorized cards from memory."));
  Serial.println(F("System ready. Present a card to access."));
  Serial.println(F("================================================="));
}

void loop() {
  // Handle flashing LED in removal mode
  if (removalMode && (millis() - flashTimer > FLASH_INTERVAL)) {
    flashTimer = millis();
    flashState = !flashState;
    if (flashState) {
      setLEDColor(COLOR_RED);
    } else {
      setLEDColor(COLOR_OFF);
    }
  }
  
  // Check if LED timer should return to blue (default state) - but not in removal mode
  if (!removalMode && ledTimerActive && (millis() - ledTimer > LED_TIMEOUT)) {
    ledTimerActive = false;
    setLEDColor(COLOR_BLUE);
    currentLEDState = COLOR_BLUE;
  }
  
  // Check if access timeout has expired
  if ((accessGranted || removalMode) && (millis() - accessStartTime > ACCESS_TIMEOUT)) {
    accessGranted = false;
    removalMode = false;
    waitingForMaster = false;
    setLEDColor(COLOR_BLUE); // Back to ready state
    currentLEDState = COLOR_BLUE;
    ledTimerActive = false; // Stop any active LED timer
    flashState = false; // Reset flash state
    if (removalMode) {
      Serial.println(F("Card removal timeout expired. System locked."));
    } else {
      Serial.println(F("Access timeout expired. System locked."));
    }
    Serial.println(F("================================================="));
  }
  
  // Look for new cards
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  // Select one of the cards
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  // Check if it's the master card
  if (isMasterCard(mfrc522.uid.uidByte)) {
    Serial.print(F("DETECTED MASTER CARD: "));
    printHex(mfrc522.uid.uidByte, mfrc522.uid.size);
    Serial.println();
    handleMasterCard();
  }
  // Check if we're in removal mode
  else if (removalMode) {
    handleCardRemoval();
  }
  // Check if access is currently granted (master card mode for adding new cards)
  else if (accessGranted) {
    handleNewCardDuringAccess();
  }
  // Normal access check - check if card is authorized (this should come before waitingForMaster)
  else if (isCardStored(mfrc522.uid.uidByte)) {
    // Reset waiting flag since we found an authorized card
    waitingForMaster = false;
    handleAuthorizedCard();
  }
  // Check if we're waiting for master card to grant access to unknown card
  else if (waitingForMaster) {
    setLEDColorWithTimer(COLOR_RED);
    accessDeniedBeep(); // Denial beep
    Serial.println(F("ACCESS DENIED - Master card required first!"));
    Serial.print(F("Unknown card: "));
    printHex(mfrc522.uid.uidByte, mfrc522.uid.size);
    Serial.println();
    Serial.println(F("Please present master card to add new cards to system."));
    Serial.println(F("================================================="));
  }
  // Unknown card - requires master card to be added
  else {
    handleUnknownCard();
  }

  // Halt PICC and stop encryption
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
  
  delay(500); // Reduced delay to allow faster double-tap detection
}

// FIXED handleMasterCard() function with extensive debugging
void handleMasterCard() {
  static unsigned long lastMasterScan = 0;
  static int consecutiveScans = 0;
  unsigned long currentTime = millis();
  unsigned long timeSinceLastScan = currentTime - lastMasterScan;
  
  Serial.println(F("--- MASTER CARD SCAN DETECTED ---"));
  Serial.print(F("Current time: "));
  Serial.println(currentTime);
  Serial.print(F("Last scan time: "));
  Serial.println(lastMasterScan);
  Serial.print(F("Time since last scan: "));
  Serial.print(timeSinceLastScan);
  Serial.println(F(" ms"));
  
  // If less than 3 seconds since last scan, increment counter
  if (timeSinceLastScan < 3000 && lastMasterScan != 0) {
    consecutiveScans++;
    Serial.print(F("WITHIN TIME WINDOW! Incrementing counter to: "));
    Serial.println(consecutiveScans);
  } else {
    consecutiveScans = 1; // Reset counter for new sequence
    Serial.print(F("TIME WINDOW EXPIRED or FIRST SCAN. Resetting counter to: "));
    Serial.println(consecutiveScans);
  }
  
  lastMasterScan = currentTime;
  
  Serial.print(F("Current system state - accessGranted: "));
  Serial.print(accessGranted);
  Serial.print(F(", removalMode: "));
  Serial.println(removalMode);
  
  // Check for double-tap (2 or more scans within the time window)
  if (consecutiveScans >= 2 && !removalMode) {
    Serial.println(F("*** DOUBLE-TAP CONDITIONS MET! ***"));
    // Double-tap detected - enter removal mode
    consecutiveScans = 0; // Reset counter
    removalMode = true;
    accessGranted = false; // Disable normal access mode
    accessStartTime = millis();
    waitingForMaster = false;
    flashTimer = millis(); // Initialize flash timer
    flashState = true; // Start with LED on
    setLEDColor(COLOR_RED); // Start with red LED
    currentLEDState = COLOR_RED;
    ledTimerActive = false;
    cardRemovalModeBeep();
    
    Serial.println(F("MASTER CARD DOUBLE-TAP DETECTED!"));
    Serial.println(F("=== CARD REMOVAL MODE ACTIVATED ==="));
    Serial.println(F("Scan cards to REMOVE them from the system."));
    Serial.print(F("Currently stored cards: "));
    Serial.println(cardCount);
    Serial.println(F("Mode expires in 5 seconds."));
    Serial.println(F("================================================="));
    return;
  } else if (consecutiveScans >= 2) {
    Serial.println(F("*** DOUBLE-TAP DETECTED BUT CONDITIONS NOT MET ***"));
    Serial.print(F("accessGranted: "));
    Serial.print(accessGranted);
    Serial.print(F(", removalMode: "));
    Serial.println(removalMode);
  }
  
  // Normal master card behavior
  setLEDColor(COLOR_ORANGE); // Orange for master card detected
  currentLEDState = COLOR_ORANGE;
  ledTimerActive = false;
  masterCardBeep();
  
  Serial.println(F("MASTER CARD DETECTED - Normal processing"));
  
  if (removalMode) {
    accessStartTime = millis();
    // Reset flash timer when extending removal mode
    flashTimer = millis();
    flashState = true;
    setLEDColor(COLOR_RED);
    Serial.println(F("Card removal time extended by 5 seconds."));
  } else if (!accessGranted) {
    accessGranted = true;
    accessStartTime = millis();
    waitingForMaster = false;
    
    Serial.println(F("Access granted for 5 seconds."));
    Serial.println(F("Present new cards to add them to the system."));
    Serial.print(F("Currently stored cards: "));
    Serial.println(cardCount);
    
    if (consecutiveScans == 1) {
      Serial.println(F("Scan master card again quickly to enter removal mode."));
    }
  } else {
    accessStartTime = millis();
    Serial.println(F("Access time extended by 5 seconds."));
  }
  Serial.println(F("================================================="));
}

void handleNewCardDuringAccess() {
  // Check if card is already stored
  if (isCardStored(mfrc522.uid.uidByte)) {
    setLEDColorWithTimer(COLOR_GREEN); // Green for recognized card
    accessGrantedBeep(); // Success beep
    Serial.println(F("Card already in system."));
    Serial.print(F("Card: "));
    printHex(mfrc522.uid.uidByte, mfrc522.uid.size);
    Serial.println();
  } else {
    // Add new card to system
    if (cardCount < MAX_CARDS) {
      addCard(mfrc522.uid.uidByte);
      setLEDColorWithTimer(COLOR_GREEN); // Green for successful addition
      cardAddedBeep(); // Special beep for new card added
      Serial.println(F("NEW CARD ADDED to system!"));
      Serial.print(F("Card: "));
      printHex(mfrc522.uid.uidByte, mfrc522.uid.size);
      Serial.println();
      Serial.print(F("Total cards in system: "));
      Serial.println(cardCount);
    } else {
      setLEDColorWithTimer(COLOR_RED); // Red for error
      accessDeniedBeep(); // Error beep
      Serial.println(F("ERROR: Maximum number of cards reached!"));
      Serial.print(F("Cannot add card: "));
      printHex(mfrc522.uid.uidByte, mfrc522.uid.size);
      Serial.println();
    }
  }
  
  // Reset access timer
  accessStartTime = millis();
  Serial.println(F("================================================="));
}

void handleAccessAttempt() {
  // This function is now split into handleAuthorizedCard() and handleUnknownCard()
  // This function is kept for compatibility but shouldn't be called
}

void handleAuthorizedCard() {
  setLEDColorWithTimer(COLOR_GREEN); // Green for access granted
  accessGrantedBeep(); // Success beep
  Serial.println(F("ACCESS GRANTED - Welcome!"));
  Serial.print(F("Authorized card: "));
  printHex(mfrc522.uid.uidByte, mfrc522.uid.size);
  Serial.println();
  Serial.println(F("Card has permanent access."));
  Serial.println(F("================================================="));
}

void handleUnknownCard() {
  setLEDColorWithTimer(COLOR_RED); // Red for access denied
  accessDeniedBeep(); // Denial beep
  Serial.println(F("ACCESS DENIED"));
  Serial.print(F("Unknown card: "));
  printHex(mfrc522.uid.uidByte, mfrc522.uid.size);
  Serial.println();
  Serial.println(F("This card is not authorized."));
  Serial.println(F("Present master card first to add new cards to system."));
  waitingForMaster = true;
  Serial.println(F("================================================="));
}

void handleCardRemoval() {
  // Check if the scanned card is in the system
  int cardIndex = findCardIndex(mfrc522.uid.uidByte);
  
  if (cardIndex >= 0) {
    // Card found - remove it
    removeCard(cardIndex);
    setLEDColorWithTimer(COLOR_RED); // Red confirmation for removal
    cardRemovedBeep();
    
    Serial.println(F("CARD REMOVED from system!"));
    Serial.print(F("Removed card: "));
    printHex(mfrc522.uid.uidByte, mfrc522.uid.size);
    Serial.println();
    Serial.print(F("Remaining cards in system: "));
    Serial.println(cardCount);
  } else {
    // Card not found in system
    setLEDColorWithTimer(COLOR_ORANGE); // Orange for "card not found"
    accessDeniedBeep();
    
    Serial.println(F("Card not found in system."));
    Serial.print(F("Card: "));
    printHex(mfrc522.uid.uidByte, mfrc522.uid.size);
    Serial.println();
    Serial.println(F("Only authorized cards can be removed."));
  }
  
  // Reset removal mode timer
  accessStartTime = millis();
  Serial.println(F("================================================="));
}

bool isMasterCard(byte* uid) {
  for (int i = 0; i < 4; i++) {
    if (uid[i] != masterCard[i]) {
      return false;
    }
  }
  return true;
}

bool isCardStored(byte* uid) {
  return findCardIndex(uid) >= 0;
}

int findCardIndex(byte* uid) {
  for (int i = 0; i < cardCount; i++) {
    bool match = true;
    for (int j = 0; j < 4; j++) {
      if (allowedCards[i][j] != uid[j]) {
        match = false;
        break;
      }
    }
    if (match) {
      return i; // Return the index of the found card
    }
  }
  return -1; // Card not found
}

void addCard(byte* uid) {
  if (cardCount < MAX_CARDS) {
    for (int i = 0; i < 4; i++) {
      allowedCards[cardCount][i] = uid[i];
    }
    cardCount++;
    saveCardsToEEPROM(); // Save to EEPROM whenever a card is added
  }
}

void removeCard(int cardIndex) {
  if (cardIndex >= 0 && cardIndex < cardCount) {
    // Shift all cards after the removed card down by one position
    for (int i = cardIndex; i < cardCount - 1; i++) {
      for (int j = 0; j < 4; j++) {
        allowedCards[i][j] = allowedCards[i + 1][j];
      }
    }
    cardCount--;
    saveCardsToEEPROM(); // Save to EEPROM whenever a card is removed
  }
}

void printHex(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? "0" : "");
    Serial.print(buffer[i], HEX);
    if (i < bufferSize - 1) Serial.print(" ");
  }
}

void setLEDColor(uint32_t color) {
  for (int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, color);
  }
  strip.show();
}

void setLEDColorWithTimer(uint32_t color) {
  setLEDColor(color);
  currentLEDState = color;
  ledTimer = millis();
  ledTimerActive = true;
}

// Buzzer Functions
void systemStartupBeep() {
  // Two short beeps for system ready
  tone(BUZZER_PIN, 1000, 200);
  delay(250);
  tone(BUZZER_PIN, 1200, 200);
  delay(250);
}

void accessGrantedBeep() {
  // Two ascending tones for access granted
  tone(BUZZER_PIN, 800, 150);
  delay(200);
  tone(BUZZER_PIN, 1200, 150);
  delay(200);
}

void accessDeniedBeep() {
  // Three short low beeps for access denied
  tone(BUZZER_PIN, 400, 200);
  delay(250);
  tone(BUZZER_PIN, 400, 200);
  delay(250);
  tone(BUZZER_PIN, 400, 200);
  delay(250);
}

void masterCardBeep() {
  // Three ascending tones for master card
  tone(BUZZER_PIN, 600, 100);
  delay(150);
  tone(BUZZER_PIN, 900, 100);
  delay(150);
  tone(BUZZER_PIN, 1200, 100);
  delay(150);
}

void cardAddedBeep() {
  // Four ascending tones for new card added
  tone(BUZZER_PIN, 600, 100);
  delay(120);
  tone(BUZZER_PIN, 800, 100);
  delay(120);
  tone(BUZZER_PIN, 1000, 100);
  delay(120);
  tone(BUZZER_PIN, 1200, 100);
  delay(120);
}

void cardRemovedBeep() {
  // Four descending tones for card removed
  tone(BUZZER_PIN, 1200, 100);
  delay(120);
  tone(BUZZER_PIN, 1000, 100);
  delay(120);
  tone(BUZZER_PIN, 800, 100);
  delay(120);
  tone(BUZZER_PIN, 600, 100);
  delay(120);
}

void cardRemovalModeBeep() {
  // Special pattern for entering removal mode
  tone(BUZZER_PIN, 800, 100);
  delay(150);
  tone(BUZZER_PIN, 400, 100);
  delay(150);
  tone(BUZZER_PIN, 800, 100);
  delay(150);
  tone(BUZZER_PIN, 400, 100);
  delay(150);
}

// EEPROM Functions
void saveCardsToEEPROM() {
  // Save card count
  EEPROM.write(EEPROM_CARD_COUNT_ADDR, cardCount);
  
  // Save each card (4 bytes per card)
  for (int i = 0; i < cardCount; i++) {
    for (int j = 0; j < 4; j++) {
      EEPROM.write(EEPROM_CARDS_START_ADDR + (i * 4) + j, allowedCards[i][j]);
    }
  }
  
  Serial.print(F("Saved "));
  Serial.print(cardCount);
  Serial.println(F(" cards to memory."));
}

void loadCardsFromEEPROM() {
  // Load card count
  cardCount = EEPROM.read(EEPROM_CARD_COUNT_ADDR);
  
  // Validate card count (prevent corruption)
  if (cardCount > MAX_CARDS || cardCount < 0) {
    cardCount = 0;
    Serial.println(F("EEPROM appears to be uninitialized. Starting fresh."));
    return;
  }
  
  // Load each card (4 bytes per card)
  for (int i = 0; i < cardCount; i++) {
    for (int j = 0; j < 4; j++) {
      allowedCards[i][j] = EEPROM.read(EEPROM_CARDS_START_ADDR + (i * 4) + j);
    }
  }
}

void clearAllCardsFromEEPROM() {
  cardCount = 0;
  EEPROM.write(EEPROM_CARD_COUNT_ADDR, 0);
  Serial.println(F("All cards cleared from memory."));
}
