#include "sms_process.h"
#include "web_handlers.h"
#include "modem.h"
#include "web_handlers.h"
#include "push.h"
#include "web_handlers.h"

// 初始化长短信缓存
void initConcatBuffer() {
  for (int i = 0; i < MAX_CONCAT_MESSAGES; i++) {
    concatBuffer[i].inUse = false;
    concatBuffer[i].receivedParts = 0;
    for (int j = 0; j < MAX_CONCAT_PARTS; j++) {
      concatBuffer[i].parts[j].valid = false;
      concatBuffer[i].parts[j].text = "";
    }
  }
}

// 查找或创建长短信缓存槽位
int findOrCreateConcatSlot(int refNumber, const char* sender, int totalParts) {
  // 先查找是否已存在
  for (int i = 0; i < MAX_CONCAT_MESSAGES; i++) {
    if (concatBuffer[i].inUse && 
        concatBuffer[i].refNumber == refNumber &&
        concatBuffer[i].sender.equals(sender)) {
      return i;
    }
  }
  
  // 查找空闲槽位
  for (int i = 0; i < MAX_CONCAT_MESSAGES; i++) {
    if (!concatBuffer[i].inUse) {
      concatBuffer[i].inUse = true;
      concatBuffer[i].refNumber = refNumber;
      concatBuffer[i].sender = String(sender);
      concatBuffer[i].totalParts = totalParts;
      concatBuffer[i].receivedParts = 0;
      concatBuffer[i].firstPartTime = millis();
      for (int j = 0; j < MAX_CONCAT_PARTS; j++) {
        concatBuffer[i].parts[j].valid = false;
        concatBuffer[i].parts[j].text = "";
      }
      return i;
    }
  }
  
  // 没有空闲槽位，查找最老的槽位覆盖
  int oldestSlot = 0;
  unsigned long oldestTime = concatBuffer[0].firstPartTime;
  for (int i = 1; i < MAX_CONCAT_MESSAGES; i++) {
    if (concatBuffer[i].firstPartTime < oldestTime) {
      oldestTime = concatBuffer[i].firstPartTime;
      oldestSlot = i;
    }
  }
  
  // 覆盖最老的槽位
  logCaptureLn(String("⚠️ 长短信缓存已满，覆盖最老的槽位"));
  concatBuffer[oldestSlot].inUse = true;
  concatBuffer[oldestSlot].refNumber = refNumber;
  concatBuffer[oldestSlot].sender = String(sender);
  concatBuffer[oldestSlot].totalParts = totalParts;
  concatBuffer[oldestSlot].receivedParts = 0;
  concatBuffer[oldestSlot].firstPartTime = millis();
  for (int j = 0; j < MAX_CONCAT_PARTS; j++) {
    concatBuffer[oldestSlot].parts[j].valid = false;
    concatBuffer[oldestSlot].parts[j].text = "";
  }
  return oldestSlot;
}

// 合并长短信各分段
String assembleConcatSms(int slot) {
  String result = "";
  for (int i = 0; i < concatBuffer[slot].totalParts; i++) {
    if (concatBuffer[slot].parts[i].valid) {
      result += concatBuffer[slot].parts[i].text;
    } else {
      result += "[缺失分段" + String(i + 1) + "]";
    }
  }
  return result;
}

// 清空长短信槽位
void clearConcatSlot(int slot) {
  concatBuffer[slot].inUse = false;
  concatBuffer[slot].receivedParts = 0;
  concatBuffer[slot].sender = "";
  concatBuffer[slot].timestamp = "";
  for (int j = 0; j < MAX_CONCAT_PARTS; j++) {
    concatBuffer[slot].parts[j].valid = false;
    concatBuffer[slot].parts[j].text = "";
  }
}

// 检查长短信超时并转发
void checkConcatTimeout() {
  unsigned long now = millis();
  for (int i = 0; i < MAX_CONCAT_MESSAGES; i++) {
    if (concatBuffer[i].inUse) {
      if (now - concatBuffer[i].firstPartTime >= CONCAT_TIMEOUT_MS) {
        logCaptureLn(String("⏰ 长短信超时，强制转发不完整消息"));
        logCaptureF("  参考号: %d, 已收到: %d/%d\n", 
                      concatBuffer[i].refNumber,
                      concatBuffer[i].receivedParts,
                      concatBuffer[i].totalParts);
        
        // 合并已收到的分段
        String fullText = assembleConcatSms(i);
        
        // 处理短信内容
        processSmsContent(concatBuffer[i].sender.c_str(), 
                         fullText.c_str(), 
                         concatBuffer[i].timestamp.c_str());
        
        // 清空槽位
        clearConcatSlot(i);
      }
    }
  }
}

// 读取串口一行（含回车换行），返回行字符串，无新行时返回空
String readSerialLine(HardwareSerial& port) {
  static char lineBuf[SERIAL_BUFFER_SIZE];
  static int linePos = 0;

  while (port.available()) {
    char c = port.read();
    if (c == '\n') {
      lineBuf[linePos] = 0;
      String res = String(lineBuf);
      linePos = 0;
      return res;
    } else if (c != '\r') {  // 跳过\r
      if (linePos < SERIAL_BUFFER_SIZE - 1)
        lineBuf[linePos++] = c;
      else
        linePos = 0;  //超长报错保护，重头计
    }
  }
  return "";
}

// 检查字符串是否为有效的十六进制PDU数据
bool isHexString(const String& str) {
  if (str.length() == 0) return false;
  for (unsigned int i = 0; i < str.length(); i++) {
    char c = str.charAt(i);
    if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))) {
      return false;
    }
  }
  return true;
}

// 检查发送者是否在号码黑名单中
bool isInNumberBlackList(const char* sender) {
  if (config.numberBlackList.length() == 0) return false;

  String originalSender = String(sender);
  bool has86 = originalSender.startsWith("+86");
  String strippedSender = has86 ? originalSender.substring(3) : "";

  int listLen = (int)config.numberBlackList.length();

  int start = 0;
  while (start <= listLen) {
    int end = config.numberBlackList.indexOf('\n', start);
    if (end == -1) end = listLen;

    String line = config.numberBlackList.substring(start, end);
    line.trim();

    if (line.length() > 0 && (line.equals(originalSender) || (has86 && line.equals(strippedSender)))) {
      return true;
    }

    start = end + 1;
  }

  return false;
}

// 检查发送者是否为管理员
bool isAdmin(const char* sender) {
  if (config.adminPhone.length() == 0) return false;
  
  // 去除可能的国际区号前缀进行比较
  String senderStr = String(sender);
  String adminStr = config.adminPhone;
  
  // 去除+86前缀
  if (senderStr.startsWith("+86")) {
    senderStr = senderStr.substring(3);
  }
  if (adminStr.startsWith("+86")) {
    adminStr = adminStr.substring(3);
  }
  
  return senderStr.equals(adminStr);
}

// 处理管理员命令
void processAdminCommand(const char* sender, const char* text) {
  String cmd = String(text);
  cmd.trim();
  
  logCaptureLn(String("处理管理员命令: " + cmd));
  
  // 处理 SMS:号码:内容 命令
  if (cmd.startsWith("SMS:")) {
    int firstColon = cmd.indexOf(':');
    int secondColon = cmd.indexOf(':', firstColon + 1);
    
    if (secondColon > firstColon + 1) {
      String targetPhone = cmd.substring(firstColon + 1, secondColon);
      String smsContent = cmd.substring(secondColon + 1);
      
      targetPhone.trim();
      smsContent.trim();
      
      logCaptureLn(String("目标号码: " + targetPhone));
      logCaptureLn(String("短信内容: " + smsContent));
      
      bool success = sendSMS(targetPhone.c_str(), smsContent.c_str());
      
      // 发送邮件通知结果
      String subject = success ? "短信发送成功" : "短信发送失败";
      String body = "管理员命令执行结果:\n";
      body += "命令: " + cmd + "\n";
      body += "目标号码: " + targetPhone + "\n";
      body += "短信内容: " + smsContent + "\n";
      body += "执行结果: " + String(success ? "成功" : "失败");
      
      sendEmailNotification(subject.c_str(), body.c_str());
    } else {
      logCaptureLn(String("SMS命令格式错误"));
      sendEmailNotification("命令执行失败", "SMS命令格式错误，正确格式: SMS:号码:内容");
    }
  }
  // 处理 RESET 命令
  else if (cmd.equals("RESET")) {
    logCaptureLn(String("执行RESET命令"));
    
    // 先发送邮件通知（因为重启后就发不了了）
    sendEmailNotification("重启命令已执行", "收到RESET命令，即将重启模组和ESP32...");
    
    // 重启模组
    resetModule();
    
    // 重启ESP32
    logCaptureLn(String("正在重启ESP32..."));
    delay(1000);
    ESP.restart();
  }
  else {
    logCaptureLn(String("未知命令: " + cmd));
  }
}

static String cachedOwnNumber = "";
String getOwnNumber() {
  if (cachedOwnNumber.length() > 0 && cachedOwnNumber != "\u672a\u65b6\u6216\u4e0d\u652f\u6301" && cachedOwnNumber != "\u672a\u77e5") {
    return cachedOwnNumber;
  }
  String resp = sendATCommand("AT+CNUM", 2000);
  if (resp.indexOf("+CNUM:") >= 0) {
    int idx = resp.indexOf(",\"");
    if (idx >= 0) {
      int endIdx = resp.indexOf("\"", idx + 2);
      if (endIdx > idx) {
        String num = resp.substring(idx + 2, endIdx);
        num.trim();
        if (num.length() > 0) {
          cachedOwnNumber = num;
          return num;
        }
      }
    }
  }
  cachedOwnNumber = "\u672a\u77e5"; // "未知"
  return cachedOwnNumber;
}

String getSmsCode(const String& smsText) {
  int len = smsText.length();
  int count = 0;
  int startIdx = -1;
  for (int i = 0; i <= len; i++) {
    char c = (i < len) ? smsText.charAt(i) : '\0';
    if (c >= '0' && c <= '9') {
      if (count == 0) {
        startIdx = i;
      }
      count++;
    } else {
      if (count >= 4 && count <= 8) {
        return smsText.substring(startIdx, startIdx + count);
      }
      count = 0;
    }
  }
  return "";
}

String formatTimestamp(const String& rawTs) {
  if (rawTs.length() >= 12) {
    bool allDigits = true;
    for (int i = 0; i < 12; i++) {
      char c = rawTs.charAt(i);
      if (c < '0' || c > '9') {
        allDigits = false;
        break;
      }
    }
    if (allDigits) {
      // Format: YYMMDDHHMMSS[TZ] -> YYYY-MM-DD HH:mm:ss
      String yy = "20" + rawTs.substring(0, 2);
      String mm = rawTs.substring(2, 4);
      String dd = rawTs.substring(4, 6);
      String hh = rawTs.substring(6, 8);
      String min = rawTs.substring(8, 10);
      String ss = rawTs.substring(10, 12);
      return yy + "-" + mm + "-" + dd + " " + hh + ":" + min + ":" + ss;
    }
  }
  
  if (rawTs.length() >= 17 && rawTs.charAt(2) == '/' && rawTs.charAt(5) == '/') {
    // yy/MM/dd,hh:mm:ss+zz -> YYYY-MM-DD HH:mm:ss
    String yy = "20" + rawTs.substring(0, 2);
    String mm = rawTs.substring(3, 5);
    String dd = rawTs.substring(6, 8);
    String hh = rawTs.substring(9, 11);
    String min = rawTs.substring(12, 14);
    String ss = rawTs.substring(15, 17);
    return yy + "-" + mm + "-" + dd + " " + hh + ":" + min + ":" + ss;
  }
  
  return rawTs;
}

// 处理最终的短信内容（管理员命令检查和转发）
void processSmsContent(const char* sender, const char* text, const char* timestamp, uint8_t modemIndex) {
  String cardTag = "[卡" + String(modemIndex) + "] ";
  logCaptureLn(String("=== 处理短信内容 ") + cardTag + "===");
  logCaptureLn(String("发送者: " + String(sender)));
  logCaptureLn(String("时间戳: " + String(timestamp)));
  logCaptureLn(String("内容: " + String(text)));
  logCaptureLn(String("===================="));

  // 检查是否在号码黑名单中
  if (isInNumberBlackList(sender)) {
    logCaptureLn(String("发送者在号码黑名单中，忽略该短信"));
    return;
  }

  // 检查是否为管理员命令
  if (isAdmin(sender)) {
    logCaptureLn(String("收到管理员短信，检查命令..."));
    String smsText = String(text);
    smsText.trim();
    
    // 检查是否为命令格式
    if (smsText.startsWith("SMS:") || smsText.equals("RESET")) {
      processAdminCommand(sender, text);
      // 命令已处理，不再发送普通通知邮件
      return;
    }
  }

  // 发送通知http（推送到所有启用的通道）
  sendSMSToServer(sender, text, timestamp);
  
  // 发送通知邮件
  String smsText = String(text);
  String senderStr = String(sender);
  senderStr.trim();
  
  String code = getSmsCode(smsText);
  String subject = "";
  if (code.length() > 0) {
    subject = cardTag + String("验证码：") + code;
  } else {
    String suffix = "";
    if (senderStr.length() >= 4) {
      suffix = senderStr.substring(senderStr.length() - 4);
    } else {
      suffix = senderStr;
    }
    subject = cardTag + suffix + String("的短信");
  }
  
  String body = smsText + "\n\n";
  body += String("来自：") + senderStr + "\n";
  body += String("接收：[模组") + String(modemIndex) + " / 卡" + String(modemIndex) + "]\n";
  body += String("时间：") + formatTimestamp(String(timestamp));
  
  sendEmailNotification(subject.c_str(), body.c_str());
}

// 处理特定模组的 URC 和 PDU
void checkSerialURC(uint8_t modemIndex) {
  enum UrcState { IDLE, WAIT_PDU };
  static UrcState state1 = IDLE;
  static UrcState state2 = IDLE;

  UrcState& state = (modemIndex == 2) ? state2 : state1;
  HardwareSerial& serial = (modemIndex == 2) ? Serial2 : Serial1;

  String line = readSerialLine(serial);
  if (line.length() == 0) return;

  // 打印到调试串口
  logCaptureLn(String("Debug[模组") + String(modemIndex) + "]> " + line);

  if (state == IDLE) {
    // 检测到短信上报URC头
    if (line.startsWith("+CMT:")) {
      logCaptureLn(String("模组 ") + String(modemIndex) + " 检测到+CMT，等待PDU数据...");
      state = WAIT_PDU;
    }
  } else if (state == WAIT_PDU) {
    // 跳过空行
    if (line.length() == 0) {
      return;
    }
    
    // 如果是十六进制字符串，认为是PDU数据
    if (isHexString(line)) {
      logCaptureLn(String("模组 ") + String(modemIndex) + " 收到PDU数据: " + line);
      logCaptureLn(String("PDU长度: " + String(line.length()) + " 字符"));
      
      // 解析PDU
      if (!pdu.decodePDU(line.c_str())) {
        logCaptureLn(String("❌ 模组 ") + String(modemIndex) + " PDU解析失败！");
      } else {
        logCaptureLn(String("✓ 模组 ") + String(modemIndex) + " PDU解析成功");
        logCaptureLn(String("=== 模组 ") + String(modemIndex) + " 短信内容 ===");
        logCaptureLn(String("发送者: " + String(pdu.getSender())));
        logCaptureLn(String("时间戳: " + String(pdu.getTimeStamp())));
        logCaptureLn(String("内容: " + String(pdu.getText())));
        
        // 获取长短信信息
        int* concatInfo = pdu.getConcatInfo();
        int refNumber = concatInfo[0];
        int partNumber = concatInfo[1];
        int totalParts = concatInfo[2];
        
        logCaptureF("模组 %d 长短信信息: 参考号=%d, 当前=%d, 总计=%d\n", modemIndex, refNumber, partNumber, totalParts);
        logCaptureLn(String("==============="));

        // 判断是否为长短信
        if (totalParts > 1 && partNumber > 0) {
          logCaptureF("📧 收到长短信分段 %d/%d (模组%d)\n", partNumber, totalParts, modemIndex);
          
          int slot = findOrCreateConcatSlot(refNumber, pdu.getSender(), totalParts);
          
          int partIndex = partNumber - 1;
          if (partIndex >= 0 && partIndex < MAX_CONCAT_PARTS) {
            if (!concatBuffer[slot].parts[partIndex].valid) {
              concatBuffer[slot].parts[partIndex].valid = true;
              concatBuffer[slot].parts[partIndex].text = String(pdu.getText());
              concatBuffer[slot].receivedParts++;
              
              if (concatBuffer[slot].receivedParts == 1) {
                concatBuffer[slot].timestamp = String(pdu.getTimeStamp());
              }
              
              logCaptureF("  已缓存分段 %d，当前已收到 %d/%d\n", 
                           partNumber, 
                           concatBuffer[slot].receivedParts, 
                           totalParts);
            } else {
              logCaptureF("  ⚠️ 分段 %d 已存在，跳过\n", partNumber);
            }
          }
          
          if (concatBuffer[slot].receivedParts >= totalParts) {
            logCaptureLn(String("✅ 长短信已收齐，开始合并转发"));
            
            String fullText = assembleConcatSms(slot);
            
            processSmsContent(concatBuffer[slot].sender.c_str(), 
                             fullText.c_str(), 
                             concatBuffer[slot].timestamp.c_str(),
                             modemIndex);
            
            clearConcatSlot(slot);
          }
        } else {
          // 普通短信，直接处理
          processSmsContent(pdu.getSender(), pdu.getText(), pdu.getTimeStamp(), modemIndex);
        }
      }
      
      state = IDLE;
    } 
    else {
      logCaptureLn(String("模组 ") + String(modemIndex) + " 收到非PDU数据，返回IDLE状态");
      state = IDLE;
    }
  }
}

void checkSerial1URC() {
  checkSerialURC(1);
}

void checkSerial2URC() {
  checkSerialURC(2);
}
