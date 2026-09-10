#include "app_state.h"

Adafruit_ST7789 display = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
HardwareSerial mavSerial(2);
WebServer server(80);

String ipStr = "192.168.6.6";
String systemMessage = "Initializing...";
String operationMessage = "Ready";
float voltage = 0.0f;
String flightMode = "-";
int gpsCount = 0;
bool isArmed = false;

uint16_t rc_fltmode_pwm_us = 0;
bool rc_fltmode_pwm_valid = false;

long detectedBaud = 0;
int mavRxPin = MAVLINK_PIN_A;
int mavTxPin = MAVLINK_PIN_B;
bool uartPinsSwapped = false;

std::map<String, float> paramMap;
std::set<uint16_t> paramIndexSet;
uint16_t param_total = 0;
bool param_ready = false;
unsigned long last_param_recv = 0;

int scrollOffset = 0;
int scrollOffsetSet = 0;

bool lastArmCommand = false;
String armErrors[10];
int armErrorCount = 0;
unsigned long lastStatusTextTime = 0;
String lastStatusText = "";

bool levelCalibrated = false;
bool accelCalibrated = false;
ServoData servoOutputs[16];

bool waitingForAck = false;
unsigned long lastCommandTime = 0;
CalibrationType currentCalibration = CALIB_NONE;
AccelCalibrationState currentAccelState = ACCEL_STATE_IDLE;
LevelCalibrationState currentLevelState = LEVEL_STATE_IDLE;
CalibrationStepState currentStepState = STEP_STATE_WAITING_PLACEMENT;
SimpleCalibrationStep currentSimpleStep = STEP_NONE;
uint8_t currentPosition = 0;

String lastSetResult = "";
unsigned long lastSetTime = 0;
std::map<String, float> pendingSetMap;

bool armDenied = false;
bool servo_param_pump_done = false;
bool fcHeartbeatSeen = false;
unsigned long lastFcHeartbeatMs = 0;
bool paramListRequestedAfterLink = false;
