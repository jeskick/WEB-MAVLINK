#ifndef BUTTONS_H
#define BUTTONS_H

#include <Arduino.h>

enum UiMode : uint8_t {
  UI_TELEMETRY = 0,
  UI_MENU = 1
};

enum BtnId : uint8_t {
  BTN_UP = 0,
  BTN_DOWN = 1,
  BTN_LEFT = 2,
  BTN_RIGHT = 3,
  BTN_OK = 4,
  BTN_COUNT = 5
};

void buttonsInit();
void buttonsTick();

extern UiMode uiMode;
extern int menuIndex;
extern String menuStatus;
extern bool menuDirty;

bool buttonsConsumeMenuDirty();
int menuItemCount();
const char* menuItemName(int index);

#endif
