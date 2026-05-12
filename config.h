#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <map>
#include <set>
#include <vector>

// OLED配置
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

// I2C引脚配置
#define I2C_SDA 18
#define I2C_SCL 17

// MAVLink串口配置
#define MAVLINK_RX 15
#define MAVLINK_TX 16

// WiFi配置（与旧版一致：仅 SSID/密码，避免与旧手机/路由兼容性问题）
#define SSID "ESP32-MAV"
#define PASSWORD "12345678"

// 时间常量
const unsigned long CALIB_MSG_DURATION = 8000;
const unsigned long CMD_TIMEOUT = 15000;

// MAVLink 链路与启动时序（飞控偶发未就绪、线材干扰时可略加大）
const unsigned long MAVLINK_UART_SETTLE_MS      = 200;   // 上电后 UART 引脚与电平稳定
const uint8_t       MAVLINK_BAUD_DETECT_PASSES  = 2;     // 波特率整轮扫描重复次数
const unsigned long MAVLINK_BAUD_STABLE_MS      = 450;   // 每种波特率下 begin 后等待首包
const unsigned long MAVLINK_BAUD_PARSE_MS       = 2400;  // 每种波特率下解析时间窗
const unsigned long MAVLINK_RX_FLUSH_MS         = 60;    // 切换波特率时清空 RX 时间窗
const unsigned long MAVLINK_AFTER_WIFI_MS       = 250;   // WiFi 就绪后短让出，过长会推迟业务
const unsigned long PARAM_LIST_STAGGER_MS       = 500;   // 连续 PARAM_REQUEST_LIST 间隔
const unsigned long PARAM_LIST_RETRY_FAST_MS    = 2000;  // 尚未收到任一 PARAM 时的重发间隔
const uint8_t       PARAM_LIST_FAST_TRIES       = 8;     // 使用上述快间隔的次数
const unsigned long PARAM_LIST_RETRY_SLOW_MS    = 6500;  // 之后降频，减轻链路压力

// 参数表突发接收与「补洞」速度（见 README / 说明：与 MP 差异）
#define MAVLINK_SERIAL_RX_BUFFER        8192    // UART RX 缓冲，须 setRxBufferSize 在 begin 之前；越大越不易在 PARAM 洪峰时丢包
const uint16_t      MAVLINK_RX_BYTES_PER_LOOP   = 1024; // 单轮默认预算（0=用此值）
const uint16_t      MAVLINK_RX_SLICE_BYTES      = 256;  // 与 Web 交错时每小口字节数
const uint8_t       MAVLINK_WEB_INTERLEAVE      = 3;    // 与 Web 交错轮数；过大增加每圈延迟，过小易顶满 UART
const unsigned long PARAM_GAP_BURST_INTERVAL_MS = 18;   // 补拉缺失索引时，每批之间的最小间隔 (ms)
const uint8_t       PARAM_GAP_BURST_COUNT       = 24;   // 每批连续发出的 PARAM_REQUEST_READ 个数
const unsigned long SERVO_PARAM_PUMP_MS         = 12;   // 舵机参数补拉间隔；仅 RX 队列较空时发送（见 pump）

// MAVLink系统配置
#define SYS_ID 1
#define COMP_ID 200
#define TARGET_SYS 1
#define TARGET_COMP 1

// 校准常量
#ifndef MAV_CMD_ACCELCAL_VEHICLE_POS
#define MAV_CMD_ACCELCAL_VEHICLE_POS 42429
#endif
#ifndef ACCELCAL_VEHICLE_POS_SUCCESS
#define ACCELCAL_VEHICLE_POS_SUCCESS 16777215
#endif
#ifndef ACCELCAL_VEHICLE_POS_FAILED
#define ACCELCAL_VEHICLE_POS_FAILED 16777216
#endif
#ifndef ACCELCAL_VEHICLE_POS_LEVEL
#define ACCELCAL_VEHICLE_POS_LEVEL 1
#endif
#ifndef ACCELCAL_VEHICLE_POS_LEFT
#define ACCELCAL_VEHICLE_POS_LEFT 2
#endif
#ifndef ACCELCAL_VEHICLE_POS_RIGHT
#define ACCELCAL_VEHICLE_POS_RIGHT 3
#endif
#ifndef ACCELCAL_VEHICLE_POS_NOSEDOWN
#define ACCELCAL_VEHICLE_POS_NOSEDOWN 4
#endif
#ifndef ACCELCAL_VEHICLE_POS_NOSEUP
#define ACCELCAL_VEHICLE_POS_NOSEUP 5
#endif
#ifndef ACCELCAL_VEHICLE_POS_BACK
#define ACCELCAL_VEHICLE_POS_BACK 6
#endif

// 飞行模式配置
struct PlaneMode {
  uint8_t num;
  const char* name;
};

const PlaneMode PLANE_MODES[] = {
  {0,  "Manual"},
  {1,  "Circle"},
  {2,  "Stabilize"},
  {3,  "Training"},
  {4,  "Acro"},
  {5,  "FBWA"},
  {6,  "FBWB"},
  {7,  "Cruise"},
  {8,  "Autotune"},
  {10, "Auto"},
  {11, "RTL"},
  {12, "Loiter"},
  {13, "Takeoff"},
  {14, "Avoid_ADSB"},
  {15, "Guided"},  
  {17, "QStabilize"},
  {18, "QHover"},
  {19, "QLoiter"},
  {20, "QLand"},
  {21, "QRTL"},
  {22, "QAutotune"},
  {23, "QAcro"},
  {24, "THERMAL"},
  {25, "Loiter TO QLand"},
  {26, "AUTOLAND"}
};

const int PLANE_MODE_COUNT = sizeof(PLANE_MODES)/sizeof(PLANE_MODES[0]);
const int FLTMODE_PARAM_COUNT = 6;
const char* FLTMODE_PARAMS[] = {"FLTMODE1", "FLTMODE2", "FLTMODE3", "FLTMODE4", "FLTMODE5", "FLTMODE6"};

// 校准位置消息
struct CalibrationPosition {
  uint8_t position;
  const char* message;
};

const CalibrationPosition CALIB_POSITIONS[] = {
  {ACCELCAL_VEHICLE_POS_LEVEL, "STEP 1/6: Place aircraft LEVEL (horizontal) on flat surface, then click 'Calibrate Accel' button"},
  {ACCELCAL_VEHICLE_POS_LEFT, "STEP 2/6: Tilt aircraft LEFT side down (roll left), then click 'Calibrate Accel' button"},
  {ACCELCAL_VEHICLE_POS_RIGHT, "STEP 3/6: Tilt aircraft RIGHT side down (roll right), then click 'Calibrate Accel' button"},
  {ACCELCAL_VEHICLE_POS_NOSEDOWN, "STEP 4/6: Tilt aircraft NOSE DOWN (pitch down), then click 'Calibrate Accel' button"},
  {ACCELCAL_VEHICLE_POS_NOSEUP, "STEP 5/6: Tilt aircraft NOSE UP (pitch up), then click 'Calibrate Accel' button"},
  {ACCELCAL_VEHICLE_POS_BACK, "STEP 6/6: Tilt aircraft BACK (tail down), then click 'Calibrate Accel' button"}
};

const int CALIB_POSITION_COUNT = sizeof(CALIB_POSITIONS)/sizeof(CALIB_POSITIONS[0]);

// 校准步骤到位置映射
const uint8_t STEP_TO_POSITION[] = {
  ACCELCAL_VEHICLE_POS_LEVEL,   // STEP_LEVEL
  ACCELCAL_VEHICLE_POS_LEFT,    // STEP_LEFT
  ACCELCAL_VEHICLE_POS_RIGHT,   // STEP_RIGHT
  ACCELCAL_VEHICLE_POS_NOSEDOWN, // STEP_NOSEDOWN
  ACCELCAL_VEHICLE_POS_NOSEUP,  // STEP_NOSEUP
  ACCELCAL_VEHICLE_POS_BACK     // STEP_BACK
};

// 校准状态枚举
enum CalibrationType {
  CALIB_NONE = 0,
  CALIB_LEVEL = 1,
  CALIB_ACCEL = 2
};

enum AccelCalibrationState {
  ACCEL_STATE_IDLE = 0,
  ACCEL_STATE_STARTING = 1,
  ACCEL_STATE_WAITING_POSITION = 2,
  ACCEL_STATE_POSITION_REQUESTED = 3,
  ACCEL_STATE_COMPLETED = 4
};

// 水平校准状态
enum LevelCalibrationState {
  LEVEL_STATE_IDLE = 0,
  LEVEL_STATE_STARTED = 1, // 已发送开始命令，等待用户确认
  LEVEL_STATE_WAITING_RESULT = 2 // 已发送最终命令，等待飞控结果
};

// 校准步骤状态
enum CalibrationStepState {
  STEP_STATE_WAITING_PLACEMENT = 0,  // 等待用户放置飞机
  STEP_STATE_SENDING_COMMAND = 1,    // 正在发送命令
  STEP_STATE_WAITING_ACK = 2,        // 等待飞控ACK
  STEP_STATE_COMPLETED = 3           // 步骤完成
};

// 简化的校准步骤
enum SimpleCalibrationStep {
  STEP_NONE = 0,
  STEP_LEVEL = 1,
  STEP_LEFT = 2,
  STEP_RIGHT = 3,
  STEP_NOSEDOWN = 4,
  STEP_NOSEUP = 5,
  STEP_BACK = 6,
  STEP_COMPLETE = 7
};

// 舵机输出数据结构
struct ServoData {
  uint16_t position;
  uint16_t min_pos;
  uint16_t max_pos;
  uint16_t trim_pos;
  uint8_t function;
  bool reversed;
  bool valid;
};

// 全局变量声明
extern String ipStr;
extern String systemMessage;
extern String operationMessage;
extern float voltage;
extern String flightMode;
extern int gpsCount;
extern bool isArmed;
extern std::map<String, float> paramMap;
extern std::set<uint16_t> paramIndexSet;
extern uint16_t param_total;
extern bool param_ready;
extern unsigned long last_param_recv;
extern bool levelCalibrated;
extern bool accelCalibrated;
extern ServoData servoOutputs[16];

// 校准状态管理
extern bool waitingForAck;
extern unsigned long lastCommandTime;
extern unsigned long commandTimeout
;
extern CalibrationType currentCalibration;
extern AccelCalibrationState currentAccelState;
extern LevelCalibrationState currentLevelState;
extern CalibrationStepState currentStepState;
extern SimpleCalibrationStep currentSimpleStep;
extern uint8_t currentPosition;

// ARM/STATUSTEXT相关
extern bool lastArmCommand;
extern String armErrors[10];
extern int armErrorCount;
extern unsigned long lastStatusTextTime;
extern String lastStatusText;

// 参数设置相关
extern String lastSetResult;
extern unsigned long lastSetTime;
extern std::map<String, float> pendingSetMap;

#endif 