#ifndef WEB_ASSETS_H
#define WEB_ASSETS_H
#include <Arduino.h>

const char WEB_STYLE_CSS[] PROGMEM = R"DELIM_CSS(
body{font-family:Arial,sans-serif;margin:0;padding:20px;background:#f5f5f5;}.container{max-width:1200px;margin:0 auto;background:white;padding:20px;border-radius:10px;box-shadow:0 2px 10px rgba(0,0,0,0.1);}.header{text-align:center;margin-bottom:30px;color:#333;}.status-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(200px,1fr));gap:15px;margin-bottom:30px;}.status-item{background:#f8f9fa;padding:15px;border-radius:8px;border-left:4px solid #007bff;}.status-item h3{margin:0 0 10px 0;color:#495057;}.status-item p{margin:0;font-size:18px;font-weight:bold;color:#28a745;}.status-item.error p{color:#dc3545;}.status-item.warning p{color:#ffc107;}.tabs{display:flex;border-bottom:2px solid #dee2e6;margin-bottom:20px;}.tab{padding:10px 20px;cursor:pointer;border:none;background:none;color:#6c757d;}.tab.active{color:#007bff;border-bottom:2px solid #007bff;}.tab-content{display:none;}.tab-content.active{display:block;}.control-section{margin-bottom:30px;}.control-section h3{color:#495057;margin-bottom:15px;}.button-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(150px,1fr));gap:10px;}button{padding:8px 6px;border:none;border-radius:5px;cursor:pointer;font-size:14px;transition:all 0.3s;}button:hover{opacity:0.8;}button:disabled{opacity:0.5;cursor:not-allowed;background-color:#6c757d !important;}.btn-primary{background:#007bff;color:white;}.btn-success{background:#28a745;color:white;}.btn-warning{background:#ffc107;color:#212529;}.btn-danger{background:#dc3545;color:white;}.btn-info{background:#17a2b8;color:white;}.az-bar{display:flex;flex-wrap:wrap;gap:5px;margin-bottom:15px;}.az-bar button{padding:2px;}.param-section{margin-top:20px;}.param-input{display:flex;gap:10px;margin-bottom:15px;}.param-input input{flex:1;padding:8px;border:1px solid #ddd;border-radius:4px;}.param-list{background:#f8f9fa;padding:15px;border-radius:5px;max-height:300px;overflow-y:auto;}.servo-table{width:100%;border-collapse:collapse;margin-top:15px;}.servo-table th,.servo-table td{border:1px solid #ddd;padding:8px;text-align:center;}.servo-table th{background:#f8f9fa;}.servo-controls{display:grid;grid-template-columns:repeat(auto-fit,minmax(200px,1fr));gap:15px;}.servo-item{background:#f8f9fa;padding:15px;border-radius:8px;}.servo-item h4{margin:0 0 10px 0;color:#495057;}.servo-control{display:flex;align-items:center;gap:10px;margin-bottom:8px;}.servo-control input{width:80px;padding:4px;border:1px solid #ddd;border-radius:3px;}.servo-control label{min-width:60px;font-size:12px;}.servo-control button{padding:4px 8px;font-size:12px;}.mode-section{margin-bottom:20px;}.mode-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(200px,1fr));gap:10px;}.mode-item{display:flex;align-items:center;gap:10px;}.mode-item select{padding:5px;border:1px solid #ddd;border-radius:3px;}.message-box{background:#e9ecef;padding:15px;border-radius:5px;margin-top:20px;min-height:50px;}.message-box h4{margin:0 0 10px 0;color:#495057;}.message-box p{margin:0;color:#6c757d;}        /* 消息样式 */.msg{font-weight:bold;margin-bottom:8px;padding:5px;border-radius:4px;text-align:center;word-wrap:break-word;overflow-wrap:break-word;white-space:normal;min-height:1.2em;}.msg-system{font-size:18px;color:#dc3545;max-width:100%;overflow:hidden;text-overflow:ellipsis;display:-webkit-box;-webkit-line-clamp:3;-webkit-box-orient:vertical;}.msg-operation{font-size:16px;color:#0066CC;background:#E6F3FF;max-width:100%;overflow:hidden;text-overflow:ellipsis;display:-webkit-box;-webkit-line-clamp:2;-webkit-box-orient:vertical;}        /* 移动端适配 */@media (max-width: 768px) {.container{padding:10px;}.status-grid{grid-template-columns:1fr;}.button-grid{grid-template-columns:1fr;}.row.space-between{flex-direction:column;gap:10px;}.row.space-between .col{flex:none;}.az-bar{justify-content:center;}.az-bar button{padding:2px 2px;font-size:12px;}}        /* 确保电压、模式、GPS在一排显示 */@media (max-width: 480px) {div[style*='display:flex'][style*='justify-content:space-between']{flex-direction:row !important;flex-wrap:nowrap !important;}div[style*='display:flex'][style*='justify-content:space-between'] > div{flex:1 !important;min-width:0 !important;font-size:12px !important;}}        /* 禁用按钮的特殊样式 */.btn-disabled {background-color: #6c757d !important;color: #fff !important;cursor: not-allowed !important;opacity: 0.6 !important;pointer-events: none !important;}        /* 校准完成按钮样式 */.btn-calibrated {background-color: #28a745 !important;color: white !important;cursor: not-allowed !important;opacity: 0.8 !important;pointer-events: none !important;}
)DELIM_CSS";

const char WEB_APP_JS[] PROGMEM = R"DELIM_JS(
function fillFlightModeSelects(){
  fetch('/api/plane_modes').then(function(r){return r.json();}).then(function(d){
    for(var i=1;i<=6;i++){
      var sel=document.getElementById('fltmode'+i);
      if(!sel) continue;
      sel.innerHTML='';
      var cur=d.cur[i-1];
      for(var k=0;k<d.modes.length;k++){
        var x=d.modes[k];
        var o=document.createElement('option');
        o.value=String(x.n);
        o.textContent=x.m;
        if(Number(x.n)===Number(cur)) o.selected=true;
        sel.appendChild(o);
      }
    }
  }).catch(function(e){ console.error(e); });
}
var levelCalibrated = false;
var accelCalibrated = false;

// 记录最近一条PreArm消息
let lastPreArmMsg = "";

function updateStatus(){
  fetch('/api/status').then(r=>r.json()).then(data=>{
    document.getElementById('sysmsg').textContent=data.systemMessage;
    document.getElementById('voltage').textContent=data.voltage+'V';
    document.getElementById('flightMode').textContent=data.flightMode;
    document.getElementById('gpsCount').textContent=data.gpsCount;
    document.getElementById('opmsg').textContent=data.operationMessage;
    var chEl=document.getElementById('fltmodeChDisp');
    var pwmEl=document.getElementById('fltmodePwmDisp');
    if(chEl){
      if(data.fltmodeCh>0){chEl.textContent='CH'+data.fltmodeCh;}
      else{chEl.textContent='--';}
    }
    if(pwmEl){
      if(data.fltmodePwmValid){pwmEl.textContent=data.fltmodePwm+' \u03bcs';}
      else{pwmEl.textContent='--';}
    }
    document.getElementById('armBtn').textContent=data.isArmed?'Disarm':'Un/lock';
    
    // 记录最近一条PreArm消息
    if (data.systemMessage && data.systemMessage.startsWith('PreArm:')) {
      lastPreArmMsg = data.systemMessage;
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
      var params="";
      for(var key in data.parameters){
        params+=key+'='+data.parameters[key]+'<br>';
      }
      document.getElementById('paramList').innerHTML=params;
      
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
    fillFlightModeSelects();
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
)DELIM_JS";

#endif
