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

// ─────────────────────────────────────────────────────────────────────────────
// Section 1: Full Breadboard Wiring Diagram (Centered, High-Res, Perfectly Scaled)
// ─────────────────────────────────────────────────────────────────────────────
doc.fillColor(COLOR_PRIMARY).fontSize(13).font('Helvetica-Bold')
   .text('1. Master Hardware Breadboard Wiring Diagram', 40, y);
y += 4;
doc.moveTo(40, y + 12).lineTo(555, y + 12).strokeColor(COLOR_ACCENT).lineWidth(1.5).stroke();
y += 18;

if (fs.existsSync(imagePath)) {
  // Center a 380x380 image on the page (Page width = 595, margin = 40)
  const imgSize = 370;
  const imgX = 40 + (515 - imgSize) / 2;
  
  // Background card / border for image
  doc.roundedRect(imgX - 4, y - 4, imgSize + 8, imgSize + 8, 6)
     .fillAndStroke('#f8fafc', '#cbd5e1');
  
  doc.image(imagePath, imgX, y, { width: imgSize, height: imgSize });
  y += imgSize + 14;
}

// Component key card cleanly below image
doc.roundedRect(40, y, 515, 125, 6).fillAndStroke('#f8fafc', '#cbd5e1');
doc.rect(40, y, 4, 125).fill(COLOR_ACCENT);

doc.fillColor(COLOR_PRIMARY).fontSize(9.5).font('Helvetica-Bold')
   .text('COMPONENTS SHOWN IN WIRING DIAGRAM:', 54, y + 10);

const compLeft = [
  ['ESP32 30-Pin Dev Board', 'Center of breadboard (30-pin DIP layout)'],
  ['MCUFRIEND 2.4" TFT Shield', 'Left side — 8-bit parallel bus + SPI SD slot'],
  ['NRF24L01 #1 (PCB Ant.)', 'Right top — CE: GPIO 22, CSN: GPIO 21'],
  ['NRF24L01 #2 (PCB Ant.)', 'Right bottom — CE: GPIO 16, CSN: GPIO 15'],
];

const compRight = [
  ['TURN ON Pushbutton', 'Lower left — GPIO 12 & GND (INPUT_PULLUP)'],
  ['TURN OFF Pushbutton', 'Lower right — GPIO 14 & GND (INPUT_PULLUP)'],
  ['BOOT Button (GPIO 0)', 'Built-in on ESP32 board (BACK navigation)'],
  ['Status LED (GPIO 2)', 'Built-in on ESP32 board (Active indicator)'],
];

let ky = y + 28;
doc.fontSize(8).font('Helvetica');

for (let i = 0; i < compLeft.length; i++) {
  // Left column
  doc.fillColor(COLOR_ACCENT).font('Helvetica-Bold').text('• ' + compLeft[i][0] + ':', 54, ky, { continued: true });
  doc.fillColor(COLOR_TEXT).font('Helvetica').text(' ' + compLeft[i][1], { width: 230 });
  
  // Right column
  doc.fillColor(COLOR_ACCENT).font('Helvetica-Bold').text('• ' + compRight[i][0] + ':', 305, ky, { continued: true });
  doc.fillColor(COLOR_TEXT).font('Helvetica').text(' ' + compRight[i][1], { width: 235 });
  
  ky += 22;
}

doc.fontSize(7.5).fillColor('#64748b').text('ESP32 Security Suite Master Pinout Guide • Page 1 of 5', 40, 785, { align: 'center', width: 515 });


// ─────────────────────────────────────────────────────────────────────────────
// PAGE 2 — Wire-by-Wire Color Legend + Full Connection Table
// ─────────────────────────────────────────────────────────────────────────────
doc.addPage();
y = 40;

doc.fillColor(COLOR_PRIMARY).fontSize(14).font('Helvetica-Bold')
   .text('1B. Breadboard Wire-by-Wire Connection Reference', 40, y);
y += 4;
doc.moveTo(40, y + 14).lineTo(555, y + 14).strokeColor(COLOR_ACCENT).lineWidth(1.5).stroke();
y += 22;

// Wire colour legend swatches
doc.fillColor('#334155').fontSize(10).font('Helvetica-Bold').text('Wire Color Key:', 40, y);
y += 14;

const wireColors = [
  ['#dc2626', 'RED',    '3.3V / 5V Power Supply Rail'],
  ['#1d4ed8', 'BLACK',  'GND Ground Rail'],
  ['#2563eb', 'BLUE',   'SPI SCK (Clock) — GPIO 18'],
  ['#7c3aed', 'PURPLE', 'SPI MOSI (Data Out) — GPIO 23'],
  ['#0891b2', 'CYAN',   'SPI MISO (Data In) — GPIO 19'],
  ['#d97706', 'ORANGE', 'NRF CE / CSN control lines'],
  ['#16a34a', 'GREEN',  'TURN ON Button wire — GPIO 12'],
  ['#ca8a04', 'YELLOW', 'TFT control lines (RS, CS, RST, WR)'],
  ['#9333ea', 'VIOLET', 'TURN OFF Button wire — GPIO 14'],
];

wireColors.forEach((w, i) => {
  const col = i % 3;
  const row = Math.floor(i / 3);
  const wx = 40 + col * 175;
  const wy = y + row * 22;
  doc.roundedRect(wx, wy, 14, 14, 2).fill(w[0]);
  doc.fillColor(COLOR_TEXT).fontSize(8).font('Helvetica-Bold')
     .text(w[1], wx + 18, wy + 2);
  doc.fillColor('#64748b').fontSize(7.5).font('Helvetica')
     .text(w[2], wx + 18, wy + 10);
});

y += Math.ceil(wireColors.length / 3) * 22 + 16;

// Full connection table
function drawTable(headers, rows, startY, colWidths) {
  let cy = startY;
  doc.rect(40, cy, 515, 18).fill('#f1f5f9');
  doc.fillColor('#334155').fontSize(9).font('Helvetica-Bold');
  let cx = 45;
  headers.forEach((h, i) => {
    doc.text(h, cx, cy + 5, { width: colWidths[i] });
    cx += colWidths[i];
  });
  cy += 18;
  rows.forEach((row, rowIndex) => {
    if (rowIndex % 2 === 1) doc.rect(40, cy, 515, 16).fill(COLOR_BG_ALT);
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

doc.fillColor(COLOR_PRIMARY).fontSize(11).font('Helvetica-Bold')
   .text('Complete Breadboard Connection Table — All Wires', 40, y);
y += 16;

y = drawTable(
  ['From Component / Pin', 'ESP32 GPIO', 'Wire Color', 'Destination / Rail'],
  [
    ['Breadboard +Rail (3.3V)', '3.3V pin',  'RED',    '→ NRF24 #1 VCC, NRF24 #2 VCC'],
    ['Breadboard −Rail (GND)',  'GND pin',   'BLACK',  '→ NRF24 #1 GND, NRF24 #2 GND, BTN GND legs'],
    ['TFT Shield 5V/3V3',      '5V / 3.3V', 'RED',    '→ Breadboard power rail'],
    ['TFT Shield GND',         'GND',        'BLACK',  '→ Breadboard GND rail'],
    ['SPI Clock (SCK)',         'GPIO 18',   'BLUE',   '→ NRF24 #1 SCK, NRF24 #2 SCK, TFT SD_SCK'],
    ['SPI MOSI',                'GPIO 23',   'PURPLE', '→ NRF24 #1 MOSI, NRF24 #2 MOSI, TFT SD_DI'],
    ['SPI MISO',                'GPIO 19',   'CYAN',   '→ NRF24 #1 MISO, NRF24 #2 MISO, TFT SD_DO'],
    ['NRF24 #1 CE',             'GPIO 22',   'ORANGE', '→ NRF24 #1 CE pin'],
    ['NRF24 #1 CSN',            'GPIO 21',   'ORANGE', '→ NRF24 #1 CSN pin'],
    ['NRF24 #2 CE',             'GPIO 16',   'ORANGE', '→ NRF24 #2 CE pin'],
    ['NRF24 #2 CSN',            'GPIO 15',   'ORANGE', '→ NRF24 #2 CSN pin'],
    ['TFT LCD_RST',             'GPIO 33',   'YELLOW', '→ TFT Shield LCD_RST pin'],
    ['TFT LCD_CS',              'GPIO 5',    'YELLOW', '→ TFT Shield LCD_CS pin'],
    ['TFT LCD_RS (DC)',         'GPIO 4',    'YELLOW', '→ TFT Shield LCD_RS pin'],
    ['TFT LCD_WR',              'GPIO 2',    'YELLOW', '→ TFT Shield LCD_WR pin'],
    ['TFT LCD_RD',              '3.3V Rail', 'RED',    '→ Tie HIGH (no data read needed)'],
    ['TFT LCD_D0',              'GPIO 12',   'BLUE',   '→ TFT Shield D0 (shared w/ TURN ON btn)'],
    ['TFT LCD_D1',              'GPIO 13',   'BLUE',   '→ TFT Shield D1 (shared w/ SD_SS)'],
    ['TFT LCD_D2',              'GPIO 26',   'BLUE',   '→ TFT Shield D2'],
    ['TFT LCD_D3',              'GPIO 25',   'BLUE',   '→ TFT Shield D3'],
    ['TFT LCD_D4',              'GPIO 17',   'BLUE',   '→ TFT Shield D4'],
    ['TFT LCD_D5',              'GPIO 27',   'BLUE',   '→ TFT Shield D5'],
    ['TFT LCD_D6',              'GPIO 14',   'BLUE',   '→ TFT Shield D6 (shared w/ TURN OFF btn)'],
    ['TFT LCD_D7',              'GPIO 32',   'BLUE',   '→ TFT Shield D7'],
    ['SD Card SS (CS)',         'GPIO 13',   'YELLOW', '→ TFT Shield SD_SS'],
    ['TURN ON Pushbutton',      'GPIO 12',   'GREEN',  'Leg 1 → GPIO 12, Leg 2 → GND Rail'],
    ['TURN OFF Pushbutton',     'GPIO 14',   'VIOLET', 'Leg 1 → GPIO 14, Leg 2 → GND Rail'],
    ['BOOT Button',             'GPIO 0',    '—',      'Built-in on ESP32 board (no wire needed)'],
    ['Status LED',              'GPIO 2',    '—',      'Built-in on ESP32 board (no wire needed)'],
  ],
  y,
  [155, 90, 80, 190]
);

// ─────────────────────────────────────────────────────────────────────────────
// PAGE 3 — ESP32 Pin Allocation Box & Hardware Interfaces
// ─────────────────────────────────────────────────────────────────────────────
doc.addPage();
y = 40;

doc.fillColor(COLOR_PRIMARY).fontSize(14).font('Helvetica-Bold').text('2. ESP32 Pin Allocation Overview', 40, y);
y += 4;
doc.moveTo(40, y + 12).lineTo(555, y + 12).strokeColor(COLOR_ACCENT).lineWidth(1.5).stroke();
y += 18;

doc.roundedRect(40, y, 515, 125, 6).fill('#0f172a');
doc.fillColor('#facc15').fontSize(11).font('Helvetica-Bold').text('ESP32 30-PIN DEV BOARD PINOUT', 40, y + 10, { align: 'center', width: 515 });

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

let py = y + 30;
doc.fontSize(8.5).font('Helvetica');
for (let i = 0; i < leftPins.length; i++) {
  doc.fillColor('#cbd5e1').font('Helvetica').text(leftPins[i][0], 65, py);
  doc.fillColor('#38bdf8').font('Helvetica-Bold').text(leftPins[i][1], 170, py);

  doc.fillColor('#38bdf8').font('Helvetica-Bold').text(rightPins[i][0], 330, py);
  doc.fillColor('#cbd5e1').font('Helvetica').text(rightPins[i][1], 410, py);
  py += 11;
}

y += 145;

// Section 3: Shared SPI Bus Table & Wi-Fi Radio on Page 3
doc.fillColor(COLOR_PRIMARY).fontSize(14).font('Helvetica-Bold').text('3. Hardware Interfaces (VSPI & ESP32 Wi-Fi Radio)', 40, y);
y += 4;
doc.moveTo(40, y + 12).lineTo(555, y + 12).strokeColor(COLOR_ACCENT).lineWidth(1.5).stroke();
y += 18;

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

doc.fontSize(7.5).fillColor('#64748b').text('ESP32 Security Suite Master Pinout Guide • Page 3 of 5', 40, 785, { align: 'center', width: 515 });

// ─────────────────────────────────────────────────────────────────────────────
// PAGE 4 — NRF24L01 Radios & 2.4" TFT Display Shield
// ─────────────────────────────────────────────────────────────────────────────
doc.addPage();
y = 40;

// Section 4: Radio 1 & 2 Tables
doc.fillColor(COLOR_PRIMARY).fontSize(14).font('Helvetica-Bold').text('4. Radio Transceiver Modules (NRF24L01)', 40, y);
y += 4;
doc.moveTo(40, y + 12).lineTo(555, y + 12).strokeColor(COLOR_ACCENT).lineWidth(1.5).stroke();
y += 16;

doc.fillColor('#0f172a').fontSize(9.5).font('Helvetica-Bold').text('Radio #1 (Lower Channels 2402–2440 MHz):', 40, y);
y += 12;

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

y += 10;
doc.fillColor('#0f172a').fontSize(9.5).font('Helvetica-Bold').text('Radio #2 (Upper Channels 2441–2480 MHz):', 40, y);
y += 12;

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

y += 14;

// Section 5: Display (MCUFRIEND 2.4" TFT LCD Shield - 8-Bit Parallel)
doc.fillColor(COLOR_PRIMARY).fontSize(14).font('Helvetica-Bold').text('5. MCUFRIEND 2.4" TFT LCD Shield (8-Bit Parallel)', 40, y);
y += 4;
doc.moveTo(40, y + 12).lineTo(555, y + 12).strokeColor(COLOR_ACCENT).lineWidth(1.5).stroke();
y += 14;

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
    ['LCD_D7', 'GPIO 32', 'Data Bit 7', '8-Bit Parallel'],
    ['SD_SS (CS)', 'GPIO 13', 'SD Card Chip Select', 'SPI CS'],
    ['SD_DI (MOSI)', 'GPIO 23', 'SD SPI Data In', 'Shared SPI'],
    ['SD_DO (MISO)', 'GPIO 19', 'SD SPI Data Out', 'Shared SPI'],
    ['SD_SCK (SCK)', 'GPIO 18', 'SD SPI Clock', 'Shared SPI']
  ],
  y,
  [100, 120, 160, 135]
);

doc.fontSize(7.5).fillColor('#64748b').text('ESP32 Security Suite Master Pinout Guide • Page 4 of 5', 40, 785, { align: 'center', width: 515 });

// ─────────────────────────────────────────────────────────────────────────────
// PAGE 5 — Pushbuttons & Power Recommendations
// ─────────────────────────────────────────────────────────────────────────────
doc.addPage();
y = 40;

// Section 6: Push Buttons
doc.fillColor(COLOR_PRIMARY).fontSize(14).font('Helvetica-Bold').text('6. Hardware Pushbuttons & System Controls', 40, y);
y += 4;
doc.moveTo(40, y + 12).lineTo(555, y + 12).strokeColor(COLOR_ACCENT).lineWidth(1.5).stroke();
y += 16;

y = drawTable(
  ['Button / LED', 'ESP32 Pin', 'Wiring Connection', 'Mode'],
  [
    ['TURN ON Button', 'GPIO 12', 'One leg to GPIO 12, other leg to GND', 'INPUT_PULLUP'],
    ['TURN OFF Button', 'GPIO 14', 'One leg to GPIO 14, other leg to GND', 'INPUT_PULLUP'],
    ['BOOT Button', 'GPIO 0', 'Built-in BOOT pushbutton on ESP32 board', 'Internal (BACK Key)'],
    ['Status LED', 'GPIO 2', 'Built-in LED on ESP32 board', 'OUTPUT (Active Status)']
  ],
  y,
  [120, 90, 205, 100]
);

y += 20;

// Section 7: Power & Capacitor Notes
doc.fillColor(COLOR_PRIMARY).fontSize(14).font('Helvetica-Bold').text('7. Power Supply & Capacitor Recommendations', 40, y);
y += 4;
doc.moveTo(40, y + 12).lineTo(555, y + 12).strokeColor(COLOR_ACCENT).lineWidth(1.5).stroke();
y += 16;

// Power Cards
doc.roundedRect(40, y, 515, 60, 4).fillAndStroke('#f8fafc', '#cbd5e1');
doc.rect(40, y, 4, 60).fill('#16a34a');
doc.fillColor('#16a34a').fontSize(9).font('Helvetica-Bold').text('STANDARD PCB ANTENNA MODULES (RECOMMENDED):', 52, y + 10);
doc.fillColor(COLOR_TEXT).fontSize(8.5).font('Helvetica').text('Draw ~12mA each. Can be powered directly from ESP32 3.3V rail without external capacitors. No decoupling required for typical breadboard setups.', 52, y + 26, { width: 490 });

y += 72;

doc.roundedRect(40, y, 515, 60, 4).fillAndStroke('#f8fafc', '#cbd5e1');
doc.rect(40, y, 4, 60).fill('#d97706');
doc.fillColor('#d97706').fontSize(9).font('Helvetica-Bold').text('HIGH-POWER PA+LNA MODULES (EXTERNAL ANTENNA):', 52, y + 10);
doc.fillColor(COLOR_TEXT).fontSize(8.5).font('Helvetica').text('Draw up to ~115mA peak each. Requires 10µF to 47µF electrolytic or ceramic capacitor soldered directly across VCC and GND pins of each NRF module to prevent brownouts.', 52, y + 26, { width: 490 });

y += 76;

// Quick Hardware Checklist Box
doc.roundedRect(40, y, 515, 100, 4).fillAndStroke('#eff6ff', '#bfdbfe');
doc.fillColor('#1e40af').fontSize(9.5).font('Helvetica-Bold').text('HARDWARE ASSEMBLY CHECKLIST', 52, y + 10);
const checklist = [
  'Common Ground (GND): Ensure all grounds (ESP32, TFT, NRF24 #1, NRF24 #2, Buttons) are tied together.',
  'Power Supply: Use a dedicated 5V 2A USB power supply or battery pack during dual radio transmission.',
  'SD Card Format: FAT32 with /wordlist.txt in root directory for offline Wi-Fi password auditing.',
  'Touchscreen Calibration: Use MCUFRIEND TouchScreen library XP=8, YP=A3, XM=A2, YM=9 (300 ohm).'
];
let cyCheck = y + 26;
doc.fontSize(8).font('Helvetica');
checklist.forEach((item, idx) => {
  doc.fillColor(COLOR_ACCENT).text('✓', 52, cyCheck);
  doc.fillColor(COLOR_TEXT).text(item, 66, cyCheck, { width: 475 });
  cyCheck += 16;
});

doc.fontSize(7.5).fillColor('#64748b').text('ESP32 Security Suite Master Pinout Guide • Page 5 of 5 • Generated for CradleGuard 3.0', 40, 785, { align: 'center', width: 515 });


doc.end();
console.log('PDF generated successfully at:', pdfPath);
