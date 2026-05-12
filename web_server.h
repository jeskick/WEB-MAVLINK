#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include "config.h"
#include <WebServer.h>

// WebServer 未知总长（分块响应）
#ifndef MAVWEB_CONTENT_LENGTH_UNKNOWN
#define MAVWEB_CONTENT_LENGTH_UNKNOWN ((size_t)-1)
#endif

// 飞控 STATUSTEXT 等常含引号/换行，不转义会破坏 JSON → 浏览器 fetch 失败、界面「闪断」
inline String jsonEscapeForJson(const String& s) {
  String o;
  if (s.length() == 0) return o;
  o.reserve(s.length() + 16);
  for (unsigned i = 0; i < s.length(); ++i) {
    char c = s[i];
    if (c == '"') {
      o += "\\\"";
      continue;
    }
    if (c == '\\') {
      o += "\\\\";
      continue;
    }
    if (c == '\n' || c == '\r' || (unsigned char)c < 0x20) {
      o += ' ';
      continue;
    }
    o += c;
  }
  return o;
}

// 外部声明
extern WebServer server;
extern std::map<String, float> paramMap;
extern std::set<uint16_t> paramIndexSet;
extern uint16_t param_total;
extern bool param_ready;
extern String systemMessage;
extern String operationMessage;
extern float voltage;
extern String flightMode;
extern int gpsCount;
extern bool isArmed;
extern uint16_t rc_fltmode_pwm_us;
extern bool rc_fltmode_pwm_valid;
extern ServoData servoOutputs[16];
extern const PlaneMode PLANE_MODES[];
extern const int PLANE_MODE_COUNT;
extern const char* FLTMODE_PARAMS[];
extern bool lastArmCommand;
extern int armErrorCount;
extern HardwareSerial mavSerial;
extern std::map<String, float> pendingSetMap;

// 工具函数：参数按首字母分组
inline std::vector<String> getParamNamesByLetter(char letter) {
  std::vector<String> result;
  for (const auto& kv : paramMap) {
    if (kv.first.length() > 0 && toupper(kv.first[0]) == toupper(letter)) {
      result.push_back(kv.first);
    }
  }
  return result;
}

// 初始化Web服务器
inline void initWebServer() {
  server.on("/", HTTP_GET, []() {
    String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
    html.reserve(62000);
    html += "<meta name='viewport' content='width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no'>";
    html += "<title>ESP32 MAVLink Ground Station</title>";
    html += "<style>";
    html += "body{font-family:Arial,sans-serif;margin:0;padding:20px;background:#f5f5f5;}";
    html += ".container{max-width:1200px;margin:0 auto;background:white;padding:20px;border-radius:10px;box-shadow:0 2px 10px rgba(0,0,0,0.1);}";
    html += ".header{text-align:center;margin-bottom:30px;color:#333;}";
    html += ".status-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(200px,1fr));gap:15px;margin-bottom:30px;}";
    html += ".status-item{background:#f8f9fa;padding:15px;border-radius:8px;border-left:4px solid #007bff;}";
    html += ".status-item h3{margin:0 0 10px 0;color:#495057;}";
    html += ".status-item p{margin:0;font-size:18px;font-weight:bold;color:#28a745;}";
    html += ".status-item.error p{color:#dc3545;}";
    html += ".status-item.warning p{color:#ffc107;}";
    html += ".tabs{display:flex;border-bottom:2px solid #dee2e6;margin-bottom:20px;}";
    html += ".tab{padding:10px 20px;cursor:pointer;border:none;background:none;color:#6c757d;}";
    html += ".tab.active{color:#007bff;border-bottom:2px solid #007bff;}";
    html += ".tab-content{display:none;}";
    html += ".tab-content.active{display:block;}";
    html += ".control-section{margin-bottom:30px;}";
    html += ".control-section h3{color:#495057;margin-bottom:15px;}";
    html += ".button-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(150px,1fr));gap:10px;}";
    html += "button{padding:8px 6px;border:none;border-radius:5px;cursor:pointer;font-size:14px;transition:all 0.3s;}";
    html += "button:hover{opacity:0.8;}";
    html += "button:disabled{opacity:0.5;cursor:not-allowed;background-color:#6c757d !important;}";
    html += ".btn-primary{background:#007bff;color:white;}";
    html += ".btn-success{background:#28a745;color:white;}";
    html += ".btn-warning{background:#ffc107;color:#212529;}";
    html += ".btn-danger{background:#dc3545;color:white;}";
    html += ".btn-info{background:#17a2b8;color:white;}";
    html += ".az-bar{display:flex;flex-wrap:wrap;gap:5px;margin-bottom:15px;}";
    html += ".az-bar button{padding:2px;}";
    html += ".param-section{margin-top:20px;}";
    html += ".param-input{display:flex;gap:10px;margin-bottom:15px;}";
    html += ".param-input input{flex:1;padding:8px;border:1px solid #ddd;border-radius:4px;}";
    html += ".param-list{background:#f8f9fa;padding:15px;border-radius:5px;max-height:300px;overflow-y:auto;}";
    html += ".param-row{cursor:pointer;padding:3px 6px;margin:2px 0;border-radius:4px;text-align:left;font-family:monospace;font-size:12px;line-height:1.35;}";
    html += ".param-row:hover{background:#e3f2fd;}";
    html += ".servo-table{width:100%;border-collapse:collapse;margin-top:15px;}";
    html += ".servo-table th,.servo-table td{border:1px solid #ddd;padding:8px;text-align:center;}";
    html += ".servo-table th{background:#f8f9fa;}";
    html += ".servo-controls{display:grid;grid-template-columns:repeat(auto-fit,minmax(200px,1fr));gap:15px;}";
    html += ".servo-item{background:#f8f9fa;padding:15px;border-radius:8px;}";
    html += ".servo-item h4{margin:0 0 10px 0;color:#495057;}";
    html += ".servo-control{display:flex;align-items:center;gap:10px;margin-bottom:8px;}";
    html += ".servo-control input{width:80px;padding:4px;border:1px solid #ddd;border-radius:3px;}";
    html += ".servo-control label{min-width:60px;font-size:12px;}";
    html += ".servo-control button{padding:4px 8px;font-size:12px;}";
    html += ".mode-section{margin-bottom:20px;}";
    html += ".mode-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(200px,1fr));gap:10px;}";
    html += ".mode-item{display:flex;align-items:center;gap:10px;}";
    html += ".mode-item select{padding:5px;border:1px solid #ddd;border-radius:3px;}";
    html += ".message-box{background:#e9ecef;padding:15px;border-radius:5px;margin-top:20px;min-height:50px;}";
    html += ".message-box h4{margin:0 0 10px 0;color:#495057;}";
    html += ".message-box p{margin:0;color:#6c757d;}";
    
    /* 消息样式：飞控多行黄区 + 单行横向滚动；操作提示蓝区横向滚动 */
    html += ".msg{font-weight:bold;margin-bottom:8px;padding:5px;border-radius:4px;text-align:center;word-wrap:break-word;overflow-wrap:break-word;white-space:normal;min-height:1.2em;}";
    html += ".sysmsg-wrap{width:100%;max-width:100%;max-height:7em;overflow-y:auto;padding:6px 8px;box-sizing:border-box;text-align:left;background:#fff8e1;border:1px solid #ffe082;border-radius:6px;}";
    html += ".sysmsg-line{font-size:13px;font-weight:bold;color:#c77800;line-height:1.45;padding:4px 2px;white-space:nowrap;overflow-x:auto;overflow-y:hidden;width:100%;box-sizing:border-box;-webkit-overflow-scrolling:touch;scrollbar-width:thin;}";
    html += ".opmsg-wrap{font-size:14px;font-weight:bold;color:#0066CC;background:#E6F3FF;margin:0 0 15px 0;padding:8px 10px;border-radius:6px;text-align:center;white-space:nowrap;overflow-x:auto;overflow-y:hidden;width:100%;box-sizing:border-box;-webkit-overflow-scrolling:touch;scrollbar-width:thin;}";
    
    /* 移动端适配 */
    html += "@media (max-width: 768px) {";
    html += ".container{padding:10px;}";
    html += ".status-grid{grid-template-columns:1fr;}";
    html += ".button-grid{grid-template-columns:1fr;}";
    html += ".row.space-between{flex-direction:column;gap:10px;}";
    html += ".row.space-between .col{flex:none;}";
    html += ".az-bar{justify-content:center;}";
    html += ".az-bar button{padding:2px 2px;font-size:12px;}";
    html += "}";
    
    /* 确保电压、模式、GPS在一排显示 */
    html += "@media (max-width: 480px) {";
    html += "div[style*='display:flex'][style*='justify-content:space-between']{";
    html += "flex-direction:row !important;";
    html += "flex-wrap:nowrap !important;";
    html += "}";
    html += "div[style*='display:flex'][style*='justify-content:space-between'] > div{";
    html += "flex:1 !important;";
    html += "min-width:0 !important;";
    html += "font-size:12px !important;";
    html += "}";
    html += "}";
    
    /* 禁用按钮的特殊样式 */
    html += ".btn-disabled {";
    html += "background-color: #6c757d !important;";
    html += "color: #fff !important;";
    html += "cursor: not-allowed !important;";
    html += "opacity: 0.6 !important;";
    html += "pointer-events: none !important;";
    html += "}";
    
    /* 校准完成按钮样式 */
    html += ".btn-calibrated {";
    html += "background-color: #28a745 !important;";
    html += "color: white !important;";
    html += "cursor: not-allowed !important;";
    html += "opacity: 0.8 !important;";
    html += "pointer-events: none !important;";
    html += "}";
    html += "</style></head><body>";
    yield();
    html += "<div class='container'>";
    // 1. 第一排：系统实时消息（动态）
    html += "<div class='row center' style='width:100%;'><div id='sysmsg' class='sysmsg-wrap'><div class='sysmsg-line'>--</div></div></div>";
    // 2. 第二排：电压/飞行模式/GPS
    html += "<div style='display:flex;justify-content:space-between;align-items:center;margin:5px 0;background:#f8f9fa;padding:5px;border-radius:5px;'>";
    html += "<div style='flex:1;text-align:center;font-weight:bold;'>volt : <span id='voltage'>-- V</span></div>";
    html += "<div style='flex:1;text-align:center;font-weight:bold;'>MODE : <span id='flightMode'>--</span></div>";
    html += "<div style='flex:1;text-align:center;font-weight:bold;'>GPS : <span id='gpsCount'>--</span></div>";
    html += "</div>";
    // 3. 第三排：操作返回结果和提示
    html += "<div class='msg-area opmsg-wrap' id='opmsg'>Operation Tips/Return Results Display Area</div>";
    // 4. 校准/解锁按钮
    html += "<div class='row space-between'>";
    html += "<button id='level-cal-btn' onclick=\"calibrateLevel()\">Calibrate Level</button>";
    html += "<button id='accel-cal-btn' onclick=\"calibrateAccel()\" style='margin:0 15px;'>Calibrate Accel</button>";
    html += "<button id='armBtn' onclick=\"toggleArmDisarm()\">Un/lock</button>";
    html += "</div>";
    // 5. 参数查询/修改
    html += "<div style='margin:18px 0;'>";
    html += "<input id='paramInput' style='width:65%;padding:6px;border-bottom:1px solid rgb(255, 0, 0);' placeholder='PARAM=VALUE,PARAM2=VAL2,... (use comma to separate)'>";
    html += "<button onclick='setParam()' style='margin-left:10px;'>RQ/SET</button>";
    html += "</div>";
    // 6. 标签页
    html += "<div class='tabs'>";
    html += "<button class='tab active' onclick=\"showTab(0)\">FLYMODEL</button>";
    html += "<button class='tab' onclick=\"showTab(1)\">Servo Out</button>";
    html += "</div>";
    html += "<div class='tab-content active' id='tab0'>";
    html += "<div style='padding:2px;'>";
    html += "<div style='display:grid;grid-template-columns:repeat(2,1fr);gap:5px;'>";
    html += "<div style='grid-column:1/-1;font-size:13px;margin:0 0 6px 0;'>FLYMODEL_CH : <strong id='fltmodeChDisp'>--</strong> &nbsp;&nbsp; PWM : <strong id='fltmodePwmDisp'>--</strong></div>";
    for (int i = 0; i < FLTMODE_PARAM_COUNT; ++i) {
      int cur = paramMap.count(FLTMODE_PARAMS[i]) ? (int)paramMap[FLTMODE_PARAMS[i]] : 0;
      html += "<div style='display:flex;align-items:center;gap:5px;margin:3px 0;'>";
      html += "<label style='min-width:20px;font-size:12px;flex-shrink:0;'>Mode" + String(i+1) + ":</label>";
      html += "<select id='fltmode" + String(i+1) + "' style='flex:1;border:1px solid #ddd;border-radius:3px;font-size:12px;min-width:0;max-width:70px;'>";
      for (int j = 0; j < PLANE_MODE_COUNT; ++j) {
        html += "<option value='" + String(PLANE_MODES[j].num) + "'";
        if (PLANE_MODES[j].num == cur) html += " selected";
        html += ">" + String(PLANE_MODES[j].name) + "</option>";
      }
      html += "</select>";
      html += "</div>";
    }
    html += "</div>";
    html += "<button onclick='saveModes()' style='background:#007bff;color:white;padding:8px 16px;border:none;border-radius:4px;cursor:pointer;margin-top:15px;width:100%;'>Save Modes</button>";
    html += "</div></div>";
    html += "<div class='tab-content' id='tab1'>";
    html += "<h4 style='margin:0 0 10px 0; color:#495057;'>Servo Output Configuration</h4>";
    html += "<div id='servo-loading' style='text-align:center; padding:20px;'>";
    html += "<p>Loading servo configuration...</p>";
    html += "</div>";
    html += "<div id='servo-table-container' style='display:none;'>";
    html += "</div>";
    html += "</div>";    
    // 7. A-Z参数分组加载
    html += "<div id='paramText' style='color:#666;font-size:14px;'>Loading parameters...</div>";
    html += "<div class='az-bar'>";
    for (char c = 'A'; c <= 'Z'; ++c) {
      html += "<button class='az-btn' onclick=\"loadParams('" + String(c) + "')\" id='az" + String(c) + "'>" + String(c) + "</button>";
    }
    html += "</div>";
    html += "<div id='paramList' class='param-list'></div>";
    html += "<div id='paramStatus' style='text-align:center;margin:10px 0;'>";
    html += "<div style='background:#f0f0f0;border-radius:10px;height:18px;overflow:hidden;margin:10px 0;'>";
    html += "<div id='paramProgress' style='background:#007bff;height:100%;width:0%;transition:width 0.3s;'></div>";
    html += "</div>";    
    html += "</div>";
    // JS动态刷新
    html += R"(
<script>
// 全局变量跟踪校准状态
var levelCalibrated = false;
var accelCalibrated = false;

// 记录最近一条PreArm消息
let lastPreArmMsg = "";

function renderSystemMessage(text) {
  var el = document.getElementById('sysmsg');
  if (!el) return;
  while (el.firstChild) el.removeChild(el.firstChild);
  var s = (text === undefined || text === null) ? '' : String(text);
  if (!s.length) {
    var d0 = document.createElement('div');
    d0.className = 'sysmsg-line';
    d0.textContent = '--';
    el.appendChild(d0);
    return;
  }
  var parts = s.split('\n');
  for (var i = 0; i < parts.length; i++) {
    var row = document.createElement('div');
    row.className = 'sysmsg-line';
    row.textContent = parts[i];
    el.appendChild(row);
  }
}

function updateStatus(){
  fetch('/api/status').then(r=>r.json()).then(data=>{
    renderSystemMessage(data.systemMessage);
    document.getElementById('voltage').textContent=data.voltage+'V';
    document.getElementById('flightMode').textContent=data.flightMode;
    document.getElementById('gpsCount').textContent=data.gpsCount;
    document.getElementById('opmsg').textContent=data.operationMessage;
    var fch=document.getElementById('fltmodeChDisp');
    var fpw=document.getElementById('fltmodePwmDisp');
    if(fch) fch.textContent=(data.fltmodeCh>0)?String(data.fltmodeCh):'--';
    if(fpw) fpw.textContent=data.fltmodePwmValid?String(data.fltmodePwm):'--';
    document.getElementById('armBtn').textContent=data.isArmed?'Disarm':'Un/lock';
    
    // 记录最近一条PreArm消息
    if (data.systemMessage && data.systemMessage.indexOf('PreArm:') >= 0) {
      var firstLine = data.systemMessage.split('\n')[0];
      lastPreArmMsg = firstLine;
    }
    
    // 更新ARM/DISARM按钮状态
    updateArmDisarmButton(data.isArmed);
    
    // 更新水平校准按钮状态
    if (data.levelCalibrated) {
      levelCalibrated = true;
      let levelBtn = document.getElementById('level-cal-btn');
      if (levelBtn) {
        levelBtn.disabled = true;
        levelBtn.textContent = 'Level Calibrated';
        levelBtn.style.background = '#888';
      }
    }
    
    // 更新加速度计校准按钮状态
    if (data.accelCalibrated) {
      accelCalibrated = true;
      let accelBtn = document.getElementById('accel-cal-btn');
      if (accelBtn) {
        accelBtn.disabled = true;
        accelBtn.textContent = 'Accel Calibrated';
        accelBtn.style.background = '#888';
      }
    }
    
    // 更新参数加载状态和进度条
    updateParamLoadingStatus(data);
  }).catch(err=>{
    console.error('Status update failed:', err);
  });
}

// 新增：更新参数加载状态和进度条
function updateParamLoadingStatus(data) {
  var azBar = document.querySelector('.az-bar');
  var paramStatus = document.getElementById('paramStatus');
  var paramInput = document.getElementById('paramInput');
  var setParamBtn = document.querySelector("button[onclick='setParam()']");
  var paramProgress = document.getElementById('paramProgress');
  var paramText = document.getElementById('paramText');
  
  if(data.paramReady) {
    // 参数加载完成
    azBar.style.display = 'flex';
    paramProgress.style.width = '100%';
    paramText.textContent = 'Total parameters: ' + data.paramTotal;
    paramText.style.color = '#28a745';
    paramInput.disabled = false;
    setParamBtn.disabled = false;
  } else {
    // 参数加载中
    azBar.style.display = 'none';
    paramInput.disabled = true;
    setParamBtn.disabled = true;
    
    // 计算进度
    if (data.paramTotal && data.paramTotal > 0) {
      var progress = (data.paramReceived / data.paramTotal) * 100;
      paramProgress.style.width = progress + '%';
      paramText.textContent = 'Loading parameters... ' + data.paramReceived + '/' + data.paramTotal;
      paramText.style.color = '#ffc107';
    } else {
      paramProgress.style.width = '0%';
      paramText.textContent = 'Loading parameters...';
      paramText.style.color = '#ffc107';
    }
    
    // 清空参数列表
    document.getElementById('paramList').innerHTML = "";
  }
}

function copyParamLineToClipboard(text) {
  if (navigator.clipboard && navigator.clipboard.writeText) {
    navigator.clipboard.writeText(text).catch(function(){ fallbackCopyParam(text); });
    return;
  }
  fallbackCopyParam(text);
}
function fallbackCopyParam(text) {
  var ta = document.createElement('textarea');
  ta.value = text;
  ta.setAttribute('readonly','');
  ta.style.position = 'fixed';
  ta.style.left = '-9999px';
  document.body.appendChild(ta);
  ta.select();
  try { document.execCommand('copy'); } catch(e) {}
  document.body.removeChild(ta);
}
(function bindParamListCopyDelegation(){
  var pl = document.getElementById('paramList');
  if (!pl || pl._paramCopyBound) return;
  pl._paramCopyBound = true;
  pl.addEventListener('click', function(ev){
    var row = ev.target.closest('.param-row');
    if (!row) return;
    var t = (row.textContent || '').trim();
    if (!t) return;
    copyParamLineToClipboard(t);
    var op = document.getElementById('opmsg');
    if (op) op.textContent = 'Copied: ' + t;
  });
})();

function showTab(idx){
  var tabs=document.querySelectorAll('.tab');
  var contents=document.querySelectorAll('.tab-content');
  for(var i=0;i<tabs.length;i++){
    if(i==idx){tabs[i].classList.add('active');contents[i].classList.add('active');}
    else{tabs[i].classList.remove('active');contents[i].classList.remove('active');}
  }
  // 切换到Servo Out时加载完整表格
  if(idx==1){
    loadServoOutputContent();
  }
}
function setParam(){
  // 检查参数是否加载完成
  fetch('/api/status').then(r=>r.json()).then(data=>{
    if(!data.paramReady) {
      document.getElementById('opmsg').textContent = 'Parameters are not fully loaded, please wait...';
      return;
    }
    
    var v=document.getElementById('paramInput').value;
    fetch('/api/set_param?input='+encodeURIComponent(v)).then(r=>r.text()).then(t=>{
      document.getElementById('opmsg').textContent=t;
    });
  });
}
function loadParams(letter){
  // 检查参数是否加载完成
  fetch('/api/status').then(r=>r.json()).then(data=>{
    if(!data.paramReady) {
      document.getElementById('opmsg').textContent = 'Parameters are not fully loaded, please wait...';
      return;
    }
    
    fetch('/api/params_by_letter?letter='+letter).then(r=>r.json()).then(data=>{
      function escHtml(s){
        return String(s).replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;');
      }
      var html = '';
      for(var key in data.parameters){
        if(!Object.prototype.hasOwnProperty.call(data.parameters,key)) continue;
        var v = data.parameters[key];
        if(typeof v === 'number') v = String(v);
        var line = key + '=' + v;
        html += '<div class="param-row" title="Click to copy line">' + escHtml(line) + '</div>';
      }
      document.getElementById('paramList').innerHTML = html;
      
      // 显示当前字母参数的总数
      var paramText = document.getElementById('paramText');
      if (paramText) {
        paramText.textContent = 'Letter ' + letter + ' total parameters: ' + data.count;
        paramText.style.color = '#007bff';
      }
    });
  });
}
function calib(type){
  fetch('/api/calib/'+type,{method:'POST'}).then(r=>r.json()).then(data=>{
    document.getElementById('opmsg').textContent=data.message;
  });
}
function toggleArmDisarm(){
  let button = document.getElementById('armBtn');
  
  // 直接使用服务器返回的isArmed状态，而不是根据按钮颜色判断
  // 获取当前状态
  fetch('/api/status')
    .then(r => r.json())
    .then(data => {
      let isCurrentlyArmed = data.isArmed;
      
      // 发送相反的命令：如果当前是上锁状态，发送解锁命令；如果当前是解锁状态，发送上锁命令
      let action = isCurrentlyArmed ? 0 : 1; // 0=上锁, 1=解锁
      
      // 显示操作消息
      document.getElementById('opmsg').textContent = action === 1 ? 'Sending ARM command...' : 'Sending DISARM command...';
      
      // 如果是解锁命令，立即开始监听错误消息
      if (action === 1) {
        // 清空之前的错误
        fetch('/clear_arm_errors')
          .catch(err => {
            console.error('Failed to clear arm errors:', err);
          });
        
        // 清除之前的定时器（如果存在）
        if (window.errorCheckInterval) {
          clearInterval(window.errorCheckInterval);
          window.errorCheckInterval = null;
        }
        
        // 添加标志防止重复显示弹窗
        window.errorModalShown = false;
        
        // 立即开始检查错误消息
        window.errorCheckInterval = setInterval(() => {
          checkArmErrors();
        }, 500); // 每500ms检查一次
        
        // 5秒后停止检查
        setTimeout(() => {
          if (window.errorCheckInterval) {
            clearInterval(window.errorCheckInterval);
            window.errorCheckInterval = null;
          }
        }, 5000);
      }
      
      // 发送ARM/DISARM命令
      return fetch('/arm_disarm?action=' + action);
    })
    .then(r => r.json())
    .then(data => {
      console.log('Arm/Disarm response:', data);
      
      // 更新操作消息
      let action = data.action;
      document.getElementById('opmsg').textContent = action === 1 ? 'ARM command sent' : 'DISARM command sent';
      
      // 更新按钮颜色
      setTimeout(() => {
        updateArmDisarmButtonColor();
      }, 2000);

      // 只要armDenied为true就弹窗
      if (data.armDenied === true || data.armDenied === 'true') {
        showForceArmDialog();
      }
    })
    .catch(err => {
      document.getElementById('opmsg').textContent = 'ARM/DISARM failed: ' + err.message;
    });
}
function saveModes(){
  console.log('saveModes function called!');
  var params = [];
  for(var i=1;i<=6;i++){
    var val=document.getElementById('fltmode'+i).value;
    params.push('FLTMODE'+i+'='+val);
  }
  function setNext(idx) {
    if(idx >= params.length) return;
    fetch('/api/set_param?input='+params[idx])
      .then(r=>r.text())
      .then(t=>{
        document.getElementById('opmsg').textContent=t;
        setTimeout(()=>setNext(idx+1), 400);
      });
  }
  setNext(0);
}

function loadServoOutputContent(){
  fetch('/servo_output').then(r=>r.text()).then(html=>{
    document.getElementById('servo-table-container').innerHTML=html;
    document.getElementById('servo-loading').style.display='none';
    document.getElementById('servo-table-container').style.display='block';
    startServoOutputUpdate();
  });
}
function startServoOutputUpdate(){
  setInterval(()=>{
    fetch('/servo_positions').then(r=>r.json()).then(data=>{
      for(var i=1;i<=13;i++){
        var posSpan=document.getElementById('servo'+i+'_pos');
        if(posSpan) posSpan.textContent=data['servo'+i];
      }
    }).catch(err=>console.error('Failed to update servo positions:',err));
  },500);
}
function updateServoParam(servoNum,param,value){
  var paramName='SERVO'+servoNum+'_'+param;
  fetch('/api/set_param?input='+encodeURIComponent(paramName+'='+value)).then(r=>r.text()).then(t=>{
    document.getElementById('opmsg').textContent=t;
  });
}

// 校准功能函数
function calibrateLevel(){
  if (levelCalibrated) {
    console.log('Level already calibrated, button should be disabled');
    return;
  }
  
  fetch('/calibrate_level').then(r=>r.text()).then(t=>{
    document.getElementById('opmsg').textContent=t;
    console.log('Level calibration response:', t);
    
    // 检查是否校准完成，使用旧代码的逻辑
    if (t.includes('success') || t.includes('completed') || t.includes('Trim OK')) {
      levelCalibrated = true;
      let levelBtn = document.getElementById('level-cal-btn');
      if (levelBtn) {
        levelBtn.disabled = true;
        levelBtn.textContent = 'Level Calibrated';
        levelBtn.style.background = '#888';
        console.log('Level button disabled based on success/completed text');
      }
    }
  }).catch(err=>{
    document.getElementById('opmsg').textContent='Level calibration failed: '+err.message;
  });
}

function calibrateAccel(){
  if (accelCalibrated) {
    console.log('Accel already calibrated, button should be disabled');
    return;
  }
  
  fetch('/calibrate_accel').then(r=>r.text()).then(t=>{
    document.getElementById('opmsg').textContent=t;
    console.log('Accel calibration response:', t);
    
    let accelBtn = document.getElementById('accel-cal-btn');
    if (accelBtn) {
      // 如果正在校准中，改变按钮文本
      if (t.includes('started') || t.includes('waiting') || t.includes('Place vehicle')) {
        accelBtn.textContent = 'Confirm Position';
      } else if (t.includes('confirmed')) {
        accelBtn.textContent = 'Calibrate Accel';
      } else if (t.includes('success') || t.includes('completed') || t.includes('Calibration OK')) {
        // 如果校准成功，禁用按钮
        accelCalibrated = true;
        accelBtn.disabled = true;
        accelBtn.textContent = 'Accel Calibrated';
        accelBtn.style.background = '#888';
        console.log('Accel button disabled based on success/completed text');
      }
    }
  }).catch(err=>{
    document.getElementById('opmsg').textContent='Accel calibration failed: '+err.message;
  });
}

function checkArmErrors(){
  fetch('/arm_errors').then(r=>r.json()).then(errors=>{
    let errorDiv=document.getElementById('arm-errors');
    if(errors.length>0){
      errorDiv.innerHTML='<strong>ARM errors:</strong><br>'+errors.join('<br>');
    }else{
      errorDiv.innerHTML='<strong>No ARM errors</strong>';
    }
  }).catch(err=>{
    document.getElementById('arm-errors').innerHTML='Check errors failed: '+err.message;
  });
}

function clearArmErrors(){
  fetch('/clear_arm_errors').then(r=>r.text()).then(t=>{
    document.getElementById('arm-errors').innerHTML='<strong>Errors cleared</strong>';
    document.getElementById('opmsg').textContent=t;
  }).catch(err=>{
    document.getElementById('opmsg').textContent='Clear errors failed: '+err.message;
  });
}

function updateArmDisarmButton(isArmed){
  let btn=document.getElementById('armBtn');
  if(isArmed){
    btn.textContent='DISARM';
    btn.style.background='#28a745'; // 绿色，已解锁
    btn.style.color='#fff';
  }else{
    btn.textContent='ARM';
    btn.style.background='#dc3545'; // 红色，未解锁
    btn.style.color='#fff';
  }
}

function updateArmDisarmButtonColor() {
  fetch('/api/status')
    .then(r => r.json())
    .then(data => {
      let button = document.getElementById('armBtn');
      if (data.isArmed) {
        // 解锁状态，按钮变黄色
        button.style.backgroundColor = '#ffff00'; // 黄色
      } else {
        // 上锁状态，按钮变红色
        button.style.backgroundColor = '#dc3545'; // 红色
      }
    })
    .catch(err => {
      console.error('Failed to update button color:', err);
    });
}

// 悬浮弹窗HTML
function showForceArmDialog() {
  if (document.getElementById('force-arm-dialog')) return;
  let dialog = document.createElement('div');
  dialog.id = 'force-arm-dialog';
  dialog.style.position = 'fixed';
  dialog.style.left = '50%';
  dialog.style.top = '30%';
  dialog.style.transform = 'translate(-50%, -50%)';
  dialog.style.background = '#222';
  dialog.style.color = '#fff';
  dialog.style.padding = '16px 18px 10px 18px';
  dialog.style.borderRadius = '8px';
  dialog.style.boxShadow = '0 2px 12px rgba(0,0,0,0.18)';
  dialog.style.zIndex = 9999;
  dialog.style.textAlign = 'center';
  dialog.style.minWidth = '260px';
  dialog.innerHTML = `
    <div style='font-size:16px;font-weight:bold;margin-bottom:6px;color:#ff0;'>⚠️ Arm failed</div>
    <div style='font-size:13px;margin-bottom:10px;line-height:1.5;'>
      Force Arm can bypass safety checks, which can lead to the vehicle crashing and causing serious injuries.<br>Do you wish to Force Arm?
    </div>
    <div style='display:flex;justify-content:center;gap:12px;margin-top:6px;'>
      <button id='force-arm-confirm' style='background:#8f0;color:#222;font-weight:bold;padding:5px 16px;border:none;border-radius:4px;cursor:pointer;'>Force Arm</button>
      <button id='force-arm-cancel' style='background:#888;color:#fff;padding:5px 16px;border:none;border-radius:4px;cursor:pointer;'>Cancel</button>
    </div>
  `;
  document.body.appendChild(dialog);
  document.getElementById('force-arm-cancel').onclick = function() {
    dialog.remove();
  };
  document.getElementById('force-arm-confirm').onclick = function() {
    dialog.remove();
    fetch('/arm_disarm?action=1&force=1').then(r=>r.json()).then(data=>{
      if (data.armDenied === true || data.armDenied === 'true') {
        document.getElementById('opmsg').textContent = 'Force Arm was also denied by the autopilot!';
      } else {
        document.getElementById('opmsg').textContent = 'Force Arm command sent.';
      }
      setTimeout(()=>{ updateArmDisarmButtonColor(); }, 2000);
    });
  };
}

// 启动定时器
setInterval(updateStatus,1000);
updateStatus();

// 页面加载时检查校准状态
window.addEventListener('load', function() {
  console.log('Page loaded, checking calibration status...');
  // 延迟一点时间确保DOM完全加载
  setTimeout(function() {
    let levelBtn = document.getElementById('level-cal-btn');
    if (levelBtn && levelBtn.textContent === 'Level Calibrated') {
      levelCalibrated = true;
      levelBtn.disabled = true;
      levelBtn.style.background = '#888';
      levelBtn.style.color = '#fff';
      levelBtn.style.cursor = 'not-allowed';
      levelBtn.onclick = null;
      levelBtn.removeAttribute('onclick');
      console.log('Level button disabled on page load');
    }
    
    // 更新ARM/DISARM按钮颜色
    updateArmDisarmButtonColor();
  }, 1000);
});
</script>
)";
    html += "</body></html>";
    server.send(200, "text/html", html);
  });

  // API接口：状态信息
  server.on("/api/status", HTTP_GET, []() {
    String json = "{";
    json += "\"systemMessage\":\"" + jsonEscapeForJson(systemMessage) + "\",";
    json += "\"operationMessage\":\"" + jsonEscapeForJson(operationMessage) + "\",";
    json += "\"voltage\":" + String(voltage,2) + ",";
    json += "\"flightMode\":\"" + jsonEscapeForJson(flightMode) + "\",";
    json += "\"gpsCount\":" + String(gpsCount) + ",";
    int flt_ch = 0;
    if (paramMap.count(String("FLTMODE_CH"))) {
      flt_ch = (int)(paramMap[String("FLTMODE_CH")] + 0.5f);
    } else if (paramMap.count(String("MODE_CH"))) {
      flt_ch = (int)(paramMap[String("MODE_CH")] + 0.5f);
    }
    json += "\"fltmodeCh\":" + String(flt_ch) + ",";
    json += "\"fltmodePwm\":" + String(rc_fltmode_pwm_us) + ",";
    json += "\"fltmodePwmValid\":" + String(rc_fltmode_pwm_valid ? "true" : "false") + ",";
    json += "\"isArmed\":" + String(isArmed ? "true" : "false") + ",";
    json += "\"paramReady\":" + String(param_ready ? "true" : "false") + ",";
    json += "\"paramTotal\":" + String(param_total) + ",";
    json += "\"paramReceived\":" + String(paramIndexSet.size()) + ",";
    json += "\"levelCalibrated\":" + String(levelCalibrated ? "true" : "false") + ",";
    json += "\"accelCalibrated\":" + String(accelCalibrated ? "true" : "false") + ",";
    json += "\"armDenied\":" + String(armDenied ? "true" : "false");
    json += "}";
    server.send(200, "application/json", json);
  });

  // API接口：参数分组A-Z
  server.on("/api/params_by_letter", HTTP_GET, []() {
    char letter = server.hasArg("letter") ? server.arg("letter")[0] : 'A';
    std::vector<String> names = getParamNamesByLetter(letter);
    String json = "{\"count\":" + String(names.size()) + ",\"parameters\":{";
    for (size_t i = 0; i < names.size(); ++i) {
      if (i > 0) json += ",";
      json += "\"" + names[i] + "\":" + String(paramMap[names[i]]);
    }
    json += "}}";
    server.send(200, "application/json", json);
  });

  // API接口：参数设置/查询
  server.on("/api/set_param", HTTP_GET, []() {
    String input = server.hasArg("input") ? server.arg("input") : "";
    input.toUpperCase();
    String result = "";
    int start = 0;
    while (start < input.length()) {
      // 查找英文逗号或中文逗号
      int comma = input.indexOf(',', start);
      int chineseComma = input.indexOf('，', start);
      
      // 取最近的分隔符
      if (comma == -1 && chineseComma == -1) {
        comma = input.length();
      } else if (comma == -1) {
        comma = chineseComma;
      } else if (chineseComma == -1) {
        // comma already set
      } else {
        comma = (comma < chineseComma) ? comma : chineseComma;
      }
      
      String token = input.substring(start, comma);
      token.trim();
      int eq = token.indexOf('=');
      if (eq > 0) {
        String key = token.substring(0, eq);
        String val = token.substring(eq + 1);
        float fval = val.toFloat();
        
        // 确定参数类型
        uint8_t param_type = MAV_PARAM_TYPE_REAL32; // 默认浮点数
        if (key.startsWith("FLTMODE")) {
          param_type = MAV_PARAM_TYPE_INT32; // 飞行模式参数是整数
        }
        
        // 发送参数设置
        mavlink_message_t msg;
        mavlink_msg_param_set_pack(SYS_ID, COMP_ID, &msg, TARGET_SYS, TARGET_COMP, key.c_str(), fval, param_type);
        uint8_t buf[MAVLINK_MAX_PACKET_LEN];
        uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);
        mavSerial.write(buf, len);
        pendingSetMap[key] = fval; // 记录待确认
        result += key + "=" + val + " SENT; ";
        Serial.println("Sent param: " + key + "=" + val + " (type: " + String(param_type) + ")");
      } else if (token.length() > 0) {
        // 查询参数
        if (paramMap.count(token)) {
          result += token + "=" + String(paramMap[token]) + "; ";
        } else {
          result += token + " NOT FOUND; ";
        }
      }
      start = comma + 1;
    }
    operationMessage = result;
    server.send(200, "text/plain", result);
  });

  // 获取参数设置结果
  server.on("/api/set_param_result", HTTP_GET, []() {
    server.send(200, "text/plain", lastSetResult);
  });

  // 水平校准
  server.on("/calibrate_level", HTTP_GET, []() {
    // 检查飞控模式
    if (flightMode != "MANUAL" && flightMode != "Manual" && flightMode != "0") {
      Serial.println("Autopilot not in MANUAL mode. Current mode: " + flightMode);
      operationMessage = "Autopilot must be in MANUAL mode for calibration. Current mode: " + flightMode;
      server.send(200, "text/plain", "Autopilot must be in MANUAL mode for calibration. Current mode: " + flightMode);
      return;
    }
    
    if (currentLevelState == LEVEL_STATE_IDLE) {
      // 步骤1：开始校准流程，提示用户放置飞机
      Serial.println("Level calibration request received. Step 1: Start.");
      levelCalibrated = false;
      currentCalibration = CALIB_LEVEL;
      currentLevelState = LEVEL_STATE_STARTED;
      operationMessage = "Place vehicle on a level surface, then click 'Calibrate Level' again to confirm.";
      server.send(200, "text/plain", operationMessage);

    } else if (currentLevelState == LEVEL_STATE_STARTED) {
      // 步骤2：用户已确认，发送实际的校准命令
      Serial.println("Level calibration request received. Step 2: Confirm & Send.");
      currentLevelState = LEVEL_STATE_WAITING_RESULT;
      waitingForAck = true;
      lastCommandTime = millis();
      operationMessage = "Sending level calibration command, waiting for result...";

      // 发送水平校准命令 - 按照Mission Planner的参数
      sendMavlinkCommand(MAV_CMD_PREFLIGHT_CALIBRATION, 0, 0, 0, 0, 2, 0, 0);
      
      Serial.println("Level calibration command sent");
      server.send(200, "text/plain", operationMessage);
    }
  });
  
  server.on("/calibrate_accel", HTTP_GET, []() {
    if (currentCalibration != CALIB_NONE && currentCalibration != CALIB_ACCEL) {
      server.send(200, "text/plain", "Another calibration in progress");
      return;
    }
    
    if (currentAccelState == ACCEL_STATE_IDLE) {
      // 开始加速度计校准 - 发送启动命令
      currentCalibration = CALIB_ACCEL;
      currentAccelState = ACCEL_STATE_STARTING;
      currentPosition = 0;
      
      // 发送加速度计校准启动命令
      sendMavlinkCommand(MAV_CMD_PREFLIGHT_CALIBRATION, 0, 0, 0, 0, 1, 0, 0);
      waitingForAck = true;
      lastCommandTime = millis();
      
      operationMessage = "Starting accelerometer calibration...";
      server.send(200, "text/plain", "Starting accelerometer calibration...");
    } else if (currentAccelState == ACCEL_STATE_POSITION_REQUESTED) {
      // 用户确认当前位置，发送确认命令
      sendMavlinkCommand(MAV_CMD_ACCELCAL_VEHICLE_POS, currentPosition, 0, 0, 0, 0, 0, 0);
      currentAccelState = ACCEL_STATE_WAITING_POSITION;
      waitingForAck = true;
      lastCommandTime = millis();
      operationMessage = "Position confirmed, waiting for next position...";
      server.send(200, "text/plain", "Position confirmed, waiting for next position...");
    } else {
      // 其他状态，返回当前状态信息
      server.send(200, "text/plain", operationMessage);
    }
  });
  server.on("/api/arm", HTTP_POST, []() {
    // 发送解锁指令
    mavlink_message_t msg;
    mavlink_msg_command_long_pack(SYS_ID, COMP_ID, &msg, TARGET_SYS, TARGET_COMP, MAV_CMD_COMPONENT_ARM_DISARM, 1, 0, 0, 0, 0, 0, 0, 0);
    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);
    mavSerial.write(buf, len);
    lastArmCommand = true;
    armErrorCount = 0;
    operationMessage = "Arm command sent";
    server.send(200, "application/json", "{\"message\":\"Unlock command sent\"}");
  });
  server.on("/api/disarm", HTTP_POST, []() {
    // 发送上锁指令
    mavlink_message_t msg;
    mavlink_msg_command_long_pack(SYS_ID, COMP_ID, &msg, TARGET_SYS, TARGET_COMP, MAV_CMD_COMPONENT_ARM_DISARM, 0, 0, 0, 0, 0, 0, 0, 0);
    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);
    mavSerial.write(buf, len);
    lastArmCommand = false;
    operationMessage = "Disarm command sent";
    server.send(200, "application/json", "{\"message\":\"Lock command sent\"}");
  });

  // 新增：获取解锁错误信息接口
  server.on("/api/arm_errors", HTTP_GET, []() {
    String json = "[";
    for (int i = 0; i < armErrorCount; i++) {
      if (i > 0) json += ",";
      json += "\"" + armErrors[i] + "\"";
    }
    json += "]";
    
    server.send(200, "application/json", json);
  });

  // 新增：清空解锁错误信息接口
  server.on("/api/clear_arm_errors", HTTP_POST, []() {
    armErrorCount = 0;
    lastArmCommand = false;
    server.send(200, "text/plain", "OK");
  });

  // 新增：校准状态检查接口
  server.on("/api/calibration_check", HTTP_GET, []() {
    String json = "{";
    json += "\"level_calibrated\":" + String(levelCalibrated ? "true" : "false") + ",";
    json += "\"accel_calibrated\":" + String(accelCalibrated ? "true" : "false");
    json += "}";
    
    server.send(200, "application/json", json);
  });

  // 新增：参数加载状态调试接口
  server.on("/api/param_debug", HTTP_GET, []() {
    String json = "{";
    json += "\"param_ready\":" + String(param_ready ? "true" : "false") + ",";
    json += "\"param_total\":" + String(param_total) + ",";
    json += "\"param_map_size\":" + String(paramMap.size()) + ",";
    json += "\"param_index_set_size\":" + String(paramIndexSet.size()) + ",";
    json += "\"last_param_recv\":" + String(last_param_recv) + ",";
    json += "\"current_time\":" + String(millis()) + ",";
    json += "\"time_since_last_param\":" + String(millis() - last_param_recv);
    json += "}";
    
    server.send(200, "application/json", json);
  });

  // Servo Out API
  server.on("/api/servos", HTTP_GET, []() {
    String json = "{\"servos\":[";
    for(int i=0;i<10;i++){
      if(i>0) json += ",";
      json += "{\"position\":"+String(servoOutputs[i].position)+"}";
    }
    json += "]}";
    server.send(200, "application/json", json);
  });

  // 新增：动态生成Servo Output表格接口
  server.on("/servo_output", HTTP_GET, []() {
    String html = "<div style='overflow-x:auto;'>";
    html += "<table id='servo-table' style='width:100%; border-collapse:collapse; font-size:12px;'>";
    html += "<thead>";
    html += "<tr style='background:#f8f9fa;'>";
    html += "<th style='border:1px solid #ddd; padding:6px;'>Ch</th>";
    html += "<th style='border:1px solid #ddd; padding:6px;'>Pos</th>";
    html += "<th style='border:1px solid #ddd; padding:6px;'>Rev</th>";
    html += "<th style='border:1px solid #ddd; padding:6px;'>Function</th>";
    html += "<th style='border:1px solid #ddd; padding:6px;'>Min</th>";
    html += "<th style='border:1px solid #ddd; padding:6px;'>Trim</th>";
    html += "<th style='border:1px solid #ddd; padding:6px;'>Max</th>";
    html += "</tr>";
    html += "</thead>";
    html += "<tbody id='servo-tbody'>";
    
    // 生成13个通道的表格行
    for(int i = 1; i <= 13; i++) {
      int idx = i - 1;  // 数组索引从0开始
      String servoNum = String(i);
      
      // 获取当前参数值
      String functionParam = "SERVO" + servoNum + "_FUNCTION";
      String minParam = "SERVO" + servoNum + "_MIN";
      String maxParam = "SERVO" + servoNum + "_MAX";
      String trimParam = "SERVO" + servoNum + "_TRIM";
      String reversedParam = "SERVO" + servoNum + "_REVERSED";
      
      float functionVal = paramMap.count(functionParam) ? paramMap[functionParam] : 0;
      float minVal = paramMap.count(minParam) ? paramMap[minParam] : 1100;
      float maxVal = paramMap.count(maxParam) ? paramMap[maxParam] : 1900;
      float trimVal = paramMap.count(trimParam) ? paramMap[trimParam] : 1500;
      float reversedVal = paramMap.count(reversedParam) ? paramMap[reversedParam] : 0;
      
      // 获取当前输出位置
      uint16_t currentPos = servoOutputs[idx].valid ? servoOutputs[idx].position : 1500;
      
      html += "<tr>";
      html += "<td style='border:1px solid #ddd; padding:6px; font-weight:bold;'>" + servoNum + "</td>";
      html += "<td style='border:1px solid #ddd; padding:6px;'><span id='servo" + servoNum + "_pos'>" + String(currentPos) + "</span></td>";
      html += "<td style='border:1px solid #ddd; padding:6px;'>";
      html += "<input type='checkbox' id='servo" + servoNum + "_rev' onchange='updateServoParam(" + servoNum + ", \"REVERSED\", this.checked ? 1 : 0)'";
      if (reversedVal > 0) html += " checked";
      html += ">";
      html += "</td>";
      html += "<td style='border:1px solid #ddd; padding:6px;'>";
      long funcRounded = (long)round((double)functionVal);
      const int funcVals[] = {0, 1, 2, 4, 19, 21, 70, 73, 74, 77, 78, 79, 80, 110, -1};
      const char* const funcLabs[] = {
        "Disabled", "RCPassThru", "Flap", "Aileron", "Elevator", "Rudder", "Throttle",
        "Throttle Left", "Throttle Right", "Elevon Left", "Elevon Right", "V-Tail Left", "V-Tail Right",
        "AirBrakes", "GPIO"
      };
      const int funcCount = (int)(sizeof(funcVals) / sizeof(funcVals[0]));
      bool funcKnown = false;
      for (int k = 0; k < funcCount; ++k) {
        if (funcRounded == (long)funcVals[k]) {
          funcKnown = true;
          break;
        }
      }
      html += "<select id='servo" + servoNum + "_func' onchange='updateServoParam(" + servoNum + ", \"FUNCTION\", this.value)' style='width:100%; font-size:10px;'>";
      if (!funcKnown) {
        html += "<option value='" + String(funcRounded) + "' selected>" + String(funcRounded) + "</option>";
      }
      for (int k = 0; k < funcCount; ++k) {
        html += "<option value='" + String(funcVals[k]) + "'" + (funcRounded == (long)funcVals[k] ? " selected" : "") + ">" + String(funcLabs[k]) + "</option>";
      }
      html += "</select>";
      html += "</td>";
      html += "<td style='border:1px solid #ddd; padding:6px;'>";
      html += "<input type='number' id='servo" + servoNum + "_min' value='" + String((int)minVal) + "' min='1000' max='1200' style='width:50px; font-size:10px;' onchange='updateServoParam(" + servoNum + ", \"MIN\", this.value)'>";
      html += "</td>";
      html += "<td style='border:1px solid #ddd; padding:6px;'>";
      html += "<input type='number' id='servo" + servoNum + "_trim' value='" + String((int)trimVal) + "' min='1400' max='1600' style='width:50px; font-size:10px;' onchange='updateServoParam(" + servoNum + ", \"TRIM\", this.value)'>";
      html += "</td>";
      html += "<td style='border:1px solid #ddd; padding:6px;'>";
      html += "<input type='number' id='servo" + servoNum + "_max' value='" + String((int)maxVal) + "' min='1800' max='2000' style='width:50px; font-size:10px;' onchange='updateServoParam(" + servoNum + ", \"MAX\", this.value)'>";
      html += "</td>";
      html += "</tr>";
    }
    html += "</tbody>";
    html += "</table>";
    html += "</div>";
    html += "<div style='margin-top:10px; font-size:11px; color:#6c757d;'>";
    html += "<strong>Note:</strong> Position values show real servo output data from autopilot.";
    html += "</div>";
    
    server.send(200, "text/html", html);
  });

  // 新增：提供舵机位置数据的接口
  server.on("/servo_positions", HTTP_GET, []() {
    String json = "{";
    for (int i = 0; i < 13; i++) {
      if (i > 0) json += ",";
      json += "\"servo" + String(i + 1) + "\":" + String(servoOutputs[i].valid ? servoOutputs[i].position : 1500);
    }
    json += "}";
    
    server.send(200, "application/json", json);
  });

  // ARM/DISARM
  server.on("/arm_disarm", HTTP_GET, []() {
    String action = server.hasArg("action") ? server.arg("action") : "";
    bool forceArm = server.hasArg("force") && server.arg("force") == "1";
    bool shouldArm = false;

    Serial.print("[ArmDisarmAPI] action=");
    Serial.print(action);
    Serial.print(", forceArm=");
    Serial.println(forceArm ? "true" : "false");

    if (forceArm) {
      Serial.println("[ForceArm] sendMavlinkCommand(MAV_CMD_COMPONENT_ARM_DISARM, 1, 2989, 0, 0, 0, 0, 0)");
      sendMavlinkCommand(MAV_CMD_COMPONENT_ARM_DISARM, 1.0f, 2989.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
      operationMessage = "Sending FORCE ARM command...";
      shouldArm = true;
    } else if (action == "1" || (!isArmed && action == "")) {
      Serial.println("[NormalArm] sendMavlinkCommand(MAV_CMD_COMPONENT_ARM_DISARM, 1, 0, 0, 0, 0, 0, 0)");
      sendMavlinkCommand(MAV_CMD_COMPONENT_ARM_DISARM, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
      operationMessage = "Sending ARM command...";
      shouldArm = true;
    } else {
      Serial.println("[Disarm] sendMavlinkCommand(MAV_CMD_COMPONENT_ARM_DISARM, 0, 0, 0, 0, 0, 0, 0)");
      sendMavlinkCommand(MAV_CMD_COMPONENT_ARM_DISARM, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
      operationMessage = "Sending DISARM command...";
      shouldArm = false;
    }

    String json = "{";
    json += "\"message\":\"" + operationMessage + "\",";
    json += "\"action\":" + String(shouldArm ? "1" : "0") + ",";
    json += "\"armDenied\":" + String(armDenied ? "true" : "false");
    json += "}";
    server.send(200, "application/json", json);
  });

  // 校准状态检查
  server.on("/calibration_check", HTTP_GET, []() {
    String json = "{";
    json += "\"level_calibrated\":" + String(levelCalibrated ? "true" : "false") + ",";
    json += "\"accel_calibrated\":" + String(accelCalibrated ? "true" : "false");
    json += "}";
    server.send(200, "application/json", json);
  });

  // ARM错误信息
  server.on("/arm_errors", HTTP_GET, []() {
    String json = "[";
    for (int i = 0; i < armErrorCount; i++) {
      if (i > 0) json += ",";
      json += "\"" + armErrors[i] + "\"";
    }
    json += "]";
    server.send(200, "application/json", json);
  });

  // 清除ARM错误
  server.on("/clear_arm_errors", HTTP_GET, []() {
    armErrorCount = 0;
    server.send(200, "text/plain", "ARM errors cleared");
  });

  server.begin();
  Serial.println("Web server started");
}

#endif 