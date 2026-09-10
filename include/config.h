#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <map>
#include <set>
#include <vector>

// 2.0" ST7789 TFT 240x320（横屏旋转后 320x240）
#define TFT_WIDTH  240
#define TFT_HEIGHT 320
#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 240

/*
 * ESP32-S3 焊接引脚（与飞控 UART 15/16 不冲突）
 *
 *  屏丝印      ESP32-S3
 *  --------    --------
 *  VCC      -> 3.3V
 *  GND      -> GND
 *  SCL/CLK  -> GPIO12
 *  SDA/MOSI -> GPIO11
 *  RES/RST  -> GPIO8
 *  DC       -> GPIO9
 *  CS       -> GPIO10
 *  BLK/BL   -> GPIO7（背光，高电平亮；也可直接接 3.3V）
 *  （无触摸 / 不用 MISO）
 */
#define TFT_SCLK 12
#define TFT_MOSI 11
#define TFT_RST   8
#define TFT_DC    9
#define TFT_CS   10
#define TFT_BL    7

// MAVLink UART: two GPIOs; firmware tries both polarities (auto swap)
#define MAVLINK_PIN_A 15
#define MAVLINK_PIN_B 16
#define MAVLINK_RX MAVLINK_PIN_A
#define MAVLINK_TX MAVLINK_PIN_B

/*
 * 5 向按键（摇杆/五向开关）—— 共地，另一端接 GPIO；固件 INPUT_PULLUP，按下为低电平
 *
 *  功能        ESP32-S3 GPIO
 *  --------    ------------
 *  上 UP    -> GPIO4
 *  下 DOWN  -> GPIO5
 *  左 LEFT  -> GPIO6
 *  右 RIGHT -> GPIO13
 *  确认 OK  -> GPIO14
 *
 *  预留未占用（后续可扩）：GPIO1/2/17/18/21 等
 *  避开：TFT 7–12、UART 15/16、USB 19/20、strap 0/3/45/46、UART0 43/44
 */
#define BTN_UP_PIN    4
#define BTN_DOWN_PIN  5
#define BTN_LEFT_PIN  6
#define BTN_RIGHT_PIN 13
#define BTN_OK_PIN    14

#define SSID "ESP32-MAV"
#define PASSWORD "12345678"

const unsigned long CALIB_MSG_DURATION = 8000;
const unsigned long CMD_TIMEOUT = 15000;

const unsigned long MAVLINK_UART_SETTLE_MS = 80;
const uint8_t MAVLINK_BAUD_DETECT_PASSES = 1;
const unsigned long MAVLINK_BAUD_STABLE_MS = 25;
const unsigned long MAVLINK_BAUD_PARSE_MS = 550;
const unsigned long MAVLINK_RX_FLUSH_MS = 15;
const unsigned long MAVLINK_AFTER_WIFI_MS = 150;
const unsigned long PARAM_LIST_STAGGER_MS = 400;
const unsigned long PARAM_LIST_RETRY_FAST_MS = 800;
const uint8_t PARAM_LIST_FAST_TRIES = 12;
const unsigned long PARAM_LIST_RETRY_SLOW_MS = 3000;
const unsigned long PARAM_STALL_RELIST_MS = 4000;
const unsigned long FC_HEARTBEAT_TIMEOUT_MS = 4000;
const unsigned long GCS_HEARTBEAT_MS = 1000;

#define MAVLINK_SERIAL_RX_BUFFER 8192
const uint16_t MAVLINK_RX_BYTES_PER_LOOP = 2048;
const uint16_t MAVLINK_RX_SLICE_BYTES = 384;
const uint8_t MAVLINK_WEB_INTERLEAVE = 3;
const unsigned long PARAM_GAP_BURST_INTERVAL_MS = 12;
const uint8_t PARAM_GAP_BURST_COUNT = 32;
const unsigned long SERVO_PARAM_PUMP_MS = 12;

#define PARAM_BITMAP_MAX 2048

// 临时：跳过飞控连接，直接看已连接界面效果（测完改回 0）
#define DEMO_SKIP_LINK 0

// GCS identity (must not collide with vehicle sysid 1)
#define SYS_ID 255
#define COMP_ID 190
#define TARGET_SYS 1
#define TARGET_COMP 1

struct PlaneMode {
  uint8_t num;
  const char* name;
};

const PlaneMode PLANE_MODES[] = {
  {0, "Manual"},
  {1, "Circle"},
  {2, "Stabilize"},
  {3, "Training"},
  {4, "Acro"},
  {5, "FBWA"},
  {6, "FBWB"},
  {7, "Cruise"},
  {8, "Autotune"},
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

const int PLANE_MODE_COUNT = sizeof(PLANE_MODES) / sizeof(PLANE_MODES[0]);
const int FLTMODE_PARAM_COUNT = 6;
static const char* const FLTMODE_PARAMS[] = {"FLTMODE1", "FLTMODE2", "FLTMODE3", "FLTMODE4", "FLTMODE5", "FLTMODE6"};

struct CalibrationPosition {
  uint8_t position;
  const char* message;
};

const CalibrationPosition CALIB_POSITIONS[] = {
  {1, "STEP 1/6: Place aircraft LEVEL (horizontal) on flat surface, then click 'Calibrate Accel' button"},
  {2, "STEP 2/6: Tilt aircraft LEFT side down (roll left), then click 'Calibrate Accel' button"},
  {3, "STEP 3/6: Tilt aircraft RIGHT side down (roll right), then click 'Calibrate Accel' button"},
  {4, "STEP 4/6: Tilt aircraft NOSE DOWN (pitch down), then click 'Calibrate Accel' button"},
  {5, "STEP 5/6: Tilt aircraft NOSE UP (pitch up), then click 'Calibrate Accel' button"},
  {6, "STEP 6/6: Tilt aircraft BACK (tail down), then click 'Calibrate Accel' button"}
};

const int CALIB_POSITION_COUNT = sizeof(CALIB_POSITIONS) / sizeof(CALIB_POSITIONS[0]);

const uint8_t STEP_TO_POSITION[] = {1, 2, 3, 4, 5, 6};

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

enum LevelCalibrationState {
  LEVEL_STATE_IDLE = 0,
  LEVEL_STATE_STARTED = 1,
  LEVEL_STATE_WAITING_RESULT = 2
};

enum CalibrationStepState {
  STEP_STATE_WAITING_PLACEMENT = 0,
  STEP_STATE_SENDING_COMMAND = 1,
  STEP_STATE_WAITING_ACK = 2,
  STEP_STATE_COMPLETED = 3
};

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

struct ServoData {
  uint16_t position;
  uint16_t min_pos;
  uint16_t max_pos;
  uint16_t trim_pos;
  uint8_t function;
  bool reversed;
  bool valid;
};

#endif
