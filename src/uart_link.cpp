#include "uart_link.h"
#include "app_state.h"
#include <Preferences.h>
#include <cstring>
#include <c_library_v2-master/ardupilotmega/mavlink.h>

static UartScanUiFn g_scanUi = nullptr;
static bool g_linkLocked = false;   // 已确认稳定链路，禁止乱切线序
static uint8_t g_lockSwap = 0;
static long g_lockBaud = 0;
static unsigned long g_bootMs = 0;

void uartLinkSetScanUi(UartScanUiFn fn) { g_scanUi = fn; }

static void notifyScanUi(long baud, int rx, int tx) {
  if (g_scanUi) g_scanUi(baud, rx, tx);
}

// 常用波特率：正常线序优先扫完，再考虑换向
static const long kBauds[] = {57600, 115200, 230400, 460800, 921600};
static const size_t kBaudCount = sizeof(kBauds) / sizeof(kBauds[0]);

// 心跳约 1Hz，窗口必须 >1s，否则「有时要按多次 reset」
static const unsigned long kListenMs = 1300;

static void flushRxQuick() {
  unsigned long t0 = millis();
  while (millis() - t0 < 20) {
    while (mavSerial.available()) (void)mavSerial.read();
  }
}

void uartLinkApply(int rxPin, int txPin, long baud) {
  mavSerial.end();
  delay(15);
  mavSerial.setRxBufferSize(MAVLINK_SERIAL_RX_BUFFER);
  mavSerial.begin(baud, SERIAL_8N1, rxPin, txPin);
  mavlink_reset_channel_status(MAVLINK_COMM_0);
  flushRxQuick();
  mavRxPin = rxPin;
  mavTxPin = txPin;
  uartPinsSwapped = (rxPin == MAVLINK_PIN_B && txPin == MAVLINK_PIN_A);
  detectedBaud = baud;
}

void uartLinkSave() {
  Preferences p;
  if (!p.begin("mavlink", false)) return;
  p.putLong("baud", detectedBaud);
  p.putUChar("swap", uartPinsSwapped ? 1 : 0);
  p.end();
}

static void pinsForSwap(uint8_t swap, int& rx, int& tx) {
  if (swap) {
    rx = MAVLINK_PIN_B;
    tx = MAVLINK_PIN_A;
  } else {
    rx = MAVLINK_PIN_A;
    tx = MAVLINK_PIN_B;
  }
}

// 必须收到飞控 HEARTBEAT（或明确来自非本机 sysid 的 HEARTBEAT），避免噪声误锁定
static bool listenForFcHeartbeat(unsigned long parseMs) {
  mavlink_message_t msg;
  mavlink_status_t status;
  memset(&msg, 0, sizeof(msg));
  memset(&status, 0, sizeof(status));
  unsigned long start = millis();
  unsigned long lastHbTx = 0;
  int hbCount = 0;

  while (millis() - start < parseMs) {
    if (millis() - lastHbTx >= 400) {
      lastHbTx = millis();
      mavlink_message_t hb;
      mavlink_msg_heartbeat_pack(SYS_ID, COMP_ID, &hb, MAV_TYPE_GCS,
                                 MAV_AUTOPILOT_INVALID, 0, 0, MAV_STATE_ACTIVE);
      uint8_t buf[MAVLINK_MAX_PACKET_LEN];
      uint16_t len = mavlink_msg_to_send_buffer(buf, &hb);
      mavSerial.write(buf, len);
    }
    while (mavSerial.available()) {
      uint8_t c = (uint8_t)mavSerial.read();
      if (!mavlink_parse_char(MAVLINK_COMM_0, c, &msg, &status)) continue;
      if (msg.sysid == SYS_ID) continue;
      if (msg.msgid == MAVLINK_MSG_ID_HEARTBEAT) {
        hbCount++;
        // 收到 1 个飞控心跳即可认定（窗口已覆盖 ≥1 个周期）
        fcHeartbeatSeen = true;
        lastFcHeartbeatMs = millis();
        return true;
      }
    }
    yield();
  }
  (void)hbCount;
  return false;
}

static bool tryCombo(int rxPin, int txPin, long baud) {
  Serial.printf("UART try baud=%ld RX=%d TX=%d\n", baud, rxPin, txPin);
  uartLinkApply(rxPin, txPin, baud);
  notifyScanUi(baud, rxPin, txPin);
  delay(40);
  if (listenForFcHeartbeat(kListenMs)) {
    Serial.printf("UART OK baud=%ld RX=%d TX=%d%s\n",
                  baud, rxPin, txPin, uartPinsSwapped ? " SWAPPED" : "");
    g_linkLocked = true;
    g_lockSwap = uartPinsSwapped ? 1 : 0;
    g_lockBaud = baud;
    uartLinkSave();
    return true;
  }
  return false;
}

bool uartLinkStart() {
  g_bootMs = millis();
  g_linkLocked = false;
  delay(100);

  Preferences p;
  long savedBaud = 0;
  uint8_t savedSwap = 255;
  if (p.begin("mavlink", true)) {
    savedBaud = p.getLong("baud", 0);
    savedSwap = p.getUChar("swap", 255);
    p.end();
  }

  // 1) 上次成功组合（加长窗口）
  if (savedSwap <= 1 && savedBaud > 0) {
    int rx, tx;
    pinsForSwap(savedSwap, rx, tx);
    if (tryCombo(rx, tx, savedBaud)) return true;
  }

  // 2) 先扫全部波特率的【正常线序】——多数用户都是正接
  for (size_t i = 0; i < kBaudCount; i++) {
    if (savedBaud == kBauds[i] && savedSwap == 0) continue;
    int rx, tx;
    pinsForSwap(0, rx, tx);
    if (tryCombo(rx, tx, kBauds[i])) return true;
  }

  // 3) 正常线序全失败后，再扫【换向】
  for (size_t i = 0; i < kBaudCount; i++) {
    if (savedBaud == kBauds[i] && savedSwap == 1) continue;
    int rx, tx;
    pinsForSwap(1, rx, tx);
    if (tryCombo(rx, tx, kBauds[i])) return true;
  }

  Serial.println("UART detect failed, keep NORMAL 57600 and wait");
  uartLinkApply(MAVLINK_PIN_A, MAVLINK_PIN_B, 57600);
  g_lockSwap = 0;
  g_lockBaud = 57600;
  notifyScanUi(57600, MAVLINK_PIN_A, MAVLINK_PIN_B);
  return false;
}

void uartLinkTick() {
  unsigned long now = millis();

  // 开机后 WiFi 射频阶段不要切串口，否则正接线也会被搅乱
  if (g_bootMs != 0 && (now - g_bootMs) < 12000) {
    return;
  }

  // 近期有飞控心跳：锁定，绝不换线
  if (fcHeartbeatSeen && lastFcHeartbeatMs != 0 &&
      (now - lastFcHeartbeatMs) < 10000) {
    g_linkLocked = true;
    return;
  }

  // 曾锁定过：先只在【同一线序】上重试波特率，不自动翻线
  static uint8_t step = 0;
  static unsigned long phaseStart = 0;
  static bool waiting = false;
  static int tryRx = MAVLINK_PIN_A;
  static int tryTx = MAVLINK_PIN_B;
  static long tryBaud = 57600;
  static uint8_t preferSwap = 0;
  static uint8_t failRounds = 0;

  if (!waiting) {
    preferSwap = g_linkLocked ? g_lockSwap : 0;
    // 同一线序扫波特率；连续多轮失败后才尝试一次换向
    uint8_t swap = preferSwap;
    if (failRounds >= (uint8_t)kBaudCount && (step % 2) == 1) {
      swap = preferSwap ? 0 : 1;
    }
    size_t bi = step % kBaudCount;
    step++;
    pinsForSwap(swap, tryRx, tryTx);
    tryBaud = kBauds[bi];
    Serial.printf("UART retune baud=%ld RX=%d TX=%d\n", tryBaud, tryRx, tryTx);
    uartLinkApply(tryRx, tryTx, tryBaud);
    mavlink_reset_channel_status(MAVLINK_COMM_0);
    notifyScanUi(tryBaud, tryRx, tryTx);
    phaseStart = now;
    waiting = true;
    fcHeartbeatSeen = false;
    return;
  }

  static mavlink_message_t msg;
  static mavlink_status_t status;
  static bool parserInited = false;
  if (!parserInited) {
    memset(&msg, 0, sizeof(msg));
    memset(&status, 0, sizeof(status));
    parserInited = true;
  }

  while (mavSerial.available()) {
    uint8_t c = (uint8_t)mavSerial.read();
    if (!mavlink_parse_char(MAVLINK_COMM_0, c, &msg, &status)) continue;
    if (msg.sysid == SYS_ID) continue;
    if (msg.msgid == MAVLINK_MSG_ID_HEARTBEAT) {
      Serial.printf("UART retune OK baud=%ld RX=%d TX=%d\n", tryBaud, tryRx, tryTx);
      fcHeartbeatSeen = true;
      lastFcHeartbeatMs = millis();
      g_linkLocked = true;
      g_lockSwap = uartPinsSwapped ? 1 : 0;
      g_lockBaud = tryBaud;
      uartLinkSave();
      paramListRequestedAfterLink = false;
      failRounds = 0;
      waiting = false;
      parserInited = false;
      return;
    }
  }

  if (now - phaseStart >= kListenMs) {
    waiting = false;
    parserInited = false;
    failRounds++;
    if (failRounds > 40) failRounds = 0;
  }
}
