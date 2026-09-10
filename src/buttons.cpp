#include "buttons.h"
#include "config.h"
#include "app_state.h"

UiMode uiMode = UI_TELEMETRY;
int menuIndex = 0;
String menuStatus = "Long-press OK to open menu";
bool menuDirty = true;

static const int BTN_PINS[BTN_COUNT] = {
  BTN_UP_PIN, BTN_DOWN_PIN, BTN_LEFT_PIN, BTN_RIGHT_PIN, BTN_OK_PIN
};

static bool rawDown[BTN_COUNT];
static bool stableDown[BTN_COUNT];
static unsigned long debounceAt[BTN_COUNT];
static unsigned long pressStart[BTN_COUNT];
static bool longFired[BTN_COUNT];

static const unsigned long DEBOUNCE_MS = 30;
static const unsigned long LONG_PRESS_MS = 800;

static const char* MENU_ITEMS[] = {
  "Level Cal (sim)",
  "Gyro Cal (sim)",
  "Accel Cal (sim)",
  "Exit menu"
};
static const int MENU_COUNT = 4;

int menuItemCount() { return MENU_COUNT; }
const char* menuItemName(int index) {
  if (index < 0 || index >= MENU_COUNT) return "";
  return MENU_ITEMS[index];
}

bool buttonsConsumeMenuDirty() {
  if (!menuDirty) return false;
  menuDirty = false;
  return true;
}

void buttonsInit() {
  for (int i = 0; i < BTN_COUNT; i++) {
    pinMode(BTN_PINS[i], INPUT_PULLUP);
    rawDown[i] = false;
    stableDown[i] = false;
    debounceAt[i] = 0;
    pressStart[i] = 0;
    longFired[i] = false;
  }
  uiMode = UI_TELEMETRY;
  menuIndex = 0;
  menuStatus = "Long-press OK open menu";
  menuDirty = true;
  Serial.println("Buttons: UP=4 DOWN=5 LEFT=6 RIGHT=13 OK=14 (active LOW)");
}

static void enterMenu() {
  uiMode = UI_MENU;
  menuIndex = 0;
  menuStatus = "UP/DN select  LEFT back  OK run";
  menuDirty = true;
  Serial.println("UI: enter menu");
}

static void exitMenu() {
  uiMode = UI_TELEMETRY;
  menuStatus = "Back to telemetry";
  menuDirty = true;
  Serial.println("UI: exit menu");
}

static void runMenuItem() {
  if (menuIndex == MENU_COUNT - 1) {
    exitMenu();
    return;
  }
  menuStatus = String("SIM OK: ") + MENU_ITEMS[menuIndex];
  menuDirty = true;
  operationMessage = menuStatus;
  Serial.println(menuStatus);
}

static void onShortPress(BtnId id) {
  if (uiMode == UI_TELEMETRY) {
    if (id == BTN_OK) {
      menuStatus = "Hold OK longer...";
      operationMessage = menuStatus;
    }
    return;
  }

  switch (id) {
    case BTN_UP:
      menuIndex = (menuIndex + MENU_COUNT - 1) % MENU_COUNT;
      menuDirty = true;
      break;
    case BTN_DOWN:
      menuIndex = (menuIndex + 1) % MENU_COUNT;
      menuDirty = true;
      break;
    case BTN_LEFT:
      exitMenu();
      break;
    case BTN_RIGHT:
      menuStatus = "RIGHT ok (no action)";
      menuDirty = true;
      break;
    case BTN_OK:
      runMenuItem();
      break;
    default:
      break;
  }
}

static void onLongPress(BtnId id) {
  if (id == BTN_OK) {
    if (uiMode == UI_TELEMETRY) {
      enterMenu();
    } else {
      exitMenu();
    }
  }
}

void buttonsTick() {
  unsigned long now = millis();

  for (int i = 0; i < BTN_COUNT; i++) {
    bool down = (digitalRead(BTN_PINS[i]) == LOW);
    if (down != rawDown[i]) {
      rawDown[i] = down;
      debounceAt[i] = now;
    }
    if ((now - debounceAt[i]) < DEBOUNCE_MS) continue;

    if (down && !stableDown[i]) {
      stableDown[i] = true;
      pressStart[i] = now;
      longFired[i] = false;
    } else if (!down && stableDown[i]) {
      stableDown[i] = false;
      if (!longFired[i]) {
        onShortPress((BtnId)i);
      }
    } else if (down && stableDown[i] && !longFired[i]) {
      if (now - pressStart[i] >= LONG_PRESS_MS) {
        longFired[i] = true;
        onLongPress((BtnId)i);
      }
    }
  }
}
