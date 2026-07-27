#include "globals.h"

HardwareSerial Serial2(0);
Config config;
Preferences preferences;
PDU pdu = PDU(4096);
WiFiClientSecure ssl_client;
SMTPClient smtp(ssl_client);
WebServer server(80);
bool configValid = false;
bool timeSynced = false;
bool modemReady = false;
bool modem1Ready = false;
bool modem2Ready = false;
unsigned long lastPrintTime = 0;
ConcatSms concatBuffer[MAX_CONCAT_MESSAGES];

bool inApConfigMode = false;
String scannedWifiListHtml = "";
bool wifiConfigSubmitted = false;
unsigned long wifiConfigSubmittedTime = 0;
bool modemEnActiveLow = true;
