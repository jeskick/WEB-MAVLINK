#ifndef BUTTONS_H
#define BUTTONS_H

#include <Arduino.h>

enum UiMode : uint8_t {
  UI_TELEMETRY = 0,
  UI_MENU = 1,
  UI_CONFIRM = 2
};

enum BtnId : uint8_t {
  BTN_UP = 0,
  BTN_DOWN = 1,
  BTN_LEFT = 2,
  BTN_RIGHT = 3,
  BTN_OK = 4,
  BTN_COUNT = 5
};

enum MenuAction : uint8_t {
  ACT_LEVEL = 0,
  ACT_GYRO = 1,
  ACT_ACCEL = 2,
  ACT_ARM = 3,
  ACT_FORCE_ARM = 4,
  ACT_EXIT = 5,
  ACT_COUNT = 6
};

void buttonsInit();
void buttonsTick();

extern UiMode uiMode;
extern int menuIndex;
extern int menuScrollTop;
extern String menuStatus;
extern bool menuDirty;
extern MenuAction confirmAction;
extern String confirmTitle;

bool buttonsConsumeMenuDirty();
int menuItemCount();
int menuVisibleRows();
void menuItemLabel(int index, char* buf, size_t buflen);

#endif
