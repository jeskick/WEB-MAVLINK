#include "buttons.h"
#include "config.h"
#include "app_state.h"
#include "mav_actions.h"

UiMode uiMode = UI_TELEMETRY;
int menuIndex = 0;
int menuScrollTop = 0;
String menuStatus = "";
bool menuDirty = true;
MenuAction confirmAction = ACT_EXIT;
String confirmTitle = "";

static const int BTN_PINS[BTN_COUNT] = {
  BTN_UP_PIN, BTN_DOWN_PIN, BTN_LEFT_PIN, BTN_RIGHT_PIN, BTN_OK_PIN
};

static bool rawDown[BTN_COUNT];
static bool stableDown[BTN_COUNT];
static unsigned long debounceAt[BTN_COUNT];
static unsigned long pressStart[BTN_COUNT];
static bool longFired[BTN_COUNT];
static String lastOpForMenu;

static const unsigned long DEBOUNCE_MS = 30;
static const unsigned long LONG_PRESS_MS = 800;
// size2 一屏约 5 行（无说明书提示后把空间还给列表）
static const int MENU_VISIBLE = 5;

int menuItemCount() { return (int)ACT_COUNT; }
int menuVisibleRows() { return MENU_VISIBLE; }

static void clampMenuScroll() {
  const int count = menuItemCount();
  if (menuIndex < 0) menuIndex = 0;
  if (menuIndex >= count) menuIndex = count - 1;
  if (menuIndex < menuScrollTop) menuScrollTop = menuIndex;
  if (menuIndex >= menuScrollTop + MENU_VISIBLE) {
    menuScrollTop = menuIndex - MENU_VISIBLE + 1;
  }
  if (menuScrollTop < 0) menuScrollTop = 0;
  int maxTop = count - MENU_VISIBLE;
  if (maxTop < 0) maxTop = 0;
  if (menuScrollTop > maxTop) menuScrollTop = maxTop;
}

void menuItemLabel(int index, char* buf, size_t buflen) {
  if (!buf || buflen == 0) return;
  buf[0] = '\0';
  // 文案控制在 size2 一行内（约 ≤22 字符含 "> "）
  switch ((MenuAction)index) {
    case ACT_LEVEL:
      if (currentLevelState == LEVEL_STATE_STARTED)
        snprintf(buf, buflen, "Level: OK again");
      else if (currentLevelState == LEVEL_STATE_WAITING_RESULT)
        snprintf(buf, buflen, "Level: wait...");
      else if (levelCalibrated)
        snprintf(buf, buflen, "Level (done)");
      else
        snprintf(buf, buflen, "Level Cal");
      break;
    case ACT_GYRO:
      snprintf(buf, buflen, "Gyro Cal");
      break;
    case ACT_ACCEL:
      if (currentAccelState == ACCEL_STATE_POSITION_REQUESTED)
        snprintf(buf, buflen, "Accel: OK pos");
      else if (currentAccelState != ACCEL_STATE_IDLE)
        snprintf(buf, buflen, "Accel: wait...");
      else if (accelCalibrated)
        snprintf(buf, buflen, "Accel (done)");
      else
        snprintf(buf, buflen, "Accel Cal");
      break;
    case ACT_ARM:
      snprintf(buf, buflen, isArmed ? "Disarm" : "Arm");
      break;
    case ACT_FORCE_ARM:
      snprintf(buf, buflen, "Force Arm");
      break;
    case ACT_EXIT:
      snprintf(buf, buflen, "Exit");
      break;
    default:
      snprintf(buf, buflen, "?");
      break;
  }
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
  menuStatus = "";
  menuDirty = true;
  Serial.println("Buttons: UP=4 DOWN=5 LEFT=6 RIGHT=13 OK=14 (active LOW)");
}

static void enterMenu() {
  uiMode = UI_MENU;
  menuIndex = 0;
  menuScrollTop = 0;
  menuStatus = "";
  menuDirty = true;
  lastOpForMenu = operationMessage;
  Serial.println("UI: enter menu");
}

static void exitMenu() {
  uiMode = UI_TELEMETRY;
  menuStatus = "";
  menuDirty = true;
  Serial.println("UI: exit menu");
}

static void enterConfirm(MenuAction act, const char* title) {
  confirmAction = act;
  confirmTitle = title;
  uiMode = UI_CONFIRM;
  menuStatus = "";
  menuDirty = true;
}

static void backFromConfirm() {
  uiMode = UI_MENU;
  menuStatus = "";
  menuDirty = true;
}

static void applyResult(const String& msg) {
  menuStatus = msg;
  operationMessage = msg;
  menuDirty = true;
  Serial.println(msg);
}

static void runSelectedAction(MenuAction act) {
  if (!fcHeartbeatSeen) {
    applyResult("No FC link");
    return;
  }

  switch (act) {
    case ACT_LEVEL:
      applyResult(actionLevelCal());
      // 第二步发送后可回遥测看结果；第一步留在菜单再按一次
      if (currentLevelState == LEVEL_STATE_WAITING_RESULT) {
        // 留在菜单，状态行会更新
      }
      break;
    case ACT_GYRO:
      applyResult(actionGyroCal());
      break;
    case ACT_ACCEL:
      applyResult(actionAccelCal());
      break;
    case ACT_ARM:
      applyResult(actionArmDisarm(false));
      uiMode = UI_MENU;
      break;
    case ACT_FORCE_ARM:
      applyResult(actionArmDisarm(true));
      uiMode = UI_MENU;
      break;
    case ACT_EXIT:
      exitMenu();
      break;
    default:
      break;
  }
}

static void onShortPress(BtnId id) {
  if (uiMode == UI_TELEMETRY) {
    return;
  }

  if (uiMode == UI_CONFIRM) {
    if (id == BTN_LEFT || id == BTN_RIGHT) {
      backFromConfirm();
      return;
    }
    if (id == BTN_OK) {
      uiMode = UI_MENU;
      runSelectedAction(confirmAction);
      return;
    }
    return;
  }

  // UI_MENU
  switch (id) {
    case BTN_UP:
      menuIndex = (menuIndex + ACT_COUNT - 1) % ACT_COUNT;
      clampMenuScroll();
      menuDirty = true;
      break;
    case BTN_DOWN:
      menuIndex = (menuIndex + 1) % ACT_COUNT;
      clampMenuScroll();
      menuDirty = true;
      break;
    case BTN_LEFT:
      if (currentCalibration != CALIB_NONE ||
          currentAccelState != ACCEL_STATE_IDLE ||
          currentLevelState != LEVEL_STATE_IDLE) {
        actionCancelCalib();
        applyResult("Cancelled");
      } else {
        exitMenu();
      }
      break;
    case BTN_RIGHT:
      if (currentAccelState == ACCEL_STATE_POSITION_REQUESTED) {
        applyResult(actionAccelCal());
      }
      break;
    case BTN_OK: {
      MenuAction act = (MenuAction)menuIndex;
      if (act == ACT_EXIT) {
        exitMenu();
        break;
      }
      if (act == ACT_ARM) {
        enterConfirm(ACT_ARM, isArmed ? "DISARM?" : "ARM?");
        break;
      }
      if (act == ACT_FORCE_ARM) {
        if (isArmed) {
          applyResult("Already armed");
          break;
        }
        enterConfirm(ACT_FORCE_ARM, "FORCE ARM?");
        break;
      }
      if (act == ACT_GYRO) {
        enterConfirm(ACT_GYRO, "GYRO CAL?");
        break;
      }
      runSelectedAction(act);
      break;
    }
    default:
      break;
  }
}

static void onLongPress(BtnId id) {
  if (id != BTN_OK) return;
  if (uiMode == UI_TELEMETRY) {
    enterMenu();
  } else if (uiMode == UI_CONFIRM) {
    backFromConfirm();
  } else {
    exitMenu();
  }
}

void buttonsTick() {
  unsigned long now = millis();

  // 菜单内同步 OP 消息到状态栏（校准进度）
  if ((uiMode == UI_MENU || uiMode == UI_CONFIRM) && operationMessage != lastOpForMenu) {
    lastOpForMenu = operationMessage;
    if (uiMode == UI_MENU) {
      menuStatus = operationMessage;
      menuDirty = true;
    }
  }

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
