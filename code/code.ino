#include "globals.h"
#include "wifi_config.h"
#include "config.h"
#include "web_handlers.h"
#include "web_handlers.h"
#include "modem.h"
#include "web_handlers.h"
#include "push.h"
#include "web_handlers.h"
#include "sms_process.h"
#include "web_handlers.h"

void syncNtpTime() {
  logCaptureLn(String("\u6b63\u5728\u540c\u6b65NTP\u65f6\u95f4..."));
  configTime(0, 0, "ntp.ntsc.ac.cn", "ntp.aliyun.com", "pool.ntp.org");
  int ntpRetry = 0;
  while (time(nullptr) < 100000 && ntpRetry < 100) {
    delay(1);
    server.handleClient();
    ntpRetry++;
  }
  if (time(nullptr) >= 100000) {
    timeSynced = true;
    logCaptureLn(String("NTP\u65f6\u95f4\u540c\u6b65\u6210\u529f"));
    time_t now = time(nullptr);
    logCapture(String("\u5f53\u524dUTC\u65f6\u95f4\u6233: "));
    logCaptureLn(String(now));
  } else {
    logCaptureLn(String("NTP\u65f6\u95f4\u540c\u6b65\u5931\u8d25\uff0c\u5c06\u4f7f\u7528\u8bbe\u5907\u65f6\u95f4"));
  }
}

void enterApConfigMode() {
  logCaptureLn(String("\u6b63\u5728\u626b\u63cf\u9644\u8fd1 WiFi..."));
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  
  // Read MAC address while in STA mode to avoid getting 00:00:00:00:00:00
  String mac = WiFi.macAddress();
  
  int n = WiFi.scanNetworks();
  logCaptureLn(String("\u626b\u63cf\u5b8c\u6210\uff0c\u53d1\u73b0 ") + String(n) + String(" \u4e2a\u7f51\u7edc"));
  
  scannedWifiListHtml = "";
  for (int i = 0; i < n; ++i) {
    String ssid = WiFi.SSID(i);
    int32_t rssi = WiFi.RSSI(i);
    String encryption = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? String("\u516c\u5f00") : String("\u52a0\u5bc6");
    scannedWifiListHtml += "<option value=\"" + ssid + "\">" + ssid + " (" + String("\u4fe1\u53f7: ") + String(rssi) + "dBm, " + encryption + ")</option>";
  }
  if (n <= 0) {
    scannedWifiListHtml = "<option value=\"\" disabled>" + String("\u672a\u626b\u63cf\u5230\u9644\u8fd1\u7684 WiFi \u7f51\u7edc") + "</option>";
  }
  
  // Clean up scan results from memory
  WiFi.scanDelete();
  
  // Explicitly set to WIFI_AP mode
  WiFi.mode(WIFI_AP);
  delay(500);
  
  // Set explicit IP for SoftAP
  IPAddress apIP(192, 168, 4, 1);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  delay(100);
  
  String macSuffix = mac.substring(mac.length() - 5);
  macSuffix.replace(":", "");
  String apSsid = "SMS_Forwarder_" + macSuffix;
  
  // Start SoftAP using default parameters (channel 1, visible, max 4 clients) with WPA2 password
  bool apSuccess = WiFi.softAP(apSsid.c_str(), "12345678");
  
  // Lower TX power AFTER starting the AP to ensure it takes effect and prevents brownout
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
  
  if (apSuccess) {
    logCaptureLn(String("\u5df2\u542f\u52a8\u70ed\u70b9: ") + apSsid);
    logCaptureLn(String("\u70ed\u70b9\u5bc6\u7801: 12345678"));
    logCaptureLn(String("\u8bf7\u8fde\u63a5\u8be5\u70ed\u70b9\u5e76\u8bbf\u95ee http://") + WiFi.softAPIP().toString() + String(" \u914d\u7f6e WiFi"));
  } else {
    logCaptureLn(String("\u26a0 \u542f\u52a8\u70ed\u70b9\u5931\u8d25!"));
  }
  inApConfigMode = true;
}

bool attemptWifiConnection() {
  logCaptureLn(String("\u6b63\u5728\u5c1d\u8bd5\u8fde\u63a5\u5230 WiFi: ") + config.wifiSsid);
  WiFi.softAPdisconnect(true);
  
  int failedAttempts = 0;
  while (failedAttempts < 3) {
    logCaptureLn(String("\u5c1d\u8bd5\u8fde\u63a5 WiFi (\u7b2c ") + String(failedAttempts + 1) + String(" \u6b21)..."));
    WiFi.mode(WIFI_STA);
    WiFi.begin(config.wifiSsid.c_str(), config.wifiPass.c_str());
    
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
      blink_short(200);
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      logCaptureLn(String("WiFi \u8fde\u63a5\u6210\u529f!"));
      logCapture(String("IP \u5730\u5740: "));
      logCaptureLn(WiFi.localIP().toString());
      inApConfigMode = false;
      configValid = isConfigValid();
      
      syncNtpTime();
      ssl_client.setInsecure();
      digitalWrite(LED_BUILTIN, LOW);
      
      if (configValid) {
        logCaptureLn(String("\u914d\u7f6e\u6709\u6548\uff0c\u53d1\u9001\u542f\u52a8\u901a\u77e5..."));
        String subject = "\u77ed\u4fe1\u8f6c\u53d1\u5668\u5df2\u542f\u52a8";
        String body = "\u8bbe\u5907\u5df2\u542f\u52a8\n\u8bbe\u5907\u5730\u5740: " + getDeviceUrl();
        sendEmailNotification(subject.c_str(), body.c_str());
      }
      
      logCaptureLn(String("WiFi 和 NTP 已就绪，正在开启并初始化双 4G 模组..."));
      modemPowerCycle(1);
      modemPowerCycle(2);
      modemInitAll();
      return true;
    } else {
      logCaptureLn(String("\u8fde\u63a5\u5931\u8d25!"));
      failedAttempts++;
    }
  }
  
  logCaptureLn(String("\u26a0 \u8fde\u7eed 3 \u6b21\u8fde\u63a5 WiFi \u5931\u8d25\u3002\u5c06\u653e\u5f03\u672c\u6b21\u914d\u7f6e\uff0c\u91cd\u65b0\u542f\u52a8\u70ed\u70b9\u7b49\u5f85\u914d\u7f61..."));
  config.wifiSsid = "";
  config.wifiPass = "";
  saveConfig();
  
  enterApConfigMode();
  return false;
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.begin(115200);
  delay(200);
  Serial1.begin(115200, SERIAL_8N1, RXD1, TXD1);
  Serial1.setRxBufferSize(SERIAL_BUFFER_SIZE);
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
  Serial2.setRxBufferSize(SERIAL_BUFFER_SIZE);
  while (Serial1.available()) Serial1.read();
  while (Serial2.available()) Serial2.read();
  
  if (MODEM1_EN_PIN >= 0) {
    pinMode(MODEM1_EN_PIN, OUTPUT);
    digitalWrite(MODEM1_EN_PIN, HIGH);
  }
  if (MODEM2_EN_PIN >= 0) {
    pinMode(MODEM2_EN_PIN, OUTPUT);
    digitalWrite(MODEM2_EN_PIN, HIGH);
  }
  logCaptureLn(String("准备连接 WiFi..."));
  
  while (Serial1.available()) Serial1.read();
  while (Serial2.available()) Serial2.read();
  initConcatBuffer();
  loadConfig();
  configValid = isConfigValid();

  WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info){
    if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
      uint8_t reason = info.wifi_sta_disconnected.reason;
      logCapture(String("⚠️ WiFi 连接断开，原因代码: ") + String(reason));
      switch (reason) {
        case 1: logCaptureLn(" (未指定错误/WIFI_REASON_UNSPECIFIED)"); break;
        case 2: logCaptureLn(" (授权过期/WIFI_REASON_AUTH_EXPIRE)"); break;
        case 15: logCaptureLn(" (密码错误或信号差/WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT)"); break;
        case 201: logCaptureLn(" (找不到AP/WIFI_REASON_NO_AP_FOUND，请确认是2.4G网络且名字无误)"); break;
        case 202: logCaptureLn(" (授权失败/WIFI_REASON_AUTH_FAIL)"); break;
        case 203: logCaptureLn(" (关联失败/WIFI_REASON_ASSOC_FAIL)"); break;
        default: logCaptureLn(""); break;
      }
    } else if (event == ARDUINO_EVENT_WIFI_STA_CONNECTED) {
      logCaptureLn(String("已连接到 AP: ") + WiFi.SSID());
    } else if (event == ARDUINO_EVENT_WIFI_STA_GOT_IP) {
      logCaptureLn(String("已成功获取 IP 地址"));
    } else if (event == ARDUINO_EVENT_WIFI_AP_START) {
      logCaptureLn(String("✓ WiFi AP 接口已成功在物理层启动"));
    } else if (event == ARDUINO_EVENT_WIFI_AP_STOP) {
      logCaptureLn(String("⚠️ WiFi AP 接口已停止"));
    }
  });

  if (config.wifiSsid.length() == 0) {
    logCaptureLn(String("未配置 WiFi，进入配网模式..."));
    enterApConfigMode();
  } else {
    logCaptureLn(String("检测到已保存的 WiFi 配置，尝试连接..."));
    WiFi.mode(WIFI_STA);
    WiFi.setTxPower(WIFI_POWER_8_5dBm);
    WiFi.setSleep(false);
    WiFi.setAutoReconnect(true);
    
    bool connected = false;
    int failedAttempts = 0;
    while (failedAttempts < 3) {
      logCaptureLn(String("尝试连接 WiFi (第 ") + String(failedAttempts + 1) + String(" 次)..."));
      WiFi.begin(config.wifiSsid.c_str(), config.wifiPass.c_str());
      unsigned long start = millis();
      while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
        blink_short(200);
      }
      if (WiFi.status() == WL_CONNECTED) {
        connected = true;
        break;
      }
      failedAttempts++;
    }
    
    if (connected) {
      logCaptureLn(String("WiFi 连接成功!"));
      logCapture(String("IP 地址: "));
      logCaptureLn(WiFi.localIP().toString());
      inApConfigMode = false;
      
      syncNtpTime();
      ssl_client.setInsecure();
      digitalWrite(LED_BUILTIN, LOW);
      
      if (configValid) {
        logCaptureLn(String("配置有效，发送启动通知..."));
        String subject = "短信转发器已启动";
        String body = "设备已启动\n设备地址: " + getDeviceUrl();
        sendEmailNotification(subject.c_str(), body.c_str());
      }
      
      logCaptureLn(String("WiFi 和 NTP 已就绪，正在开启并初始化双 4G 模组..."));
      modemPowerCycle(1);
      modemPowerCycle(2);
      modemInitAll();
    } else {
      logCaptureLn(String("⚠️ 无法连接到已保存的 WiFi，进入配网模式..."));
      enterApConfigMode();
    }
  }

  server.on("/", handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/save_wifi", HTTP_POST, handleSaveWifi);
  server.on("/tools", handleRoot);
  server.on("/sms", handleRoot);
  server.on("/sendsms", HTTP_POST, handleSendSms);
  server.on("/ping", HTTP_POST, handlePing);
  server.on("/query", handleQuery);
  server.on("/flight", handleFlightMode);
  server.on("/at", handleATCommand);
  server.on("/log", handleLog);
  server.on("/modem", handleModem);
  server.on("/wifi", handleWifi);
  server.begin();
  logCaptureLn(String("HTTP服务器已启动"));
}

void loop() {
  server.handleClient();
  if (wifiConfigSubmitted) {
    if (millis() - wifiConfigSubmittedTime > 1000) {
      wifiConfigSubmitted = false;
      attemptWifiConnection();
    }
  }
  if (!inApConfigMode && !configValid) {
    if (millis() - lastPrintTime >= 1000) {
      lastPrintTime = millis();
      logCaptureLn(String("⚠️ 请访问 " + getDeviceUrl() + " 配置系统参数"));
    }
  }
  checkConcatTimeout();
  if (Serial.available()) {
    char c = Serial.read();
    Serial1.write(c);
  }
  checkSerial1URC();
  checkSerial2URC();
}
