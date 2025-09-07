#include "RTC_module.h"

RTC_DS3231 rtc;

bool updateRTC() {
  if (!rtc.begin()) {
    // Serial.println("RTC init failed!");
    return false;
  }
  rtc.adjust(DateTime(getNtpEpoch()));
  return true;
}

DateTime getRTC() {
  return rtc.now();
}

String getDateString() {
  DateTime now = getRTC();
  char buf[20];
  snprintf(buf, sizeof(buf), "%04d-%02d-%02d", now.year(), now.month(), now.day());
  return String(buf);
}

String getTimeString() {
  DateTime now = getRTC();
  char buf[10];
  snprintf(buf, sizeof(buf), "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
  return String(buf);
}