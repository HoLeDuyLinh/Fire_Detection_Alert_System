#ifndef SIM_A7680C_H
#define SIM_A7680C_H
#include <Arduino.h>

#define DEFAULT_PHONE_NUMBER "0901368614"

bool sim_init(Stream &serial);  
bool sim_send_sms(const char* message);  
void sim_set_phone_number(const char* phone);  
bool sim_wait_response(const char* expected, unsigned long timeout = 2000); 



#endif
