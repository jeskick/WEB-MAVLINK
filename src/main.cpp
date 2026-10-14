#include "uart_link.h"
#include "tft_display.h"
#include "web_server.h"
#include "buttons.h"

static void onUartScanUi(long baud, int rxPin, int txPin) {
  operationMessage = String("Scan ") + String(baud) + " RX" + String(rxPin) + " TX" + String(txPin);
  updateDisplay();
}

void setup() {
  Serial.begin(115200);
  Serial.println("=== ESP32 MAVLink Ground Station Starting ===");

  initDisplay();
  buttonsInit();
  uartLinkSetScanUi(onUartScanUi);

  Serial.println("Step 2: UART fast baud + TX/RX swap detect...");
  operationMessage = "Scanning UART...";
  updateDisplay();
  uartLinkStart();
  for (int i = 0; i < 50; i++) {
    sendGcsHeartbeat();
    handleMavlink(MAVLINK_RX_SLICE_BYTES);
    buttonsTick();
    if ((i % 8) == 0) updateDisplay();
    yield();
    delay(20);
  }
  operationMessage = fcHeartbeatSeen ? "Link OK" : "Waiting FC...";
  updateDisplay();

  Serial.println("Step 3: WiFi AP...");
  operationMessage = "Starting WiFi...";
  updateDisplay();
  WiFi.persistent(false);
  WiFi.mode(WIFI_AP);
  delay(50);
  WiFi.softAP(SSID, PASSWORD);
  IPAddress local_IP(192, 168, 6, 6);
  IPAddress gateway(192, 168, 6, 6);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.softAPConfig(local_IP, gateway, subnet);
  WiFi.setSleep(false);
  delay(500);
  ipStr = WiFi.softAPIP().toString();
  Serial.println("WiFi AP: " + ipStr);

  delay(MAVLINK_AFTER_WIFI_MS);
  mavlink_reset_channel_status(MAVLINK_COMM_0);

  initWebServer();

  for (int i = 0; i < 16; i++) {
    servoOutputs[i].valid = false;
    servoOutputs[i].position = 1500;
    servoOutputs[i].min_pos = 1100;
    servoOutputs[i].max_pos = 1900;
    servoOutputs[i].trim_pos = 1500;
    servoOutputs[i].function = 0;
    servoOutputs[i].reversed = false;
  }

  lastStatusTextTime = millis();
  lastStatusText = "System Ready";
  systemMessage = "Ready";
  operationMessage = fcHeartbeatSeen ? "Loading params..." : "Waiting FC...";
  updateDisplay();

  Serial.printf("UART RX=%d TX=%d baud=%ld swapped=%d\n",
                mavRxPin, mavTxPin, detectedBaud, uartPinsSwapped ? 1 : 0);
  Serial.println("Buttons ready: long-press OK (~0.8s) to open sim menu");
}

void loop() {
  buttonsTick();

  static unsigned long lastHb = 0;
  if (millis() - lastHb >= GCS_HEARTBEAT_MS) {
    lastHb = millis();
    sendGcsHeartbeat();
  }

  uartLinkTick();

  for (uint8_t p = 0; p < MAVLINK_WEB_INTERLEAVE; p++) {
    handleMavlink(MAVLINK_RX_SLICE_BYTES);
    server.handleClient();
    yield();
  }
  handleMavlink(0);

  checkAndRequestMissingParams();
  if (param_ready) {
    pumpServoParamRequests();
  }

  updateSystemMessage();

  static unsigned long lastDisp = 0;
  unsigned long interval = (uiMode != UI_TELEMETRY) ? 80 : (fcHeartbeatSeen ? 400 : 50);
  if (millis() - lastDisp > interval) {
    lastDisp = millis();
    updateDisplay();
  }

  if (waitingForAck && millis() - lastCommandTime > CMD_TIMEOUT) {
    operationMessage = "Calibration timeout - Please try again.";
    waitingForAck = false;
    currentCalibration = CALIB_NONE;
    currentAccelState = ACCEL_STATE_IDLE;
  }

  server.handleClient();
  handleMavlink(MAVLINK_RX_SLICE_BYTES);
}
