# ESP32 BT Jammer & Wi-Fi Security Suite — Complete Project & Build Guide

A complete, multi-tool security research and penetration testing device powered by an **ESP32**, **MCUFRIEND 2.4" TFT LCD Shield (8-bit Parallel)**, and **2x NRF24L01 Transceiver Modules**.

Features a **3D Wireframe Boot Animation**, **Touchscreen Landscape UI**, **Wi-Fi Scanner & Promiscuous Sniffer**, **Deauth Audit Injector**, **MicroSD Wordlist Passphrase Engine**, **Hashcat 22000 Exporter**, and a **Web Control Dashboard**.

---

## ⚠️ LEGAL & ETHICAL WARNING

**This repository is strictly for educational purposes, security research, and authorized penetration testing!**
DO NOT use signal jamming, packet injection, or network auditing on public, emergency, or unauthorized wireless networks. Unauthorized interference or network access is illegal in most countries and carries severe penalties.

---

## 📋 Bill of Materials (Required Parts)

- **ESP32 30-Pin Development Board** (NodeMCU-32S / ESP32-WROOM-32)
- **MCUFRIEND 2.4" TFT LCD Shield for Arduino UNO** (8-Bit Parallel Bus, red PCB)
- **2x NRF24L01 2.4GHz RF Transceiver Modules** (Black PCB version with built-in meander trace antenna)
- **1x MicroSD Card** (Formatted FAT32, optional for wordlist loading)
- **2x Tactile Pushbuttons** (Optional for physical ON/OFF control)
- **Breadboard & Jumper Wires** (Female-to-Male and Male-to-Male)
- **5V USB Power Source** (5V 1A–2A Power Bank or USB adapter)

---

## 🔌 STEP 1: Hardware Wiring Instructions

Connect the modules according to the pinout below:

### A. MCUFRIEND 2.4" TFT LCD Shield to ESP32
| Display Pin | ESP32 Pin | Function |
| :--- | :--- | :--- |
| **LCD_RST** | **GPIO 33** | Hardware Reset |
| **LCD_CS** | **GPIO 5** | Display Chip Select |
| **LCD_RS** | **GPIO 4** | Register / Command Select |
| **LCD_WR** | **GPIO 2** | Write Enable |
| **LCD_RD** | **3.3V Rail** | Read Enable (Connect to 3.3V) |
| **5V / 3V3 / GND** | **5V / 3.3V / GND** | Power & Ground Rails |
| **LCD_D0 – LCD_D7** | **GPIO 12, 13, 26, 25, 17, 27, 14, 32** | 8-Bit Parallel Data Bus |
| **SD_SS (CS)** | **GPIO 13** | SD Card Chip Select |
| **SD_DI / SD_DO / SD_SCK** | **GPIO 23 / 19 / 18** | Shared SPI Bus |

### B. Radio Transceivers (NRF24L01 Black PCB)
- **Radio 1 (Lower Channels 2402–2440 MHz)**:
  - `CE` $\rightarrow$ **GPIO 22** | `CSN` $\rightarrow$ **GPIO 21** | `SCK` $\rightarrow$ **GPIO 18** | `MOSI` $\rightarrow$ **GPIO 23** | `MISO` $\rightarrow$ **GPIO 19** | `VCC` $\rightarrow$ **3.3V** | `GND` $\rightarrow$ **GND**
- **Radio 2 (Upper Channels 2441–2480 MHz)**:
  - `CE` $\rightarrow$ **GPIO 16** | `CSN` $\rightarrow$ **GPIO 15** | `SCK` $\rightarrow$ **GPIO 18** | `MOSI` $\rightarrow$ **GPIO 23** | `MISO` $\rightarrow$ **GPIO 19** | `VCC` $\rightarrow$ **3.3V** | `GND` $\rightarrow$ **GND**

*Note: Built-in PCB antenna NRF24L01 modules draw only ~12mA each and do NOT require capacitors.*

---

## 💾 STEP 2: MicroSD Card Setup

1. Insert your MicroSD card into your computer and format it as **FAT32**.
2. Open the [`SD_Files/`](file:///c:/Users/Lenovo/Downloads/wifi+bluettoth/wifi-bluetooth_cracker/SD_Files) folder in this repository.
3. Copy **`wordlist.txt`** to the root directory of your MicroSD card.
4. Plug the MicroSD card into the SD card slot on the back of your 2.4" TFT Display Shield.

---

## 💻 STEP 3: Arduino IDE Configuration & Libraries

1. Download and install [Arduino IDE](https://www.arduino.cc/en/software).
2. Open Arduino IDE -> **File** -> **Preferences** and add the ESP32 Board Manager URL:
   `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
3. Go to **Tools** -> **Board** -> **Boards Manager...**, search for `esp32` by Espressif Systems and click **Install**.
4. Install the required libraries via **Tools** -> **Manage Libraries...**:
   - **`MCUFRIEND_kbv`** by David Prentice
   - **`TouchScreen`** by Adafruit
   - **`RF24`** by TMRh20
   - **`Adafruit GFX Library`** by Adafruit

---

## ⚡ STEP 4: Compiling & Uploading Firmware

1. Open [`BluetoothJammer/BluetoothJammer.ino`](file:///c:/Users/Lenovo/Downloads/wifi+bluettoth/wifi-bluetooth_cracker/BluetoothJammer/BluetoothJammer.ino) in Arduino IDE.
2. Select Board: **Tools** -> **Board** -> **ESP32 Arduino** -> **ESP32 Dev Module**.
3. Select Port: **Tools** -> **Port** -> Choose your ESP32 COM port.
4. Click **Upload** (press the physical **BOOT** button on the ESP32 if uploading pauses at `Connecting...`).

---

## 🎮 STEP 5: How to Operate the System

1. **3D Startup Boot Animation**:
   - On power-up, the display renders a 3D rotating wireframe cube and progress bar while initializing hardware.

2. **Touchscreen Navigation**:
   - Tap **MODE** at the bottom of the screen (or press the physical **BOOT** button on GPIO 0) to cycle modes:
     `BLE JAMMER` $\rightarrow$ `BT CLASSIC` $\rightarrow$ `WIFI SCANNER` $\rightarrow$ `WIFI HANDSHAKE` $\rightarrow$ `WIFI CRACKER` $\rightarrow$ `WEB PORTAL`
   - Tap **TURN ON / RUN** to start active scanning or operation.
   - Tap **TURN OFF / IDLE** to pause.

3. **Web Control Dashboard (`http://192.168.4.1`)**:
   - In `WEB PORTAL` mode, connect your phone or laptop Wi-Fi to **`ESP32-Security-Tool`** (Password: `12345678`).
   - Open browser at `http://192.168.4.1` for live network scanning and Hashcat hash exporting.

4. **Serial CLI Controls**:
   - Connect Serial Monitor at **115200 baud**:
     - Type `scan` to trigger immediate Wi-Fi scanning.
     - Type `hashcat` to print captured 4-way handshakes and PMKIDs in Hashcat mode 22000 format.

---

## 📜 License

[GNU General Public License v3.0](https://choosealicense.com/licenses/gpl-3.0/)