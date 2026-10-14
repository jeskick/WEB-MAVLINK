#ifndef MAV_ACTIONS_H
#define MAV_ACTIONS_H

#include <Arduino.h>

String actionLevelCal();
String actionGyroCal();
String actionAccelCal();
String actionArmDisarm(bool force);
void actionCancelCalib();

#endif
