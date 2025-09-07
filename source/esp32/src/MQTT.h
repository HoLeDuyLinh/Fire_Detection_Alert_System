#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#define SENSOR_TOPIC "esp32/sensors"
#define LIMIT_VALUE_TOPIC "esp32/limit_value"
#define THRESHOLD_VALUE_TOPIC "esp32/threshold"
#define PHONE_NUMBER_TOPIC "esp32/Phone"
#define CURRENT_PHONE_NUMBER "esp32/Current_Phone"
#define WARING_TOPIC "esp32/Warning"

void mqtt_init(WiFiClient& wifiClient, const char* server, uint16_t port, const char* clientId);
void mqtt_loop();                 // Gọi trong loop()
void mqtt_connect();             // Thử kết nối nếu mất
bool mqtt_is_connected();        // Kiểm tra trạng thái MQTT
void mqtt_publish(const char* topic, const char* payload);
void mqtt_publish_retained(const char* topic, const char* payload);
void mqtt_subscribe(const char* topic);
void mqtt_set_callback(MQTT_CALLBACK_SIGNATURE);

void mqtt_set_auth(const char* user, const char* pass);
#endif
