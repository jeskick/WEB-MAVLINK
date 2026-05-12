#ifndef OLED_DISPLAY_H
#define OLED_DISPLAY_H

#include "config.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// 外部声明
extern Adafruit_SSD1306 display;
extern String systemMessage;
extern float voltage;
extern String flightMode;
extern int gpsCount;
extern String operationMessage;
extern bool param_ready;
extern std::map<String, float> paramMap;
extern uint16_t param_total;
extern ServoData servoOutputs[16];

// 滚动相关
extern int scrollOffset;
extern int scrollOffsetSet;

// 按换行拆分 systemMessage，最多 maxParts 段（每段可再截断列宽）
inline int oledSplitByNewline(const String& s, String* parts, int maxParts) {
  int n = 0;
  int start = 0;
  int len = (int)s.length();
  for (int i = 0; i <= len && n < maxParts; ++i) {
    if (i == len || s.charAt(i) == '\n') {
      parts[n++] = s.substring(start, i);
      start = i + 1;
    }
  }
  return n;
}

inline int oledLongestLineLenExceedCols(const String& msg, int kCols) {
  if (msg.length() == 0) return 0;
  int longest = 0;
  int start = 0;
  for (int i = 0; i <= (int)msg.length(); ++i) {
    if (i == (int)msg.length() || msg.charAt(i) == '\n') {
      int segLen = i - start;
      if (segLen > longest) longest = segLen;
      start = i + 1;
    }
  }
  return longest > kCols ? longest : 0;
}

// OLED初始化
inline void initOLED() {
  Serial.println("Initializing OLED display...");
  Serial.print("I2C SDA: ");
  Serial.println(I2C_SDA);
  Serial.print("I2C SCL: ");
  Serial.println(I2C_SCL);
  Serial.print("Screen address: 0x");
  Serial.println(SCREEN_ADDRESS, HEX);

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    Serial.println("Please check wiring and I2C address");
    return;
  }

  Serial.println("OLED display initialized successfully");
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);
  display.println("ESP32 MAVLink");
  display.setCursor(0, 16);
  display.println("Initializing...");
  display.display();
  delay(1000);
}

// OLED显示主函数：上半屏 4 行显示飞控多行状态，垂直窗口滚动；下半屏电压/操作/参数
inline void updateOLED() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  const int kCols = 21;
  String sysParts[12];
  int nsys = oledSplitByNewline(systemMessage, sysParts, 12);
  if (nsys == 0) {
    sysParts[0] = "-";
    nsys = 1;
  }

  static uint32_t oledSysWinT = 0;
  static int oledSysWin = 0;
  int maxStart = nsys > 4 ? (nsys - 4) : 0;
  if (maxStart > 0) {
    if (millis() - oledSysWinT > 2800) {
      oledSysWinT = millis();
      oledSysWin++;
      if (oledSysWin > maxStart) oledSysWin = 0;
    }
  } else {
    oledSysWin = 0;
  }
  int start = oledSysWin;

  for (int r = 0; r < 4; ++r) {
    display.setCursor(0, r * 8);
    String line = (start + r < nsys) ? sysParts[start + r] : "";
    line.trim();
    if ((int)line.length() > kCols) {
      int period = (int)line.length();
      int sp = scrollOffset % period;
      String show = line.substring(sp);
      if ((int)show.length() > kCols) show = show.substring(0, kCols);
      display.println(show.length() ? show : " ");
    } else {
      display.println(line.length() ? line : " ");
    }
  }

  // 电压 / 模式 / GPS（y=32）
  display.setCursor(0, 32);
  display.print("V:");
  display.print(voltage, 1);
  display.print(" M:");
  String fm = flightMode;
  if ((int)fm.length() > 8) fm = fm.substring(0, 8);
  display.print(fm);
  display.print(" G:");
  display.print(gpsCount);

  // 操作提示（y=40、48）：横向滚动（scrollOffsetSet 由 loop 按 operationMessage 驱动）
  display.setCursor(0, 40);
  String opFull = operationMessage;
  if ((int)opFull.length() > kCols) {
    int period = (int)opFull.length();
    int sp = scrollOffsetSet % period;
    String opMsg = opFull.substring(sp);
    if ((int)opMsg.length() > kCols) opMsg = opMsg.substring(0, kCols);
    display.println(opMsg.length() ? opMsg : " ");
  } else {
    display.println(opFull.length() ? opFull : " ");
  }

  display.setCursor(0, 48);
  if ((int)opFull.length() > kCols) {
    int period = (int)opFull.length();
    int sp = scrollOffsetSet % period;
    int sp2 = (sp + kCols) % period;
    String op2 = opFull.substring(sp2);
    if ((int)op2.length() > kCols) op2 = op2.substring(0, kCols);
    display.println(op2.length() ? op2 : " ");
  } else {
    display.println(" ");
  }

  // 参数进度（y=56）
  display.setCursor(0, 56);
  if (!param_ready) {
    display.print("P:");
    display.print(paramMap.size());
    display.print("/");
    display.println(param_total);
  } else {
    display.println("PARAM:OK");
  }

  display.display();
}

inline void showDisplay() {
  updateOLED();
}

#endif
