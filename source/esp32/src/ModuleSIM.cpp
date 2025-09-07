#include "ModuleSIM.h"

static Stream* simSerial = nullptr;
static String sim_phone_number = "";

bool sim_wait_response(const char* expected, unsigned long timeout) {
  unsigned long start = millis();
  String response = "";
  while (millis() - start < timeout) {
    while (simSerial->available()) {
      char c = simSerial->read();
      response += c;
      if (response.indexOf(expected) != -1) {
        return true;
      }
    }
  }
  return false;
}

bool sim_init(Stream &serial) {
  simSerial = &serial;

  
  simSerial->println("AT");
  delay(200);
  sim_wait_response("OK");

  
  simSerial->println("ATE0");
  sim_wait_response("OK");

  
  simSerial->println("AT+CREG?");
  sim_wait_response("+CREG: 0,1");

  // Chọn SMS text mode và bộ ký tự GSM để tránh lỗi unicode
  simSerial->println("AT+CMGF=1");
  sim_wait_response("OK");
  simSerial->println("AT+CSCS=\"GSM\"");
  sim_wait_response("OK");
  simSerial->println("AT+CSMP=17,167,0,0"); // cấu hình SMS mặc định
  return true;
}

void sim_set_phone_number(const char* phone) {
  // Lọc chỉ chữ số, chấp nhận bắt đầu bằng ‘+’ cho quốc tế
  String raw = String(phone);
  String digits = "";
  for (size_t i = 0; i < raw.length(); ++i) {
    char c = raw[i];
    if ((c >= '0' && c <= '9') || (c == '+' && digits.length() == 0)) {
      digits += c;
    }
  }
  sim_phone_number = digits;
}

bool sim_send_sms(const char* message) {
  if (sim_phone_number.length() == 0 || simSerial == nullptr) {
    return false;  
  }

  simSerial->print("AT+CMGS=\"");
  simSerial->print(sim_phone_number);
  simSerial->println("\"");
  if (!sim_wait_response(">", 5000)) return false;

  simSerial->print(message);
  simSerial->write(26);  

  // Một số module trả về "+CMGS:" rồi mới "OK", hoặc mất thời gian dài hơn
  if (sim_wait_response("+CMGS", 20000)) return true;
  return sim_wait_response("OK", 20000);
}
