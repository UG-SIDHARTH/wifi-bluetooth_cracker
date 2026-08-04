const PDFDocument = require('pdfkit');
const fs = require('fs');
const path = require('path');

const pdfPath = path.join(__dirname, 'ESP32_UI_Design_Model.pdf');
const artifactDir = 'C:/Users/Lenovo/.gemini/antigravity-ide/brain/05788616-5a0f-446c-994d-73f52ab0c9aa';

const imgMainMenu   = path.join(artifactDir, 'screen_main_menu_1785855024769.png');
const imgWifiSub    = path.join(artifactDir, 'screen_wifi_submenu_1785855043955.png');
const imgSelectWifi = path.join(artifactDir, 'screen_select_wifi_1785855063794.png');

const doc = new PDFDocument({ margin: 40, size: 'A4' });
doc.pipe(fs.createWriteStream(pdfPath));

// ── Colors ──────────────────────────────────────────────────────────────
const C_BG       = '#0f172a';
const C_HEADER   = '#1e3a8a';
const C_TEXT     = '#1e293b';
const C_MUTED    = '#64748b';
const C_ACCENT   = '#2563eb';
const C_GREEN    = '#16a34a';
const C_RED      = '#dc2626';
const C_YELLOW   = '#ca8a04';
const C_PURPLE   = '#7c3aed';
const C_TEAL     = '#0d9488';
const C_CARD     = '#1e293b';
const C_ALT      = '#f8fafc';

// ── Helper: section title ────────────────────────────────────────────────
function sectionTitle(text, y) {
  doc.fillColor(C_TEXT).fontSize(15).font('Helvetica-Bold').text(text, 40, y);
  doc.moveTo(40, y + 20).lineTo(555, y + 20).strokeColor(C_ACCENT).lineWidth(2).stroke();
  return y + 28;
}

// ── Helper: draw a TFT frame border ─────────────────────────────────────
function tftFrame(x, y, w, h, label) {
  // Outer device bezel
  doc.roundedRect(x - 6, y - 6, w + 12, h + 22, 5).fillAndStroke('#2d3748', '#94a3b8');
  // Screen area
  doc.rect(x, y, w, h).fill('#0f172a');
  // Label below
  if (label) {
    doc.fillColor(C_MUTED).fontSize(8).font('Helvetica')
       .text(label, x, y + h + 8, { width: w, align: 'center' });
  }
}

// ── Helper: small screen header bar ─────────────────────────────────────
function screenHeader(x, y, w, title, showBack) {
  doc.rect(x, y, w, 16).fill(C_HEADER);
  doc.fillColor('#ffffff').fontSize(7).font('Helvetica-Bold');
  if (showBack) {
    doc.text('< ' + title, x + 4, y + 5, { width: w - 8 });
  } else {
    doc.text(title, x + 4, y + 5, { width: w - 8, align: 'center' });
  }
}

// ── Helper: mini rounded button ──────────────────────────────────────────
function miniBtn(x, y, w, h, color, textColor, label, fontSize) {
  doc.roundedRect(x, y, w, h, 3).fill(color);
  doc.fillColor(textColor).fontSize(fontSize || 7).font('Helvetica-Bold')
     .text(label, x + 2, y + (h / 2) - ((fontSize || 7) / 2), { width: w - 4, align: 'center' });
}

// ── Helper: annotation arrow ─────────────────────────────────────────────
function annotate(x1, y1, x2, y2, text) {
  doc.moveTo(x1, y1).lineTo(x2, y2).strokeColor(C_ACCENT).lineWidth(1).stroke();
  doc.circle(x1, y1, 2).fill(C_ACCENT);
  doc.fillColor(C_ACCENT).fontSize(7).font('Helvetica').text(text, x2 + 2, y2 - 4, { width: 100 });
}

// ════════════════════════════════════════════════════════════════════════
// PAGE 1  —  Cover + Navigation Flow
// ════════════════════════════════════════════════════════════════════════
doc.rect(40, 40, 515, 70).fill(C_BG);
doc.fillColor('#38bdf8').fontSize(20).font('Helvetica-Bold')
   .text('ESP32 SECURITY SUITE', 55, 52);
doc.fillColor('#93c5fd').fontSize(11).font('Helvetica')
   .text('TFT Display UI Design Model  —  CradleGuard 3.0', 55, 76);
doc.fillColor(C_MUTED).fontSize(9)
   .text('2.4" MCUFRIEND TFT LCD  |  320 × 240 px  |  Landscape', 55, 94);

// Warning
doc.rect(40, 120, 515, 30).fill('#fef2f2');
doc.rect(40, 120, 4, 30).fill('#ef4444');
doc.fillColor('#991b1b').fontSize(8).font('Helvetica-Bold')
   .text('LEGAL DISCLAIMER:', 50, 126);
doc.font('Helvetica').fontSize(7.5)
   .text('This system is strictly for authorized security research and educational use in controlled environments.', 50, 137, { width: 500 });

let y = 160;
y = sectionTitle('Screen Navigation Flow', y);

// ── Flow diagram boxes ───────────────────────────────────────────────────
const bw = 110, bh = 30, gap = 14;
const row1y = y + 10;

// Boot animation
doc.roundedRect(222, row1y, bw, bh, 4).fillAndStroke('#334155', '#64748b');
doc.fillColor('#facc15').fontSize(8).font('Helvetica-Bold')
   .text('BOOT ANIMATION', 222, row1y + 10, { width: bw, align: 'center' });

// Arrow down
doc.moveTo(277, row1y + bh).lineTo(277, row1y + bh + 18)
   .strokeColor(C_ACCENT).lineWidth(1.5).stroke();
doc.polygon([272, row1y + bh + 18], [282, row1y + bh + 18], [277, row1y + bh + 24])
   .fill(C_ACCENT);

const row2y = row1y + bh + 24;
// Main menu
doc.roundedRect(222, row2y, bw, bh, 4).fillAndStroke(C_HEADER, C_ACCENT);
doc.fillColor('#ffffff').fontSize(8).font('Helvetica-Bold')
   .text('MAIN MENU', 222, row2y + 10, { width: bw, align: 'center' });

const row3y = row2y + bh + 22;
const col1x = 60, col2x = 222, col3x = 385;

// Arrow to WIFI SCANNER
doc.moveTo(277, row2y + bh).lineTo(277, row3y - 5)
   .strokeColor('#94a3b8').lineWidth(1).stroke();
// Branch left
doc.moveTo(277, row3y - 5).lineTo(col1x + bw/2, row3y - 5)
   .strokeColor('#94a3b8').lineWidth(1).stroke();
// Branch right  
doc.moveTo(277, row3y - 5).lineTo(col3x + bw/2, row3y - 5)
   .strokeColor('#94a3b8').lineWidth(1).stroke();
// Arrows down from branches
[[col1x, C_GREEN], [col2x, '#b45309'], [col3x, C_RED]].forEach(([cx, col]) => {
  doc.moveTo(cx + bw/2, row3y - 5).lineTo(cx + bw/2, row3y)
     .strokeColor('#94a3b8').lineWidth(1).stroke();
  doc.polygon([cx + bw/2 - 4, row3y], [cx + bw/2 + 4, row3y], [cx + bw/2, row3y + 5])
     .fill('#94a3b8');
});

// Boxes row 3
[
  [col1x, '1. WIFI SCANNER', C_GREEN],
  [col2x, '2. WIFI+BT JAMMER', '#b45309'],
  [col3x, '3. RESET', '#334155']
].forEach(([cx, lbl, col]) => {
  doc.roundedRect(cx, row3y, bw, bh, 4).fillAndStroke(col, '#94a3b8');
  doc.fillColor('#ffffff').fontSize(7).font('Helvetica-Bold')
     .text(lbl, cx, row3y + 10, { width: bw, align: 'center' });
});

// WIFI SCANNER sub-branches
const row4y = row3y + bh + 20;
doc.moveTo(col1x + bw/2, row3y + bh).lineTo(col1x + bw/2, row4y - 5)
   .strokeColor('#94a3b8').lineWidth(1).stroke();
doc.moveTo(col1x + bw/2, row4y - 5).lineTo(col1x + 20, row4y - 5)
   .strokeColor('#94a3b8').lineWidth(1).stroke();
doc.moveTo(col1x + bw/2, row4y - 5).lineTo(col1x + bw - 20, row4y - 5)
   .strokeColor('#94a3b8').lineWidth(1).stroke();

const sw = 90, sh = 28;
const s1x = col1x - 10, s2x = col1x + 30;
[[s1x, '1.1 SELECT WIFI', C_TEAL], [s2x + sw + 4, '1.2 FIND PASSWORD', C_PURPLE]].forEach(([sx, lbl, col]) => {
  doc.moveTo(sx + sw/2, row4y - 5).lineTo(sx + sw/2, row4y)
     .strokeColor('#94a3b8').lineWidth(1).stroke();
  doc.polygon([sx + sw/2 - 3, row4y], [sx + sw/2 + 3, row4y], [sx + sw/2, row4y + 4])
     .fill('#94a3b8');
  doc.roundedRect(sx, row4y, sw, sh, 4).fillAndStroke(col, '#94a3b8');
  doc.fillColor('#ffffff').fontSize(6.5).font('Helvetica-Bold')
     .text(lbl, sx, row4y + 9, { width: sw, align: 'center' });
});

// Reset sub
doc.moveTo(col3x + bw/2, row3y + bh).lineTo(col3x + bw/2, row4y)
   .strokeColor('#94a3b8').lineWidth(1).stroke();
doc.roundedRect(col3x + 5, row4y, bw - 10, sh, 4).fillAndStroke('#991b1b', '#94a3b8');
doc.fillColor('#ffffff').fontSize(6.5).font('Helvetica-Bold')
   .text('CONFIRM (YES/NO)', col3x + 5, row4y + 9, { width: bw - 10, align: 'center' });

// BOOT = back legend
y = row4y + sh + 20;
doc.rect(40, y, 515, 28).fill('#f1f5f9');
doc.fillColor(C_TEXT).fontSize(9).font('Helvetica-Bold').text('Navigation Controls:', 50, y + 7);
doc.font('Helvetica').fontSize(8)
   .text('BOOT button (GPIO 0) = GO BACK one level     |     Tap header "< ..." = GO BACK     |     Physical ON btn = context action', 170, y + 9, { width: 380 });

// ════════════════════════════════════════════════════════════════════════
// PAGE 2  —  Screen Mockups with Photos (Main Menu, WiFi Sub, Select WiFi)
// ════════════════════════════════════════════════════════════════════════
doc.addPage();
y = 40;
y = sectionTitle('Screen Designs — Rendered Mockups', y);

const SW = 200, SH = 150; // scaled screen size for PDF

// ── Row 1: Main Menu + WiFi Sub ──────────────────────────────────────────
y += 6;
doc.fillColor(C_TEXT).fontSize(10).font('Helvetica-Bold').text('Screen 1: Main Menu', 40, y);
doc.fillColor(C_TEXT).fontSize(10).font('Helvetica-Bold').text('Screen 2: Wi-Fi Scanner Sub-Menu', 290, y);
y += 14;

if (fs.existsSync(imgMainMenu)) {
  doc.image(imgMainMenu, 40, y, { width: SW, height: SH });
}
if (fs.existsSync(imgWifiSub)) {
  doc.image(imgWifiSub, 290, y, { width: SW, height: SH });
}

// Annotations for Screen 1
doc.fillColor(C_MUTED).fontSize(7.5).font('Helvetica');
const ann1 = [
  [40,  y + SH + 8,  'Header: "CRADLEGUARD 3.0" (Blue bar)'],
  [40,  y + SH + 18, 'Button 1: WIFI SCANNER  →  Dark Green  →  GPIO 12 tap or touch'],
  [40,  y + SH + 28, 'Button 2: WIFI+BT JAMMER  →  Dark Red  →  Activates dual NRF24'],
  [40,  y + SH + 38, 'Button 3: RESET  →  Slate Blue  →  Calls ESP.restart()'],
];
ann1.forEach(([ax, ay, txt]) => {
  doc.text('•  ' + txt, ax, ay, { width: 230 });
});

// Annotations for Screen 2
const ann2 = [
  [290, y + SH + 8,  '"< WIFI SCANNER" header  →  tap to go back'],
  [290, y + SH + 18, 'Button 1.1: SELECT WIFI  →  Teal  →  Scans & lists APs'],
  [290, y + SH + 28, 'Button 1.2: FIND PASSWORD  →  Violet  →  Runs wordlist audit'],
  [290, y + SH + 38, 'BOOT button (GPIO 0) returns to Main Menu'],
];
ann2.forEach(([ax, ay, txt]) => {
  doc.text('•  ' + txt, ax, ay, { width: 235 });
});

y += SH + 60;

// ── Row 2: Select WiFi ────────────────────────────────────────────────────
doc.fillColor(C_TEXT).fontSize(10).font('Helvetica-Bold').text('Screen 3: 1.1 Select Wi-Fi Network', 40, y);
y += 14;

if (fs.existsSync(imgSelectWifi)) {
  doc.image(imgSelectWifi, 40, y, { width: SW, height: SH });
}

const ann3 = [
  'Header "< SELECT WIFI"  →  tap to return to sub-menu',
  'List rows: tap any row to highlight/select that network',
  'Highlighted row  →  filled Cyan, text dark',
  'SCAN button  →  Green, triggers scanWiFiNetworks()',
  'CONFIRM CHOICE  →  Cyan, saves selectedTargetIndex & goes back',
  'Physical TURN ON (GPIO 12) also triggers a new scan',
];
doc.fillColor(C_MUTED).fontSize(7.5).font('Helvetica');
ann3.forEach((txt, i) => {
  doc.text('•  ' + txt, 40, y + SH + 8 + i * 10, { width: 235 });
});

// ════════════════════════════════════════════════════════════════════════
// PAGE 3  —  Drawn Mockups: Find Password, Jammer, Reset
// ════════════════════════════════════════════════════════════════════════
doc.addPage();
y = 40;
y = sectionTitle('Screen Designs — Diagram Mockups', y);

// ── FUNCTION: draw a screen panel ────────────────────────────────────────
const PW = 160, PH = 120;

function drawScreenPanel(px, py, title, showBack, drawFn, label, notes) {
  // Bezel
  doc.roundedRect(px - 5, py - 5, PW + 10, PH + 20, 4)
     .fillAndStroke('#2d3748', '#475569');
  // Screen bg
  doc.rect(px, py, PW, PH).fill('#0f172a');
  // Header
  doc.rect(px, py, PW, 14).fill(C_HEADER);
  doc.fillColor('#fff').fontSize(6.5).font('Helvetica-Bold')
     .text((showBack ? '< ' : '') + title, px + 3, py + 4, { width: PW - 6 });
  // Custom content
  drawFn(px, py + 14);
  // Panel label
  doc.fillColor(C_TEXT).fontSize(8).font('Helvetica-Bold')
     .text(label, px - 5, py + PH + 8, { width: PW + 10, align: 'center' });
  // Notes
  if (notes && notes.length) {
    doc.fillColor(C_MUTED).fontSize(7).font('Helvetica');
    notes.forEach((n, i) => {
      doc.text('• ' + n, px - 5, py + PH + 22 + i * 9, { width: PW + 10 });
    });
  }
}

// ── Screen 4: Find Password ───────────────────────────────────────────────
y += 10;
doc.fillColor(C_TEXT).fontSize(10).font('Helvetica-Bold')
   .text('Screen 4: 1.2 Find Password / Dictionary Audit', 40, y);
y += 14;

drawScreenPanel(40, y, 'FIND PASSWORD', true, (sx, sy) => {
  // Target AP row
  doc.fillColor('#64748b').fontSize(5.5).font('Helvetica').text('Target AP:', sx + 4, sy + 3);
  doc.fillColor('#38bdf8').text('HomeNetwork', sx + 48, sy + 3);
  // Status box
  doc.rect(sx + 2, sy + 14, PW - 4, 28).fill('#1e293b');
  doc.fillColor('#94a3b8').fontSize(5).text('Status:', sx + 5, sy + 17);
  doc.fillColor('#22c55e').text('Ready for Dictionary Test', sx + 5, sy + 24);
  doc.fillColor('#94a3b8').text('Dictionary: SD /wordlist.txt | Internal: 10 keys', sx + 5, sy + 33);
  // Instructions
  doc.fillColor('#eab308').fontSize(5).text('Tap START to run audit engine.', sx + 4, sy + 47);
  doc.fillColor('#94a3b8').text('Uses wordlist to test passphrases.', sx + 4, sy + 54);
  doc.text('Results saved to SD /cracked_keys.txt', sx + 4, sy + 61);
  // START button
  doc.roundedRect(sx + 4, sy + 72, PW - 8, 18, 3).fill('#7c3aed');
  doc.fillColor('#fff').fontSize(6.5).font('Helvetica-Bold')
     .text('>> START AUDIT', sx + 4, sy + 77, { width: PW - 8, align: 'center' });
  // Back hint
  doc.fillColor('#475569').fontSize(4.5).font('Helvetica')
     .text('BOOT / tap header to go back', sx + 4, sy + 96);
}, 'Screen 4: Find Password', [
  'Tap START AUDIT → calls runDictionaryCrack()',
  'Result shows in Status box (green = found)',
  'cracked_keys.txt written to SD card'
]);

// ── Screen 5: Jammer ─────────────────────────────────────────────────────
drawScreenPanel(240, y, 'WIFI+BT JAMMER', true, (sx, sy) => {
  // Warning bar
  doc.rect(sx + 2, sy + 2, PW - 4, 13).fill('#7f1d1d');
  doc.fillColor('#fbbf24').fontSize(5).font('Helvetica-Bold')
     .text('! FOR AUTHORIZED/LAB USE ONLY !', sx + 2, sy + 5, { width: PW - 4, align: 'center' });
  // Status box
  doc.rect(sx + 2, sy + 18, PW - 4, 28).fill('#1e293b');
  doc.fillColor('#94a3b8').fontSize(5).font('Helvetica').text('MODE:', sx + 5, sy + 22);
  doc.fillColor('#22c55e').text('IDLE', sx + 32, sy + 22);
  doc.fillColor('#94a3b8').text('Total RF hops transmitted:', sx + 5, sy + 30);
  doc.fillColor('#38bdf8').text('0', sx + 5, sy + 38);
  // ON button
  doc.roundedRect(sx + 2, sy + 50, 70, 18, 3).fill('#16a34a');
  doc.fillColor('#fff').fontSize(6).font('Helvetica-Bold')
     .text('JAMMER ON', sx + 2, sy + 55, { width: 70, align: 'center' });
  // OFF button
  doc.roundedRect(sx + 76, sy + 50, 80, 18, 3).fill('#dc2626');
  doc.fillColor('#fff').text('JAMMER OFF', sx + 76, sy + 55, { width: 80, align: 'center' });
  // Coverage
  doc.fillColor('#94a3b8').fontSize(4.5).font('Helvetica')
     .text('Radio 1: 2402-2440 MHz  (BLE ch 0-38)', sx + 4, sy + 74);
  doc.text('Radio 2: 2441-2480 MHz  (BLE ch 39-79)', sx + 4, sy + 81);
  doc.fillColor('#475569').text('BOOT / tap header to return to menu', sx + 4, sy + 91);
}, 'Screen 5: WiFi+BT Jammer', [
  'JAMMER ON → actionActive = true, LED on',
  'JAMMER OFF → actionActive = false',
  'RF hops counter updates live every 500ms'
]);

// ── Screen 6: Reset ───────────────────────────────────────────────────────
drawScreenPanel(440, y, 'RESET DEVICE', true, (sx, sy) => {
  // Title
  doc.fillColor('#fbbf24').fontSize(7.5).font('Helvetica-Bold')
     .text('CONFIRM RESET?', sx + 2, sy + 10, { width: PW - 4, align: 'center' });
  // Description
  doc.fillColor('#94a3b8').fontSize(5).font('Helvetica')
     .text('This will restart the ESP32.', sx + 8, sy + 26);
  doc.text('All running scans will stop.', sx + 8, sy + 33);
  // YES button
  doc.roundedRect(sx + 6, sy + 46, 58, 22, 4).fill('#dc2626');
  doc.fillColor('#fff').fontSize(8).font('Helvetica-Bold')
     .text('YES', sx + 6, sy + 51, { width: 58, align: 'center' });
  // NO button
  doc.roundedRect(sx + 72, sy + 46, 80, 22, 4).fill('#16a34a');
  doc.fillColor('#0f172a').fontSize(8).text('NO', sx + 72, sy + 51, { width: 80, align: 'center' });
  // hint
  doc.fillColor('#475569').fontSize(4.5).font('Helvetica')
     .text('NO or BOOT to cancel', sx + 4, sy + 76);
}, 'Screen 6: Reset', [
  'YES → ESP.restart() after 1.5s delay',
  'NO / BOOT → back to Main Menu',
  'Displays "RESTARTING..." before reset'
]);

// ════════════════════════════════════════════════════════════════════════
// PAGE 4  —  Interaction Map & Control Reference
// ════════════════════════════════════════════════════════════════════════
doc.addPage();
y = 40;
y = sectionTitle('Touch Zone Map & Control Reference', y);

// Interaction table
y += 10;
function drawTable2(headers, rows, sy, colW) {
  let cy = sy;
  // Header row
  doc.rect(40, cy, 515, 18).fill('#f1f5f9');
  doc.fillColor('#334155').fontSize(9).font('Helvetica-Bold');
  let cx = 45;
  headers.forEach((h, i) => { doc.text(h, cx, cy + 5, { width: colW[i] }); cx += colW[i]; });
  cy += 18;
  rows.forEach((row, ri) => {
    if (ri % 2 === 1) doc.rect(40, cy, 515, 16).fill('#f8fafc');
    doc.fillColor(C_TEXT).fontSize(8).font('Helvetica');
    cx = 45;
    row.forEach((cell, i) => {
      if (i === 0) doc.font('Helvetica-Bold').fillColor(C_ACCENT);
      else doc.font('Helvetica').fillColor(C_TEXT);
      doc.text(cell, cx, cy + 4, { width: colW[i] });
      cx += colW[i];
    });
    cy += 16;
  });
  return cy;
}

y = drawTable2(
  ['Screen', 'Touch Zone / Action', 'ESP32 Event Triggered', 'GPIO / Button'],
  [
    ['Main Menu', 'Tap WIFI SCANNER row', 'setScreen(SCREEN_WIFI_SUBMENU)', 'Touch'],
    ['Main Menu', 'Tap WIFI+BT JAMMER row', 'setMode(BLE) + setScreen(SCREEN_JAMMER)', 'Touch'],
    ['Main Menu', 'Tap RESET row', 'setScreen(SCREEN_RESET)', 'Touch'],
    ['WiFi Sub', 'Tap 1.1 SELECT WIFI', 'setScreen(SCREEN_WIFI_SELECT) + scan', 'Touch'],
    ['WiFi Sub', 'Tap 1.2 FIND PASSWORD', 'setMode(WIFI_CRACK) + crack screen', 'Touch'],
    ['WiFi Sub', 'Tap header "<"', 'setScreen(SCREEN_MAIN_MENU)', 'Touch / BOOT btn'],
    ['Select WiFi', 'Tap a network row', 'selectedTargetIndex = i; redraw', 'Touch'],
    ['Select WiFi', 'Tap >> SCAN', 'scanWiFiNetworks(); redraw list', 'Touch / GPIO 12'],
    ['Select WiFi', 'Tap CONFIRM CHOICE', 'setScreen(SCREEN_WIFI_SUBMENU)', 'Touch'],
    ['Find Password', 'Tap >> START AUDIT', 'runDictionaryCrack(); redraw', 'Touch / GPIO 12'],
    ['Find Password', 'Tap header "<"', 'setScreen(SCREEN_WIFI_SUBMENU)', 'Touch / BOOT btn'],
    ['Jammer', 'Tap JAMMER ON', 'actionActive = true; LED HIGH', 'Touch / GPIO 12'],
    ['Jammer', 'Tap JAMMER OFF', 'actionActive = false; LED LOW', 'Touch / GPIO 14'],
    ['Jammer', 'Tap header "<"', 'setScreen(SCREEN_MAIN_MENU)', 'Touch / BOOT btn'],
    ['Reset', 'Tap YES', 'delay(1500) + ESP.restart()', 'Touch'],
    ['Reset', 'Tap NO', 'setScreen(SCREEN_MAIN_MENU)', 'Touch / BOOT btn'],
  ],
  y, [80, 160, 180, 95]
);

y += 20;
y = sectionTitle('Physical Button Mapping', y);
y += 6;

y = drawTable2(
  ['Button', 'GPIO Pin', 'Wiring', 'Context Action'],
  [
    ['TURN ON Button', 'GPIO 12', 'One leg → GPIO 12, other → GND (INPUT_PULLUP)', 'Scan / Start Audit / Jammer ON'],
    ['TURN OFF Button', 'GPIO 14', 'One leg → GPIO 14, other → GND (INPUT_PULLUP)', 'Jammer OFF / Stop Action'],
    ['BOOT Button', 'GPIO 0', 'Built-in ESP32 BOOT pushbutton', 'Navigate BACK one screen level'],
    ['Status LED', 'GPIO 2', 'Built-in ESP32 LED', 'HIGH when jammer/action is active'],
  ],
  y, [90, 70, 220, 135]
);

y += 25;
// Color legend
y = sectionTitle('UI Color Palette Reference', y);
y += 8;

const colors = [
  ['Deep Navy (BG)',    '#0f172a', 'Screen background'],
  ['Header Blue',      '#1e3a8a', 'Top header bars'],
  ['Dark Green',       '#07C000', 'WIFI SCANNER button / JAMMER ON'],
  ['Dark Red',         '#A00000', 'WIFI+BT JAMMER button / JAMMER OFF'],
  ['Teal',             '#03EF00', 'SELECT WIFI button'],
  ['Violet',           '#8010FF', 'FIND PASSWORD / START AUDIT button'],
  ['Slate',            '#2945FF', 'RESET button background'],
  ['Cyan Accent',      '#00FFFF', 'Selected items / CONFIRM button'],
  ['Yellow Warning',   '#FFE000', 'Warning bars and instruction text'],
];

colors.forEach((c, i) => {
  const cx = 40 + (i % 3) * 175;
  const cy = y + Math.floor(i / 3) * 22;
  doc.roundedRect(cx, cy, 14, 14, 2).fill(c[1]);
  doc.rect(cx + 14, cy, 1, 14).fill('#e2e8f0');
  doc.fillColor(C_TEXT).fontSize(8).font('Helvetica-Bold').text(c[0], cx + 18, cy + 2);
  doc.fillColor(C_MUTED).fontSize(7).font('Helvetica').text(c[2], cx + 18, cy + 10);
});

// Footer
doc.fontSize(8).fillColor(C_MUTED)
   .text('CradleGuard 3.0 UI Design Model — ESP32 TFT Display (320×240 Landscape) — BluetoothJammer.ino',
         40, 780, { align: 'center', width: 515 });

doc.end();
console.log('UI Design PDF generated:', pdfPath);
