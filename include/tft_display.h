#ifndef TFT_DISPLAY_H
#define TFT_DISPLAY_H

#include "config.h"
#include "app_state.h"
#include "buttons.h"
#include <math.h>
#include <cstring>

// 配色（RGB565）
#define C_BG        0x10A2   // 页面底
#define C_TELEM_BG  0x1928   // 遥测整条统一底
#define C_CARD      0x1B2E
#define C_TEXT      0xEF7D   // 近白
#define C_MUTED     0x8430   // 标签灰
#define C_ACCENT    0x07FF   // 青（电压）
#define C_WARN      0xFE60   // 黄（模式）
#define C_OK        0x07E0   // 绿（GPS / ARM）
#define C_ARM_DIS   0xFBE0   // 橙红（未解锁）
// OP/SYS：户外高对比（纯白 / 亮黄绿 / 亮黄）
#define C_OP_LABEL  0x07FF   // 亮青标签
#define C_OP_TEXT   0xFFFF   // 纯白正文
#define C_SYS_LABEL 0xFFE0   // 纯黄标签
#define C_SYS_TEXT  0xFFE0   // 纯黄正文（阳光下最醒目）
#define C_BLACK     0x0000
#define C_ORANGE    0xFD20
#define C_CYAN      0x07FF
#define C_GRAY      0x8410
#define C_LBLUE     0x7D7C
#define C_DKGRAY    0x18E3

#ifndef TFT_BLACK
#define TFT_BLACK   C_BLACK
#define TFT_WHITE   C_TEXT
#define TFT_YELLOW  C_WARN
#define TFT_CYAN    C_CYAN
#define TFT_GREEN   C_OK
#define TFT_RED     0xF800
#define TFT_ORANGE  C_ORANGE
#define TFT_DKGRAY  C_DKGRAY
#define TFT_GRAY    C_GRAY
#define TFT_BLUE    0x2A5F
#define TFT_LBLUE   C_LBLUE
#endif

static bool g_dispWasLoading = true;
static uint8_t g_msgScrollIdx = 0;
static unsigned long g_msgScrollAt = 0;
static String g_lastOpDrawn;
static String g_lastSysDrawn;
static uint8_t g_lastScrollIdx = 255;
static int g_msgAreaY = 80;
static unsigned long g_lastMsgPaintMs = 0;

inline int tftCharsPerLine(uint8_t textSize) {
  int cw = 6 * (int)textSize;
  if (cw < 1) cw = 1;
  return (SCREEN_WIDTH - 16) / cw;
}

inline void tftFillMsgLines(const String& src, String* lines, int maxLines, int& outCount, int charsPerLine) {
  outCount = 0;
  String s = src;
  s.replace('\r', ' ');
  int start = 0;
  while (start <= (int)s.length() && outCount < maxLines) {
    int nl = s.indexOf('\n', start);
    String seg = (nl < 0) ? s.substring(start) : s.substring(start, nl);
    seg.trim();
    if (seg.length() == 0) seg = " ";
    while ((int)seg.length() > 0 && outCount < maxLines) {
      if ((int)seg.length() <= charsPerLine) {
        lines[outCount++] = seg;
        break;
      }
      lines[outCount++] = seg.substring(0, charsPerLine);
      seg = seg.substring(charsPerLine);
    }
    if (nl < 0) break;
    start = nl + 1;
  }
  if (outCount == 0) {
    lines[0] = "--";
    outCount = 1;
  }
}

inline String tftPadLine(const String& s, int width) {
  String out = s;
  while ((int)out.length() < width) out += ' ';
  if ((int)out.length() > width) out = out.substring(0, width);
  return out;
}

inline void drawTelemCell(int x, int y, const char* label, const String& value, uint16_t valueColor) {
  display.setTextSize(1);
  display.setTextColor(C_MUTED, C_TELEM_BG);
  display.setCursor(x, y);
  display.print(label);
  display.setTextSize(2);
  display.setTextColor(valueColor, C_TELEM_BG);
  display.setCursor(x, y + 16);
  display.print(tftPadLine(value, 7));
}

inline void drawLoadingScreen(bool forceFull) {
  static uint8_t frame = 0;
  static unsigned long lastAnim = 0;
  static long lastBaudShown = -1;
  static int lastRx = -1, lastTx = -1;
  bool tick = false;
  if (millis() - lastAnim >= 60) {
    lastAnim = millis();
    frame++;
    tick = true;
  }
  bool linkMetaChanged =
      (detectedBaud != lastBaudShown) || (mavRxPin != lastRx) || (mavTxPin != lastTx);
  if (!tick && !forceFull && !linkMetaChanged) return;

  if (forceFull || !g_dispWasLoading) {
    display.fillScreen(C_BG);
    display.fillRect(0, 0, SCREEN_WIDTH, 52, C_CARD);
    display.setTextSize(3);
    display.setTextColor(C_ACCENT);
    display.setCursor(28, 12);
    display.print("CONNECTING");
  }

  const int cx = 160, cy = 108, r = 46;
  display.fillRect(cx - r - 8, cy - r - 8, (r + 8) * 2, (r + 8) * 2, C_BG);
  display.drawCircle(cx, cy, r, 0x3186);
  display.drawCircle(cx, cy, r - 3, 0x3186);

  float a0 = frame * 0.40f;
  for (int i = 0; i < 16; i++) {
    float a = a0 + i * 0.18f;
    uint16_t c = (i >= 12) ? C_CYAN : ((i >= 7) ? C_LBLUE : 0x2104);
    int x1 = cx + (int)((r - 16) * cosf(a));
    int y1 = cy + (int)((r - 16) * sinf(a));
    int x2 = cx + (int)((r - 2) * cosf(a));
    int y2 = cy + (int)((r - 2) * sinf(a));
    display.drawLine(x1, y1, x2, y2, c);
    if (i >= 12) display.drawLine(x1 + 1, y1, x2 + 1, y2, c);
  }
  int pulse = 7 + ((frame % 16) < 8 ? (frame % 8) : (8 - (frame % 8)));
  display.fillCircle(cx, cy, pulse, C_CYAN);

  display.fillRect(0, 160, SCREEN_WIDTH, 80, C_BG);
  display.setTextSize(3);
  display.setTextColor(C_TEXT);
  char baudBuf[28];
  snprintf(baudBuf, sizeof(baudBuf), "%ld", detectedBaud > 0 ? detectedBaud : 0);
  int baudW = (int)strlen(baudBuf) * 18;
  display.setCursor((SCREEN_WIDTH - baudW - 40) / 2, 164);
  display.print(baudBuf);
  display.setTextSize(2);
  display.setTextColor(C_MUTED);
  display.setCursor((SCREEN_WIDTH - baudW - 40) / 2 + baudW + 8, 172);
  display.print("bps");

  display.setTextSize(2);
  display.setTextColor(C_ACCENT);
  char pinBuf[36];
  snprintf(pinBuf, sizeof(pinBuf), "RX %d    TX %d", mavRxPin, mavTxPin);
  int pinW = (int)strlen(pinBuf) * 12;
  display.setCursor((SCREEN_WIDTH - pinW) / 2, 196);
  display.print(pinBuf);

  display.setTextSize(2);
  display.setTextColor(uartPinsSwapped ? C_ORANGE : C_OK);
  const char* pol = uartPinsSwapped ? "SWAPPED" : "NORMAL";
  int polW = (int)strlen(pol) * 12;
  display.setCursor((SCREEN_WIDTH - polW) / 2, 218);
  display.print(pol);

  lastBaudShown = detectedBaud;
  lastRx = mavRxPin;
  lastTx = mavTxPin;
  g_dispWasLoading = true;
}

// 用「前景+背景」覆写文字，避免整块 fillRect 闪白/闪黑
inline void drawMsgLineOpaque(int x, int y, const String& text, uint16_t fg, uint16_t bg, int chars) {
  display.setTextSize(2);
  display.setTextColor(fg, bg);
  display.setCursor(x, y);
  display.print(tftPadLine(text, chars));
}

inline void drawOpSysMessages(int y, bool redrawOp, bool redrawSys) {
  const int cpl = tftCharsPerLine(2);
  const int lineH = 16;
  const int opTop = y + 12;
  const int sysLabelY = opTop + 2 * lineH + 8;
  const int sysTop = sysLabelY + 12;
  int sysMax = (SCREEN_HEIGHT - sysTop - 2) / lineH;
  if (sysMax < 1) sysMax = 1;
  if (sysMax > 5) sysMax = 5;

  if (redrawOp) {
    display.setTextSize(1);
    display.setTextColor(C_OP_LABEL, C_BG);
    display.setCursor(8, y);
    display.print("OP ");

    String opLines[12];
    int opN = 0;
    tftFillMsgLines(operationMessage, opLines, 12, opN, cpl);
    for (int i = 0; i < 2; i++) {
      String line = (i < opN) ? opLines[i] : String("");
      drawMsgLineOpaque(8, opTop + i * lineH, line, C_OP_TEXT, C_BG, cpl);
    }
    g_lastOpDrawn = operationMessage;
  }

  if (redrawSys) {
    display.setTextSize(1);
    display.setTextColor(C_SYS_LABEL, C_BG);
    display.setCursor(8, sysLabelY);
    display.print("SYS");

    String sysLines[20];
    int sysN = 0;
    tftFillMsgLines(systemMessage, sysLines, 20, sysN, cpl);
    int start = 0;
    if (sysN > sysMax) {
      int pages = sysN - sysMax + 1;
      start = g_msgScrollIdx % pages;
    }

    for (int i = 0; i < sysMax; i++) {
      int li = start + i;
      String line = (li < sysN) ? sysLines[li] : String("");
      drawMsgLineOpaque(8, sysTop + i * lineH, line, C_SYS_TEXT, C_BG, cpl);
    }
    g_lastSysDrawn = systemMessage;
    g_lastScrollIdx = g_msgScrollIdx;
  }
}

inline void drawConnectedScreen(bool forceFull) {
  if (forceFull || g_dispWasLoading) {
    display.fillScreen(C_BG);
    g_lastOpDrawn = "";
    g_lastSysDrawn = "";
    g_lastScrollIdx = 255;
    g_lastMsgPaintMs = 0;
  }

  static float lastV = -999;
  static String lastMode;
  static int lastGps = -1;
  static bool lastArm = false;
  static bool lastParamReady = true;
  static uint16_t lastGot = 0xFFFF;
  static uint16_t lastTot = 0xFFFF;
  static int lastMsgY = -1;

  bool telemDirty = forceFull || g_dispWasLoading ||
                    fabsf(voltage - lastV) > 0.05f || flightMode != lastMode ||
                    gpsCount != lastGps || isArmed != lastArm;

  const int telemH = 58;
  if (telemDirty) {
    // 整条统一背景色
    display.fillRoundRect(4, 4, SCREEN_WIDTH - 8, telemH - 6, 6, C_TELEM_BG);

    char vbuf[16];
    snprintf(vbuf, sizeof(vbuf), "%.1fV", voltage);
    String fm = flightMode;
    if ((int)fm.length() > 6) fm = fm.substring(0, 6);
    char gbuf[8];
    snprintf(gbuf, sizeof(gbuf), "%d", gpsCount);

    // 文字分色：电压青 / 模式黄 / GPS绿 / ARM绿或橙
    drawTelemCell(14, 12, "VOLT", String(vbuf), C_ACCENT);
    drawTelemCell(96, 12, "MODE", fm, C_WARN);
    drawTelemCell(188, 12, "GPS", String(gbuf), C_OK);
    drawTelemCell(248, 12, "ARM",
                  isArmed ? String("ARM") : String("DIS"),
                  isArmed ? C_OK : C_ARM_DIS);

    lastV = voltage;
    lastMode = flightMode;
    lastGps = gpsCount;
    lastArm = isArmed;
  }

  int y = telemH;
  uint16_t got = (uint16_t)paramIndexSet.size();
  uint16_t tot = param_total;
  bool barVisible = !param_ready;
  bool barDirty = forceFull || g_dispWasLoading ||
                  barVisible != !lastParamReady ||
                  (barVisible && (got != lastGot || tot != lastTot));
  // 仅当参数条出现/消失时才改消息区布局
  bool layoutChanged = forceFull || g_dispWasLoading || (barVisible != !lastParamReady) ||
                       (y != lastMsgY && lastMsgY >= 0 && barVisible != !lastParamReady);

  if (barVisible) {
    if (barDirty) {
      display.fillRect(0, y, SCREEN_WIDTH, 26, C_BG);
      display.fillRoundRect(4, y + 1, SCREEN_WIDTH - 8, 22, 4, C_CARD);
      int pct = (tot > 0) ? (int)((got * 100UL) / tot) : 0;
      if (pct > 100) pct = 100;
      display.setTextSize(1);
      display.setTextColor(C_ACCENT, C_CARD);
      display.setCursor(12, y + 4);
      display.printf("PARAMS  %u / %u   ", got, tot);
      display.drawRect(12, y + 14, SCREEN_WIDTH - 28, 5, C_MUTED);
      if (pct > 0) display.fillRect(13, y + 15, ((SCREEN_WIDTH - 30) * pct) / 100, 3, C_ACCENT);
      lastGot = got;
      lastTot = tot;
    }
    y += 26;
    lastParamReady = false;
  } else {
    if (!lastParamReady) {
      // 参数条刚消失：清掉那一行，消息区上移
      display.fillRect(0, telemH, SCREEN_WIDTH, 26, C_BG);
      layoutChanged = true;
      lastParamReady = true;
    }
    lastGot = got;
    lastTot = tot;
  }

  if (lastMsgY < 0) layoutChanged = true;
  if (y != lastMsgY) {
    layoutChanged = true;
    lastMsgY = y;
  }
  g_msgAreaY = y;

  String sysLinesTmp[20];
  int sysNtmp = 0;
  tftFillMsgLines(systemMessage, sysLinesTmp, 20, sysNtmp, tftCharsPerLine(2));
  int sysMaxGuess = (SCREEN_HEIGHT - (y + 12 + 32 + 8 + 12) - 2) / 16;
  if (sysMaxGuess < 1) sysMaxGuess = 1;
  bool needScroll = sysNtmp > sysMaxGuess;

  bool scrollTick = false;
  if (needScroll && (millis() - g_msgScrollAt >= 3500)) {
    g_msgScrollAt = millis();
    g_msgScrollIdx++;
    scrollTick = true;
  }

  bool opChanged = (operationMessage != g_lastOpDrawn);
  bool sysChanged = (systemMessage != g_lastSysDrawn) || scrollTick;

  // 消息变更防抖：避免 STATUSTEXT / 参数进度把 OP/SYS 刷到频闪
  const unsigned long MSG_MIN_INTERVAL_MS = 600;
  bool allowPaint = layoutChanged || forceFull || g_dispWasLoading ||
                    (millis() - g_lastMsgPaintMs >= MSG_MIN_INTERVAL_MS);

  if (layoutChanged) {
    display.fillRect(0, y, SCREEN_WIDTH, SCREEN_HEIGHT - y, C_BG);
    drawOpSysMessages(y, true, true);
    g_lastMsgPaintMs = millis();
  } else if (allowPaint && (opChanged || sysChanged)) {
    drawOpSysMessages(y, opChanged, sysChanged);
    g_lastMsgPaintMs = millis();
  }

  g_dispWasLoading = false;
}

inline void drawMenuScreen(bool forceFull) {
  static int lastIdx = -1;
  static String lastStatus;
  bool dirty = forceFull || buttonsConsumeMenuDirty() ||
               menuIndex != lastIdx || menuStatus != lastStatus;
  if (!dirty) return;

  display.fillScreen(C_BG);
  display.fillRect(0, 0, SCREEN_WIDTH, 36, C_TELEM_BG);
  display.setTextSize(2);
  display.setTextColor(C_ACCENT, C_TELEM_BG);
  display.setCursor(12, 10);
  display.print("MENU (sim)");

  display.setTextSize(1);
  display.setTextColor(C_MUTED, C_BG);
  display.setCursor(12, 44);
  display.print("UP/DN move  LEFT back  OK confirm");

  const int count = menuItemCount();
  for (int i = 0; i < count; i++) {
    int y = 64 + i * 28;
    bool sel = (i == menuIndex);
    uint16_t rowBg = sel ? C_CARD : C_BG;
    display.fillRoundRect(8, y, SCREEN_WIDTH - 16, 24, 4, rowBg);
    display.setTextSize(2);
    display.setTextColor(sel ? C_WARN : C_TEXT, rowBg);
    display.setCursor(18, y + 4);
    if (sel) display.print("> ");
    else display.print("  ");
    display.print(menuItemName(i));
  }

  display.setTextSize(1);
  display.setTextColor(C_OK, C_BG);
  display.setCursor(12, SCREEN_HEIGHT - 18);
  display.print(tftPadLine(menuStatus, 48));

  lastIdx = menuIndex;
  lastStatus = menuStatus;
}

inline void initDisplay() {
  Serial.println("Init ST7789 240x320 landscape...");
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
  display.init(TFT_WIDTH, TFT_HEIGHT);
  display.setRotation(1);
  display.setSPISpeed(40000000);
  display.setTextWrap(false);

  g_dispWasLoading = false;
  operationMessage = "Booting display...";
  drawLoadingScreen(true);
  Serial.println("ST7789 ready");
}

inline void initOLED() { initDisplay(); }

inline void updateDisplay() {
  static UiMode lastUi = UI_TELEMETRY;
  bool leavingMenu = (lastUi == UI_MENU && uiMode != UI_MENU);
  lastUi = uiMode;

  if (uiMode == UI_MENU) {
    drawMenuScreen(leavingMenu);  // entering already sets menuDirty
    return;
  }

#if DEMO_SKIP_LINK
  bool linkOk = true;
#else
  bool linkOk = fcHeartbeatSeen && (lastFcHeartbeatMs != 0) &&
                (millis() - lastFcHeartbeatMs < 5000);
#endif
  if (!linkOk) {
    drawLoadingScreen(!g_dispWasLoading || leavingMenu);
  } else {
    drawConnectedScreen(g_dispWasLoading || leavingMenu);
  }
}

inline void updateOLED() { updateDisplay(); }

#endif
