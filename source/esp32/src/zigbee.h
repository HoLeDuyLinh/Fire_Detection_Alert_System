#pragma once
#include <Arduino.h>

#define UART_TX 14  // TX to Zigbee
#define UART_RX 27  // RX from Zigbee

extern HardwareSerial zigbee;


void zigbee_init();
void zigbee_send(String payload);
String zigbee_recive();
void zigbee_flush();