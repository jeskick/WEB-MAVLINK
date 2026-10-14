#include "mav_actions.h"
#include "app_state.h"
#include "config.h"
#include <c_library_v2-master/ardupilotmega/mavlink.h>

void sendMavlinkCommand(uint16_t command, float param1, float param2, float param3,
                        float param4, float param5, float param6, float param7);

String actionLevelCal() {
  if (flightMode != "MANUAL" && flightMode != "Manual" && flightMode != "0") {
    operationMessage = "Need MANUAL mode. Now: " + flightMode;
    return operationMessage;
  }

  if (currentLevelState == LEVEL_STATE_IDLE) {
    levelCalibrated = false;
    currentCalibration = CALIB_LEVEL;
    currentLevelState = LEVEL_STATE_STARTED;
    operationMessage = "Level: place flat, OK";
    return operationMessage;
  }

  if (currentLevelState == LEVEL_STATE_STARTED) {
    currentLevelState = LEVEL_STATE_WAITING_RESULT;
    waitingForAck = true;
    lastCommandTime = millis();
    operationMessage = "Sending level calibration...";
    sendMavlinkCommand(MAV_CMD_PREFLIGHT_CALIBRATION, 0, 0, 0, 0, 2, 0, 0);
    return operationMessage;
  }

  return operationMessage.length() ? operationMessage : String("Level cal busy");
}

String actionGyroCal() {
  if (isArmed) {
    operationMessage = "Disarm before gyro cal";
    return operationMessage;
  }
  currentCalibration = CALIB_NONE;
  waitingForAck = true;
  lastCommandTime = millis();
  operationMessage = "Sending gyro calibration...";
  sendMavlinkCommand(MAV_CMD_PREFLIGHT_CALIBRATION, 1, 0, 0, 0, 0, 0, 0);
  return operationMessage;
}

String actionAccelCal() {
  if (currentCalibration != CALIB_NONE && currentCalibration != CALIB_ACCEL) {
    operationMessage = "Another calibration in progress";
    return operationMessage;
  }

  if (currentAccelState == ACCEL_STATE_IDLE) {
    if (isArmed) {
      operationMessage = "Disarm before accel cal";
      return operationMessage;
    }
    currentCalibration = CALIB_ACCEL;
    currentAccelState = ACCEL_STATE_STARTING;
    currentPosition = 0;
    sendMavlinkCommand(MAV_CMD_PREFLIGHT_CALIBRATION, 0, 0, 0, 0, 1, 0, 0);
    waitingForAck = true;
    lastCommandTime = millis();
    operationMessage = "Accel cal started...";
    return operationMessage;
  }

  if (currentAccelState == ACCEL_STATE_POSITION_REQUESTED) {
    sendMavlinkCommand(MAV_CMD_ACCELCAL_VEHICLE_POS, currentPosition, 0, 0, 0, 0, 0, 0);
    currentAccelState = ACCEL_STATE_WAITING_POSITION;
    waitingForAck = true;
    lastCommandTime = millis();
    operationMessage = "Pos confirmed, waiting next...";
    return operationMessage;
  }

  return operationMessage.length() ? operationMessage : String("Accel: waiting FC...");
}

String actionArmDisarm(bool force) {
  armDenied = false;
  armErrorCount = 0;

  if (force) {
    lastArmCommand = true;
    operationMessage = "Sending FORCE ARM...";
    sendMavlinkCommand(MAV_CMD_COMPONENT_ARM_DISARM, 1.0f, 2989.0f, 0, 0, 0, 0, 0);
    return operationMessage;
  }

  if (isArmed) {
    lastArmCommand = false;
    operationMessage = "Sending DISARM...";
    sendMavlinkCommand(MAV_CMD_COMPONENT_ARM_DISARM, 0, 0, 0, 0, 0, 0, 0);
  } else {
    lastArmCommand = true;
    operationMessage = "Sending ARM...";
    sendMavlinkCommand(MAV_CMD_COMPONENT_ARM_DISARM, 1.0f, 0, 0, 0, 0, 0, 0);
  }
  return operationMessage;
}

void actionCancelCalib() {
  currentCalibration = CALIB_NONE;
  currentAccelState = ACCEL_STATE_IDLE;
  currentLevelState = LEVEL_STATE_IDLE;
  waitingForAck = false;
  operationMessage = "Calibration cancelled";
}
