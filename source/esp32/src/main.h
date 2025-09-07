#pragma once
#include <Arduino.h>
#if DEBUG
#define LOG(format, ...) Serial.printf("[DEBUG] " format "\n", ##__VA_ARGS__)
#else
#define LOG(format, ...)
#endif

#define PRINT_SPACE(x, y)               \
    do                                  \
    {                                   \
        lcd.setCursor(0, y);            \
        lcd.print(String(' ', 20 - x)); \
        lcd.setCursor(x, y);            \
    } while (0)
/*
File System Manager Library
*/
#include "FileSystemManager.h"
/*
Flame Sensor library
*/
#include "Flame_Sensor.h"
/*
Beep-Beep Library
*/
#include "beep-beep.h"
/*
DHT22 Library
*/
#include "DHT22.h"
/*
MQ_X Library
*/
#include "MQ_X.h"
#define GAS_PIN 35
#define CO_PIN 34

#define DEFAULT_CO_LIMIT 50
#define DEFAULT_GAS_LIMIT 50
#define DEFAULT_TEMP_LIMIT 60
#define DEFAULT_HUMI_LIMIT 30
#define DEFAULT_MLX90614_AMB_LIMIT 80
#define DEFAULT_MLX90614_OBJ_LIMIT 80
/*
LCD Library
*/
#include "LiquidCrystal_I2C.h"
/*
MLX90614 Library
*/
#include <Adafruit_MLX90614.h>
Adafruit_MLX90614 mlx = Adafruit_MLX90614();
/*
JSON Library
*/
#include "ArduinoJson.h"
/*
Zigbee Library
*/
#include "zigbee.h"
/*
NTP Library
*/
#include "NTP_module.h"
/*
RTC Library
*/
#include "RTC_module.h"
/*
Deboune Button Library
*/
#include <Bounce2.h>
/*
OTA Library
*/
#include "OTA.h"
/*
Module SIM
*/
#include "ModuleSIM.h"
#define SIM_SERIAL Serial2
#define SIM_TX 17
#define SIM_RX 16
/*
MQTT Library
*/
#include "MQTT.h"
#define MQTT_SERVER "bd92efa291a44e2497fa3e60484e25c3.s1.eu.hivemq.cloud"
#define MQTT_USERNAME "linh_mqtt"
#define MQTT_PASSWORD "Linhsama22122003"
#define MQTT_PORT 8883

#define SENSOR_INTERVAL 500
#define MQTT_INTERVAL 1000

// *** SỬA LỖI: Thêm screen_pi_warning vào enum ***
enum screen_index_t
{
    screen_1 = 0,
    screen_2,
    screen_setting,
    screen_pi_warning, // Màn hình cảnh báo từ Pi
    co_edit,
    gas_edit,
    temp_edit,
    humi_edit,
    mlx_ambient_edit,
    mlx_object_edit
};

enum button_t
{
    NO_BUTTON_PRESS = -1,
    BUTTON_1 = 0,
    BUTTON_2,
    BUTTON_3,
    BUTTON_4,
    BUTTON_5,
    BUTTON_6,
    BUTTON_7
};

typedef struct
{
    float CO_warning_value;
    float gas_waring_value;
    float temp_waring_value;
    float humi_waring_value;
    float mlx90614_ambient_waring_value;
    float mlx90614_object_waring_value;
} warning_value_t;

typedef struct
{
    float CO_value;
    float gas_value;
    float temp_value;
    float humi_value;
    float mlx90614_temperature;
    float mlx90614_object;
} sensor_value_t;

