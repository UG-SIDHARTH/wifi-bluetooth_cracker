# =======================================================
# CRADLEGUARD 3.0 - MICROSD CARD STORAGE & UI ASSETS
# =======================================================

Format your MicroSD card as FAT32 (up to 32GB supported).
Copy all files from this `SD_Files/` folder directly to the root directory of your MicroSD card (e.g. `E:/`).

## Files & Folder Structure:

1. **`splash.bmp`**:
   - High-resolution 320x240 24-bit uncompressed Windows BMP image.
   - Automatically loaded and displayed on the 2.4" TFT screen during device startup.
   - You can replace this with any custom 320x240 24-bit BMP image created in Photoshop, GIMP, or Paint!

2. **`theme.txt`**:
   - Dynamic UI color scheme & configuration file.
   - Customize background color, header color, accent colors, button colors, and display titles in standard 16-bit RGB565 format (e.g., `0x0821`, `0x07E0`).
   - Automatically parsed by the ESP32 firmware on boot.

3. **`lyrics.txt`**:
   - Synchronized text lines and status banner animations.
   - Formatted as `[mm:ss.xx] Text message`.
   - Streamed and displayed across the TFT screen during operation.

4. **`wordlist.txt`**:
   - Plaintext dictionary of target passwords (one passphrase per line).
   - Used by the embedded Wi-Fi WPA2 dictionary cracking engine.

5. **`cracked_keys.txt`** (Created automatically by the ESP32):
   - When a passphrase match is discovered during a dictionary audit, the ESP32 automatically logs the target SSID, MAC address, and cracked key to this file.

6. **`captures/`** (Folder created automatically by ESP32):
   - Stores captured WPA 4-way EAPOL handshakes and PMKIDs formatted in Hashcat 22000 standard.
