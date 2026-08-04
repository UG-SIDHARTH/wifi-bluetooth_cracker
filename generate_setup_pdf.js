const PDFDocument = require('pdfkit');
const fs = require('fs');
const path = require('path');

const pdfPath = path.join(__dirname, 'ESP32_Project_Setup_Guide.pdf');

const doc = new PDFDocument({ margin: 40, size: 'A4' });
doc.pipe(fs.createWriteStream(pdfPath));

// Colors
const COLOR_PRIMARY = '#0f172a';
const COLOR_HEADER = '#1e3a8a';
const COLOR_ACCENT = '#2563eb';
const COLOR_WARN_BG = '#fef2f2';
const COLOR_WARN_BORDER = '#ef4444';
const COLOR_WARN_TEXT = '#991b1b';
const COLOR_TEXT = '#1e293b';
const COLOR_MUTED = '#64748b';
const COLOR_BG_ALT = '#f8fafc';
const COLOR_GREEN = '#16a34a';

// Header Banner
doc.rect(40, 40, 515, 65).fill(COLOR_PRIMARY);
doc.fillColor('#ffffff').fontSize(16).font('Helvetica-Bold').text('ESP32 BT & WI-FI SECURITY SUITE', 55, 50);
doc.fontSize(11).font('Helvetica').fillColor('#93c5fd').text('Complete Step-by-Step Project Setup & Build Guide', 55, 74);

// Warning Box
doc.rect(40, 115, 515, 42).fill(COLOR_WARN_BG);
doc.rect(40, 115, 5, 42).fill(COLOR_WARN_BORDER);
doc.fillColor(COLOR_WARN_TEXT).fontSize(9).font('Helvetica-Bold').text('LEGAL & EDUCATIONAL DISCLAIMER:', 55, 121);
doc.font('Helvetica').fontSize(8).text('This device design and documentation are strictly for educational and authorized penetration testing in controlled environments. Operating RF jammers or packet injection without authorization is illegal.', 55, 133, { width: 490 });

let y = 170;

function sectionTitle(text) {
  doc.fillColor(COLOR_PRIMARY).fontSize(13).font('Helvetica-Bold').text(text, 40, y);
  y += 4;
  doc.moveTo(40, y + 10).lineTo(555, y + 10).strokeColor(COLOR_ACCENT).lineWidth(1.5).stroke();
  y += 18;
}

function drawTable(headers, rows, colWidths) {
  let cy = y;
  doc.rect(40, cy, 515, 18).fill('#f1f5f9');
  doc.fillColor('#334155').fontSize(8.5).font('Helvetica-Bold');
  let cx = 45;
  headers.forEach((h, i) => {
    doc.text(h, cx, cy + 5, { width: colWidths[i] });
    cx += colWidths[i];
  });
  cy += 18;
  rows.forEach((row, rowIndex) => {
    if (rowIndex % 2 === 1) doc.rect(40, cy, 515, 16).fill(COLOR_BG_ALT);
    doc.fillColor(COLOR_TEXT).fontSize(8).font('Helvetica');
    cx = 45;
    row.forEach((cell, i) => {
      if (i === 0) doc.font('Helvetica-Bold').fillColor(COLOR_ACCENT);
      else doc.font('Helvetica').fillColor(COLOR_TEXT);
      doc.text(cell, cx, cy + 4, { width: colWidths[i] });
      cx += colWidths[i];
    });
    cy += 16;
  });
  y = cy + 10;
}

// SECTION 1: BILL OF MATERIALS
sectionTitle('1. Bill of Materials (Required Hardware)');
drawTable(
  ['Component Name', 'Specification / Details', 'Quantity / Purpose'],
  [
    ['ESP32 Dev Board', 'ESP32-WROOM-32 / NodeMCU-32S (30-Pin)', '1x Main Microcontroller'],
    ['2.4" TFT LCD Shield', 'MCUFRIEND 8-bit Parallel (Red PCB)', '1x Touchscreen Display'],
    ['NRF24L01 Modules', '2.4GHz Transceiver (Built-in PCB Antenna)', '2x Dual RF Hopping Radios'],
    ['MicroSD Card', 'FAT32 Formatted (up to 32GB)', '1x Offline Wordlist Storage'],
    ['Tactile Pushbuttons', '6x6mm Pushbutton Switches', '2x Physical Controls (ON/OFF)'],
    ['Breadboard & Jumpers', '830-Point Breadboard + F-M / M-M Wires', '1x Prototype Wiring Assembly'],
    ['5V USB Power Source', '5V 1A - 2A Power Bank or USB Cable', '1x Power Supply']
  ],
  [140, 235, 140]
);

// SECTION 2: HARDWARE WIRING SUMMARY
sectionTitle('2. Step 1: Hardware Wiring Overview');
doc.fillColor(COLOR_TEXT).fontSize(8.5).font('Helvetica');
doc.text('Connect all components according to the master pin allocation schema:', 40, y);
y += 12;

drawTable(
  ['Interface / Device', 'ESP32 Pins Connected', 'Wiring Description'],
  [
    ['Radio 1 (NRF24 #1)', 'CE: GPIO 22 | CSN: GPIO 21', 'Lower Channel Range (2402 - 2440 MHz)'],
    ['Radio 2 (NRF24 #2)', 'CE: GPIO 16 | CSN: GPIO 15', 'Upper Channel Range (2441 - 2480 MHz)'],
    ['Shared SPI Bus', 'SCK: GPIO 18 | MOSI: GPIO 23 | MISO: GPIO 19', 'Connected to Radio 1, Radio 2, & SD Card'],
    ['TFT LCD Bus', 'LCD_RST: 33, CS: 5, RS: 4, WR: 2', 'Data Bus D0-D7: GPIO 12, 13, 26, 25, 17, 27, 14, 32'],
    ['Physical Buttons', 'TURN ON: GPIO 12 | TURN OFF: GPIO 14', 'One leg to GPIO pin, other leg to GND Rail'],
    ['Power Connections', 'VCC: 3.3V Rail | Display Power: 5V/3.3V', 'Common Ground (GND) Rail connected to all devices']
  ],
  [130, 200, 185]
);

// PAGE 2
doc.addPage();
y = 40;

// SECTION 3: MICROSD CARD SETUP
sectionTitle('3. Step 2: MicroSD Card Preparation');
doc.fillColor(COLOR_TEXT).fontSize(8.5).font('Helvetica');
doc.text('1. Insert MicroSD card into your computer and format as FAT32.', 50, y); y += 14;
doc.text('2. Open the SD_Files/ folder in the project repository.', 50, y); y += 14;
doc.text('3. Copy wordlist.txt to the root directory of your MicroSD card (e.g. E:/wordlist.txt).', 50, y); y += 14;
doc.text('4. Insert the MicroSD card into the SD slot on the back of the 2.4" TFT Display Shield.', 50, y); y += 20;

// SECTION 4: ARDUINO IDE CONFIGURATION & LIBRARIES
sectionTitle('4. Step 3: Arduino IDE Configuration & Libraries');
doc.fillColor(COLOR_TEXT).fontSize(8.5).font('Helvetica');
doc.text('1. Download & Install Arduino IDE (v2.0 or newer recommended from arduino.cc).', 50, y); y += 14;
doc.text('2. Open Arduino IDE -> File -> Preferences.', 50, y); y += 14;
doc.text('3. In Additional Boards Manager URLs, paste the ESP32 package link:', 50, y); y += 14;
doc.fillColor(COLOR_ACCENT).font('Helvetica-Bold').text('   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json', 50, y); y += 16;
doc.fillColor(COLOR_TEXT).font('Helvetica').text('4. Go to Tools -> Board -> Boards Manager..., search for "esp32" by Espressif Systems and click Install.', 50, y); y += 16;
doc.text('5. Open Library Manager (Tools -> Manage Libraries...) and install the following required libraries:', 50, y); y += 18;

drawTable(
  ['Library Name', 'Author', 'Purpose'],
  [
    ['MCUFRIEND_kbv', 'David Prentice', '8-Bit Parallel TFT LCD Driver'],
    ['TouchScreen', 'Adafruit', 'Resistive Touchscreen Reading'],
    ['RF24', 'TMRh20', 'NRF24L01 2.4GHz Transceiver Control'],
    ['Adafruit GFX Library', 'Adafruit', 'Core Graphics Drawing Primitives']
  ],
  [150, 150, 215]
);

// SECTION 5: FIRMWARE COMPILATION & UPLOADING
sectionTitle('5. Step 4: Compiling & Uploading Firmware');
doc.fillColor(COLOR_TEXT).fontSize(8.5).font('Helvetica');
doc.text('1. Open BluetoothJammer/BluetoothJammer.ino in Arduino IDE.', 50, y); y += 14;
doc.text('2. Select Board: Tools -> Board -> ESP32 Arduino -> ESP32 Dev Module.', 50, y); y += 14;
doc.text('3. Select COM Port: Tools -> Port -> Select your connected ESP32 serial COM port.', 50, y); y += 14;
doc.text('4. Click Upload (Right Arrow button).', 50, y); y += 14;
doc.fillColor(COLOR_WARN_TEXT).font('Helvetica-Bold').text('Note: If uploading pauses at "Connecting...", press & hold the physical BOOT button (GPIO 0) on the ESP32.', 50, y, { width: 480 }); y += 22;

// SECTION 6: OPERATING INSTRUCTIONS
sectionTitle('6. Step 5: System Operation & UI Controls');
doc.fillColor(COLOR_TEXT).fontSize(8.5).font('Helvetica');
doc.text('On power-up, the device renders a 3D rotating wireframe cube boot animation and enters the Main Menu:', 40, y); y += 16;

drawTable(
  ['UI Menu Option', 'Function / Description', 'Controls'],
  [
    ['1. WIFI SCANNER', 'Sub-menu with 1.1 SELECT WIFI & 1.2 FIND PASSWORD options', 'Touch screen or TURN ON btn'],
    ['2. WIFI+BT JAMMER', 'Dual NRF24 RF signal hopping (2.4GHz BLE & BT Classic)', 'Touch JAMMER ON / OFF'],
    ['3. RESET', 'Restart device to initial state', 'Touch YES / NO confirmation']
  ],
  [130, 245, 140]
);

doc.fillColor(COLOR_PRIMARY).fontSize(9.5).font('Helvetica-Bold').text('Additional Interface Controls:', 40, y); y += 14;
doc.fillColor(COLOR_TEXT).fontSize(8.5).font('Helvetica');
doc.text('• BOOT Button (GPIO 0): Acts as BACK key to navigate to previous menu.', 50, y); y += 13;
doc.text('• Web Portal (http://192.168.4.1): Connect Wi-Fi to "ESP32-Security-Tool" (Pass: 12345678) for web control.', 50, y); y += 13;
doc.text('• Serial CLI: Open Serial Monitor at 115200 baud (Commands: scan, deauth, hashcat).', 50, y); y += 20;

// Footer
doc.fontSize(8).fillColor(COLOR_MUTED)
   .text('Document generated automatically for CradleGuard 3.0 ESP32 Security Suite Setup Guide.', 40, 780, { align: 'center', width: 515 });

doc.end();
console.log('Setup Guide PDF generated successfully at:', pdfPath);
