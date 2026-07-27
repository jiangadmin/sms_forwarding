#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WebServer.h>
#include <Preferences.h>
#include <pdulib.h>
#define ENABLE_SMTP
#define ENABLE_DEBUG
#include <ReadyMail.h>
#include "config_types.h"

// 双模组串口映射与引脚定义
// 模组1 (Modem 1)
#define TXD1 20
#define RXD1 21
#define MODEM1_EN_PIN -1  // 不使用 EN 引脚

// 模组2 (Modem 2)
#define TXD2 0
#define RXD2 1
#define MODEM2_EN_PIN -1  // 不使用 EN 引脚

// 兼容别名
#define TXD TXD1
#define RXD RXD1
#define MODEM_EN_PIN MODEM1_EN_PIN

// LED引脚定义（用于通过CI验证，给个假的）
#ifndef LED_BUILTIN
#define LED_BUILTIN 8
#endif

#define SERIAL_BUFFER_SIZE 500
#define MAX_PDU_LENGTH 300

// 声明 Serial2 控制器
extern HardwareSerial Serial2;

// 全局变量声明
extern Config config;
extern Preferences preferences;
extern PDU pdu;
extern WiFiClientSecure ssl_client;
extern SMTPClient smtp;
extern WebServer server;
extern bool configValid;
extern bool timeSynced;
extern bool modemReady;
extern bool modem1Ready;
extern bool modem2Ready;
extern unsigned long lastPrintTime;
extern ConcatSms concatBuffer[MAX_CONCAT_MESSAGES];
extern bool inApConfigMode;
extern String scannedWifiListHtml;
extern bool wifiConfigSubmitted;
extern unsigned long wifiConfigSubmittedTime;
extern bool modemEnActiveLow;

#endif
