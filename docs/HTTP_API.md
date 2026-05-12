# HTTP API 参考

本文档与 `web_server.h` 中 `initWebServer()` 内 `server.on(...)` 注册保持一致；若代码有改动，请以源码为准。

**说明**

- 除特别标注外，响应体多为 **JSON** 或 **纯文本**；浏览器地址栏直接打开时多为 **GET**。
- **`/api/set_param`** 的 `input` 支持英文逗号 `,` 与中文逗号 `，` 分隔多条；含 `=` 的 token 视为 **PARAM_SET**（`FLTMODE*` 按整型发送，其余默认 REAL32）；不含 `=` 的 token 视为 **在本地 `paramMap` 中查询**（不会向飞控发读请求，除非参数已由列表拉取）。
- **`/arm_disarm`** 中 `force=1` 会走强制解锁相关 MAVLink 参数组合，**有安全风险**，仅用于地面调试。

---

## 路由一览

| 方法 | 路径 | 查询参数 / 体 | 响应类型 | 说明 |
|------|------|---------------|----------|------|
| GET | `/` | — | `text/html` | 主控制台单页：系统消息、遥测、参数 A–Z、FLYMODEL / Servo Out 标签页、校准与解锁等。 |
| GET | `/api/status` | — | `application/json` | `systemMessage`、`operationMessage`、`voltage`、`flightMode`、`gpsCount`、`fltmodeCh`、`fltmodePwm`、`fltmodePwmValid`、`isArmed`、`paramReady`、`paramTotal`、`paramReceived`、`levelCalibrated`、`accelCalibrated`、`armDenied`。字符串字段经 JSON 转义。 |
| GET | `/api/params_by_letter` | `letter`（单字符，缺省为 `A`） | `application/json` | `count` 与 `parameters` 对象：该字母开头参数名 → 当前值（来自 `paramMap`）。 |
| GET | `/api/set_param` | `input`（必填/可为空） | `text/plain` | 见上文「说明」。结果写入 `operationMessage` 并作为响应正文返回。 |
| GET | `/api/set_param_result` | — | `text/plain` | 返回最近一次参数设置结果文案 `lastSetResult`。 |
| GET | `/calibrate_level` | — | `text/plain` | 两步式水平校准：需在 **MANUAL**；第一次点击提示放置飞机，第二次发送 `MAV_CMD_PREFLIGHT_CALIBRATION`（与 MP 类似参数）。 |
| GET | `/calibrate_accel` | — | `text/plain` | 加速度计校准：启动或确认当前姿态面（与内部状态机配合）。 |
| POST | `/api/arm` | — | `application/json` | `{"message":"Unlock command sent"}`；发送解锁 `COMMAND_LONG`。 |
| POST | `/api/disarm` | — | `application/json` | `{"message":"Lock command sent"}`；发送上锁。 |
| GET | `/api/arm_errors` | — | `application/json` | JSON 数组：解锁相关错误/拒绝文案字符串列表。 |
| POST | `/api/clear_arm_errors` | — | `text/plain` | `OK`；清空错误计数与部分 ARM 状态。 |
| GET | `/api/calibration_check` | — | `application/json` | `level_calibrated`、`accel_calibrated`（蛇形命名）。 |
| GET | `/api/param_debug` | — | `application/json` | 参数加载调试：`param_ready`、`param_total`、`param_map_size`、`param_index_set_size`、`last_param_recv`、`current_time`、`time_since_last_param`。 |
| GET | `/api/servos` | — | `application/json` | `servos` 数组，前 **10** 路 `position`（与页面部分逻辑一致）。 |
| GET | `/servo_output` | — | `text/html` | 1～13 通道舵机表：位置、反转、功能、Min/Trim/Max；内嵌 `updateServoParam` 调用 `/api/set_param`。 |
| GET | `/servo_positions` | — | `application/json` | `servo1`…`servo13` 当前 PWM；无有效数据时默认 `1500`。 |
| GET | `/arm_disarm` | `action`（可选：`1` 倾向解锁；空且已解锁逻辑见代码）、`force=1`（强制解锁分支） | `application/json` | `message`、`action`（1/0）、`armDenied`。主页「解锁」按钮通过 `fetch` 调用。 |
| GET | `/calibration_check` | — | `application/json` | 与 `/api/calibration_check` 相同字段，**重复注册**的兼容路径。 |
| GET | `/arm_errors` | — | `application/json` | 与 `/api/arm_errors` 相同，兼容路径。 |
| GET | `/clear_arm_errors` | — | `text/plain` | `ARM errors cleared`；与 POST `/api/clear_arm_errors` 类似，**GET** 兼容实现。 |

---

## 与页面脚本的对应关系（摘要）

| 页面/脚本行为 | 典型路径 |
|---------------|----------|
| 定时刷新状态条 | `GET /api/status` |
| A–Z 参数列表 | `GET /api/params_by_letter?letter=X` |
| 参数框 RQ/SET、舵机单元格修改 | `GET /api/set_param?input=...` |
| FLYMODEL 保存 | 连续 `GET /api/set_param?input=FLTMODE1=...`（共 6 次） |
| 水平 / 加速度计按钮 | `GET /calibrate_level`、`GET /calibrate_accel` |
| Servo 标签页表格 HTML | `GET /servo_output`；位置轮询 `GET /servo_positions` |
| 解锁按钮 | `GET /arm_disarm?action=...`（可选 `force=1`） |

---

## 版本

文档生成时以仓库内 `web_server.h` 为准；更新 API 时请同步修改本文件。
