# 源码结构导读

本文档说明 **WEB-MAVLINK** 各文件职责、调用关系与扩展入口，便于二次开发。

---

## 1. 顶层入口：`WEB-Mavlink.ino`

| 区块 | 内容 |
|------|------|
| 全局对象 | `Adafruit_SSD1306 display`、`HardwareSerial mavSerial(2)`、`WebServer server(80)` |
| 全局状态 | IP、消息字符串、电压、模式、GPS、参数映射、`servoOutputs[]`、校准状态机等 |
| `detectMavlinkBaudrate()` | 轮询常见波特率，用 `mavlink_parse_char` 收到合法包即锁定 |
| `setup()` | I2C/OLED → MAVLink 串口 → WiFi AP → `initWebServer()` → 初始化舵机缓存 → `requestParamList()` |
| `loop()` | `server.handleClient()` → `handleMavlink()` → `updateSystemMessage()` → OLED 节流刷新 → 校准超时 → 参数补请求 → 参数就绪后 `requestServoParameters()` |

---

## 2. `config.h`

- **硬件**：OLED 分辨率、`I2C_SDA`/`I2C_SCL`、`MAVLINK_RX`/`MAVLINK_TX`。
- **网络**：`SSID`、`PASSWORD`。
- **MAVLink**：`SYS_ID`、`COMP_ID`、`TARGET_SYS`、`TARGET_COMP`。
- **Plane 模式表**：`PLANE_MODES`、`FLTMODE_PARAMS`。
- **校准**：`CalibrationType`、`AccelCalibrationState`、ArduPilot 加速度计位置相关宏与 `CALIB_POSITIONS`。
- **`ServoData`**：16 路结构（网页表格主要用前 13 路）。
- **extern 声明**：与各 `.h` / `.ino` 中定义的全局变量对应。

修改车型（例如 Copter）时，优先替换 **模式映射** 与 **舵机/参数语义** 相关部分。

---

## 3. `mavlink_handler.h`

### 3.1 发送侧

- `requestParamList()` / `requestParam(index)`：`PARAM_REQUEST_LIST` / `PARAM_REQUEST_READ`。
- `sendMavlinkCommand(...)`：`COMMAND_LONG`。
- `checkAndRequestMissingParams()`：按索引补拉缺失参数（带简单节流）。
- 参数设置确认逻辑与 `PARAM_VALUE`、pending 映射交互（详见文件内 `COMMAND_ACK` 分支）。

### 3.2 接收侧（`handleMavlink` 中 `switch (msg.msgid)`）

| Message ID | 用途 |
|------------|------|
| `HEARTBEAT` | 模式名、`MAV_MODE_FLAG_SAFETY_ARMED` |
| `SYS_STATUS` | 电池电压 mV → V |
| `BATTERY_STATUS` | 备用电压源 |
| `GPS_RAW_INT` | 卫星数（与 fix 类型判断） |
| `PARAM_VALUE` | 填充 `paramMap`、`paramIndexSet`、`param_total` |
| `SERVO_OUTPUT_RAW` | 更新 `servoOutputs[]` |
| `RC_CHANNELS` / `RC_CHANNELS_RAW` | 按 `FLTMODE_CH` 更新 `rc_fltmode_pwm_us`、`rc_fltmode_pwm_valid`（供 `/api/status` 显示当前模式通道 PWM） |
| `COMMAND_ACK` | 校准与 ARM、参数写入等 ACK 处理 |
| `COMMAND_LONG` / `COMMAND_INT` | 部分校准协议交互 |
| `STATUSTEXT` | `processStatusText`，刷新 UI 消息与 ARM 拒绝原因数组 |

### 3.3 其它

- `updateSystemMessage()`：组合系统提示文案（与 OLED/网页共用数据源）。
- `armDenied` 等标志在网页 JSON 中暴露。

---

## 4. `web_server.h`

- **`initWebServer()`**：注册全部路由并 `server.begin()`。
- **`jsonEscapeForJson()`**：将字符串转义后嵌入 JSON，避免 STATUSTEXT 等含引号内容破坏 `/api/status` 等响应。
- **主页面 `/`**：单页 HTML，主要区域包括：
  - 系统消息区、电压/模式/GPS、操作提示、水平/加速度计校准与解锁按钮。
  - 参数输入框与 **A–Z** 分组参数列表；**整行点击复制** 参数名（事件委托，带剪贴板 API 不可用时的降级提示）。
  - **Tab「FLYMODEL」**：`FLTMODE1`…`FLTMODE6` 下拉（`saveModes()` 仅提交各 `select` 的数值）；每行右侧（桌面）或下拉框下方（窄屏 `@media (max-width:768px)`）显示 **六档模式开关 PWM 参考区间**（纯展示，不参与保存）。区间与 ArduPilot `RC_Channel::read_6pos_switch()` 固定分界一致：`0-1230`、`1231-1360`、…、`1750+`（与 Mission Planner 飞行模式页一致）。
  - **Tab「Servo Out」**：舵机参数表与实时位置轮询。
- **`/api/status`**：JSON 中含 `fltmodeCh`、`fltmodePwm`、`fltmodePwmValid` 等，供页眉「当前模式通道 / 当前 PWM」刷新。

扩展新功能时：在 `initWebServer()` 内增加 `server.on(...)`，必要时在 `mavlink_handler.h` 增加发送/解析分支。已登记路由见 [HTTP_API.md](HTTP_API.md)。

---

## 5. `oled_display.h`

- `initOLED()`：`display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)`，失败则打印日志但不阻塞主程序。
- `updateOLED()`：四行布局——滚动系统消息、电压/模式、滚动操作消息、参数加载进度。

---

## 6. `web_assets.h`（可选）

- 可将大块 CSS/JS 放入 Flash 字符串常量，减轻 `web_server.h` 中 `/` 路由体积。
- 维护脚本见 `tools/patch_web_index.py`（当前主分支仍以单文件内嵌 HTML 为主时，可不启用该路径）。

---

## 7. `tools/`

| 文件 | 说明 |
|------|------|
| `patch_web_index.py` | 实验性：把 `/` 拆成 `style.css`、`app.js` 并由 `web_assets.h` 提供。 |
| `check_delim.py` | 与参数解析分隔符相关的检查脚本。 |

---

## 8. `c_library_v2-master/`

- 官方生成的 **MAVLink v2** C 库树状目录。
- 工程实际包含路径主要为：`common/`、`ardupilotmega/`（本工程解码主要用该 dialect）。
- **体积大**：属正常现象；若需精简可考虑换用最小 dialect 重新生成（需同步修改解码逻辑）。

---

## 9. 数据流小结

```
UART 字节流 → mavlink_parse_char → 更新全局遥测/参数/舵机/模式通道 PWM
                    ↑
浏览器请求 → WebServer 回调 → 读全局状态 / 写 MAVLink 发送缓冲区 → UART
```

全局变量较多，适合小型单文件草图；若重构可引入命名空间或小型类封装 UART/Web 状态。
