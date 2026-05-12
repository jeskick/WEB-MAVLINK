#ifndef MAVLINK_HANDLER_H
#define MAVLINK_HANDLER_H

#include "config.h"
#include <c_library_v2-master/common/mavlink.h>
#include <c_library_v2-master/ardupilotmega/mavlink.h>

// 外部声明
extern HardwareSerial mavSerial;
extern std::map<String, float> paramMap;
extern std::set<uint16_t> paramIndexSet;
extern uint16_t param_total;
extern bool param_ready;
extern unsigned long last_param_recv;
extern String systemMessage;
extern String operationMessage;
extern float voltage;
extern String flightMode;
extern int gpsCount;
extern bool isArmed;
extern ServoData servoOutputs[16];

// ARM/STATUSTEXT相关
extern bool lastArmCommand;
extern String armErrors[10];
extern int armErrorCount;
extern unsigned long lastStatusTextTime;
extern String lastStatusText;

// 校准状态相关
extern CalibrationType currentCalibration;
extern AccelCalibrationState currentAccelState;
extern LevelCalibrationState currentLevelState;
extern CalibrationStepState currentStepState;
extern SimpleCalibrationStep currentSimpleStep;
extern uint8_t currentPosition;
extern bool waitingForAck;
extern unsigned long lastCommandTime;
extern bool levelCalibrated;
extern bool accelCalibrated;

// 参数设置相关
extern String lastSetResult;
extern unsigned long lastSetTime;
extern std::map<String, float> pendingSetMap;

// 飞行模式 RC 通道 PWM（由 RC_CHANNELS + 参数 FLTMODE_CH 更新）
extern uint16_t rc_fltmode_pwm_us;
extern bool rc_fltmode_pwm_valid;

// 在文件顶部全局变量区添加：
bool armDenied = false;
bool servo_param_pump_done = false;

// 飞控状态多行缓冲（STATUSTEXT 连续多条时保留，避免只记住最后一条）
#ifndef STATUS_MSG_RING_MAX
#define STATUS_MSG_RING_MAX 10
#endif
static String g_statusMsgRing[STATUS_MSG_RING_MAX];
static uint8_t g_statusMsgRingCount = 0;

inline void statusRingPushDistinct(const String& s) {
  if (s.length() == 0) return;
  if (g_statusMsgRingCount > 0 && g_statusMsgRing[g_statusMsgRingCount - 1] == s) return;
  if (g_statusMsgRingCount < STATUS_MSG_RING_MAX) {
    g_statusMsgRing[g_statusMsgRingCount++] = s;
  } else {
    for (uint8_t i = 0; i < STATUS_MSG_RING_MAX - 1; ++i) {
      g_statusMsgRing[i] = g_statusMsgRing[i + 1];
    }
    g_statusMsgRing[STATUS_MSG_RING_MAX - 1] = s;
    g_statusMsgRingCount = STATUS_MSG_RING_MAX;
  }
}

inline String statusRingJoin() {
  String o;
  for (uint8_t i = 0; i < g_statusMsgRingCount; ++i) {
    if (i) o += '\n';
    o += g_statusMsgRing[i];
  }
  return o;
}

// 函数声明
void handleMavlink(uint16_t maxRxBytes = 0);
void processStatusText(const mavlink_statustext_t& statustext);
void updateSystemMessage();

// 校准相关函数
String getCalibrationPositionMessage(uint8_t position);
void sendMavlinkCommand(uint16_t command, float param1, float param2, float param3, float param4, float param5, float param6, float param7);

// 请求参数列表
void requestParamList() {
  mavlink_message_t msg;
  mavlink_msg_param_request_list_pack(SYS_ID, COMP_ID, &msg, TARGET_SYS, TARGET_COMP);
  uint8_t buf[MAVLINK_MAX_PACKET_LEN];
  uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);
  mavSerial.write(buf, len);
  Serial.println("Requesting parameter list...");
  operationMessage = "Requesting parameters...";
}

// 请求特定参数
void requestParam(uint16_t index) {
  mavlink_message_t msg;
  mavlink_msg_param_request_read_pack(SYS_ID, COMP_ID, &msg, TARGET_SYS, TARGET_COMP, NULL, index);
  uint8_t buf[MAVLINK_MAX_PACKET_LEN];
  uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);
  mavSerial.write(buf, len);
}

// 检查并请求缺失的参数
void checkAndRequestMissingParams() {
  if (param_ready) return;

  // 尚未收到任何 PARAM_VALUE：setup 已 stagger 发过列表，此处按快/慢间隔补发
  if (param_total == 0) {
    static unsigned long last_list_request = 0;
    static uint8_t list_tries = 0;
    if (last_list_request == 0) {
      last_list_request = millis();
      return;
    }
    unsigned long gap = (list_tries < PARAM_LIST_FAST_TRIES)
                           ? PARAM_LIST_RETRY_FAST_MS
                           : PARAM_LIST_RETRY_SLOW_MS;
    if (millis() - last_list_request < gap) return;
    last_list_request = millis();
    list_tries++;
    requestParamList();
    Serial.println("Retry: PARAM_REQUEST_LIST (no PARAM_VALUE yet)");
    return;
  }

  static unsigned long last_request = 0;
  if (millis() - last_request < PARAM_GAP_BURST_INTERVAL_MS) return;

  // 一批补多个缺失索引（仍受 PARAM_GAP_BURST_INTERVAL_MS 节流），避免丢包后 100ms×N 极慢
  uint8_t sent = 0;
  for (uint16_t i = 0; i < param_total && sent < PARAM_GAP_BURST_COUNT; i++) {
    if (paramIndexSet.find(i) == paramIndexSet.end()) {
      requestParam(i);
      sent++;
    }
  }
  if (sent > 0) {
    last_request = millis();
  }
}

static uint16_t mavlink_rc_raw8_us(const mavlink_rc_channels_raw_t& rc, int ch1based) {
  switch (ch1based) {
    case 1: return rc.chan1_raw;
    case 2: return rc.chan2_raw;
    case 3: return rc.chan3_raw;
    case 4: return rc.chan4_raw;
    case 5: return rc.chan5_raw;
    case 6: return rc.chan6_raw;
    case 7: return rc.chan7_raw;
    case 8: return rc.chan8_raw;
    default: return UINT16_MAX;
  }
}

static uint16_t mavlink_rc_channel_us(const mavlink_rc_channels_t& rc, int ch1based) {
  switch (ch1based) {
    case 1: return rc.chan1_raw;
    case 2: return rc.chan2_raw;
    case 3: return rc.chan3_raw;
    case 4: return rc.chan4_raw;
    case 5: return rc.chan5_raw;
    case 6: return rc.chan6_raw;
    case 7: return rc.chan7_raw;
    case 8: return rc.chan8_raw;
    case 9: return rc.chan9_raw;
    case 10: return rc.chan10_raw;
    case 11: return rc.chan11_raw;
    case 12: return rc.chan12_raw;
    case 13: return rc.chan13_raw;
    case 14: return rc.chan14_raw;
    case 15: return rc.chan15_raw;
    case 16: return rc.chan16_raw;
    case 17: return rc.chan17_raw;
    case 18: return rc.chan18_raw;
    default: return UINT16_MAX;
  }
}

// MAVLink消息处理：maxRxBytes==0 时用 MAVLINK_RX_BYTES_PER_LOOP；否则限制本调用读字节数，便于与 Web 交错
void handleMavlink(uint16_t maxRxBytes) {
  static mavlink_message_t msg;
  static mavlink_status_t status;

  int rxBudget = (maxRxBytes == 0) ? (int)MAVLINK_RX_BYTES_PER_LOOP : (int)maxRxBytes;
  int consumed = 0;
  while (mavSerial.available() && rxBudget-- > 0) {
    uint8_t c = mavSerial.read();
    if ((++consumed & 0xFF) == 0) yield();

    if (mavlink_parse_char(MAVLINK_COMM_0, c, &msg, &status)) {
      switch (msg.msgid) {
        case MAVLINK_MSG_ID_HEARTBEAT: {
          mavlink_heartbeat_t heartbeat;
          mavlink_msg_heartbeat_decode(&msg, &heartbeat);
          // 飞行模式
          for (int i = 0; i < PLANE_MODE_COUNT; ++i) {
            if (PLANE_MODES[i].num == heartbeat.custom_mode) {
              flightMode = PLANE_MODES[i].name;
              break;
            }
          }
          // 解锁状态
          isArmed = (heartbeat.base_mode & MAV_MODE_FLAG_SAFETY_ARMED);
          break;
        }
        case MAVLINK_MSG_ID_SYS_STATUS: {
          mavlink_sys_status_t sys_status;
          mavlink_msg_sys_status_decode(&msg, &sys_status);
          voltage = sys_status.voltage_battery / 1000.0;
          break;
        }
        case MAVLINK_MSG_ID_BATTERY_STATUS: {
          mavlink_battery_status_t bat;
          mavlink_msg_battery_status_decode(&msg, &bat);
          if (bat.voltages[0] != UINT16_MAX) {
            voltage = bat.voltages[0] / 1000.0;
          }
          break;
        }
        case MAVLINK_MSG_ID_GPS_RAW_INT: {
          mavlink_gps_raw_int_t gps_raw;
          mavlink_msg_gps_raw_int_decode(&msg, &gps_raw);
          gpsCount = (gps_raw.fix_type >= 2) ? gps_raw.satellites_visible : 0;
          break;
        }
        case MAVLINK_MSG_ID_RC_CHANNELS: {
          mavlink_rc_channels_t rc;
          mavlink_msg_rc_channels_decode(&msg, &rc);
          int ch = 0;
          if (paramMap.count(String("FLTMODE_CH"))) {
            ch = (int)(paramMap[String("FLTMODE_CH")] + 0.5f);
          } else if (paramMap.count(String("MODE_CH"))) {
            ch = (int)(paramMap[String("MODE_CH")] + 0.5f);
          }
          if (ch >= 1 && ch <= 18 && rc.chancount > 0 && (unsigned)ch <= rc.chancount) {
            uint16_t v = mavlink_rc_channel_us(rc, ch);
            if (v != UINT16_MAX) {
              rc_fltmode_pwm_us = v;
              rc_fltmode_pwm_valid = true;
              break;
            }
          }
          rc_fltmode_pwm_valid = false;
          rc_fltmode_pwm_us = 0;
          break;
        }
        case MAVLINK_MSG_ID_RC_CHANNELS_RAW: {
          mavlink_rc_channels_raw_t rcr;
          mavlink_msg_rc_channels_raw_decode(&msg, &rcr);
          if (rcr.port != 0) break;
          int ch = 0;
          if (paramMap.count(String("FLTMODE_CH"))) {
            ch = (int)(paramMap[String("FLTMODE_CH")] + 0.5f);
          } else if (paramMap.count(String("MODE_CH"))) {
            ch = (int)(paramMap[String("MODE_CH")] + 0.5f);
          }
          if (ch >= 1 && ch <= 8) {
            uint16_t v = mavlink_rc_raw8_us(rcr, ch);
            if (v != UINT16_MAX) {
              rc_fltmode_pwm_us = v;
              rc_fltmode_pwm_valid = true;
              break;
            }
          }
          rc_fltmode_pwm_valid = false;
          rc_fltmode_pwm_us = 0;
          break;
        }
        case MAVLINK_MSG_ID_PARAM_VALUE: {
          mavlink_param_value_t param;
          mavlink_msg_param_value_decode(&msg, &param);
          char key[17];
          strncpy(key, param.param_id, 16);
          key[16] = '\0';
          String keyStr = String(key);
          paramMap[keyStr] = param.param_value;
          paramIndexSet.insert(param.param_index);
          if (param_total == 0) param_total = param.param_count;
          
          // 使用paramIndexSet.size()来判断参数是否全部接收完成
          if (paramIndexSet.size() >= param_total && param_total > 0) {
            if (!param_ready) {
              param_ready = true;
              operationMessage = "Params OK " + String(paramMap.size()) + "/" + String(param_total);
              Serial.println("Parameter loading completed! Total: " + String(param_total) + ", Received: " + String(paramIndexSet.size()));
            }
          }
          
          last_param_recv = millis();
          
          // 检查是否为待确认的参数，反馈最终结果
          if (pendingSetMap.count(keyStr)) {
            float target = pendingSetMap[keyStr];
            if (fabs(param.param_value - target) < 1e-3) {
              lastSetResult = "SET OK: " + keyStr + "=" + String(param.param_value);
              pendingSetMap.erase(keyStr);
            } else {
              lastSetResult = "SET FAIL: " + keyStr + "=" + String(param.param_value) + " (target " + String(target) + ")";
            }
            lastSetTime = millis();
          }
          break;
        }
        case MAVLINK_MSG_ID_SERVO_OUTPUT_RAW: {
          mavlink_servo_output_raw_t servo_raw;
          mavlink_msg_servo_output_raw_decode(&msg, &servo_raw);
          uint16_t* arr[] = {
            &servo_raw.servo1_raw, &servo_raw.servo2_raw, &servo_raw.servo3_raw, &servo_raw.servo4_raw,
            &servo_raw.servo5_raw, &servo_raw.servo6_raw, &servo_raw.servo7_raw, &servo_raw.servo8_raw
          };
          for (int i = 0; i < 8; i++) {
            if (*arr[i] != UINT16_MAX) {
              servoOutputs[i].position = *arr[i];
              servoOutputs[i].valid = true;
            }
          }
          break;
        }
        case MAVLINK_MSG_ID_COMMAND_ACK: {
          mavlink_command_ack_t ack;
          mavlink_msg_command_ack_decode(&msg, &ack);
          Serial.print("Command ACK received - Command: ");
          Serial.print(ack.command);
          Serial.print(", Result: ");
          Serial.println(ack.result);
          
          if (ack.command == MAV_CMD_COMPONENT_ARM_DISARM) {
            if (ack.result == MAV_RESULT_ACCEPTED) {
              isArmed = lastArmCommand;
              operationMessage = lastArmCommand ? "ARM command executed successfully" : "DISARM command executed successfully";
              Serial.println(operationMessage);
              armDenied = false;
            } else {
              // 只要不是ACCEPTED都认为被拒绝，可以尝试force arm
              operationMessage = lastArmCommand ? "ARM command denied/failed" : "DISARM command denied/failed";
              Serial.println(operationMessage);
              armDenied = true;
              if (lastArmCommand && (ack.result == MAV_RESULT_DENIED || ack.result == MAV_RESULT_FAILED)) {
                armErrorCount = 0;
              }
            }
          }
          
          // 处理校准命令的ACK
          if (ack.command == MAV_CMD_PREFLIGHT_CALIBRATION) {
            if (ack.result == MAV_RESULT_ACCEPTED) {
              if (currentCalibration == CALIB_LEVEL) {
                operationMessage = "Command accepted, waiting for final result...";
                Serial.println(operationMessage);
                // 注意：不重置waitingForAck，继续等待最终结果
              } else if (currentCalibration == CALIB_ACCEL) {
                Serial.println("Accel calibration command accepted by autopilot");
                operationMessage = "Accel calibration command accepted by autopilot";
                waitingForAck = false;
                if (currentAccelState == ACCEL_STATE_STARTING) {
                  currentAccelState = ACCEL_STATE_WAITING_POSITION;
                  Serial.println("Accel calibration started, waiting for position requests...");
                }
              }
            } else if (ack.result == MAV_RESULT_DENIED) {
              if (currentCalibration == CALIB_LEVEL) {
                operationMessage = "Level calibration command rejected! Result: " + String(ack.result);
                currentCalibration = CALIB_NONE;
                currentLevelState = LEVEL_STATE_IDLE;
                waitingForAck = false;
                Serial.println(operationMessage);
              } else if (currentCalibration == CALIB_ACCEL) {
                Serial.println("Accel calibration command denied by autopilot");
                operationMessage = "Accel calibration command denied by autopilot - Please check autopilot status and try again";
                currentCalibration = CALIB_NONE;
                currentAccelState = ACCEL_STATE_IDLE;
                currentSimpleStep = STEP_NONE;
                currentStepState = STEP_STATE_WAITING_PLACEMENT;
                waitingForAck = false;
              }
            } else if (ack.result == MAV_RESULT_FAILED) {
              if (currentCalibration == CALIB_LEVEL) {
                operationMessage = "Level calibration command failed! Result: " + String(ack.result);
                currentCalibration = CALIB_NONE;
                currentLevelState = LEVEL_STATE_IDLE;
                waitingForAck = false;
                Serial.println(operationMessage);
              } else if (currentCalibration == CALIB_ACCEL) {
                Serial.println("Accel calibration command failed");
                operationMessage = "Accel calibration command failed - Please check autopilot status and try again";
                currentCalibration = CALIB_NONE;
                currentAccelState = ACCEL_STATE_IDLE;
                currentSimpleStep = STEP_NONE;
                currentStepState = STEP_STATE_WAITING_PLACEMENT;
                waitingForAck = false;
              }
            }
          }
          
          // 处理加速度校准位置命令的ACK
          if (ack.command == MAV_CMD_ACCELCAL_VEHICLE_POS) {
            if (ack.result == MAV_RESULT_ACCEPTED) {
              Serial.println("Position command accepted by autopilot");
              operationMessage = "Position command accepted by autopilot";
              waitingForAck = false;
              currentStepState = STEP_STATE_COMPLETED;
              Serial.println("Position command completed, ready for next step");
            } else if (ack.result == MAV_RESULT_DENIED) {
              Serial.println("Position command denied by autopilot");
              operationMessage = "Position command denied by autopilot - Please check aircraft position and try again";
              currentCalibration = CALIB_NONE;
              currentAccelState = ACCEL_STATE_IDLE;
              currentSimpleStep = STEP_NONE;
              currentStepState = STEP_STATE_WAITING_PLACEMENT;
              waitingForAck = false;
            } else if (ack.result == MAV_RESULT_FAILED) {
              Serial.println("Position command failed");
              operationMessage = "Position command failed - Please check aircraft position and try again";
              currentCalibration = CALIB_NONE;
              currentAccelState = ACCEL_STATE_IDLE;
              currentSimpleStep = STEP_NONE;
              currentStepState = STEP_STATE_WAITING_PLACEMENT;
              waitingForAck = false;
            }
          }
          break;
        }
        case MAVLINK_MSG_ID_COMMAND_LONG: {
          mavlink_command_long_t cmd_long;
          mavlink_msg_command_long_decode(&msg, &cmd_long);

          if (cmd_long.command == MAV_CMD_ACCELCAL_VEHICLE_POS && currentCalibration == CALIB_ACCEL) {
            // 只有在加速度计校准期间才处理这些消息
            Serial.println("*** ACCELCAL_VEHICLE_POS command received from autopilot ***");
            
            if (cmd_long.param1 >= ACCELCAL_VEHICLE_POS_LEVEL && cmd_long.param1 <= ACCELCAL_VEHICLE_POS_BACK) {
              // 飞控要求GCS将飞机放在特定位置
              Serial.print("Autopilot requesting position: ");
              Serial.println(cmd_long.param1);
              
              if (currentCalibration == CALIB_ACCEL) {
                currentPosition = (uint8_t)cmd_long.param1;
                currentAccelState = ACCEL_STATE_POSITION_REQUESTED;
                
                // 使用新的校准位置消息函数
                operationMessage = getCalibrationPositionMessage((uint8_t)cmd_long.param1);
              }
            } else if (cmd_long.param1 == ACCELCAL_VEHICLE_POS_SUCCESS) {
              if (!accelCalibrated) {
                Serial.println("Accel calibration completed successfully!");
                operationMessage = "Accel calibration completed successfully!";
                currentCalibration = CALIB_NONE;
                currentAccelState = ACCEL_STATE_IDLE;
                accelCalibrated = true;
              }
            } else if (cmd_long.param1 == ACCELCAL_VEHICLE_POS_FAILED) {
              Serial.println("Accel calibration failed!");
              operationMessage = "Accel calibration failed!";
              currentCalibration = CALIB_NONE;
              currentAccelState = ACCEL_STATE_IDLE;
            }
          }
          break;
        }
        case MAVLINK_MSG_ID_COMMAND_INT: {
          // 检查加速度计校准完成消息
          mavlink_command_int_t cmd_int;
          mavlink_msg_command_int_decode(&msg, &cmd_int);
          
          if (cmd_int.command == MAV_CMD_ACCELCAL_VEHICLE_POS && currentCalibration == CALIB_ACCEL) {
            // 只有在加速度计校准期间才处理这些消息
            if (cmd_int.param1 == ACCELCAL_VEHICLE_POS_SUCCESS) {
              if (!accelCalibrated) {
                Serial.println("Accel calibration completed successfully!");
                operationMessage = "Accel calibration completed successfully!";
                currentCalibration = CALIB_NONE;
                currentAccelState = ACCEL_STATE_IDLE;
                accelCalibrated = true;
              }
            } else if (cmd_int.param1 == ACCELCAL_VEHICLE_POS_FAILED) {
              Serial.println("Accel calibration failed!");
              operationMessage = "Accel calibration failed!";
              currentCalibration = CALIB_NONE;
              currentAccelState = ACCEL_STATE_IDLE;
            } else if (cmd_int.param1 >= ACCELCAL_VEHICLE_POS_LEVEL && cmd_int.param1 <= ACCELCAL_VEHICLE_POS_BACK) {
              // 飞控要求GCS将飞机放在特定位置
              Serial.print("Autopilot requesting position: ");
              Serial.println(cmd_int.param1);
              
              if (currentCalibration == CALIB_ACCEL) {
                currentPosition = (uint8_t)cmd_int.param1;
                currentAccelState = ACCEL_STATE_POSITION_REQUESTED;
                
                // 使用新的校准位置消息函数
                operationMessage = getCalibrationPositionMessage((uint8_t)cmd_int.param1);
              }
            }
          }
          break;
        }
        case MAVLINK_MSG_ID_STATUSTEXT: {
          mavlink_statustext_t statustext;
          mavlink_msg_statustext_decode(&msg, &statustext);
          processStatusText(statustext);
          break;
        }
      }
    }
  }
}

// STATUSTEXT消息处理
void processStatusText(const mavlink_statustext_t& statustext) {
  char text[51];
  memset(text, 0, sizeof(text));
  memcpy(text, statustext.text, 50);
  text[50] = '\0';
  String statusStr = String(text);
  lastStatusTextTime = millis();
  if (statusStr.length() > 100) statusStr = statusStr.substring(0, 100);

  Serial.print("Status text from autopilot (severity ");
  Serial.print(statustext.severity);
  Serial.print("): ");
  Serial.println(text);

  // 过滤掉不需要持续显示的消息
  if (statusStr.indexOf("System initialized") >= 0 || 
      statusStr.indexOf("System ready") >= 0 ||
      statusStr.indexOf("Ready") >= 0) {
    // 这些消息不设置到lastStatusText中，避免进入轮播队列
    return;
  }

  // 水平校准完成检测
  if (currentCalibration == CALIB_LEVEL && 
      (statusStr.indexOf("Level") >= 0 || statusStr.indexOf("level") >= 0) &&
      (statusStr.indexOf("complete") >= 0 || statusStr.indexOf("success") >= 0 || 
       statusStr.indexOf("done") >= 0 || statusStr.indexOf("finished") >= 0)) {
    Serial.println("Level calibration completed successfully!");
    operationMessage = "Level calibration completed successfully! Button locked.";
    currentCalibration = CALIB_NONE;
    currentLevelState = LEVEL_STATE_IDLE;
    waitingForAck = false;
    levelCalibrated = true;
    systemMessage = "Level Cal: Completed - Button Locked";
    lastStatusText = systemMessage;
    statusRingPushDistinct(systemMessage);
    systemMessage = statusRingJoin();
    return;
  }

  // 水平校准成功检测 - 检测"Trim OK"消息
  if (currentCalibration == CALIB_LEVEL && statusStr.indexOf("Trim OK") >= 0) {
    Serial.println("Level calibration completed successfully! (Trim OK detected)");
    operationMessage = "Level calibration completed successfully! Trim values updated.";
    currentCalibration = CALIB_NONE;
    currentLevelState = LEVEL_STATE_IDLE;
    waitingForAck = false;
    levelCalibrated = true;
    systemMessage = "Level Cal: Completed - Trim OK";
    lastStatusText = systemMessage;
    statusRingPushDistinct(systemMessage);
    systemMessage = statusRingJoin();
    return;
  }

  // 校准相关
  if (statusStr.indexOf("calibrat") >= 0 || statusStr.indexOf("Calibrat") >= 0 ||
      statusStr.indexOf("ACCEL") >= 0 || statusStr.indexOf("accel") >= 0) {
    systemMessage = "Calibration: " + statusStr;
    lastStatusText = systemMessage;
    statusRingPushDistinct(systemMessage);
  }
  // 加速度校准位置请求
  else if (currentCalibration == CALIB_ACCEL && 
           (statusStr.indexOf("Place") >= 0 || statusStr.indexOf("position") >= 0 || 
            statusStr.indexOf("LEVEL") >= 0 || statusStr.indexOf("LEFT") >= 0 || 
            statusStr.indexOf("RIGHT") >= 0 || statusStr.indexOf("NOSE") >= 0 || 
            statusStr.indexOf("BACK") >= 0)) {
    
    // 解析位置请求
    if (statusStr.indexOf("LEVEL") >= 0) {
      currentPosition = ACCELCAL_VEHICLE_POS_LEVEL;
      currentSimpleStep = STEP_LEVEL;
    } else if (statusStr.indexOf("LEFT") >= 0) {
      currentPosition = ACCELCAL_VEHICLE_POS_LEFT;
      currentSimpleStep = STEP_LEFT;
    } else if (statusStr.indexOf("RIGHT") >= 0) {
      currentPosition = ACCELCAL_VEHICLE_POS_RIGHT;
      currentSimpleStep = STEP_RIGHT;
    } else if (statusStr.indexOf("NOSE DOWN") >= 0) {
      currentPosition = ACCELCAL_VEHICLE_POS_NOSEDOWN;
      currentSimpleStep = STEP_NOSEDOWN;
    } else if (statusStr.indexOf("NOSE UP") >= 0) {
      currentPosition = ACCELCAL_VEHICLE_POS_NOSEUP;
      currentSimpleStep = STEP_NOSEUP;
    } else if (statusStr.indexOf("BACK") >= 0) {
      currentPosition = ACCELCAL_VEHICLE_POS_BACK;
      currentSimpleStep = STEP_BACK;
    }
    
    currentAccelState = ACCEL_STATE_POSITION_REQUESTED;
    operationMessage = "Position requested: " + statusStr + " - Click 'Calibrate Accel' when ready";
    systemMessage = "Accel Cal: " + statusStr;
    lastStatusText = systemMessage;
    statusRingPushDistinct(systemMessage);
    
    Serial.print("Position requested: ");
    Serial.print(currentPosition);
    Serial.print(" (step ");
    Serial.print(currentSimpleStep);
    Serial.println(")");
  }
  // 解锁相关
  else if (lastArmCommand && (statusStr.indexOf("Arm:") >= 0 || statusStr.indexOf("PreArm:") >= 0)) {
    if (armErrorCount < 10) {
      if (statusStr.indexOf("Arm:") == 0) {
        armErrors[armErrorCount] = statusStr;
        if (armErrors[armErrorCount].length() > 100)
          armErrors[armErrorCount] = armErrors[armErrorCount].substring(0, 100);
        armErrorCount++;
      } else if (armErrorCount > 0) {
        String newError = armErrors[armErrorCount - 1] + statusStr;
        if (newError.length() > 100) newError = newError.substring(0, 100);
        armErrors[armErrorCount - 1] = newError;
      }
    }
    // 对PreArm消息不添加"ARM Error:"前缀，直接显示原始消息
    if (statusStr.indexOf("PreArm:") >= 0) {
      systemMessage = statusStr;
      lastStatusText = systemMessage;
    } else {
      systemMessage = "ARM Error: " + statusStr;
      lastStatusText = systemMessage;
    }
    statusRingPushDistinct(systemMessage);
  }
  // PreArm消息处理（非ARM命令相关的PreArm消息）
  else if (statusStr.indexOf("PreArm:") >= 0) {
    systemMessage = statusStr;
    lastStatusText = systemMessage;
    statusRingPushDistinct(systemMessage);
  }
  // 其他重要系统消息
  else if (statustext.severity >= MAV_SEVERITY_WARNING ||
           statusStr.indexOf("Error") >= 0 || statusStr.indexOf("Warning") >= 0 ||
           statusStr.indexOf("Failed") >= 0 || statusStr.indexOf("not") >= 0) {
    systemMessage = statusStr;
    lastStatusText = systemMessage;
    statusRingPushDistinct(systemMessage);
  }
  // 一般通知（过滤掉不需要的消息）
  else if (statustext.severity >= MAV_SEVERITY_NOTICE) {
    // 只处理真正需要显示的消息
    if (statusStr.indexOf("GPS") >= 0 || statusStr.indexOf("EKF") >= 0 || 
        statusStr.indexOf("AHRS") >= 0 || statusStr.indexOf("IMU") >= 0 ||
        statusStr.indexOf("Compass") >= 0 || statusStr.indexOf("compass") >= 0) {
      systemMessage = statusStr;
      lastStatusText = systemMessage;
      statusRingPushDistinct(systemMessage);
    }
  }

  if (g_statusMsgRingCount > 0) {
    systemMessage = statusRingJoin();
  }
}

// 系统消息定时刷新（loop中调用）：多行缓冲已在 processStatusText 中写入环形队列
void updateSystemMessage() {
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck < 100) return;
  lastCheck = millis();

  if (lastStatusText.indexOf("System initialized") >= 0 ||
      lastStatusText.indexOf("System ready") >= 0 ||
      lastStatusText.indexOf("Ready") >= 0) {
    lastStatusText = "";
  }

  if (g_statusMsgRingCount == 0) {
    systemMessage = "System Ready";
  } else {
    systemMessage = statusRingJoin();
  }
}

// 通用MAVLink命令发送函数
void sendMavlinkCommand(uint16_t command, float param1, float param2, float param3, float param4, float param5, float param6, float param7) {
  mavlink_message_t msg;
  mavlink_msg_command_long_pack(SYS_ID, COMP_ID, &msg, TARGET_SYS, TARGET_COMP, command, 0, param1, param2, param3, param4, param5, param6, param7);

  uint8_t buf[MAVLINK_MAX_PACKET_LEN];
  uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);
  mavSerial.write(buf, len);

  Serial.println("==== MAVLink Command Debug ====");
  Serial.print("SYS_ID: "); Serial.println(SYS_ID);
  Serial.print("COMP_ID: "); Serial.println(COMP_ID);
  Serial.print("TARGET_SYS: "); Serial.println(TARGET_SYS);
  Serial.print("TARGET_COMP: "); Serial.println(TARGET_COMP);
  Serial.print("command: "); Serial.println(command);
  Serial.print("confirmation: 0\n");
  Serial.print("param1: "); Serial.println(param1, 2);
  Serial.print("param2: "); Serial.println(param2, 2);
  Serial.print("param3: "); Serial.println(param3, 2);
  Serial.print("param4: "); Serial.println(param4, 2);
  Serial.print("param5: "); Serial.println(param5, 2);
  Serial.print("param6: "); Serial.println(param6, 2);
  Serial.print("param7: "); Serial.println(param7, 2);
  Serial.println("==============================");

  Serial.print("Sent command: ");
  Serial.print(command);
  Serial.print(" with params: ");
  Serial.print(param1);
  Serial.print(", ");
  Serial.print(param2);
  Serial.print(", ");
  Serial.print(param3);
  Serial.print(", ");
  Serial.print(param4);
  Serial.print(", ");
  Serial.print(param5);
  Serial.print(", ");
  Serial.print(param6);
  Serial.print(", ");
  Serial.println(param7);
}

// 获取校准位置消息
String getCalibrationPositionMessage(uint8_t position) {
  for (int i = 0; i < CALIB_POSITION_COUNT; i++) {
    if (CALIB_POSITIONS[i].position == position) {
      return String(CALIB_POSITIONS[i].message);
    }
  }
  return "Unknown position requested by autopilot";
}

// 非阻塞补拉舵机相关参数（原 80×delay(50) 会卡死 loop，导致网页打不开、手机报 WiFi 密码错等假阳性）
void pumpServoParamRequests() {
  if (!param_ready || servo_param_pump_done) return;
  // RX 积压时优先让 handleMavlink 消化，避免 TX/RX 与 WiFi 同时抢时间片
  if (mavSerial.available() > 320) return;

  static const char* const SERVO_REQ_NAMES[80] = {
    "SERVO1_FUNCTION", "SERVO1_MIN", "SERVO1_MAX", "SERVO1_TRIM", "SERVO1_REVERSED",
    "SERVO2_FUNCTION", "SERVO2_MIN", "SERVO2_MAX", "SERVO2_TRIM", "SERVO2_REVERSED",
    "SERVO3_FUNCTION", "SERVO3_MIN", "SERVO3_MAX", "SERVO3_TRIM", "SERVO3_REVERSED",
    "SERVO4_FUNCTION", "SERVO4_MIN", "SERVO4_MAX", "SERVO4_TRIM", "SERVO4_REVERSED",
    "SERVO5_FUNCTION", "SERVO5_MIN", "SERVO5_MAX", "SERVO5_TRIM", "SERVO5_REVERSED",
    "SERVO6_FUNCTION", "SERVO6_MIN", "SERVO6_MAX", "SERVO6_TRIM", "SERVO6_REVERSED",
    "SERVO7_FUNCTION", "SERVO7_MIN", "SERVO7_MAX", "SERVO7_TRIM", "SERVO7_REVERSED",
    "SERVO8_FUNCTION", "SERVO8_MIN", "SERVO8_MAX", "SERVO8_TRIM", "SERVO8_REVERSED",
    "SERVO9_FUNCTION", "SERVO9_MIN", "SERVO9_MAX", "SERVO9_TRIM", "SERVO9_REVERSED",
    "SERVO10_FUNCTION", "SERVO10_MIN", "SERVO10_MAX", "SERVO10_TRIM", "SERVO10_REVERSED",
    "SERVO11_FUNCTION", "SERVO11_MIN", "SERVO11_MAX", "SERVO11_TRIM", "SERVO11_REVERSED",
    "SERVO12_FUNCTION", "SERVO12_MIN", "SERVO12_MAX", "SERVO12_TRIM", "SERVO12_REVERSED",
    "SERVO13_FUNCTION", "SERVO13_MIN", "SERVO13_MAX", "SERVO13_TRIM", "SERVO13_REVERSED",
    "SERVO14_FUNCTION", "SERVO14_MIN", "SERVO14_MAX", "SERVO14_TRIM", "SERVO14_REVERSED",
    "SERVO15_FUNCTION", "SERVO15_MIN", "SERVO15_MAX", "SERVO15_TRIM", "SERVO15_REVERSED",
    "SERVO16_FUNCTION", "SERVO16_MIN", "SERVO16_MAX", "SERVO16_TRIM", "SERVO16_REVERSED"
  };

  static int idx = 0;
  static unsigned long lastPump = 0;
  if (millis() - lastPump < SERVO_PARAM_PUMP_MS) return;
  lastPump = millis();

  if (idx == 0) {
    operationMessage = "Servo params loading...";
  }

  if (idx >= 80) {
    servo_param_pump_done = true;
    operationMessage = "Servo params done";
    Serial.println("Servo parameter requests finished (non-blocking pump)");
    return;
  }

  mavlink_message_t msg;
  mavlink_msg_param_request_read_pack(SYS_ID, COMP_ID, &msg, TARGET_SYS, TARGET_COMP, SERVO_REQ_NAMES[idx], -1);
  uint8_t buf[MAVLINK_MAX_PACKET_LEN];
  uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);
  mavSerial.write(buf, len);
  idx++;
}

#endif
