#include "config.h"
#include "mavlink_handler.h"
#include "oled_display.h"
#include "web_server.h"
#include <cstring>

// 全局对象
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
HardwareSerial mavSerial(2);
WebServer server(80);

// 全局变量定义
String ipStr = "192.168.6.6";
String systemMessage = "Initializing...";
String operationMessage = "Ready";
float voltage = 0.0;
String flightMode = "-";
int gpsCount = 0;
bool isArmed = false;

// 飞行模式开关通道 PWM（RC_CHANNELS / RC_CHANNELS_RAW + FLTMODE_CH 或 MODE_CH）
uint16_t rc_fltmode_pwm_us = 0;
bool rc_fltmode_pwm_valid = false;

// 自动串口波特率检测
long detectedBaud = 0;
const long commonBaudRates[] = {57600, 115200, 230400, 460800, 921600};


// 参数相关
std::map<String, float> paramMap;
std::set<uint16_t> paramIndexSet;
uint16_t param_total = 0;
bool param_ready = false;
unsigned long last_param_recv = 0;

// OLED滚动相关
int scrollOffset = 0;
int scrollOffsetSet = 0;

// ARM/DISARM相关变量
bool lastArmCommand = false;
String armErrors[10];
int armErrorCount = 0;
unsigned long lastStatusTextTime = 0;
String lastStatusText = "";

// 校准状态
bool levelCalibrated = false;
bool accelCalibrated = false;

// 舵机输出数据
ServoData servoOutputs[16];

// 校准状态管理
bool waitingForAck = false;
unsigned long lastCommandTime = 0;
CalibrationType currentCalibration = CALIB_NONE;
AccelCalibrationState currentAccelState = ACCEL_STATE_IDLE;
LevelCalibrationState currentLevelState = LEVEL_STATE_IDLE;
CalibrationStepState currentStepState = STEP_STATE_WAITING_PLACEMENT;
SimpleCalibrationStep currentSimpleStep = STEP_NONE;
uint8_t currentPosition = 0;

// 参数设置相关
String lastSetResult = "";
unsigned long lastSetTime = 0;
std::map<String, float> pendingSetMap;

static void flushMavlinkSerialRx(HardwareSerial& serial, unsigned long windowMs) {
  unsigned long t0 = millis();
  while (millis() - t0 < windowMs) {
    while (serial.available()) {
      (void)serial.read();
    }
    yield();
  }
}

// 自动串口波特率检测（多轮 + 每档冲洗解析状态，避免错波特残留半包误判）
bool detectMavlinkBaudrate(HardwareSerial& serial, int rxPin, int txPin) {
  delay(MAVLINK_UART_SETTLE_MS);

  for (uint8_t pass = 0; pass < MAVLINK_BAUD_DETECT_PASSES; pass++) {
    if (pass > 0) {
      Serial.printf("MAVLink baud detect pass %u/%u\n",
                    (unsigned)(pass + 1), (unsigned)MAVLINK_BAUD_DETECT_PASSES);
    }

    for (size_t i = 0; i < sizeof(commonBaudRates) / sizeof(long); i++) {
      long baud = commonBaudRates[i];
      serial.end();
      delay(30);
      serial.setRxBufferSize(MAVLINK_SERIAL_RX_BUFFER);
      serial.begin(baud, SERIAL_8N1, rxPin, txPin);
      mavlink_reset_channel_status(MAVLINK_COMM_0);
      flushMavlinkSerialRx(serial, MAVLINK_RX_FLUSH_MS);
      delay(MAVLINK_BAUD_STABLE_MS);

      mavlink_message_t msg;
      mavlink_status_t status;
      memset(&msg, 0, sizeof(msg));
      memset(&status, 0, sizeof(status));

      unsigned long start = millis();
      while (millis() - start < MAVLINK_BAUD_PARSE_MS) {
        while (serial.available()) {
          uint8_t c = serial.read();
          if (mavlink_parse_char(MAVLINK_COMM_0, c, &msg, &status)) {
            Serial.printf("MAVLink baud ok: %ld (pass %u)\n", baud, (unsigned)(pass + 1));
            detectedBaud = baud;
            return true;
          }
        }
        yield();
      }
    }
  }

  serial.end();
  return false;
}


void setup() {
  /*
   * 启动阶段（自下而上）：外设与显示 → 飞控串口就绪 → 短 MAVLink 预热 → WiFi 热点
   * → 清串口半包 → Web 路由注册 → 业务默认值 → 向飞控要参数表 → OLED 提示
   * 原则：飞控链路与解析状态在「开射频」前尽量稳定；热点用与旧固件相同的 softAP / Config 顺序。
   */
  Serial.begin(115200);
  Serial.println("=== ESP32 MAVLink Ground Station Starting ===");

  // 初始化I2C和OLED
  Serial.println("Step 1: Initializing I2C and OLED...");
  Wire.begin(I2C_SDA, I2C_SCL);
  delay(100);
  initOLED();
  Serial.println("OLED initialization completed");

  // 初始化MAVLink串口
  Serial.println("Step 2: Initializing MAVLink serial...");

  // 自动识别 MAVLink 波特率
  if (!detectMavlinkBaudrate(mavSerial, MAVLINK_RX, MAVLINK_TX)) {
    Serial.println("auto MAVLink baudrate failed, fallback 230400");
    detectedBaud = 230400;
  }
  if (detectedBaud <= 0) {
    detectedBaud = 230400;
  }
  mavSerial.setRxBufferSize(MAVLINK_SERIAL_RX_BUFFER);
  mavSerial.begin(detectedBaud, SERIAL_8N1, MAVLINK_RX, MAVLINK_TX);
  mavlink_reset_channel_status(MAVLINK_COMM_0);
  flushMavlinkSerialRx(mavSerial, MAVLINK_RX_FLUSH_MS);

  Serial.println("MAVLink serial initialized");

  /*
   * Step 2b：开 WiFi 前短预热。射频初始化会占 CPU/中断；先让 MAVLink 与飞控跑一小段时间，
   * 业务关系上仍是「飞控链路优先于热点」，与旧版「只是偶尔多 reset」的直觉一致。
   */
  Serial.println("Step 2b: MAVLink warm-up before WiFi RF...");
  for (int i = 0; i < 40; i++) {
    handleMavlink(MAVLINK_RX_SLICE_BYTES);
    yield();
    delay(4);
  }

  // Step 3：WiFi 与旧版顺序一致 — softAP 后再 softAPConfig，避免非常规顺序带来的兼容问题
  Serial.println("Step 3: Initializing WiFi AP...");
  WiFi.persistent(false);
  WiFi.mode(WIFI_AP);
  delay(100);
  if (!WiFi.softAP(SSID, PASSWORD)) {
    Serial.println("WiFi.softAP failed");
  }
  IPAddress local_IP(192, 168, 6, 6);
  IPAddress gateway(192, 168, 6, 6);
  IPAddress subnet(255, 255, 255, 0);
  if (!WiFi.softAPConfig(local_IP, gateway, subnet)) {
    Serial.println("WiFi.softAPConfig failed");
  }
  WiFi.setSleep(false);
  delay(800);
  ipStr = WiFi.softAPIP().toString();
  Serial.println("WiFi AP initialized: " + ipStr);

  delay(MAVLINK_AFTER_WIFI_MS);
  mavlink_reset_channel_status(MAVLINK_COMM_0);
  flushMavlinkSerialRx(mavSerial, MAVLINK_RX_FLUSH_MS);

  // Step 4：Web 在热点与 IP 稳定后再起，避免客户端早连时空白重试放大阻塞感
  Serial.println("Step 4: Initializing Web server...");
  initWebServer();
  Serial.println("Web server initialized");

  // 初始化舵机数据
  Serial.println("Step 5: Initializing servo data...");
  for (int i = 0; i < 16; i++) {
    servoOutputs[i].valid = false;
    servoOutputs[i].position = 1500;
    servoOutputs[i].min_pos = 1100;
    servoOutputs[i].max_pos = 1900;
    servoOutputs[i].trim_pos = 1500;
    servoOutputs[i].function = 0;
    servoOutputs[i].reversed = false;
  }
  Serial.println("Servo data initialized");

  // 初始化STATUSTEXT相关变量
  Serial.println("Step 6: Initializing status variables...");
  lastStatusTextTime = millis();
  lastStatusText = "System Ready";

  // 请求参数列表（ stagger 双发，降低「仅第一次丢包」概率）
  Serial.println("Step 7: Requesting parameter list...");
  requestParamList();
  delay(PARAM_LIST_STAGGER_MS);
  requestParamList();
  Serial.println("Parameter list requests sent (staggered x2)");

  systemMessage = "ESP32 MAVLink";
  operationMessage = "Loading params...";
  updateOLED();

  Serial.println("=== ESP32 MAVLink Ground Station initialized ===");
  Serial.println("IP: " + ipStr);
  Serial.println("Ready for operation");
}

void loop() {
  /*
   * 调度：多轮「小口收 MAV + handleClient + yield」，再收一轮默认预算。
   * 避免单轮 handleMavlink 吞满 RX 时 WiFi/HTTP 长时间得不到调度（手机断连、页面闪断）。
   */
  for (uint8_t p = 0; p < MAVLINK_WEB_INTERLEAVE; p++) {
    handleMavlink(MAVLINK_RX_SLICE_BYTES);
    server.handleClient();
    yield();
  }
  handleMavlink(0);

  if (!param_ready && param_total > 0 && millis() - last_param_recv > 30000) {
    Serial.println("Parameter loading timeout - forcing completion");
    Serial.println("Total expected: " + String(param_total) + ", Received: " + String(paramIndexSet.size()));
    param_ready = true;
    operationMessage = "Params timeout " + String(paramIndexSet.size()) + "/" + String(param_total);
  }

  checkAndRequestMissingParams();
  if (param_ready) {
    pumpServoParamRequests();
  }

  updateSystemMessage();

  static unsigned long lastOLEDUpdate = 0;
  if (millis() - lastOLEDUpdate > 200) {
    lastOLEDUpdate = millis();

    int maxChars = 21;
    int sysWrap = oledLongestLineLenExceedCols(systemMessage, maxChars);
    if (sysWrap > 0) {
      scrollOffset++;
      if (scrollOffset >= sysWrap) scrollOffset = 0;
    } else {
      scrollOffset = 0;
    }
    if ((int)operationMessage.length() > maxChars) {
      scrollOffsetSet++;
      if (scrollOffsetSet >= (int)operationMessage.length()) scrollOffsetSet = 0;
    } else {
      scrollOffsetSet = 0;
    }

    updateOLED();
  }

  if (waitingForAck && millis() - lastCommandTime > CMD_TIMEOUT) {
    Serial.println("Calibration command timeout - resetting state");
    operationMessage = "Calibration timeout - Please try again.";
    waitingForAck = false;
    currentCalibration = CALIB_NONE;
    currentAccelState = ACCEL_STATE_IDLE;
  }

  server.handleClient();
  handleMavlink(MAVLINK_RX_SLICE_BYTES);

  static unsigned long lastDebugTime = 0;
  if (!servo_param_pump_done && millis() - lastDebugTime > 5000) {
    lastDebugTime = millis();
    Serial.println("System running - Voltage: " + String(voltage, 2) + "V, Mode: " + flightMode + ", Params: " + String(paramMap.size()) + "/" + String(param_total));
  }
} 