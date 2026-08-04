/*
  Code for ESP32 Bluetooth Jammer with 2x NRF24L01 modules, ILI9341 Touchscreen Display,
  and Dedicated Physical / Touch Buttons for TURN ON and TURN OFF.

  More info: https://github.com/stuthemoo/ESP32BluetoothJammer

  FOR EDUCATIONAL PURPOSES ONLY!
  This code repository is for educational purposes only! DO NOT use a signal jamming device as they can interfere with public safety communications and emergency services. In many countries the use of signal jammers is prohibited by law and intentional interference with wireless signals can result in severe penalties.
*/

#include <SPI.h>
#include <RF24.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>

// ----------------------------------------------------
// Pin Definitions
// ----------------------------------------------------
#define BUTTON_BOOT_PIN 0  // Built-in BOOT button (Mode Toggle / Reset)
#define LED_PIN         2  // Built-in LED

// Dedicated Physical Hardware Buttons (Active LOW with internal pullups)
#define BUTTON_ON_PIN   12 // Physical Pushbutton for TURN ON
#define BUTTON_OFF_PIN  14 // Physical Pushbutton for TURN OFF

// TFT Display Pins (ILI9341 SPI)
#define TFT_CS   5
#define TFT_DC   4
#define TFT_RST  33
#define TFT_BL   32  // Backlight PWM pin

// Touchscreen Pins (XPT2046 SPI)
#define TOUCH_CS 27

// 16 Mhz SPI speed for RF24
constexpr int SPI_SPEED = 16000000;

// Radio 1 on CE 22, CSN 21
RF24 radio1(22, 21, SPI_SPEED);

// Radio 2 on CE 16, CSN 15
RF24 radio2(16, 15, SPI_SPEED);

// Display & Touch Objects
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
XPT2046_Touchscreen touch(TOUCH_CS);

// ----------------------------------------------------
// Channels & Variables
// ----------------------------------------------------
// BLE Advertising channels (2402MHz, 2426MHz, 2480MHz)
int ble_channels[] = { 2, 26, 80 };

// Bluetooth classic channels (2402–2480 MHz)
int bluetooth_channels[79];

// Channel split variables
uint8_t num_channels = sizeof(ble_channels) / sizeof(ble_channels[0]);
uint8_t split_index = num_channels / 2;
uint8_t endCh1 = split_index - 1;
uint8_t startCh2 = split_index;
uint8_t endCh2 = num_channels - 1;

// State flags
bool classicMode = false;
bool jammingOn = false;
unsigned long lastHopTime = 0;
unsigned long lastOutputTime = 0;
unsigned long lastUIDrawTime = 0;
unsigned long lastTouchTime = 0;
unsigned long hopInterval = 50;  // microseconds
unsigned long hopCount = 0;

// Physical button state tracking
bool lastBootState = HIGH;
bool lastOnState   = HIGH;
bool lastOffState  = HIGH;

// Payload data
uint8_t payload[32];

// UI Color Palette (RGB565)
#define COLOR_BG         0x0821  // Deep Navy / Dark Grey
#define COLOR_CARD       0x18E3  // Dark Card Background
#define COLOR_HEADER     0x001F  // Header Blue
#define COLOR_TEXT       0xFFFF  // White Text
#define COLOR_TEXT_MUTED 0x9CD3  // Light Grey Text
#define COLOR_ACCENT     0x07FF  // Cyan Accent
#define COLOR_GREEN      0x07E0  // Bright Green
#define COLOR_RED        0xF800  // Bright Red
#define COLOR_YELLOW     0xFFE0  // Yellow

// ----------------------------------------------------
// Function Declarations
// ----------------------------------------------------
void setupRadio(RF24& radio);
void setClassicMode();
void setBleMode();
void sendRandomPacket();
void initDisplay();
void drawStaticUI();
void updateDisplayUI(bool forceRedraw = false);
void checkTouch();
void checkPhysicalButtons();

// ----------------------------------------------------
// Setup
// ----------------------------------------------------
void setup() {
  delay(1000);

  // Setup Serial
  Serial.begin(115200);
  Serial.println("\n--- FOR EDUCATIONAL PURPOSES ONLY! ---");
  Serial.println("ESP32 Bluetooth Jammer starting with Dual Hardware & Touch Buttons...\n");

  // Setup Push Buttons & LED
  pinMode(BUTTON_BOOT_PIN, INPUT_PULLUP);
  pinMode(BUTTON_ON_PIN, INPUT_PULLUP);
  pinMode(BUTTON_OFF_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH); // Turn on backlight

  // Generate random payload
  for (uint8_t i = 0; i < 32; i++) payload[i] = random(256);

  // Initialize Bluetooth Classic channels (2402–2480 MHz)
  for (int i = 0; i < 79; i++) {
    bluetooth_channels[i] = i + 2;
  }

  // Initialize Display & Touch
  initDisplay();

  // Initialize Radios
  bool r1_ok = radio1.begin();
  bool r2_ok = radio2.begin();

  if (!r1_ok) Serial.println("Radio 1 init failed. Check wiring.");
  if (!r2_ok) Serial.println("Radio 2 init failed. Check wiring.");

  if (r1_ok) setupRadio(radio1);
  if (r2_ok) setupRadio(radio2);

  if (r1_ok && r2_ok) {
    radio1.setChannel(ble_channels[0]);
    radio2.setChannel(ble_channels[1]);
    radio1.startConstCarrier(RF24_PA_MAX, ble_channels[0]);
    radio2.startConstCarrier(RF24_PA_MAX, ble_channels[1]);
  }

  // Draw full UI on screen
  drawStaticUI();
  updateDisplayUI(true);

  Serial.println("Setup complete. Dedicated TURN ON (GPIO 12) & TURN OFF (GPIO 14) ready.\n");
}

// ----------------------------------------------------
// Radio Setup Helper
// ----------------------------------------------------
void setupRadio(RF24& radio) {
  radio.setAutoAck(false);
  radio.setRetries(0, 0);
  radio.disableCRC();
  radio.setDataRate(RF24_2MBPS);
  radio.setPALevel(RF24_PA_MAX, true);
  radio.setChannel(ble_channels[0]);
  radio.setPayloadSize(32);
  radio.stopListening();
  radio.powerUp();
  radio.setAddressWidth(3);
  radio.setAutoAck(0x00);
}

// ----------------------------------------------------
// Mode Switchers
// ----------------------------------------------------
void setClassicMode() {
  num_channels = sizeof(bluetooth_channels) / sizeof(bluetooth_channels[0]);
  split_index = num_channels / 2;
  endCh1 = split_index - 1;
  startCh2 = split_index;
  endCh2 = num_channels - 1;

  radio1.setChannel(bluetooth_channels[0]);
  radio2.setChannel(bluetooth_channels[startCh2]);
  radio1.startConstCarrier(RF24_PA_MAX, bluetooth_channels[0]);
  radio2.startConstCarrier(RF24_PA_MAX, bluetooth_channels[startCh2]);

  hopCount = 0;
  classicMode = true;
  updateDisplayUI(true);
}

void setBleMode() {
  num_channels = sizeof(ble_channels) / sizeof(ble_channels[0]);
  split_index = num_channels / 2;
  endCh1 = split_index - 1;
  startCh2 = split_index;
  endCh2 = num_channels - 1;

  radio1.setChannel(ble_channels[0]);
  radio2.setChannel(ble_channels[1]);
  radio1.startConstCarrier(RF24_PA_MAX, ble_channels[0]);
  radio2.startConstCarrier(RF24_PA_MAX, ble_channels[1]);

  hopCount = 0;
  classicMode = false;
  updateDisplayUI(true);
}

// ----------------------------------------------------
// Main Loop
// ----------------------------------------------------
void loop() {
  // 1. Check Serial Debug Commands
  if (Serial.available() > 0) {
    String serialInput = Serial.readStringUntil('\n');
    serialInput.trim();
    if (serialInput == "debug1") {
      Serial.println("Radio 1 info:");
      radio1.printPrettyDetails();
    } else if (serialInput == "debug2") {
      Serial.println("Radio 2 info:");
      radio2.printPrettyDetails();
    }
  }

  // 2. Check Touch Screen Inputs
  checkTouch();

  // 3. Check Physical Push Buttons (GPIO 12 & 14 & BOOT 0)
  checkPhysicalButtons();

  // 4. Update TFT Display UI periodically (every 250ms)
  if (millis() - lastUIDrawTime >= 250) {
    lastUIDrawTime = millis();
    updateDisplayUI(false);
  }

  // 5. Transmit RF Packets if Jamming is Active
  if (jammingOn && (micros() - lastHopTime >= hopInterval)) {
    sendRandomPacket();
    hopCount++;
    lastHopTime = micros();

    if (millis() - lastOutputTime >= 5000) {
      lastOutputTime = millis();
      Serial.print("Transmitted to ");
      Serial.print(classicMode ? "Classic" : "BLE");
      Serial.println(" channels " + String(hopCount) + " times.");
    }
  }
}

// ----------------------------------------------------
// Packet Transmission
// ----------------------------------------------------
void sendRandomPacket() {
  static uint8_t ch1 = 0;
  static uint8_t ch2 = startCh2;

  if (classicMode) {
    radio1.setChannel(bluetooth_channels[ch1]);
    radio2.setChannel(bluetooth_channels[ch2]);
    ch1 = (ch1 + 1);
    if (ch1 > endCh1) ch1 = 0;
    ch2 = (ch2 + 1);
    if (ch2 > endCh2) ch2 = startCh2;
  } else {
    radio1.setChannel(ble_channels[ch1]);
    radio2.setChannel(ble_channels[ch2]);
    ch1 = (ch1 + 1);
    if (ch1 > endCh1) ch1 = 0;
    ch2 = (ch2 + 1);
    if (ch2 > endCh2) ch2 = startCh2;
  }

  radio1.write(&payload, sizeof(payload));
  radio2.write(&payload, sizeof(payload));
}

// ----------------------------------------------------
// Display & Touchscreen Functions
// ----------------------------------------------------
void initDisplay() {
  tft.begin();
  tft.setRotation(1); // Landscape (320x240)
  tft.fillScreen(COLOR_BG);

  touch.begin();
  touch.setRotation(1);
}

void drawStaticUI() {
  tft.fillScreen(COLOR_BG);

  // Top Header Bar
  tft.fillRect(0, 0, 320, 35, COLOR_HEADER);
  tft.setTextColor(COLOR_TEXT);
  tft.setTextSize(2);
  tft.setCursor(15, 8);
  tft.print("CRADLEGUARD RF JAMMER");

  // Status Info Card Container
  tft.drawRoundRect(10, 42, 300, 105, 8, COLOR_ACCENT);
  tft.fillRoundRect(11, 43, 298, 103, 8, COLOR_CARD);

  // Status Labels
  tft.setTextSize(1);
  tft.setTextColor(COLOR_TEXT_MUTED);
  tft.setCursor(25, 55);
  tft.print("JAMMER STATE:");

  tft.setCursor(25, 80);
  tft.print("TARGET MODE:");

  tft.setCursor(25, 105);
  tft.print("TOTAL HOPS:");

  // Interactive Touch Buttons:
  // Button 1: [ TURN ON ]  (X=15, Y=158, W=135, H=36) - Green
  // Button 2: [ TURN OFF ] (X=165, Y=158, W=135, H=36) - Red
  // Button 3: [ MODE BLE / CLASSIC ] (X=15, Y=200, W=285, H=32) - Cyan
  tft.fillRoundRect(15, 158, 135, 36, 6, COLOR_GREEN);
  tft.drawRoundRect(15, 158, 135, 36, 6, COLOR_TEXT);
  tft.setTextColor(COLOR_TEXT);
  tft.setTextSize(2);
  tft.setCursor(32, 168);
  tft.print("TURN ON");

  tft.fillRoundRect(165, 158, 135, 36, 6, COLOR_RED);
  tft.drawRoundRect(165, 158, 135, 36, 6, COLOR_TEXT);
  tft.setTextColor(COLOR_TEXT);
  tft.setTextSize(2);
  tft.setCursor(178, 168);
  tft.print("TURN OFF");
}

void updateDisplayUI(bool forceRedraw) {
  static bool lastJammingState = false;
  static bool lastModeState = false;
  static unsigned long lastDisplayedHops = 0;

  // Update Jammer State Label
  if (forceRedraw || jammingOn != lastJammingState) {
    lastJammingState = jammingOn;

    tft.fillRect(130, 52, 170, 20, COLOR_CARD);
    tft.setCursor(130, 55);
    tft.setTextSize(2);
    if (jammingOn) {
      tft.setTextColor(COLOR_RED);
      tft.print("TRANSMITTING");
    } else {
      tft.setTextColor(COLOR_YELLOW);
      tft.print("IDLE");
    }
  }

  // Update Target Mode Label & Mode Touch Button
  if (forceRedraw || classicMode != lastModeState) {
    lastModeState = classicMode;

    tft.fillRect(130, 77, 170, 20, COLOR_CARD);
    tft.setCursor(130, 80);
    tft.setTextSize(2);
    tft.setTextColor(COLOR_ACCENT);
    tft.print(classicMode ? "BT CLASSIC" : "BLE ADV");

    // Redraw Mode Button (X=15, Y=200, W=285, H=32)
    tft.fillRoundRect(15, 200, 285, 32, 6, COLOR_HEADER);
    tft.drawRoundRect(15, 200, 285, 32, 6, COLOR_TEXT);
    tft.setTextColor(COLOR_TEXT);
    tft.setTextSize(1);
    tft.setCursor(45, 212);
    tft.print(classicMode ? "MODE: BT CLASSIC (TAP TO SWITCH BLE)" : "MODE: BLE ADV (TAP TO SWITCH CLASSIC)");
  }

  // Update Hop Count Text
  if (forceRedraw || hopCount != lastDisplayedHops) {
    lastDisplayedHops = hopCount;
    tft.fillRect(130, 102, 170, 20, COLOR_CARD);
    tft.setCursor(130, 105);
    tft.setTextSize(2);
    tft.setTextColor(COLOR_TEXT);
    tft.print(hopCount);
  }
}

// ----------------------------------------------------
// Touch Input Handling
// ----------------------------------------------------
void checkTouch() {
  if (!touch.touched()) return;

  if (millis() - lastTouchTime < 300) return;

  TS_Point p = touch.getPoint();
  lastTouchTime = millis();

  // Convert raw touch coordinates to screen landscape space (320x240)
  int touchX = map(p.x, 200, 3700, 0, 320);
  int touchY = map(p.y, 200, 3700, 0, 240);

  // Button 1: [ TURN ON ] (X: 15-150, Y: 158-194)
  if (touchX >= 15 && touchX <= 150 && touchY >= 158 && touchY <= 194) {
    if (!jammingOn) {
      jammingOn = true;
      digitalWrite(LED_PIN, HIGH);
      Serial.println("\nTouch: TURN ON pressed -> Transmission started.");
      updateDisplayUI(true);
    }
  }

  // Button 2: [ TURN OFF ] (X: 165-300, Y: 158-194)
  if (touchX >= 165 && touchX <= 300 && touchY >= 158 && touchY <= 194) {
    if (jammingOn) {
      jammingOn = false;
      digitalWrite(LED_PIN, LOW);
      Serial.println("\nTouch: TURN OFF pressed -> Transmission stopped.");
      updateDisplayUI(true);
    }
  }

  // Button 3: [ MODE TOGGLE ] (X: 15-300, Y: 200-232)
  if (touchX >= 15 && touchX <= 300 && touchY >= 200 && touchY <= 232) {
    if (classicMode) {
      setBleMode();
      Serial.println("\nTouch: Switched to BLE advertising mode.");
    } else {
      setClassicMode();
      Serial.println("\nTouch: Switched to BT Classic mode.");
    }
  }
}

// ----------------------------------------------------
// Physical Push Button Handling (Dedicated ON & OFF)
// ----------------------------------------------------
void checkPhysicalButtons() {
  bool onState   = digitalRead(BUTTON_ON_PIN);
  bool offState  = digitalRead(BUTTON_OFF_PIN);
  bool bootState = digitalRead(BUTTON_BOOT_PIN);

  // Physical TURN ON Button (GPIO 12) pressed
  if (onState == LOW && lastOnState == HIGH) {
    if (!jammingOn) {
      jammingOn = true;
      digitalWrite(LED_PIN, HIGH);
      Serial.println("\nPhysical Hardware Button: TURN ON pressed.");
      updateDisplayUI(true);
    }
    delay(150);
  }

  // Physical TURN OFF Button (GPIO 14) pressed
  if (offState == LOW && lastOffState == HIGH) {
    if (jammingOn) {
      jammingOn = false;
      digitalWrite(LED_PIN, LOW);
      Serial.println("\nPhysical Hardware Button: TURN OFF pressed.");
      updateDisplayUI(true);
    }
    delay(150);
  }

  // Physical BOOT Button (GPIO 0) pressed
  if (bootState == LOW && lastBootState == HIGH) {
    if (classicMode) {
      setBleMode();
    } else {
      setClassicMode();
    }
    delay(150);
  }

  lastOnState   = onState;
  lastOffState  = offState;
  lastBootState = bootState;
}