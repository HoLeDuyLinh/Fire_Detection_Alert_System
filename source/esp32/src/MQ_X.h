#ifndef MQSENSOR_H
#define MQSENSOR_H
#include <Arduino.h>


class MQSensor
{
public:
  MQSensor(uint8_t analogPin, float RL_kOhm, float R0_kOhm, float vADC = 3.3);

  void begin();
  float readPPM();
  float readRatio();
  float readRs();
  float readVoltage();
  float getR0(); 
  void set_A(float A_value);
  void set_B(float B_value);
  void calibrate(int seconds = 5);
private:
  uint8_t _pin;
  float _RL;  
  float _R0;  
  float _vADC; 
  float A = 1000.0; 
  float B = -1.6;   
};

#endif
