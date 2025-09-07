#include "MQTT.h"



static PubSubClient mqttClient;
static const char* mqtt_server = nullptr;
static uint16_t mqtt_port = 1883;
static const char* mqtt_client_id = nullptr;

static unsigned long lastReconnectAttempt = 0;

static const char* mqtt_username = nullptr;
static const char* mqtt_password = nullptr;

void mqtt_set_auth(const char* user, const char* pass) {
    mqtt_username = user;
    mqtt_password = pass;
}


void mqtt_callback_default(char* topic, byte* payload, unsigned int length) {
    // // Serial.print("Message [");
    // // Serial.print(topic);
    // // Serial.print("]: ");
    // for (unsigned int i = 0; i < length; i++) {
    //     Serial.print((char)payload[i]);
    // }
    // Serial.println();
}

void mqtt_init(WiFiClient& wifiClient, const char* server, uint16_t port, const char* clientId) {
    mqtt_server = server;
    mqtt_port = port;
    mqtt_client_id = clientId;

    mqttClient.setClient(wifiClient);
    mqttClient.setServer(mqtt_server, mqtt_port);
    mqttClient.setCallback(mqtt_callback_default);
}

void mqtt_loop() {
    if (!mqttClient.connected()) {
        unsigned long now = millis();
        if (now - lastReconnectAttempt > 60000) {  // 60 giây
            lastReconnectAttempt = now;
            mqtt_connect();
        }
    } else {
        mqttClient.loop();
    }
}

void mqtt_connect() {
    if (mqtt_username && mqtt_password) {
        if (mqttClient.connect(mqtt_client_id, mqtt_username, mqtt_password)) {
            // Serial.println("[MQTT] Connected with auth.");
        } else {
            // Serial.print("[MQTT] Auth failed, rc=");
            // Serial.println(mqttClient.state());
        }
    } else {
        if (mqttClient.connect(mqtt_client_id)) {
            // Serial.println("[MQTT] Connected without auth.");
        } else {
            // Serial.print("[MQTT] Failed, rc=");
            // Serial.println(mqttClient.state());
        }
    }

    // Nếu kết nối thành công, bạn có thể sub lại các topic
    if (mqttClient.connected()) {
        mqttClient.subscribe(PHONE_NUMBER_TOPIC);
        mqttClient.subscribe(THRESHOLD_VALUE_TOPIC);
    }
}


bool mqtt_is_connected() {
    return mqttClient.connected();
}

void mqtt_publish(const char* topic, const char* payload) {
    if (!mqttClient.connected()) {
        // Serial.println("[MQTT] No connected, skip publish.");
        return;
    }

    if (topic == nullptr || payload == nullptr) {
        // Serial.println("[MQTT] Topic or Payload is nullptr!");
        return;
    }

    if (strlen(payload) > 200) {  
        // Serial.printf("[MQTT] Payload too long: %u bytes\n", strlen(payload));
        return;
    }

    bool ok = mqttClient.publish(topic, payload);
    if (!ok) {
        // Serial.println("[MQTT] Publish fail.");
    }
}


void mqtt_publish_retained(const char* topic, const char* payload) {
    if (!mqttClient.connected()) {
        return;
    }
    if (topic == nullptr || payload == nullptr) {
        return;
    }
    if (strlen(payload) > 200) {
        return;
    }
    // QoS 0, retained = true
    mqttClient.publish(topic, payload, true);
}






void mqtt_subscribe(const char* topic) {
    if (mqttClient.connected()) {
        mqttClient.subscribe(topic);
    }
}

void mqtt_set_callback(MQTT_CALLBACK_SIGNATURE) {
    mqttClient.setCallback(callback);
}
