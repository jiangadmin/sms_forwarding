#ifndef MODEM_H
#define MODEM_H

#include "globals.h"

HardwareSerial& getModemSerial(uint8_t modemIndex = 1);
int8_t getModemEnPin(uint8_t modemIndex = 1);

String sendATCommand(const char* cmd, unsigned long timeout, uint8_t modemIndex = 1);
void modemPowerCycle(uint8_t modemIndex = 1);
void resetModule(uint8_t modemIndex = 1);
void modemInit(uint8_t modemIndex = 1);
void modemInitAll();
bool sendATandWaitOK(const char* cmd, unsigned long timeout, uint8_t modemIndex = 1);
bool waitCEREG(uint8_t modemIndex = 1);
void blink_short(unsigned long gap_time = 500);
bool sendSMS(const char* phoneNumber, const char* message, uint8_t modemIndex = 1);

#endif
