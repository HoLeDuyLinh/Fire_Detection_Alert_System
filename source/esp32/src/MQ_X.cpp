#include "MQ_X.h"
#include "LiquidCrystal_I2C.h"

MQSensor::MQSensor(uint8_t analogPin, float RL_kOhm, float R0_kOhm, float vADC)
    : _pin(analogPin), _RL(RL_kOhm), _R0(R0_kOhm), _vADC(vADC) {}

void MQSensor::begin()
{
    pinMode(_pin, INPUT);
}

float MQSensor::readVoltage()
{
    int adc = analogRead(_pin);
    return (adc / 4095.0) * 3.0;
}

float MQSensor::readRs()
{
    float Vout = readVoltage();
    return ((_vADC - Vout) * _RL) / Vout;
}

float MQSensor::readRatio()
{
    return readRs() / _R0;
}

float MQSensor::readPPM()
{
    float ratio = readRatio();
    if (ratio <= 0 || isnan(ratio) || isinf(ratio) || ratio > 1000.0)
    {
        return 0;
    }

    float ppm = A * pow(ratio, B);

    if (isnan(ppm) || isinf(ppm) || ppm < 0)
    {
        return 0;
    }
    ppm = constrain(ppm, 0, 9999);
    return ppm;
}

float MQSensor::getR0()
{
    return _R0;
}

void MQSensor::set_A(float A_value)
{
    this->A = A_value;
}

void MQSensor::set_B(float B_value)
{
    this->B = B_value;
}

void MQSensor::calibrate(int seconds)
{
    Serial.println("Calibrating sensor in clean air...");
    float rsTotal = 0;
    int samples = seconds;

    for (int i = 0; i < samples; i++)
    {
        float rs = readRs();
        rsTotal += rs;
        lcd.setCursor(0, 2);
        lcd.print("RS:");
        lcd.setCursor(4, 2);
        lcd.print(rs);

        float vout = readVoltage();
        lcd.setCursor(0, 3);
        lcd.print("ADC:");
        lcd.print(vout);

        delay(1000);
    }

    float rsAvg = rsTotal / samples;
    _R0 = rsAvg / 27.5;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Calibration complete");
    delay(1000);
    lcd.clear();
}