const PDFDocument = require('pdfkit');
const fs = require('fs');
const path = require('path');

const pdfPath = path.join(__dirname, 'ESP32_Bluetooth_Jammer_Pinout.pdf');
const imagePath = path.join(__dirname, 'Images', 'wiring-full-breadboard.png');

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
const COLOR_BG_ALT = '#f8fafc';

// Header
doc.rect(40, 40, 515, 65).fill(COLOR_PRIMARY);
doc.fillColor('#ffffff').fontSize(16).font('Helvetica-Bold').text('ESP32 BT JAMMER & WI-FI SECURITY SUITE', 55, 50);
doc.fontSize(11).font('Helvetica').fillColor('#93c5fd').text('Master Hardware Pinout, RF Transceiver & Wiring Diagram Guide', 55, 74);

// Warning Box
doc.rect(40, 115, 515, 45).fill(COLOR_WARN_BG);
doc.rect(40, 115, 5, 45).fill(COLOR_WARN_BORDER);
doc.fillColor(COLOR_WARN_TEXT).fontSize(9).font('Helvetica-Bold').text('LEGAL & EDUCATIONAL DISCLAIMER:', 55, 122);
doc.font('Helvetica').fontSize(8.5).text('This device design and documentation are strictly for educational and authorized penetration testing in controlled environments. Operating RF jammers or packet injection without authorization is illegal.', 55, 136, { width: 490 });

let y = 175;

// Section 1: Full Breadboard Wiring Diagram
doc.fillColor(COLOR_PRIMARY).fontSize(14).font('Helvetica-Bold').text('1. Full Master Hardware Breadboard Wiring Diagram', 40, y);
y += 20;

if (fs.existsSync(imagePath)) {
  doc.image(imagePath, 40, y, { width: 515 });
  y += 280;
}

// Section 2: ESP32 Pin Allocation Box (Moved to Page 2)
doc.addPage();
y = 40;

doc.fillColor(COLOR_PRIMARY).fontSize(14).font('Helvetica-Bold').text('2. ESP32 Pin Allocation Overview', 40, y);
y += 20;

doc.rect(40, y, 515, 115).fill('#1e293b');
doc.fillColor('#facc15').fontSize(11).font('Helvetica-Bold').text('ESP32 30-PIN DEV BOARD LAYOUT', 40, y + 8, { align: 'center', width: 515 });

const leftPins = [
  ['Radio 1 CE', 'GPIO 22'],
  ['Radio 2 CE', 'GPIO 16'],
  ['Touch T_CS', 'GPIO 27'],
  ['TFT Backlight', 'GPIO 32'],
  ['TFT Reset', 'GPIO 33'],
  ['Physical ON Btn', 'GPIO 12'],
  ['Physical OFF Btn', 'GPIO 14'],
  ['Power Rail', '3.3V']
];

const rightPins = [
  ['GPIO 21', 'Radio 1 CSN'],
  ['GPIO 15', 'Radio 2 CSN'],
  ['GPIO 4',  'TFT DC / RS'],
  ['GPIO 5',  'TFT CS'],
  ['GPIO 18', 'SPI SCK (Clock)'],
  ['GPIO 19', 'SPI MISO'],
  ['GPIO 23', 'SPI MOSI'],
  ['GND',     'Ground Rail']
];

let py = y + 28;
doc.fontSize(8.5).font('Helvetica');
for (let i = 0; i < leftPins.length; i++) {
  doc.fillColor('#cbd5e1').text(leftPins[i][0], 65, py);
  doc.fillColor('#38bdf8').font('Helvetica-Bold').text(leftPins[i][1], 170, py);

  doc.fillColor('#38bdf8').font('Helvetica-Bold').text(rightPins[i][0], 330, py);
  doc.fillColor('#cbd5e1').font('Helvetica').text(rightPins[i][1], 410, py);
  py += 10.5;
}

// PAGE 2
doc.addPage();
y = 40;

// Section 3: Shared SPI Bus Table & Wi-Fi Radio
doc.fillColor(COLOR_PRIMARY).fontSize(14).font('Helvetica-Bold').text('3. Hardware Interfaces (VSPI & ESP32 Wi-Fi Radio)', 40, y);
y += 20;

function drawTable(headers, rows, startY, colWidths) {
  let cy = startY;
  // Header
  doc.rect(40, cy, 515, 18).fill('#f1f5f9');
  doc.fillColor('#334155').fontSize(9).font('Helvetica-Bold');
  let cx = 45;
  headers.forEach((h, i) => {
    doc.text(h, cx, cy + 5, { width: colWidths[i] });
    cx += colWidths[i];
  });
  cy += 18;

  // Rows
  rows.forEach((row, rowIndex) => {
    if (rowIndex % 2 === 1) {
      doc.rect(40, cy, 515, 16).fill(COLOR_BG_ALT);
    }
    doc.fillColor(COLOR_TEXT).fontSize(8.5).font('Helvetica');
    cx = 45;
    row.forEach((cell, i) => {
      if (i === 1) doc.font('Helvetica-Bold').fillColor(COLOR_ACCENT);
      else doc.font('Helvetica').fillColor(COLOR_TEXT);
      doc.text(cell, cx, cy + 4, { width: colWidths[i] });
      cx += colWidths[i];
    });
    cy += 16;
  });
  return cy;
}

y = drawTable(
  ['Signal / Interface', 'ESP32 Pin', 'Connected Modules / Mode', 'Type'],
  [
    ['SCK (Clock)', 'GPIO 18', 'Radio 1, Radio 2, TFT SCK, Touch T_CLK', 'Shared SPI'],
    ['MISO (Master In)', 'GPIO 19', 'Radio 1, Radio 2, TFT SDO, Touch T_DO', 'Shared SPI'],
    ['MOSI (Master Out)', 'GPIO 23', 'Radio 1, Radio 2, TFT SDI, Touch T_DIN', 'Shared SPI'],
    ['Wi-Fi 802.11 b/g/n', 'Internal RF', 'AP Scanning, Promiscuous Sniffer, Web Portal', 'Internal RF']
  ],
  y,
  [120, 80, 225, 90]
);

y += 15;

// Section 4: Radio 1 & 2 Tables
doc.fillColor(COLOR_PRIMARY).fontSize(14).font('Helvetica-Bold').text('4. Radio Transceiver Modules (NRF24L01)', 40, y);
y += 18;

doc.fillColor('#0f172a').fontSize(10).font('Helvetica-Bold').text('Radio #1 (Lower Channels 2402-2440 MHz):', 40, y);
y += 14;

y = drawTable(
  ['NRF24 #1 Pin', 'ESP32 Pin', 'Function', 'Signal Category'],
  [
    ['VCC / GND', '3.3V / GND', 'Power Rail (+3.3V Max)', 'Power / Ground'],
    ['CE', 'GPIO 22', 'Chip Enable', 'Dedicated Control'],
    ['CSN', 'GPIO 21', 'SPI Chip Select', 'Dedicated SPI CS'],
    ['SCK / MOSI / MISO', 'GPIO 18 / 23 / 19', 'Hardware SPI Bus', 'Shared SPI']
  ],
  y,
  [120, 110, 160, 125]
);

y += 12;
doc.fillColor('#0f172a').fontSize(10).font('Helvetica-Bold').text('Radio #2 (Upper Channels 2441-2480 MHz):', 40, y);
y += 14;

y = drawTable(
  ['NRF24 #2 Pin', 'ESP32 Pin', 'Function', 'Signal Category'],
  [
    ['VCC / GND', '3.3V / GND', 'Power Rail (+3.3V Max)', 'Power / Ground'],
    ['CE', 'GPIO 16', 'Chip Enable', 'Dedicated Control'],
    ['CSN', 'GPIO 15', 'SPI Chip Select', 'Dedicated SPI CS'],
    ['SCK / MOSI / MISO', 'GPIO 18 / 23 / 19', 'Hardware SPI Bus', 'Shared SPI']
  ],
  y,
  [120, 110, 160, 125]
);

y += 15;

// Section 5: Display (2.4" TFT LCD Shield - 8-Bit Parallel)
doc.fillColor(COLOR_PRIMARY).fontSize(14).font('Helvetica-Bold').text('5. 2.4" TFT LCD Shield (8-Bit Parallel Bus)', 40, y);
y += 18;

y = drawTable(
  ['Shield Pin', 'ESP32 Pin', 'Function', 'Signal Category'],
  [
    ['5V / 3V3 / GND', '5V / 3.3V / GND', 'Display Power & Ground', 'Power / Ground'],
    ['LCD_RST', 'GPIO 33', 'Hardware Reset', 'Dedicated Control'],
    ['LCD_CS', 'GPIO 5', 'Chip Select', 'Dedicated Control'],
    ['LCD_RS', 'GPIO 4', 'Register / Command Select', 'Dedicated Control'],
    ['LCD_WR', 'GPIO 2', 'Write Enable', 'Dedicated Control'],
    ['LCD_RD', '3.3V (Tie HIGH)', 'Read Enable (Not Used)', 'Power'],
    ['LCD_D0', 'GPIO 12', 'Data Bit 0', '8-Bit Parallel'],
    ['LCD_D1', 'GPIO 13', 'Data Bit 1', '8-Bit Parallel'],
    ['LCD_D2', 'GPIO 26', 'Data Bit 2', '8-Bit Parallel'],
    ['LCD_D3', 'GPIO 25', 'Data Bit 3', '8-Bit Parallel'],
    ['LCD_D4', 'GPIO 17', 'Data Bit 4', '8-Bit Parallel'],
    ['LCD_D5', 'GPIO 27', 'Data Bit 5', '8-Bit Parallel'],
    ['LCD_D6', 'GPIO 14', 'Data Bit 6', '8-Bit Parallel'],
    ['LCD_D7', 'GPIO 32', 'Data Bit 7', '8-Bit Parallel']
  ],
  y,
  [100, 120, 160, 135]
);

// PAGE 3
doc.addPage();
y = 40;

// Section 6: Push Buttons
doc.fillColor(COLOR_PRIMARY).fontSize(14).font('Helvetica-Bold').text('6. Hardware Pushbuttons & System Controls', 40, y);
y += 18;

y = drawTable(
  ['Button / LED', 'ESP32 Pin', 'Wiring Connection', 'Mode'],
  [
    ['TURN ON Button', 'GPIO 12', 'One leg to GPIO 12, other leg to GND', 'INPUT_PULLUP'],
    ['TURN OFF Button', 'GPIO 14', 'One leg to GPIO 14, other leg to GND', 'INPUT_PULLUP'],
    ['BOOT Button', 'GPIO 0', 'Built-in BOOT pushbutton on ESP32 board', 'Internal'],
    ['Status LED', 'GPIO 2', 'Built-in LED on ESP32 board', 'OUTPUT']
  ],
  y,
  [120, 90, 205, 100]
);

y += 25;

// Section 7: Power & Capacitor Notes
doc.fillColor(COLOR_PRIMARY).fontSize(14).font('Helvetica-Bold').text('7. Power Supply & Capacitor Recommendations', 40, y);
y += 18;

doc.fillColor(COLOR_TEXT).fontSize(9.5).font('Helvetica');
doc.text('1. Built-in PCB Antenna Modules: Draw ~12mA each. Can be powered directly from ESP32 3.3V pin without capacitors.', 50, y);
y += 16;
doc.text('2. High-Power PA+LNA Modules: Draw ~115mA peak each. Add a 10uF-47uF capacitor across VCC & GND of each module to prevent brownouts.', 50, y);
y += 25;

doc.fontSize(8.5).fillColor('#64748b').text('Document generated automatically for CradleGuard 3.0 ESP32 Security Suite.', 40, 780, { align: 'center', width: 515 });

doc.end();
console.log('PDF generated successfully at:', pdfPath);
