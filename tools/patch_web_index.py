# -*- coding: utf-8 -*-
"""Replace bulky / route in web_server.h with flash-served assets + small HTML."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
path = ROOT / "web_server.h"
text = path.read_text(encoding="utf-8")

if '#include "web_assets.h"' not in text:
    text = text.replace(
        "#include <WebServer.h>\n\n// \u98de\u63a7",
        '#include <WebServer.h>\n#include "web_assets.h"\n\n// \u98de\u63a7',
        1,
    )

start = text.index('  server.on("/", HTTP_GET, []() {')
end = text.index("  // API\u63a5\u53e3\uff1a\u72b6\u6001\u4fe1\u606f")

NEW = r'''  server.on("/style.css", HTTP_GET, []() {
    server.send_P(200, PSTR("text/css"), WEB_STYLE_CSS);
  });
  server.on("/app.js", HTTP_GET, []() {
    server.send_P(200, PSTR("application/javascript"), WEB_APP_JS);
  });
  server.on("/api/plane_modes", HTTP_GET, []() {
    String json = "{\"modes\":[";
    for (int j = 0; j < PLANE_MODE_COUNT; ++j) {
      if (j > 0) json += ",";
      json += "{\"n\":" + String(PLANE_MODES[j].num) + ",\"m\":\"" + jsonEscapeForJson(String(PLANE_MODES[j].name)) + "\"}";
    }
    json += "],\"cur\":[";
    for (int i = 0; i < FLTMODE_PARAM_COUNT; ++i) {
      if (i > 0) json += ",";
      String key = String(FLTMODE_PARAMS[i]);
      int v = paramMap.count(key) ? (int)paramMap[key] : 0;
      json += String(v);
    }
    json += "]}";
    server.send(200, "application/json", json);
  });
  server.on("/", HTTP_GET, []() {
    yield();
    String html;
    html.reserve(9600);
    html += F("<!DOCTYPE html><html><head><meta charset='UTF-8'>");
    html += F("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no\">");
    html += F("<title>ESP32 MAVLink Ground Station</title>");
    html += F("<link rel=\"stylesheet\" href=\"/style.css\">");
    html += F("</head><body><div class='container'>");
    html += F("<div class='row center'><div id='sysmsg' style='font-size:16px;font-weight:bold;text-align:center;color:rgb(255,0,0);'>--</div></div>");
    html += F("<div style='display:flex;justify-content:space-between;align-items:center;margin:5px 0;background:#f8f9fa;padding:5px;border-radius:5px;'>");
    html += F("<div style='flex:1;text-align:center;font-weight:bold;'>volt : <span id='voltage'>-- V</span></div>");
    html += F("<div style='flex:1;text-align:center;font-weight:bold;'>MODE : <span id='flightMode'>--</span></div>");
    html += F("<div style='flex:1;text-align:center;font-weight:bold;'>GPS : <span id='gpsCount'>--</span></div></div>");
    html += F("<div class='msg-area' id='opmsg' style='text-align:center;font-size:14px;margin:0 0 15px 0;'>Operation Tips/Return Results Display Area</div>");
    html += F("<div class='row space-between'>");
    html += F("<button id='level-cal-btn' onclick=\"calibrateLevel()\">Calibrate Level</button>");
    html += F("<button id='accel-cal-btn' onclick=\"calibrateAccel()\" style='margin:0 15px;'>Calibrate Accel</button>");
    html += F("<button id='armBtn' onclick=\"toggleArmDisarm()\">Un/lock</button></div>");
    html += F("<div style='margin:18px 0;'><input id='paramInput' style='width:65%;padding:6px;border-bottom:1px solid rgb(255, 0, 0);' placeholder='PARAM=VALUE,...'><button onclick='setParam()' style='margin-left:10px;'>RQ/SET</button></div>");
    html += F("<div class='tabs'><button class='tab active' onclick=\"showTab(0)\">FLYMODEL</button><button class='tab' onclick=\"showTab(1)\">Servo Out</button></div>");
    html += F("<div class='tab-content active' id='tab0'><div style='padding:2px;'><div style='display:grid;grid-template-columns:repeat(2,1fr);gap:5px;'>");
    html += F("<div style='grid-column:1/-1;display:flex;flex-wrap:wrap;justify-content:space-between;align-items:center;gap:8px;margin:0 0 8px 0;padding:8px;background:#e8f4fc;border-radius:6px;border:1px solid #bee5eb;font-size:13px;'>");
    html += F("<span>Flight mode channel <small>(FLTMODE_CH / MODE_CH)</small>: <strong id='fltmodeChDisp'>--</strong></span>");
    html += F("<span>PWM: <strong id='fltmodePwmDisp'>--</strong></span></div>");
    for (int i = 0; i < FLTMODE_PARAM_COUNT; ++i) {
      html += F("<div style='display:flex;align-items:center;gap:5px;margin:3px 0;'><label style='min-width:20px;font-size:12px;flex-shrink:0;'>Mode");
      html += String(i + 1);
      html += F(":</label><select id='fltmode");
      html += String(i + 1);
      html += F("' style='flex:1;border:1px solid #ddd;border-radius:3px;font-size:12px;min-width:0;max-width:70px;'></select></div>");
    }
    html += F("</div><button onclick='saveModes()' style='background:#007bff;color:white;padding:8px 16px;border:none;border-radius:4px;cursor:pointer;margin-top:15px;width:100%;'>Save Modes</button></div></div>");
    html += F("<div class='tab-content' id='tab1'><h4 style='margin:0 0 10px 0; color:#495057;'>Servo Output Configuration</h4>");
    html += F("<div id='servo-loading' style='text-align:center; padding:20px;'><p>Loading servo configuration...</p></div>");
    html += F("<div id='servo-table-container' style='display:none;'></div></div>");
    html += F("<div id='paramText' style='color:#666;font-size:14px;'>Loading parameters...</div><div class='az-bar'>");
    for (char c = 'A'; c <= 'Z'; ++c) {
      html += F("<button class='az-btn' onclick=\"loadParams('");
      html += String(c);
      html += F("')\" id='az");
      html += String(c);
      html += F("'>");
      html += String(c);
      html += F("</button>");
    }
    html += F("</div><div id='paramList'></div><div id='paramStatus' style='text-align:center;margin:10px 0;'><div style='background:#f0f0f0;border-radius:10px;height:18px;overflow:hidden;margin:10px 0;'><div id='paramProgress' style='background:#007bff;height:100%;width:0%;transition:width 0.3s;'></div></div></div>");
    html += F("<script src=\"/app.js\"></script></div></body></html>");
    yield();
    server.send(200, "text/html", html);
  });

'''

path.write_text(text[:start] + NEW + text[end:], encoding="utf-8")
print("patched", path)
