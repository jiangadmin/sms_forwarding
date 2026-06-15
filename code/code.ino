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
      
      logCaptureLn(String("WiFi \u548c NTP \u5df2\u5c31\u7eea\uff0c\u6b63\u5728\u5f00\u542f\u5e76\u521d\u59cb\u5316 4G \u6a21\u7ec4..."));
      modemPowerCycle();
      modemInit();
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
  Serial1.begin(115200, SERIAL_8N1, RXD, TXD);
  Serial1.setRxBufferSize(SERIAL_BUFFER_SIZE);
  while (Serial1.available()) Serial1.read();
  
  pinMode(MODEM_EN_PIN, OUTPUT);
  digitalWrite(MODEM_EN_PIN, LOW);
  logCaptureLn(String("\u5df2\u5c06 4G \u6a21\u7ec4\u4e0b\u7535\uff0c\u51c6\u5907\u8fde\u63a5 WiFi..."));
  
  while (Serial1.available()) Serial1.read();
  initConcatBuffer();
  loadConfig();
  configValid = isConfigValid();

  WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info){
    if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
      uint8_t reason = info.wifi_sta_disconnected.reason;
      logCapture(String("\u26a0 WiFi \u8fde\u63a5\u65ad\u5f00\uff0c\u539f\u56e0\u4ee3\u7801: ") + String(reason));
      switch (reason) {
        case 1: logCaptureLn(" (\u672a\u6307\u5b9a\u9519\u8bef/WIFI_REASON_UNSPECIFIED)"); break;
        case 2: logCaptureLn(" (\u6388\u6743\u8fc7\u671f/WIFI_REASON_AUTH_EXPIRE)"); break;
        case 15: logCaptureLn(" (\u5bc6\u7801\u9519\u8bef\u6216\u4fe1\u53f7\u5dee/WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT)"); break;
        case 201: logCaptureLn(" (\u627e\u4e0d\u5230AP/WIFI_REASON_NO_AP_FOUND\uff0c\u8bf7\u786e\u8ba4\u662f2.4G\u7f51\u7edc\u4e14\u540d\u5b57\u65e0\u8bef)"); break;
        case 202: logCaptureLn(" (\u6388\u6743\u5931\u8d25/WIFI_REASON_AUTH_FAIL)"); break;
        case 203: logCaptureLn(" (\u5173\u8054\u5931\u8d25/WIFI_REASON_ASSOC_FAIL)"); break;
        default: logCaptureLn(""); break;
      }
    } else if (event == ARDUINO_EVENT_WIFI_STA_CONNECTED) {
      logCaptureLn(String("\u5df2\u8fde\u63a5\u5230 AP: ") + WiFi.SSID());
    } else if (event == ARDUINO_EVENT_WIFI_STA_GOT_IP) {
      logCaptureLn(String("\u5df2\u6210\u529f\u83b7\u53d6 IP \u5730\u5740"));
    } else if (event == ARDUINO_EVENT_WIFI_AP_START) {
      logCaptureLn(String("\u2713 WiFi AP \u63a5\u53e3\u5df2\u6210\u529f\u5728\u7269\u7406\u5c42\u542f\u52a8"));
    } else if (event == ARDUINO_EVENT_WIFI_AP_STOP) {
      logCaptureLn(String("\u26a0 WiFi AP \u63a5\u53e3\u5df2\u505c\u6b62"));
    }
  });

  if (config.wifiSsid.length() == 0) {
    logCaptureLn(String("\u672a\u914d\u7f6e WiFi\uff0c\u8fdb\u5165\u914d\u7f61\u6a21\u5f0f..."));
    enterApConfigMode();
  } else {
    logCaptureLn(String("\u68c0\u6d4b\u5230\u5df2\u4fdd\u5b58\u7684 WiFi \u914d\u7f6e\uff0c\u5c1d\u8bd5\u8fde\u63a5..."));
    WiFi.mode(WIFI_STA);
    WiFi.setTxPower(WIFI_POWER_8_5dBm);
    WiFi.setSleep(false);
    WiFi.setAutoReconnect(true);
    
    bool connected = false;
    int failedAttempts = 0;
    while (failedAttempts < 3) {
      logCaptureLn(String("\u5c1d\u8bd5\u8fde\u63a5 WiFi (\u7b2c ") + String(failedAttempts + 1) + String(" \u6b21)..."));
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
      logCaptureLn(String("WiFi \u8fde\u63a5\u6210\u529f!"));
      logCapture(String("IP \u5730\u5740: "));
      logCaptureLn(WiFi.localIP().toString());
      inApConfigMode = false;
      
      syncNtpTime();
      ssl_client.setInsecure();
      digitalWrite(LED_BUILTIN, LOW);
      
      if (configValid) {
        logCaptureLn(String("\u914d\u7f6e\u6709\u6548\uff0c\u53d1\u9001\u542f\u52a8\u901a\u77e5..."));
        String subject = "\u77ed\u4fe1\u8f6c\u53d1\u5668\u5df2\u542f\u52a8";
        String body = "\u8bbe\u5907\u5df2\u542f\u52a8\n\u8bbe\u5907\u5730\u5740: " + getDeviceUrl();
        sendEmailNotification(subject.c_str(), body.c_str());
      }
      
      logCaptureLn(String("WiFi \u548c NTP \u5df2\u5c31\u7eea\uff0c\u6b63\u5728\u5f00\u542f\u5e76\u521d\u59cb\u5316 4G \u6a21\u7ec4..."));
      modemPowerCycle();
      modemInit();
    } else {
      logCaptureLn(String("\u26a0 \u65e0\u6cd5\u8fde\u63a5\u5230\u5df2\u4fdd\u5b58\u7684 WiFi\uff0c\u8fdb\u5165\u914d\u7f61\u6a21\u5f0f..."));
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
  logCaptureLn(String("HTTP\u670d\u52a1\u5668\u5df2\u542f\u52a8"));
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
      logCaptureLn(String("\u26a0 \u8bf7\u8bbf\u95ee " + getDeviceUrl() + " \u914d\u7f6e\u7cfb\u7edf\u53c2\u6570"));
    }
  }
  checkConcatTimeout();
  if (Serial.available()) Serial1.write(Serial.read());
  checkSerial1URC();
}
