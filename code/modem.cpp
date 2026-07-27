#include "modem.h"
#include "web_handlers.h"

HardwareSerial& getModemSerial(uint8_t modemIndex) {
  if (modemIndex == 2) return Serial2;
  return Serial1;
}

int8_t getModemEnPin(uint8_t modemIndex) {
  if (modemIndex == 2) return MODEM2_EN_PIN;
  return MODEM1_EN_PIN;
}

// 发送AT命令并获取响应
String sendATCommand(const char* cmd, unsigned long timeout, uint8_t modemIndex) {
  HardwareSerial& serial = getModemSerial(modemIndex);
  while (serial.available()) serial.read();
  serial.println(cmd);
  
  unsigned long start = millis();
  String resp = "";
  while (millis() - start < timeout) {
    if (serial.available()) {
      char c = serial.read();
      resp += c;
      if (resp.indexOf("OK") >= 0 || resp.indexOf("ERROR") >= 0) {
        // 读取剩余数据（最多 50ms）
        unsigned long t = millis();
        while (millis() - t < 50) {
          if (serial.available()) resp += (char)serial.read();
          yield();
        }
        return resp;
      }
    }
    yield();
  }
  return resp;
}

// "模组断电重启"函数
void modemPowerCycle(uint8_t modemIndex) {
  int8_t enPin = getModemEnPin(modemIndex);
  if (enPin < 0) {
    logCaptureLn(String("模组 ") + String(modemIndex) + " 未配置/未连接 EN 引脚，跳过硬件开关机");
    return;
  }
  pinMode(enPin, OUTPUT);

  logCaptureLn(String("模组 ") + String(modemIndex) + " EN 拉高：关闭模组");
  digitalWrite(enPin, HIGH);
  delay(1500);  // 关机时间给够

  logCaptureLn(String("模组 ") + String(modemIndex) + " EN 拉低：开启模组");
  digitalWrite(enPin, LOW);
  delay(8000);  // 等模组完全启动再发AT
}

// 重启模组（EN引脚断电重启 + 重新初始化）
void resetModule(uint8_t modemIndex) {
  logCaptureLn(String("正在硬重启模组 ") + String(modemIndex) + "（EN 断电重启）...");
  modemPowerCycle(modemIndex);
  modemInit(modemIndex);
}

// 模组 AT 初始化流程
void modemInit(uint8_t modemIndex) {
  logCaptureLn(String("开始初始化 4G 模组 ") + String(modemIndex) + "...");
  HardwareSerial& serial = getModemSerial(modemIndex);
  
  serial.end();
  delay(100);
  if (modemIndex == 2) {
    serial.begin(115200, SERIAL_8N1, RXD2, TXD2);
  } else {
    serial.begin(115200, SERIAL_8N1, RXD1, TXD1);
  }
  serial.setRxBufferSize(SERIAL_BUFFER_SIZE);
  
  // 清掉上电噪声/残留
  while (serial.available()) serial.read();

  int atRetry = 0;
  // 循环握手最多 15 次
  while (!sendATandWaitOK("AT", 1000, modemIndex)) {
    logCaptureLn(String("模组 ") + String(modemIndex) + " AT未响应，重试...");
    blink_short();
    atRetry++;
    if (atRetry >= 15) {
      logCaptureLn(String("⚠️ 模组 ") + String(modemIndex) + " AT 握手失败次数过多，跳过初始化以开启 Web 服务并检查接线");
      if (modemIndex == 2) modem2Ready = false;
      else modem1Ready = false;
      modemReady = modem1Ready || modem2Ready;
      return;
    }
  }
  logCaptureLn(String("模组 ") + String(modemIndex) + " AT响应正常");

  // 继续配置模组参数
  logCaptureLn(String("开始配置模组 ") + String(modemIndex) + " 参数...");

  int cgactRetry = 0;
  while (!sendATandWaitOK("AT+CGACT=0,1", 5000, modemIndex)) {
    logCaptureLn(String("设置CGACT失败，重试..."));
    blink_short();
    cgactRetry++;
    if (cgactRetry >= 5) {
      logCaptureLn(String("⚠️ 设置CGACT失败次数过多，继续初始化..."));
      break;
    }
  }
  logCaptureLn(String("已禁用模组 ") + String(modemIndex) + " 数据连接(AT+CGACT=0,1)，防止流量消耗");

  int cnmiRetry = 0;
  while (!sendATandWaitOK("AT+CNMI=2,2,0,0,0", 1000, modemIndex)) {
    logCaptureLn(String("设置CNMI失败，重试..."));
    blink_short();
    cnmiRetry++;
    if (cnmiRetry >= 5) {
      logCaptureLn(String("⚠️ 设置CNMI失败次数过多，继续初始化..."));
      break;
    }
  }
  logCaptureLn(String("模组 ") + String(modemIndex) + " CNMI参数设置完成");

  int cmgfRetry = 0;
  while (!sendATandWaitOK("AT+CMGF=0", 1000, modemIndex)) {
    logCaptureLn(String("设置PDU模式失败，重试..."));
    blink_short();
    cmgfRetry++;
    if (cmgfRetry >= 5) {
      logCaptureLn(String("⚠️ 设置PDU模式失败次数过多，继续初始化..."));
      break;
    }
  }
  logCaptureLn(String("模组 ") + String(modemIndex) + " PDU模式设置完成");

  int ceregRetry = 0;
  while (!waitCEREG(modemIndex) && ceregRetry < 30) {
    logCaptureLn(String("等待模组 ") + String(modemIndex) + " 网络注册...");
    ceregRetry++;
    blink_short();
  }
  if (ceregRetry < 30) {
    logCaptureLn(String("模组 ") + String(modemIndex) + " 网络已注册");
    if (modemIndex == 2) modem2Ready = true;
    else modem1Ready = true;
  } else {
    logCaptureLn(String("⚠️ 模组 ") + String(modemIndex) + " 网络注册超时（无SIM卡或信号差），模组功能不可用");
    if (modemIndex == 2) modem2Ready = false;
    else modem1Ready = false;
  }
  modemReady = modem1Ready || modem2Ready;
}

void modemInitAll() {
  modemInit(1);
  modemInit(2);
}

void blink_short(unsigned long gap_time) {
  digitalWrite(LED_BUILTIN, LOW);
  delay(50);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(gap_time);
}

bool sendATandWaitOK(const char* cmd, unsigned long timeout, uint8_t modemIndex) {
  HardwareSerial& serial = getModemSerial(modemIndex);
  while (serial.available()) serial.read();
  serial.println(cmd);
  unsigned long start = millis();
  String resp = "";
  while (millis() - start < timeout) {
    if (serial.available()) {
      char c = serial.read();
      resp += c;
      if (resp.indexOf("OK") >= 0) return true;
      if (resp.indexOf("ERROR") >= 0) return false;
    }
    yield();
  }
  return false;
}

// 检测网络注册状态（LTE/4G）
bool waitCEREG(uint8_t modemIndex) {
  HardwareSerial& serial = getModemSerial(modemIndex);
  serial.println("AT+CEREG?");
  unsigned long start = millis();
  String resp = "";
  while (millis() - start < 2000) {
    if (serial.available()) {
      char c = serial.read();
      resp += c;
      if (resp.indexOf("+CEREG:") >= 0) {
        if (resp.indexOf(",1") >= 0 || resp.indexOf(",5") >= 0) return true;
        if (resp.indexOf(",0") >= 0 || resp.indexOf(",2") >= 0 || 
            resp.indexOf(",3") >= 0 || resp.indexOf(",4") >= 0) return false;
      }
    }
    yield();
  }
  return false;
}

// 发送短信（PDU模式）
bool sendSMS(const char* phoneNumber, const char* message, uint8_t modemIndex) {
  logCaptureLn(String("准备通过模组 ") + String(modemIndex) + " 发送短信...");
  logCapture(String("目标号码: ")); logCaptureLn(String(phoneNumber));
  logCapture(String("短信内容: ")); logCaptureLn(String(message));

  HardwareSerial& serial = getModemSerial(modemIndex);

  // 使用pdulib编码PDU
  pdu.setSCAnumber();  // 使用默认短信中心
  int pduLen = pdu.encodePDU(phoneNumber, message);
  
  if (pduLen < 0) {
    logCapture(String("PDU编码失败，错误码: "));
    logCaptureLn(String(pduLen));
    return false;
  }
  
  logCapture(String("PDU数据: ")); logCaptureLn(String(pdu.getSMS()));
  logCapture(String("PDU长度: ")); logCaptureLn(String(pduLen));
  
  // 发送AT+CMGS命令
  String cmgsCmd = "AT+CMGS=";
  cmgsCmd += pduLen;
  
  while (serial.available()) serial.read();
  serial.println(cmgsCmd);
  
  // 等待 > 提示符
  unsigned long start = millis();
  bool gotPrompt = false;
  while (millis() - start < 5000) {
    if (serial.available()) {
      char c = serial.read();
      logCapture(String(c));
      if (c == '>') {
        gotPrompt = true;
        break;
      }
    }
    yield();
  }
  
  if (!gotPrompt) {
    logCaptureLn(String("未收到>提示符"));
    return false;
  }
  
  // 发送PDU数据
  serial.print(pdu.getSMS());
  serial.write(0x1A);  // Ctrl+Z 结束
  
  // 等待响应
  start = millis();
  String resp = "";
  while (millis() - start < 30000) {
    while (serial.available()) {
      char c = serial.read();
      resp += c;
      logCapture(String(c));
      if (resp.indexOf("OK") >= 0) {
        logCaptureLn(String("\n短信发送成功"));
        return true;
      }
      if (resp.indexOf("ERROR") >= 0) {
        logCaptureLn(String("\n短信发送失败"));
        return false;
      }
    }
    yield();
  }
  logCaptureLn(String("短信发送超时"));
  return false;
}
