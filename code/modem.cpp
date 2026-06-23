#include "modem.h"
#include "web_handlers.h"

// 发送AT命令并获取响应
String sendATCommand(const char* cmd, unsigned long timeout) {
  while (Serial1.available()) Serial1.read();
  Serial1.println(cmd);
  
  unsigned long start = millis();
  String resp = "";
  while (millis() - start < timeout) {
    if (Serial1.available()) {
      char c = Serial1.read();
      resp += c;
      if (resp.indexOf("OK") >= 0 || resp.indexOf("ERROR") >= 0) {
        // 读取剩余数据（最多 50ms）
        unsigned long t = millis();
        while (millis() - t < 50) {
          if (Serial1.available()) resp += (char)Serial1.read();
          yield();
        }
        return resp;
      }
    }
    yield();
  }
  return resp;
}

// 新增"模组断电重启"函数
void modemPowerCycle() {
  pinMode(MODEM_EN_PIN, OUTPUT);

  logCaptureLn(String("EN 拉高：关闭模组"));
  digitalWrite(MODEM_EN_PIN, HIGH);
  delay(1500);  // 关机时间给够

  logCaptureLn(String("EN 拉低：开启模组"));
  digitalWrite(MODEM_EN_PIN, LOW);
  delay(8000);  // 等模组完全启动再发AT（从 6 秒增加到 8 秒以确保搜网和就绪）
}

// 重启模组（EN引脚断电重启 + 重新初始化）
void resetModule() {
  logCaptureLn(String("正在硬重启模组（EN 断电重启）..."));
  modemPowerCycle();
  modemInit();
}

// 模组 AT 初始化流程（setup 中调用，resetModule 后也调用）
void modemInit() {
  logCaptureLn(String("开始初始化 4G 模组..."));
  
  // 按照 globals.h 定义的引脚初始化串口
  Serial1.end();
  delay(100);
  Serial1.begin(115200, SERIAL_8N1, RXD, TXD);
  Serial1.setRxBufferSize(SERIAL_BUFFER_SIZE);
  
  // 清掉上电噪声/残留
  while (Serial1.available()) Serial1.read();

  int atRetry = 0;
  // 循环握手最多 15 次，给够模组开机启动的时间（每次最长等待 1000ms）
  while (!sendATandWaitOK("AT", 1000)) {
    logCaptureLn(String("AT未响应，重试..."));
    blink_short();
    atRetry++;
    if (atRetry >= 15) {
      logCaptureLn(String("⚠️ 模组 AT 握手失败次数过多，跳过初始化以开启 Web 服务并检查接线"));
      modemReady = false;
      return;
    }
  }
  logCaptureLn(String("模组AT响应正常"));

  // 继续配置模组参数
  logCaptureLn(String("开始配置模组参数..."));

  int cgactRetry = 0;
  while (!sendATandWaitOK("AT+CGACT=0,1", 5000)) {
    logCaptureLn(String("设置CGACT失败，重试..."));
    blink_short();
    cgactRetry++;
    if (cgactRetry >= 5) {
      logCaptureLn(String("⚠️ 设置CGACT失败次数过多，继续初始化..."));
      break;
    }
  }
  logCaptureLn(String("已禁用数据连接(AT+CGACT=0,1)，防止流量消耗"));

  int cnmiRetry = 0;
  while (!sendATandWaitOK("AT+CNMI=2,2,0,0,0", 1000)) {
    logCaptureLn(String("设置CNMI失败，重试..."));
    blink_short();
    cnmiRetry++;
    if (cnmiRetry >= 5) {
      logCaptureLn(String("⚠️ 设置CNMI失败次数过多，继续初始化..."));
      break;
    }
  }
  logCaptureLn(String("CNMI参数设置完成"));

  int cmgfRetry = 0;
  while (!sendATandWaitOK("AT+CMGF=0", 1000)) {
    logCaptureLn(String("设置PDU模式失败，重试..."));
    blink_short();
    cmgfRetry++;
    if (cmgfRetry >= 5) {
      logCaptureLn(String("⚠️ 设置PDU模式失败次数过多，继续初始化..."));
      break;
    }
  }
  logCaptureLn(String("PDU模式设置完成"));

  int ceregRetry = 0;
  while (!waitCEREG() && ceregRetry < 30) {
    logCaptureLn(String("等待网络注册..."));
    ceregRetry++;
    blink_short();
  }
  if (ceregRetry < 30) {
    logCaptureLn(String("网络已注册"));
    modemReady = true;
  } else {
    logCaptureLn(String("⚠️ 网络注册超时（无SIM卡或信号差），模组功能不可用"));
    modemReady = false;
  }
}

void blink_short(unsigned long gap_time) {
  digitalWrite(LED_BUILTIN, LOW);
  delay(50);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(gap_time);
}

bool sendATandWaitOK(const char* cmd, unsigned long timeout) {
  while (Serial1.available()) Serial1.read();
  Serial1.println(cmd);
  unsigned long start = millis();
  String resp = "";
  while (millis() - start < timeout) {
    if (Serial1.available()) {
      char c = Serial1.read();
      resp += c;
      if (resp.indexOf("OK") >= 0) return true;
      if (resp.indexOf("ERROR") >= 0) return false;
    }
    yield();
  }
  return false;
}

// 检测网络注册状态（LTE/4G）
// CEREG状态: 1=已注册本地, 5=已注册漫游
bool waitCEREG() {
  Serial1.println("AT+CEREG?");
  unsigned long start = millis();
  String resp = "";
  while (millis() - start < 2000) {
    if (Serial1.available()) {
      char c = Serial1.read();
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
bool sendSMS(const char* phoneNumber, const char* message) {
  logCaptureLn(String("准备发送短信..."));
  logCapture(String("目标号码: ")); logCaptureLn(String(phoneNumber));
  logCapture(String("短信内容: ")); logCaptureLn(String(message));

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
  
  while (Serial1.available()) Serial1.read();
  Serial1.println(cmgsCmd);
  
  // 等待 > 提示符
  unsigned long start = millis();
  bool gotPrompt = false;
  while (millis() - start < 5000) {
    if (Serial1.available()) {
      char c = Serial1.read();
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
  Serial1.print(pdu.getSMS());
  Serial1.write(0x1A);  // Ctrl+Z 结束
  
  // 等待响应
  start = millis();
  String resp = "";
  while (millis() - start < 30000) {
    while (Serial1.available()) {
      char c = Serial1.read();
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
