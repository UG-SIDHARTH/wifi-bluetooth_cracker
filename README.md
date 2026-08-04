# ESP32 Bluetooth & Wi-Fi Security, Scanning & Cracking Suite

A multi-tool security research device powered by an ESP32, 2x NRF24L01 transceivers, and an ILI9341 2.8" SPI Touchscreen display.

Features full support for **Bluetooth RF Jamming (BLE & Classic)**, **Wi-Fi Access Point & Client Scanning**, **EAPOL 4-Way Handshake & PMKID Promiscuous Sniffing**, **Deauthentication Attack Auditing**, **Embedded Password Dictionary Engine**, **Hashcat Mode 22000 Exporter**, and a **Web Control Dashboard**.

---

## ⚠️ LEGAL & ETHICAL WARNING

**This repository is strictly for educational purposes, security research, and authorized penetration testing!**
DO NOT use signal jamming, packet injection, or network auditing on public, emergency, or unauthorized wireless networks. Unauthorized interference or network access is illegal in most countries and carries severe penalties.

---

## 🚀 System Features Matrix

| Feature | Protocol / Chip | Details |
| :--- | :--- | :--- |
| **BLE Signal Jammer** | 2x NRF24L01 | Jams BLE advertising channels (2402MHz, 2426MHz, 2480MHz). |
| **BT Classic Jammer** | 2x NRF24L01 | High-speed hopping across all 79 Bluetooth Classic channels (2402–2480 MHz). |
| **Wi-Fi AP & Client Scanner** | Built-in ESP32 Wi-Fi | Scans 2.4GHz channels 1–14, detailing SSID, RSSI, BSSID (MAC), Channel, Security (OPEN, WEP, WPA, WPA2, WPA3). |
| **Wi-Fi Handshake & PMKID Sniffer** | Built-in ESP32 Wi-Fi | Promiscuous mode sniffer for EAPOL key frames (0x888e) and RSN IE PMKID hashes. |
| **Deauthentication Audit Injector** | Built-in ESP32 Wi-Fi | Injects 802.11 Deauth management frames (`0xC0`) to force target station re-authentication. |
| **Embedded Passphrase Cracker** | ESP32 CPU Engine | Fast dictionary passphrase verification engine testing weak WPA default passphrases. |
| **Hashcat 22000 Exporter** | Serial & Web Interface | Formats captured EAPOL handshakes and PMKID data into standard `WPA*01*...` / `WPA*02*...` format for cracking with Hashcat or Aircrack-ng. |
| **Web Portal & SoftAP Server** | ESP32 WebServer | Creates local Access Point `ESP32-Security-Tool` with Web Control Dashboard at `http://192.168.4.1`. |
| **Hardware & Touch UI** | ILI9341 + XPT2046 | Live landscape dashboard with interactive touch buttons and physical pushbuttons (GPIO 12 = ON/RUN, GPIO 14 = OFF/IDLE, GPIO 0 = Mode Toggle). |

---

## 💾 SD Card Information

> [!NOTE]
> **NO SD CARD REQUIRED!**
> 
> Although standard ILI9341 display modules include a physical SD card slot on the back of the PCB, **this firmware does not require an SD card**. All UI graphics, fonts, web pages, and scanning logic are stored directly inside the ESP32's internal flash memory. Captured handshakes and hashes can be downloaded live over the Web Portal (`http://192.168.4.1`) or copied from the Serial Monitor (`115200` baud). You can leave the SD card slot completely empty.

---

## ⚡ Performance & Thermal Safety

- **Dual-Core 240 MHz Distribution**: Background hardware Wi-Fi/RF sniffing runs on Core 0, while UI display rendering runs on Core 1, keeping CPU load below ~30%.
- **Zero Radio Contention**: The internal Wi-Fi chip turns off (`WiFi.mode(WIFI_OFF)`) during Bluetooth jamming modes, avoiding radio conflicts and minimizing power draw.
- **Power & Capacitors**: Power the system with a 5V (1A–2A) USB source. For high-power PA+LNA NRF24L01 modules, attach a **10µF to 47µF capacitor** across `VCC` and `GND` to prevent brownouts.

---

## 🛠️ Parts List

- [ESP32 Dev Board](https://s.click.aliexpress.com/e/_oBzks2E)
- [ILI9341 2.8" SPI Touchscreen Display](https://s.click.aliexpress.com/e/_oBzks2E)
- [2x NRF24L01+PA+LNA](https://s.click.aliexpress.com/e/_okUsZpp)
- [2x Capacitors (10µF – 47µF)](https://s.click.aliexpress.com/e/_olkWSDz)
- [2x Push Buttons (Active LOW)](https://s.click.aliexpress.com/e/_on6KzoP)
- [Breadboard & Jumper Wires](https://s.click.aliexpress.com/e/_ooo7z5h)
- [5V Power Supply / Power Bank](https://s.click.aliexpress.com/e/_oneC3BV)

---

## 🔌 Hardware Wiring Diagram & Pinout

<img src="https://github.com/stuthemoo/ESP32BluetoothJammer/raw/main/Images/wiring-breadboard.png" width="1000" alt="Breadboard Wiring Diagram">

For complete pinout charts and hardware connection guides, refer to `ESP32_Bluetooth_Jammer_Pinout.html` and `ESP32_Bluetooth_Jammer_Pinout.pdf`.

---

## 💻 Uploading Instructions

1. Download and install [Arduino IDE](https://www.arduino.cc/en/software).
2. Add ESP32 Board Manager URL (`https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`).
3. Install the required libraries via Arduino Library Manager:
   - `RF24` by TMRh20
   - `Adafruit GFX Library` by Adafruit
   - `Adafruit ILI9341` by Adafruit
   - `XPT2046_Touchscreen` by Paul Stoffregen
4. Open `BluetoothJammer/BluetoothJammer.ino`.
5. Select **ESP32 Dev Module**, choose your serial port, and click **Upload**.

---

## ⚡ Serial CLI Commands

Connect to the ESP32 at `115200` baud rate to issue command line directives:

- `scan`: Trigger immediate 2.4GHz Wi-Fi scan.
- `deauth`: Launch deauth frame injection against selected AP target.
- `hashcat`: Print all captured EAPOL handshakes & PMKID hashes in Hashcat mode 22000 output format.

---

## 📜 License

[GNU General Public License v3.0](https://choosealicense.com/licenses/gpl-3.0/)