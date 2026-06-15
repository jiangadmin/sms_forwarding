#include "config_types.h"

const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>SMS Forwarding</title>
  <link href="https://fonts.googleapis.com/css2?family=Outfit:wght@300;400;500;600;700&display=swap" rel="stylesheet">
  <style>
    :root {
      --ink: #f3f4f6;
      --body: #d1d5db;
      --mute: #9ca3af;
      --canvas: rgba(255, 255, 255, 0.03);
      --canvas-soft: #090d16;
      --canvas-soft-2: rgba(255, 255, 255, 0.06);
      --hairline: rgba(255, 255, 255, 0.08);
      --hairline-strong: rgba(255, 255, 255, 0.15);
      --link: #8b5cf6;
      --error: #ef4444;
      --warning-soft: rgba(245, 158, 11, 0.1);
      --sidebar-w: 220px;
      --radius-sm: 8px;
      --radius-md: 16px;
      --radius-pill: 100px;
      --shadow-card: 0 8px 32px 0 rgba(0, 0, 0, 0.37);
    }
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body {
      font-family: 'Outfit', -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
      font-size: 14px; font-weight: 400; line-height: 1.5;
      color: var(--ink); background: var(--canvas-soft);
      display: flex; min-height: 100vh;
      overflow-x: hidden; position: relative;
    }
    body::before, body::after {
      content: '';
      position: absolute;
      width: 400px;
      height: 400px;
      border-radius: 50%;
      background: linear-gradient(135deg, #6366f1 0%, #a855f7 100%);
      filter: blur(150px);
      opacity: 0.08;
      z-index: 0;
      pointer-events: none;
    }
    body::before { top: -100px; right: -100px; }
    body::after { bottom: -100px; left: calc(var(--sidebar-w) - 100px); }

    /* Sidebar */
    .sidebar {
      position: fixed; top: 0; left: 0; bottom: 0; width: var(--sidebar-w);
      background: rgba(10, 15, 28, 0.6);
      backdrop-filter: blur(20px);
      -webkit-backdrop-filter: blur(20px);
      border-right: 1px solid var(--hairline);
      display: flex; flex-direction: column;
      z-index: 100; overflow-y: auto;
    }
    .sidebar-brand { padding: 22px 18px 16px; border-bottom: 1px solid var(--hairline); }
    .sidebar-brand h2 { font-size: 17px; font-weight: 700; color: #fff; letter-spacing: -0.4px; }
    .sidebar-brand span { font-size: 10px; color: rgba(255,255,255,0.4); display: block; margin-top: 2px; font-family: 'SF Mono',monospace; }
    .sidebar-nav { flex: 1; padding: 10px; }
    .sidebar-nav a {
      display: flex; align-items: center; gap: 10px; padding: 10px 14px;
      border-radius: var(--radius-sm); color: rgba(255,255,255,0.6);
      font-size: 13px; font-weight: 500; text-decoration: none;
      transition: all 0.2s ease; margin-bottom: 2px; cursor: pointer;
      border: 1px solid transparent;
    }
    .sidebar-nav a:hover { background: rgba(255,255,255,0.05); color: rgba(255,255,255,0.9); }
    .sidebar-nav a.active {
      background: linear-gradient(135deg, rgba(99, 102, 241, 0.15) 0%, rgba(168, 85, 247, 0.15) 100%);
      border: 1px solid rgba(139, 92, 246, 0.3);
      color: #fff;
    }
    .sidebar-nav a .ico { font-size: 15px; width: 20px; text-align: center; flex-shrink: 0; }
    .sidebar-divider { height: 1px; background: var(--hairline); margin: 8px 12px; }
    .sidebar-section-label { font-size: 10px; color: rgba(255,255,255,0.3); padding: 6px 14px 4px; text-transform: uppercase; letter-spacing: 0.6px; font-family: 'SF Mono',monospace; }
    .sidebar-footer { padding: 12px 16px; border-top: 1px solid var(--hairline); }
    .sidebar-footer .btn { width: 100%; }

    /* Main */
    .main {
      margin-left: var(--sidebar-w); flex: 1; padding: 32px;
      max-width: 840px; width: 100%; position: relative; z-index: 1;
    }
    .page-title {
      font-size: 26px; font-weight: 700;
      background: linear-gradient(135deg, #6366f1 0%, #a855f7 100%);
      -webkit-background-clip: text; -webkit-text-fill-color: transparent;
      letter-spacing: -0.5px; margin-bottom: 6px;
    }
    .page-subtitle { font-size: 13px; color: var(--mute); margin-bottom: 24px; }

    /* Card */
    .card {
      background: var(--canvas);
      border: 1px solid var(--hairline);
      backdrop-filter: blur(20px);
      -webkit-backdrop-filter: blur(20px);
      border-radius: var(--radius-md);
      box-shadow: var(--shadow-card);
      margin-bottom: 18px;
    }
    .card-header { padding: 18px 22px 0; font-size: 15px; font-weight: 600; color: var(--ink); letter-spacing: -0.2px; display: flex; align-items: center; gap: 8px; }
    .card-body { padding: 18px 22px 22px; }
    .card-header + .card-body { padding-top: 12px; }

    /* Panel hide/show */
    .panel { display: none; }
    .panel.active { display: block; animation: fadeIn 0.4s cubic-bezier(0.16, 1, 0.3, 1); }
    @keyframes fadeIn {
      from { opacity: 0; transform: translateY(10px); }
      to { opacity: 1; transform: translateY(0); }
    }

    /* Form */
    .form-group { margin-bottom: 16px; }
    .form-group:last-child { margin-bottom: 0; }
    .form-label { display: block; font-size: 12px; font-weight: 500; color: var(--body); margin-bottom: 6px; letter-spacing: 0.2px; }
    .form-input, .form-select, .form-textarea {
      width: 100%; padding: 9px 12px; font-size: 13px; font-family: inherit;
      border: 1px solid var(--hairline); border-radius: var(--radius-sm);
      background: rgba(255, 255, 255, 0.05); color: var(--ink);
      transition: all 0.2s ease; outline: none;
    }
    .form-input:focus, .form-select:focus, .form-textarea:focus {
      border-color: #8b5cf6;
      box-shadow: 0 0 0 3px rgba(139, 92, 246, 0.25);
    }
    .form-select { cursor: pointer; }
    .form-select option { background-color: #090d16; color: #f3f4f6; }
    .form-textarea { resize: vertical; min-height: 80px; line-height: 1.5; }
    .form-hint { font-size: 11px; color: var(--mute); margin-top: 4px; line-height: 1.4; }
    .form-warning {
      font-size: 12px; color: #fbbf24;
      background: rgba(245, 158, 11, 0.1);
      border: 1px solid rgba(245, 158, 11, 0.2);
      padding: 10px 14px; border-radius: var(--radius-sm);
      margin-bottom: 14px; line-height: 1.5;
    }
    .form-row { display: flex; gap: 14px; }
    .form-row .form-group { flex: 1; }

    /* Buttons */
    .btn {
      display: inline-flex; align-items: center; justify-content: center; gap: 6px;
      padding: 8px 16px; font-size: 13px; font-weight: 600; font-family: inherit;
      border-radius: var(--radius-pill); border: none; cursor: pointer;
      transition: all 0.2s ease; line-height: 1.4; white-space: nowrap;
    }
    .btn:disabled { opacity: 0.5; cursor: not-allowed; }
    .btn-primary {
      background: linear-gradient(135deg, #6366f1 0%, #a855f7 100%);
      color: #fff;
      box-shadow: 0 4px 12px rgba(99, 102, 241, 0.2);
    }
    .btn-primary:hover {
      transform: translateY(-1px);
      box-shadow: 0 6px 16px rgba(99, 102, 241, 0.3);
      filter: brightness(1.1);
    }
    .btn-primary:active { transform: translateY(0); }
    .btn-secondary {
      background: rgba(255, 255, 255, 0.05);
      color: var(--ink);
      border: 1px solid var(--hairline);
    }
    .btn-secondary:hover {
      background: rgba(255, 255, 255, 0.1);
      transform: translateY(-1px);
    }
    .btn-secondary:active { transform: translateY(0); }
    .btn-danger {
      background: linear-gradient(135deg, #ef4444 0%, #b91c1c 100%);
      color: #fff;
      box-shadow: 0 4px 12px rgba(239, 68, 68, 0.2);
    }
    .btn-danger:hover {
      transform: translateY(-1px);
      box-shadow: 0 6px 16px rgba(239, 68, 68, 0.3);
      filter: brightness(1.1);
    }
    .btn-danger:active { transform: translateY(0); }
    .btn-sm { padding: 5px 10px; font-size: 12px; border-radius: var(--radius-sm); }
    .btn-white { background: #fff; color: #090d16; }
    .btn-white:hover { background: #e5e7eb; transform: translateY(-1px); }
    .btn-white:active { transform: translateY(0); }
    .btn-block { width: 100%; justify-content: center; }
    .btn-save { padding: 10px 20px; font-size: 14px; margin-top: 4px; }

    /* Push Channel */
    .push-channel {
      border: 1px solid var(--hairline); border-radius: var(--radius-sm);
      padding: 14px; margin-bottom: 10px; background: rgba(255, 255, 255, 0.01);
      transition: all 0.2s ease;
    }
    .push-channel:hover { border-color: rgba(139, 92, 246, 0.3); }
    .push-channel-header { display: flex; align-items: center; gap: 8px; margin-bottom: 10px; }
    .push-channel-header label { font-size: 13px; font-weight: 600; color: var(--ink); cursor: pointer; }
    .push-channel-header input[type="checkbox"] { width: 15px; height: 15px; accent-color: #8b5cf6; }
    .push-channel-body { display: none; }
    .push-channel.enabled .push-channel-body { display: block; }
    .push-channel.enabled { border-color: rgba(139, 92, 246, 0.3); background: rgba(139, 92, 246, 0.03); }
    .push-channel-body .form-group { margin-bottom: 12px; }
    .push-channel-body .form-group:last-child { margin-bottom: 0; }
    .push-channel-body label { display: block; font-size: 12px; font-weight: 500; color: var(--body); margin-bottom: 6px; letter-spacing: 0.1px; }
    .push-channel-body input[type="text"], .push-channel-body input[type="password"], .push-channel-body select, .push-channel-body textarea {
      width: 100%; padding: 8px 12px; font-size: 13px; font-family: inherit;
      border: 1px solid var(--hairline); border-radius: var(--radius-sm);
      background: rgba(255, 255, 255, 0.05); color: var(--ink);
      transition: all 0.2s ease; outline: none;
    }
    .push-channel-body input:focus, .push-channel-body select:focus, .push-channel-body textarea:focus {
      border-color: #8b5cf6;
      box-shadow: 0 0 0 3px rgba(139, 92, 246, 0.25);
    }
    .push-channel-body select { cursor: pointer; }
    .push-channel-body textarea { resize: vertical; min-height: 60px; line-height: 1.5; }
    .push-type-hint { font-size: 11px; color: var(--body); margin-top: 6px; padding: 8px 12px; background: rgba(255, 255, 255, 0.02); border-radius: var(--radius-sm); font-family: 'SF Mono',monospace; line-height: 1.5; border: 1px solid var(--hairline); }

    /* Result Boxes */
    .result-box {
      margin-top: 12px; padding: 10px 14px; border-radius: var(--radius-sm);
      display: none; font-size: 12px; line-height: 1.5;
    }
    .result-success {
      background: rgba(16, 185, 129, 0.1); border: 1px solid rgba(16, 185, 129, 0.2);
      color: #34d399; display: block;
    }
    .result-error {
      background: rgba(239, 68, 68, 0.1); border: 1px solid rgba(239, 68, 68, 0.2);
      color: #f87171; display: block;
    }
    .result-loading {
      background: rgba(245, 158, 11, 0.1); border: 1px solid rgba(245, 158, 11, 0.2);
      color: #fbbf24; display: block;
    }
    .result-info {
      background: rgba(59, 130, 246, 0.1); border: 1px solid rgba(59, 130, 246, 0.2);
      color: #60a5fa; display: block;
    }
    .info-table { width: 100%; border-collapse: collapse; margin-top: 4px; font-size: 12px; }
    .info-table td { padding: 8px 12px; border-bottom: 1px solid var(--hairline); }
    .info-table td:first-child { font-weight: 500; color: var(--mute); width: 40%; }

    /* Overview */
    .overview-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 14px; }
    .overview-item {
      background: rgba(255, 255, 255, 0.01);
      border: 1px solid var(--hairline);
      border-radius: var(--radius-sm);
      padding: 14px;
    }
    .overview-item .label { font-size: 10px; color: var(--mute); text-transform: uppercase; letter-spacing: 0.4px; font-family: 'SF Mono',monospace; margin-bottom: 4px; }
    .overview-item .value { font-size: 14px; font-weight: 600; color: var(--ink); }

    /* Tools */
    .btn-row { display: flex; gap: 8px; flex-wrap: wrap; }
    .btn-row .btn { flex: 1; min-width: 90px; }
    .btn-row + .btn-row { margin-top: 8px; }
    #atLog {
      background: #05080f; color: #50e3c2; font-family: 'SF Mono',monospace;
      min-height: 130px; max-height: 260px; overflow-y: auto; padding: 12px 14px;
      border-radius: var(--radius-sm); margin-bottom: 10px; font-size: 12px;
      white-space: pre-wrap; word-break: break-all; line-height: 1.5;
      border: 1px solid var(--hairline);
    }
    .at-bar { display: flex; gap: 6px; }
    .at-bar input { flex: 1; font-family: 'SF Mono',monospace; }
    .at-bar .btn { min-width: 60px; }

    /* Responsive */
    @media (max-width: 700px) {
      .sidebar { width: 50px; }
      .sidebar-brand h2 { font-size: 0; }
      .sidebar-brand h2::first-letter { font-size: 16px; }
      .sidebar-brand span, .sidebar-section-label { display: none; }
      .sidebar-nav a { padding: 10px; justify-content: center; }
      .sidebar-nav a span:not(.ico) { display: none; }
      .sidebar-nav a .ico { font-size: 16px; }
      .sidebar-divider { margin: 6px 8px; }
      .sidebar-footer { padding: 8px; }
      .sidebar-footer .btn span { display: none; }
      .sidebar-footer .btn { padding: 6px; font-size: 11px; }
      .main { margin-left: 50px; padding: 18px 14px; }
      :root { --sidebar-w: 50px; }
      .overview-grid { grid-template-columns: 1fr; }
    }
  </style>
</head>
<body>
  <aside class="sidebar">
    <div class="sidebar-brand">
      <h2>SMS FWD</h2>
      <span>短信转发器</span>
    </div>
    <nav class="sidebar-nav">
      <div class="sidebar-section-label">配置</div>
      <a data-panel="overview" class="active"><span class="ico">🏠</span> <span>系统概览</span></a>
      <a data-panel="wifi"><span class="ico">📶</span> <span>WiFi 设置</span></a>
      <a data-panel="account"><span class="ico">🔐</span> <span>账号管理</span></a>
      <a data-panel="email"><span class="ico">📧</span> <span>邮件通知</span></a>
      <a data-panel="push"><span class="ico">🔗</span> <span>推送通道</span></a>
      <a data-panel="admin"><span class="ico">👤</span> <span>管理员 &amp; 黑名单</span></a>
      <div class="sidebar-divider"></div>
      <div class="sidebar-section-label">工具</div>
      <a data-panel="sendsms"><span class="ico">📤</span> <span>发送短信</span></a>
      <a data-panel="diagnose"><span class="ico">📊</span> <span>模组诊断</span></a>
      <a data-panel="network"><span class="ico">🌐</span> <span>网络测试</span></a>
      <a data-panel="modem"><span class="ico">✈</span> <span>模组控制</span></a>
      <a data-panel="atterm"><span class="ico">💻</span> <span>AT 终端</span></a>
      <a data-panel="log"><span class="ico">📋</span> <span>系统日志</span></a>
    </nav>
    <div class="sidebar-footer">
      <button class="btn btn-white btn-sm btn-block" onclick="switchPanel('account')"><span>修改密码</span> 🔑</button>
    </div>
  </aside>

  <main class="main">

    <!-- ===== Overview ===== -->
    <div class="panel active" id="panel-overview">
      <h1 class="page-title">系统概览</h1>
      <p class="page-subtitle">设备状态与基本信息</p>
      <div class="card">
        <div class="card-header">📡 设备信息</div>
        <div class="card-body">
          <div class="overview-grid">
            <div class="overview-item"><div class="label">Device IP</div><div class="value" id="ovIp">%IP%</div></div>
            <div class="overview-item"><div class="label">WiFi SSID</div><div class="value" id="ovSsid">%WIFI_SSID%</div></div>
            <div class="overview-item"><div class="label">Free Heap</div><div class="value" id="ovHeap">%FREE_HEAP%</div></div>
            <div class="overview-item"><div class="label">Uptime</div><div class="value" id="ovUptime">%UPTIME%</div></div>
          </div>
        </div>
      </div>
      <div class="card">
        <div class="card-header">⚙ 配置状态</div>
        <div class="card-body">
          <table class="info-table">
            <tr><td>模组状态</td><td id="cfgModem">%MODEM_CHECK%</td></tr>
            <tr><td>邮件通知</td><td id="cfgEmail">%SMTP_CHECK%</td></tr>
            <tr><td>推送通道</td><td id="cfgPush">%PUSH_COUNT% 个已启用</td></tr>
            <tr><td>管理员号码</td><td>%ADMIN_PHONE%</td></tr>
          </table>
        </div>
      </div>
    </div>

    <!-- ===== Account ===== -->
    <div class="panel" id="panel-account">
      <h1 class="page-title">账号管理</h1>
      <p class="page-subtitle">修改 Web 管理界面的登录凭据</p>
      <form action="/save" method="POST" id="mainForm">
      <div class="card">
        <div class="card-header">🔐 登录凭据</div>
        <div class="card-body">
          <div class="form-warning">首次使用请立即修改默认密码！默认: )rawliteral" DEFAULT_WEB_USER " / " DEFAULT_WEB_PASS R"rawliteral(</div>
          <div class="form-row">
            <div class="form-group"><label class="form-label">管理账号</label><input class="form-input" type="text" name="webUser" value="%WEB_USER%" placeholder="admin"></div>
            <div class="form-group"><label class="form-label">管理密码</label><input class="form-input" type="password" name="webPass" value="%WEB_PASS%" placeholder="设置复杂密码"></div>
          </div>
        </div>
      </div>
      <button type="submit" class="btn btn-primary btn-block btn-save">保存配置</button>
      </form>
    </div>

    <!-- ===== Email ===== -->
    <div class="panel" id="panel-email">
      <h1 class="page-title">邮件通知</h1>
      <p class="page-subtitle">配置 SMTP 服务器以接收短信邮件通知</p>
      <form action="/save" method="POST" id="mainForm2">
      <div class="card">
        <div class="card-header">📧 SMTP 设置</div>
        <div class="card-body">
          <div class="form-row">
            <div class="form-group"><label class="form-label">SMTP 服务器</label><input class="form-input" type="text" name="smtpServer" value="%SMTP_SERVER%" placeholder="smtp.qq.com"></div>
            <div class="form-group"><label class="form-label">SMTP 端口</label><input class="form-input" type="number" name="smtpPort" value="%SMTP_PORT%" placeholder="465"></div>
          </div>
          <div class="form-row">
            <div class="form-group"><label class="form-label">邮箱账号</label><input class="form-input" type="text" name="smtpUser" value="%SMTP_USER%" placeholder="your@qq.com"></div>
            <div class="form-group"><label class="form-label">密码 / 授权码</label><input class="form-input" type="password" name="smtpPass" value="%SMTP_PASS%" placeholder="授权码"></div>
          </div>
          <div class="form-group"><label class="form-label">接收邮件地址</label><input class="form-input" type="text" name="smtpSendTo" value="%SMTP_SEND_TO%" placeholder="receiver@example.com"></div>
        </div>
      </div>
      <button type="submit" class="btn btn-primary btn-block btn-save">保存配置</button>
      </form>
    </div>

    <!-- ===== Push Channels ===== -->
    <div class="panel" id="panel-push">
      <h1 class="page-title">推送通道</h1>
      <p class="page-subtitle">最多 5 个独立推送通道，支持 POST JSON、Bark、钉钉、飞书、PushPlus、Server酱、Gotify、Telegram</p>
      <form action="/save" method="POST" id="mainForm3">
      <div class="card">
        <div class="card-header">🔗 通道配置</div>
        <div class="card-body">
          %PUSH_CHANNELS%
        </div>
      </div>
      <button type="submit" class="btn btn-primary btn-block btn-save">保存配置</button>
      </form>
    </div>

    <!-- ===== Admin & Blacklist ===== -->
    <div class="panel" id="panel-admin">
      <h1 class="page-title">管理员 &amp; 黑名单</h1>
      <p class="page-subtitle">远程控制权限与短信过滤</p>
      <form action="/save" method="POST" id="mainForm4">
      <div class="card">
        <div class="card-header">👤 管理员手机号</div>
        <div class="card-body">
          <div class="form-group">
            <input class="form-input" type="text" name="adminPhone" value="%ADMIN_PHONE%" placeholder="13800138000">
            <p class="form-hint">此号码可通过短信发送远程指令（SMS:号码:内容 发短信、RESET 重启）</p>
          </div>
        </div>
      </div>
      <div class="card">
        <div class="card-header">🚫 号码黑名单</div>
        <div class="card-body">
          <div class="form-group">
            <textarea class="form-textarea" name="numberBlackList" rows="5" placeholder="每行一个号码">%NUMBER_BLACK_LIST%</textarea>
            <p class="form-hint">黑名单号码发来的短信将被自动忽略</p>
          </div>
        </div>
      </div>
      <button type="submit" class="btn btn-primary btn-block btn-save">保存配置</button>
      </form>
    </div>

    <!-- ===== WiFi Settings ===== -->
    <div class="panel" id="panel-wifi">
      <h1 class="page-title">WiFi 设置</h1>
      <p class="page-subtitle">修改设备连接的 WiFi 账号和密码</p>
      <form action="/save" method="POST" id="mainFormWifi">
      <div class="card">
        <div class="card-header">📶 WiFi 配置</div>
        <div class="card-body">
          <div class="form-group">
            <label class="form-label">WiFi SSID (2.4G)</label>
            <input class="form-input" type="text" name="wifiSsid" value="%WIFI_SSID_VAL%" placeholder="SSID" required>
          </div>
          <div class="form-group">
            <label class="form-label">WiFi 密码</label>
            <input class="form-input" type="password" name="wifiPass" value="%WIFI_PASS_VAL%" placeholder="密码">
          </div>
          <p class="form-hint">修改后点击保存，设备将自动写入存储并重启以尝试连接新 WiFi。</p>
        </div>
      </div>
      <button type="submit" class="btn btn-primary btn-block btn-save">保存并应用</button>
      </form>
    </div>

    <!-- ===== Send SMS ===== -->
    <div class="panel" id="panel-sendsms">
      <h1 class="page-title">发送短信</h1>
      <p class="page-subtitle">通过模组直接发送短信</p>
      <div class="card">
        <div class="card-header">📤 新建短信</div>
        <div class="card-body">
          <form action="/sendsms" method="POST" target="_self">
            <div class="form-group"><label class="form-label">目标号码</label><input class="form-input" type="text" name="phone" placeholder="13800138000" required></div>
            <div class="form-group"><label class="form-label">短信内容</label><textarea class="form-textarea" name="content" placeholder="输入短信内容..." required oninput="updateCount(this)"></textarea><p class="form-hint">已输入 <span id="charCount">0</span> 字符</p></div>
            <button type="submit" class="btn btn-primary" style="padding:9px 18px;">发送短信</button>
          </form>
        </div>
      </div>
    </div>

    <!-- ===== Diagnostics ===== -->
    <div class="panel" id="panel-diagnose">
      <h1 class="page-title">模组诊断</h1>
      <p class="page-subtitle">查询模组状态、SIM 卡与网络信息</p>
      <div class="card">
        <div class="card-header">📊 查询</div>
        <div class="card-body">
          <div class="btn-row"><button class="btn btn-secondary" onclick="queryInfo('ati')">固件信息</button><button class="btn btn-secondary" onclick="queryInfo('signal')">信号质量</button></div>
          <div class="btn-row"><button class="btn btn-secondary" onclick="queryInfo('siminfo')">SIM 卡信息</button><button class="btn btn-secondary" onclick="queryInfo('network')">网络状态</button><button class="btn btn-secondary" onclick="queryInfo('wifi')">WiFi 状态</button></div>
          <div class="result-box" id="queryResult"></div>
        </div>
      </div>
    </div>

    <!-- ===== Network Test ===== -->
    <div class="panel" id="panel-network">
      <h1 class="page-title">网络测试</h1>
      <p class="page-subtitle">通过模组数据连接测试网络连通性</p>
      <div class="card">
        <div class="card-header">🌐 Ping</div>
        <div class="card-body">
          <button class="btn btn-secondary" id="pingBtn" onclick="confirmPing()">Ping 8.8.8.8</button>
          <p class="form-hint">通过模组执行 Ping，消耗极少流量</p>
          <div class="result-box" id="pingResult"></div>
        </div>
      </div>
      <div class="card">
        <div class="card-header">📡 WiFi 控制</div>
        <div class="card-body">
          <button class="btn btn-danger" onclick="wifiRestart()">重启 WiFi</button>
          <p class="form-hint">断开当前 WiFi 连接并重新连接</p>
          <div class="result-box" id="wifiResult"></div>
        </div>
      </div>
    </div>

    <!-- ===== Modem Control ===== -->
    <div class="panel" id="panel-modem">
      <h1 class="page-title">模组控制</h1>
      <p class="page-subtitle">模组重启、飞行模式、信号查询等操作</p>
      <div class="card">
        <div class="card-header">🔄 模组重启</div>
        <div class="card-body">
          <div class="btn-row"><button class="btn btn-danger" onclick="modemAction('restart')">软重启 (AT+CFUN)</button><button class="btn btn-danger" onclick="modemAction('hardreset')">硬重启 (EN引脚)</button></div>
          <p class="form-hint">软重启发送 AT+CFUN=1,1 指令（15s 超时）；硬重启通过 EN 引脚断电后重新上电</p>
          <div class="result-box" id="modemRstResult"></div>
        </div>
      </div>
      <div class="card">
        <div class="card-header">📶 信号查询</div>
        <div class="card-body">
          <div class="btn-row"><button class="btn btn-primary" onclick="modemAction('signal')">查询信号强度</button><button class="btn btn-primary" onclick="modemAction('operator')">查询运营商</button><button class="btn btn-primary" onclick="modemAction('imei')">查询 IMEI</button></div>
          <div class="result-box" id="modemQueryResult"></div>
        </div>
      </div>
      <div class="card">
        <div class="card-header">✈ 飞行模式</div>
        <div class="card-body">
          <div class="btn-row"><button class="btn btn-danger" id="flightBtn" onclick="toggleFlightMode()">切换飞行模式</button><button class="btn btn-secondary" onclick="queryFlightMode()">查询状态</button></div>
          <p class="form-hint">飞行模式开启后模组射频关闭，无法收发短信</p>
          <div class="result-box" id="flightResult"></div>
        </div>
      </div>
    </div>

    <!-- ===== AT Terminal ===== -->
    <div class="panel" id="panel-atterm">
      <h1 class="page-title">AT 指令终端</h1>
      <p class="page-subtitle">直接向模组发送 AT 指令并接收响应</p>
      <div class="card">
        <div class="card-header">💻 终端</div>
        <div class="card-body">
          <div id="atLog">就绪 — 输入 AT 指令开始调试</div>
          <div class="at-bar"><input class="form-input" type="text" id="atCmd" placeholder="AT+CSQ"><button class="btn btn-primary btn-sm" onclick="sendAT()" id="atBtn">发送</button></div>
          <div class="btn-row" style="margin-top:8px;"><button class="btn btn-secondary btn-sm" onclick="clearATLog()">清空日志</button></div>
          <p class="form-hint">直接向模组串口发送指令并接收响应，请谨慎操作</p>
        </div>
      </div>
    </div>

    <!-- ===== System Log ===== -->
    <div class="panel" id="panel-log">
      <h1 class="page-title">系统日志</h1>
      <p class="page-subtitle">实时查看设备串口日志输出 <span id="logStatus" style="color:#34d399;">● 自动刷新中</span></p>
      <div class="card">
        <div class="card-header">📋 日志输出</div>
        <div class="card-body">
          <div id="logView" style="background:#05080f;color:#d4d4d4;padding:12px;border-radius:8px;font-family:'SF Mono',monospace;font-size:12px;line-height:1.6;max-height:60vh;overflow-y:auto;white-space:pre-wrap;word-break:break-all;min-height:300px;border:1px solid var(--hairline);">加载中...</div>
          <div class="btn-row" style="margin-top:8px;">
            <button class="btn btn-secondary btn-sm" onclick="clearLogUI()">清空显示</button>
            <button class="btn btn-secondary btn-sm" onclick="refreshLog()">手动刷新</button>
            <label style="margin-left:8px;font-size:13px;cursor:pointer;"><input type="checkbox" id="logAuto" checked onchange="toggleLogAuto()"> 自动刷新</label>
          </div>
          <p class="form-hint">显示设备运行时输出的日志信息，每2秒自动刷新。日志最多保留最近120条。</p>
        </div>
      </div>
    </div>

  </main>

  <script>
    // ---- Panel switching ----
    function switchPanel(name) {
      document.querySelectorAll('.panel').forEach(function(p) { p.classList.remove('active'); });
      document.getElementById('panel-' + name).classList.add('active');
      document.querySelectorAll('.sidebar-nav a').forEach(function(a) { a.classList.remove('active'); });
      document.querySelector('.sidebar-nav a[data-panel="' + name + '"]').classList.add('active');
    }
    document.querySelectorAll('.sidebar-nav a').forEach(function(a) {
      a.addEventListener('click', function() { switchPanel(this.dataset.panel); });
    });

    // ---- Push Channel JS ----
    function toggleChannel(idx) {
      var ch = document.getElementById('channel' + idx);
      var cb = document.getElementById('push' + idx + 'en');
      if (cb.checked) ch.classList.add('enabled'); else ch.classList.remove('enabled');
    }
    function updateTypeHint(idx) {
      var sel = document.getElementById('push' + idx + 'type');
      var hint = document.getElementById('hint' + idx);
      var extra = document.getElementById('extra' + idx);
      var custom = document.getElementById('custom' + idx);
      var type = parseInt(sel.value);
      extra.style.display = 'none'; custom.style.display = 'none';
      document.getElementById('key1label' + idx).innerText = '参数 1';
      document.getElementById('key2label' + idx).innerText = '参数 2';
      document.getElementById('key1' + idx).placeholder = '';
      document.getElementById('key2' + idx).placeholder = '';
      var kg = document.getElementById('key2group' + idx);
      if (kg) kg.style.display = 'none';
      if (type == 1) hint.innerHTML = 'POST JSON<br>{"sender":"+8613800138000","message":"...","timestamp":"2026-01-01 12:00:00"}';
      else if (type == 2) hint.innerHTML = 'Bark (iOS)<br>POST {"title":"发送者","body":"短信内容"}';
      else if (type == 3) hint.innerHTML = 'GET 请求<br>URL?sender=xxx&message=xxx&timestamp=xxx';
      else if (type == 4) { hint.innerHTML = '钉钉机器人<br>填写 Webhook 地址，加签需填 Secret'; extra.style.display='block'; document.getElementById('key1label'+idx).innerText='Secret（加签密钥，可选）'; document.getElementById('key1'+idx).placeholder='SEC...'; }
      else if (type == 5) { hint.innerHTML = 'PushPlus<br>填写 Token，URL 留空使用默认'; extra.style.display='block'; document.getElementById('key1label'+idx).innerText='Token'; document.getElementById('key1'+idx).placeholder='pushplus token'; if(kg)kg.style.display='block'; document.getElementById('key2label'+idx).innerText='发送渠道'; document.getElementById('key2'+idx).placeholder='wechat / extension / app'; }
      else if (type == 6) { hint.innerHTML = 'Server酱<br>填写 SendKey，URL 留空使用默认'; extra.style.display='block'; document.getElementById('key1label'+idx).innerText='SendKey'; document.getElementById('key1'+idx).placeholder='SCT...'; }
      else if (type == 7) { hint.innerHTML = '自定义模板<br>使用 {sender} {message} {timestamp} 占位符'; custom.style.display='block'; }
      else if (type == 8) { hint.innerHTML = '飞书机器人<br>填写 Webhook 地址，签名验证需填 Secret'; extra.style.display='block'; document.getElementById('key1label'+idx).innerText='Secret（签名密钥，可选）'; document.getElementById('key1'+idx).placeholder='飞书签名密钥'; }
      else if (type == 9) { hint.innerHTML = 'Gotify<br>填写服务器地址 + 应用 Token'; extra.style.display='block'; document.getElementById('key1label'+idx).innerText='Token（应用 Token）'; document.getElementById('key1'+idx).placeholder='A...'; }
      else if (type == 10) { hint.innerHTML = 'Telegram Bot<br>Chat ID（参数1）+ Bot Token（参数2）'; extra.style.display='block'; document.getElementById('key1label'+idx).innerText='Chat ID'; document.getElementById('key1'+idx).placeholder='123456789'; if(kg)kg.style.display='block'; document.getElementById('key2label'+idx).innerText='Bot Token'; document.getElementById('key2'+idx).placeholder='12345678:ABC...'; }
    }
    document.addEventListener('DOMContentLoaded', function() {
      for (var i = 0; i < 5; i++) { toggleChannel(i); updateTypeHint(i); }
    });

    // ---- Send SMS ----
    function updateCount(el) { document.getElementById('charCount').textContent = el.value.length; }

    // ---- Query ----
    function queryInfo(type) {
      var r = document.getElementById('queryResult');
      r.className = 'result-box result-loading'; r.textContent = '查询中...';
      fetch('/query?type=' + type).then(function(rr){return rr.json()}).then(function(d){
        if(d.success){r.className='result-box result-info';r.innerHTML=d.message;}
        else{r.className='result-box result-error';r.innerHTML='查询失败: '+d.message;}
      }).catch(function(e){r.className='result-box result-error';r.textContent='请求失败: '+e;});
    }

    // ---- Ping ----
    function confirmPing(){if(confirm('确定要执行 Ping 吗？将消耗少量流量。'))doPing();}
    function doPing(){
      var b=document.getElementById('pingBtn'),r=document.getElementById('pingResult');
      b.disabled=true;b.textContent='Pinging...';
      r.className='result-box result-loading';r.textContent='正在 Ping 8.8.8.8（最长 30 秒）...';
      fetch('/ping',{method:'POST'}).then(function(rr){return rr.json()}).then(function(d){
        b.disabled=false;b.textContent='Ping 8.8.8.8';
        if(d.success){r.className='result-box result-success';r.innerHTML='Ping 成功 — '+d.message;}
        else{r.className='result-box result-error';r.innerHTML='Ping 失败 — '+d.message;}
      }).catch(function(e){b.disabled=false;b.textContent='Ping 8.8.8.8';r.className='result-box result-error';r.textContent='请求失败: '+e;});
    }

    // ---- WiFi Control ----
    function wifiRestart(){
      if(!confirm('确定要重启WiFi吗？网页将暂时不可用。'))return;
      var r=document.getElementById('wifiResult');
      r.className='result-box result-loading';r.textContent='WiFi 重启中（约5秒）...';
      fetch('/wifi?action=restart').then(function(rr){return rr.json()}).then(function(d){
        r.className=d.success?'result-box result-success':'result-box result-error';
        r.textContent=d.message;
      }).catch(function(e){r.className='result-box result-error';r.textContent='请求失败: '+e;});
    }

    // ---- Flight Mode ----
    function queryFlightMode(){
      var r=document.getElementById('flightResult');
      r.className='result-box result-loading';r.textContent='查询中...';
      fetch('/flight?action=query').then(function(rr){return rr.json()}).then(function(d){
        if(d.success){r.className='result-box result-info';r.innerHTML=d.message;}
        else{r.className='result-box result-error';r.innerHTML='查询失败: '+d.message;}
      }).catch(function(e){r.className='result-box result-error';r.textContent='请求失败: '+e;});
    }
    function toggleFlightMode(){
      if(!confirm('确定要切换飞行模式吗？'))return;
      var b=document.getElementById('flightBtn'),r=document.getElementById('flightResult');
      b.disabled=true;r.className='result-box result-loading';r.textContent='切换中...';
      fetch('/flight?action=toggle').then(function(rr){return rr.json()}).then(function(d){
        b.disabled=false;
        if(d.success){r.className='result-box result-success';r.innerHTML=d.message;}
        else{r.className='result-box result-error';r.innerHTML='切换失败: '+d.message;}
      }).catch(function(e){b.disabled=false;r.className='result-box result-error';r.textContent='请求失败: '+e;});
    }

    // ---- Modem Control ----
    function modemAction(action){
      var names={'restart':'软重启','hardreset':'硬重启','signal':'信号查询','operator':'运营商查询','imei':'IMEI查询'};
      var name=names[action]||action;
      var resultEl=null;
      if(action==='restart'||action==='hardreset') resultEl=document.getElementById('modemRstResult');
      else resultEl=document.getElementById('modemQueryResult');
      if(action==='hardreset'){
        if(!confirm('硬重启将断电重启模组，确定继续？'))return;
        resultEl.className='result-box result-loading';resultEl.textContent='硬重启中（约10秒）...';
        fetch('/modem?action=hardreset').then(function(rr){return rr.json()}).then(function(d){
          resultEl.className='result-box result-success';resultEl.textContent=d.message+' — 稍后请手动查询信号确认恢复';
        }).catch(function(e){resultEl.className='result-box result-error';resultEl.textContent='请求失败: '+e;});
        return;
      }
      resultEl.className='result-box result-loading';resultEl.textContent=name+'中...';
      fetch('/modem?action='+action).then(function(rr){return rr.json()}).then(function(d){
        if(d.success){resultEl.className='result-box result-success';resultEl.innerHTML=name+'成功: '+d.message;}
        else{resultEl.className='result-box result-error';resultEl.innerHTML=name+'失败: '+d.message;}
      }).catch(function(e){resultEl.className='result-box result-error';resultEl.textContent='请求失败: '+e;});
    }

    // ---- AT Terminal ----
    function addLog(msg,type){
      type=type||'resp';var log=document.getElementById('atLog'),div=document.createElement('div'),b=document.createElement('b');
      if(type==='user'){b.style.color='#fff';b.textContent='> ';}
      else if(type==='error'){b.style.color='#f44336';b.textContent='! ';}
      else{b.style.color='#50e3c2';b.textContent='';}
      div.appendChild(b);div.appendChild(document.createTextNode(msg));
      log.appendChild(div);log.scrollTop=log.scrollHeight;
    }
    function sendAT(){
      var inp=document.getElementById('atCmd'),cmd=inp.value.trim();if(!cmd)return;
      var btn=document.getElementById('atBtn');btn.disabled=true;btn.textContent='...';
      addLog(cmd,'user');inp.value='';
      fetch('/at?cmd='+encodeURIComponent(cmd)).then(function(rr){return rr.json()}).then(function(d){
        addLog(d.message,d.success?'resp':'error');
      }).catch(function(e){addLog('网络错误: '+e,'error')}).finally(function(){btn.disabled=false;btn.textContent='发送';});
    }
    function clearATLog(){var l=document.getElementById('atLog');l.innerHTML='';addLog('日志已清空','resp');}
    document.getElementById('atCmd').addEventListener('keydown',function(e){if(e.key==='Enter')sendAT();});

    // ---- Log Viewer ----
    var logTimer = null;
    function startLogPoll() {
      if (logTimer) return;
      logTimer = setInterval(refreshLog, 2000);
    }
    function stopLogPoll() {
      if (logTimer) { clearInterval(logTimer); logTimer = null; }
    }
    function toggleLogAuto() {
      if (document.getElementById('logAuto').checked) startLogPoll();
      else stopLogPoll();
    }
    function clearLogUI() { document.getElementById('logView').textContent = ''; }
    function refreshLog() {
      var el = document.getElementById('logView');
      fetch('/log').then(function(r) { return r.json(); }).then(function(lines) {
        if (!Array.isArray(lines)) return;
        el.textContent = lines.join('\n');
        el.scrollTop = el.scrollHeight;
      }).catch(function() { if (el.textContent === '加载中...') el.textContent = '无法获取日志'; });
    }
    var _origSwitchPanel = switchPanel;
    switchPanel = function(name) {
      _origSwitchPanel(name);
      if (name === 'log') { refreshLog(); startLogPoll(); }
      else stopLogPoll();
    };
    document.addEventListener('DOMContentLoaded', function() { refreshLog(); startLogPoll(); });
  </script>
</body>
</html>
)rawliteral";

const char* wifiProvisionPage = R"rawliteral(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>SMS Forwarder | WiFi Provisioning</title>
  <link href="https://fonts.googleapis.com/css2?family=Outfit:wght@300;400;500;600;700&display=swap" rel="stylesheet">
  <style>
    :root {
      --bg: #090d16;
      --card-bg: rgba(255, 255, 255, 0.03);
      --card-border: rgba(255, 255, 255, 0.08);
      --accent: linear-gradient(135deg, #6366f1 0%, #a855f7 100%);
      --accent-solid: #8b5cf6;
      --text: #f3f4f6;
      --text-muted: #9ca3af;
      --input-bg: rgba(255, 255, 255, 0.05);
      --input-border: rgba(255, 255, 255, 0.1);
    }
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body {
      font-family: 'Outfit', -apple-system, sans-serif;
      background-color: var(--bg);
      color: var(--text);
      min-height: 100vh;
      display: flex;
      align-items: center;
      justify-content: center;
      padding: 20px;
      overflow-x: hidden;
      position: relative;
    }
    body::before, body::after {
      content: '';
      position: absolute;
      width: 300px;
      height: 300px;
      border-radius: 50%;
      background: var(--accent);
      filter: blur(120px);
      opacity: 0.15;
      z-index: 0;
    }
    body::before { top: 10%; left: 10%; }
    body::after { bottom: 10%; right: 10%; }
    
    .container {
      background: var(--card-bg);
      border: 1px solid var(--card-border);
      backdrop-filter: blur(20px);
      -webkit-backdrop-filter: blur(20px);
      border-radius: 24px;
      padding: 40px;
      width: 100%;
      max-width: 480px;
      box-shadow: 0 20px 50px rgba(0, 0, 0, 0.3);
      z-index: 1;
      position: relative;
      animation: fadeIn 0.6s cubic-bezier(0.16, 1, 0.3, 1);
    }
    @keyframes fadeIn {
      from { opacity: 0; transform: translateY(20px); }
      to { opacity: 1; transform: translateY(0); }
    }
    .header {
      text-align: center;
      margin-bottom: 30px;
    }
    .logo {
      font-size: 32px;
      font-weight: 700;
      background: var(--accent);
      -webkit-background-clip: text;
      -webkit-text-fill-color: transparent;
      margin-bottom: 8px;
      letter-spacing: -1px;
    }
    .subtitle {
      font-size: 14px;
      color: var(--text-muted);
      line-height: 1.6;
    }
    .form-group {
      margin-bottom: 20px;
    }
    .label {
      display: block;
      font-size: 13px;
      font-weight: 500;
      color: var(--text);
      margin-bottom: 8px;
      letter-spacing: 0.5px;
    }
    .select, .input {
      width: 100%;
      padding: 12px 16px;
      font-size: 14px;
      font-family: inherit;
      color: var(--text);
      background: var(--input-bg);
      border: 1px solid var(--input-border);
      border-radius: 12px;
      outline: none;
      transition: all 0.3s ease;
    }
    .select:focus, .input:focus {
      border-color: var(--accent-solid);
      box-shadow: 0 0 0 3px rgba(139, 92, 246, 0.25);
    }
    .select option {
      background-color: var(--bg);
      color: var(--text);
    }
    .btn {
      width: 100%;
      padding: 14px;
      border: none;
      border-radius: 12px;
      background: var(--accent);
      color: white;
      font-size: 15px;
      font-weight: 600;
      cursor: pointer;
      transition: all 0.3s ease;
      box-shadow: 0 4px 15px rgba(99, 102, 241, 0.3);
    }
    .btn:hover {
      transform: translateY(-2px);
      box-shadow: 0 6px 20px rgba(99, 102, 241, 0.4);
    }
    .btn:active {
      transform: translateY(0);
    }
    .footer-text {
      text-align: center;
      font-size: 12px;
      color: var(--text-muted);
      margin-top: 24px;
      line-height: 1.5;
    }
  </style>
</head>
<body>
  <div class="container">
    <div class="header">
      <div class="logo">SMS Forwarder</div>
      <div class="subtitle">未检测到可用 WiFi 或连接失败，请配置并连接网络。</div>
    </div>
    <form action="/save_wifi" method="POST">
      <div class="form-group">
        <label class="label">WiFi 名称 (SSID)</label>
        <select class="select" name="ssid" required>
          <option value="" disabled selected>请选择 WiFi 网络...</option>
          %WIFI_LIST%
        </select>
      </div>
      <div class="form-group">
        <label class="label">WiFi 密码</label>
        <input class="input" type="password" name="password" placeholder="请输入 WiFi 密码">
      </div>
      <button type="submit" class="btn">连接网络</button>
    </form>
    <div class="footer-text">
      设备已进入配网状态。配置完成后，设备将自动尝试联网。如果三次失败，热点将重新开启供您配置。
    </div>
  </div>
</body>
</html>
)rawliteral";
