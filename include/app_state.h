#ifndef APP_STATE_H
#define APP_STATE_H

#include "config.h"

extern Adafruit_ST7789 display;
extern HardwareSerial mavSerial;
extern WebServer server;

extern String ipStr;
extern String systemMessage;
extern String operationMessage;
extern float voltage;
extern String flightMode;
extern int gpsCount;
extern bool isArmed;

extern uint16_t rc_fltmode_pwm_us;
extern bool rc_fltmode_pwm_valid;

extern long detectedBaud;
extern int mavRxPin;
extern int mavTxPin;
extern bool uartPinsSwapped;

extern std::map<String, float> paramMap;
extern std::set<uint16_t> paramIndexSet;
extern uint16_t param_total;
extern bool param_ready;
extern unsigned long last_param_recv;

extern int scrollOffset;
extern int scrollOffsetSet;

extern bool lastArmCommand;
extern String armErrors[10];
extern int armErrorCount;
extern unsigned long lastStatusTextTime;
extern String lastStatusText;

extern bool levelCalibrated;
extern bool accelCalibrated;
extern ServoData servoOutputs[16];

extern bool waitingForAck;
extern unsigned long lastCommandTime;
extern CalibrationType currentCalibration;
extern AccelCalibrationState currentAccelState;
extern LevelCalibrationState currentLevelState;
extern CalibrationStepState currentStepState;
extern SimpleCalibrationStep currentSimpleStep;
extern uint8_t currentPosition;

extern String lastSetResult;
extern unsigned long lastSetTime;
extern std::map<String, float> pendingSetMap;

extern bool armDenied;
extern bool servo_param_pump_done;
extern bool fcHeartbeatSeen;
extern unsigned long lastFcHeartbeatMs;
extern bool paramListRequestedAfterLink;

#endif
