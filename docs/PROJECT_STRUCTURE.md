# 源码结构导读

本文档说明 **WEB-MAVLINK**（PlatformIO）各文件职责与扩展入口。

---

## 1. 工程布局

```
WEB-Mavlink/
├── platformio.ini           # env:esp32s3，ST7789 依赖
├── src/
│   ├── main.cpp             # setup / loop
│   ├── app_state.cpp        # 全局对象与状态定义
│   ├── uart_link.cpp        # 波特率 + TX/RX 自动换线（NVS）
│   └── buttons.cpp          # 五向按键扫描 + 模拟菜单
├── include/
│   ├── config.h             # 引脚、WiFi、时序、模式表、按键脚
│   ├── app_state.h
│   ├── uart_link.h
│   ├── buttons.h
│   ├── mavlink_handler.h    # MAVLink 收发与参数/校准
│   ├── web_server.h         # 网页 UI、API、OTA
│   └── tft_display.h        # ST7789 等待/遥测/菜单界面
├── build/
│   └── WEB-Mavlink.bin      # 仅保留最新 OTA 固件（由 firmware.bin 复制）
├── docs/
├── c_library_v2-master/
└── README.md
```

编译：`pio run -e esp32s3`；上传：`pio run -e esp32s3 -t upload`。  
OTA 发布前：`Copy-Item .pio\build\esp32s3\firmware.bin build\WEB-Mavlink.bin`

---

## 2. `config.h`

- **显示**：ST7789 240×320，横屏 320×240；SPI 脚 12/11/8/9/10/7。
- **UART**：GPIO15/16，自动正反线。
- **按键**：UP/DOWN/LEFT/RIGHT/OK → GPIO4/5/6/13/14（INPUT_PULLUP）。
- **网络 / MAVLink ID / Plane 模式表 / 校准枚举**。

---

## 3. `mavlink_handler.h`

发送：`requestParamList`、GCS heartbeat、校准 `COMMAND_LONG` 等。  
接收：`HEARTBEAT`、`SYS_STATUS`/`BATTERY_STATUS`、`GPS_RAW_INT`、`PARAM_VALUE`、`SERVO_OUTPUT_RAW`、`RC_CHANNELS*`、`COMMAND_ACK`、`STATUSTEXT` 等。

---

## 4. `web_server.h`

`initWebServer()` 注册路由；主页 `/`；状态 `/api/status`；OTA `POST /ota`。详见 [HTTP_API.md](HTTP_API.md)。

---

## 5. `tft_display.h` / `buttons`

- 等待连接页、已连接遥测页（VOLT/MODE/GPS/ARM + OP/SYS）、参数进度条。
- 长按 OK 进入模拟菜单；上下选择、左返回、OK 确认（后续接真实校准）。

---

## 6. `uart_link`

波特率扫描、正反线、NVS 记忆；见飞控 HEARTBEAT 后锁定。
