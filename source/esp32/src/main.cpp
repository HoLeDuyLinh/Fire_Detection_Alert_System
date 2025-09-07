#include "main.h"
#include <WiFiClientSecure.h>
#include <WiFiManager.h>

//----------------------------------------------------------------
// KHAI BÁO BIẾN VÀ ĐỐI TƯỢNG
//----------------------------------------------------------------
LiquidCrystal_I2C lcd(0x27, 20, 4);
MQSensor gasSensor(GAS_PIN, 10.0, 1.0);
MQSensor coSensor(CO_PIN, 10.0, 1.0);
#define NUM_BUTTONS 7
const uint8_t buttonPins[NUM_BUTTONS] = {13, 23, 32, 33, 19, 18, 5};
Bounce debouncers[NUM_BUTTONS];
String sys_time;
WiFiClientSecure secureClient;
bool last_fire_state = 0;
bool send_sms_request = false;
uint8_t fire_state = 1;
uint32_t uptime = 0;
volatile bool buttonInterruptFlag = false;
long sensorPrevious = 0;
long mqttPrevious = 0;
unsigned long lastSensorSmsTime = 0;
unsigned long lastPiSmsTime = 0;
String lastSentFireLevel = "";
float temp, humi, ambient, co, gas, lux;
volatile unsigned long lastInterrupt1Time = 0;
volatile unsigned long lastInterrupt2Time = 0;
int pointer_index = 0;
int selectedIndex = 0;
const int menuSize = 6;
const char *menu_item[6] = {"CO", "GAS", "Temp", "Humi", "AMB", "OBJ"};
warning_value_t wn_value = {0};
sensor_value_t sensor_value = {0};
screen_index_t screen_index = screen_1;
screen_index_t last_sreen_index = screen_2;
button_t button;
String payload_data;
String last_fire_level;
String pi_warning_message = "";
char phone_number[12] = "";

//----------------------------------------------------------------
// KHAI BÁO NGUYÊN MẪU HÀM (FUNCTION PROTOTYPES)
//----------------------------------------------------------------
void sensor_init();
button_t handlerButton();
void get_waring_value();
void screen1();
void screen2();
void setting_screen();
void screen_warning_value();
void display_warning_value_Menu();
void screen_wifi();
void printFireState();
void handler_selecter_warning_value(uint8_t selectedIndex);
void handle_warning_signal_from_pi(String fire_level);
void display_pi_warning_screen();
void handle_update_threshold(String json_payload);
void edit_limit(float &value, const char *text, const char *unit, screen_index_t screen_index);
void handle_Reach_Limit_Value();
void hard_reset();
bool setWifiByCode(const char* ssid, const char* password);
void my_mqtt_callback(char *topic, byte *payload, unsigned int length);

//----------------------------------------------------------------
// CÁC HÀM TIỆN ÍCH
//----------------------------------------------------------------
static String buildSensorJson() {
    JsonDocument doc;
    String payload_data;
    doc["humidity"] = sensor_value.humi_value;
    doc["temperature"] = sensor_value.temp_value;
    doc["co_level"] = sensor_value.CO_value;
    doc["gas_level"] = sensor_value.gas_value;
    doc["flame"] = fire_state;
    doc["ambient_temp"] = sensor_value.mlx90614_temperature;
    doc["object_temp"] = sensor_value.mlx90614_object;
    serializeJson(doc, payload_data);
    return payload_data;
}

static String buildLimitValueJson() {
    JsonDocument doc;
    String payload_data;
    doc["humidity_limit"] = wn_value.humi_waring_value;
    doc["temperature_limit"] = wn_value.temp_waring_value;
    doc["co_level_limit"] = wn_value.CO_warning_value;
    doc["gas_level_limit"] = wn_value.gas_waring_value;
    doc["ambient_temp_limit"] = wn_value.mlx90614_ambient_waring_value;
    doc["object_temp_limit"] = wn_value.mlx90614_object_waring_value;
    serializeJson(doc, payload_data);
    return payload_data;
}

//----------------------------------------------------------------
// TÁC VỤ (TASKS)
//----------------------------------------------------------------
void taskCore1(void *parameter) {
    while (1) {
        if (WiFi.status() == WL_CONNECTED) {
            OTA_loop();
        }
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

void taskCore2(void *parameter) {
    while (1) {
        if (send_sms_request) {
            send_sms_request = false;
            if (millis() - lastSensorSmsTime > 45000) {
                String sms_text = buildSensorJson();
                if (sim_send_sms(sms_text.c_str())) {
                    Serial.println("SMS Sent Done (Sensor Alert)");
                    lastSensorSmsTime = millis();
                } else {
                    Serial.println("SMS Sent Fail (Sensor Alert)");
                }
            } else {
                Serial.println("Sensor SMS spam protection: Waiting...");
            }
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

//----------------------------------------------------------------
// HÀM SETUP CHÍNH
//----------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    for (int i = 0; i < NUM_BUTTONS; i++) {
        pinMode(buttonPins[i], INPUT_PULLUP);
        debouncers[i].attach(buttonPins[i]);
        debouncers[i].interval(20);
    }
    lcd.init();
    lcd.backlight();
    lcd.clear();
    if (initFS()) {
        lcd.print("FS Init Success");
    } else {
        lcd.print("FS Init Fail");
    }
    delay(1000);
    lcd.clear();
    SIM_SERIAL.begin(115200, SERIAL_8N1, SIM_RX, SIM_TX);
    memcpy(phone_number, (readFile(SPIFFS, "/phone_number.bin")).c_str(), 12);
    if (sim_init(SIM_SERIAL)) {
        lcd.print("SIM Init Success");
    } else {
        lcd.print("SIM Init Fail");
    }
    delay(1000);
    if (strlen(phone_number) < 9) {
        memcpy(phone_number, (const char *)DEFAULT_PHONE_NUMBER, strlen(DEFAULT_PHONE_NUMBER));
    }
    sim_set_phone_number(phone_number);
    WiFiManager wm;
    wm.setConfigPortalTimeout(60);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Connecting WiFi...");
    lcd.setCursor(0, 1);
    lcd.print("Please wait...");
    delay(1000);
    
    if (!wm.autoConnect("ESP-WiFi-Setup")) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("WiFi Failed!");
        lcd.setCursor(0, 1);
        lcd.print("Enter AP Config");
        lcd.setCursor(0, 2);
        lcd.print("192.168.4.1");
        lcd.setCursor(0, 3);
        lcd.print("");
        delay(3000);
        
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Entering OFFLINE mode");
        delay(2000);
    } else {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("WiFi Connected!");
        lcd.setCursor(0, 1);
        lcd.print("IP: ");
        lcd.print(WiFi.localIP());
        lcd.setCursor(0, 2);
        lcd.print("SSID: ");
        lcd.print(WiFi.SSID());
        delay(3000);
    }
    if (WiFi.status() == WL_CONNECTED) {
        secureClient.setInsecure();
        mqtt_init(secureClient, "bd92efa291a44e2497fa3e60484e25c3.s1.eu.hivemq.cloud", 8883, "esp123456");
        mqtt_set_auth("linh_mqtt", "Linhsama22122003");
        mqtt_set_callback(my_mqtt_callback);
        mqtt_connect();
        mqtt_subscribe(PHONE_NUMBER_TOPIC);
        mqtt_subscribe(THRESHOLD_VALUE_TOPIC);
        syncNTPTime();
    }
    updateRTC();
    zigbee_init();
    get_waring_value();
    sensor_init();
    beep_init();
    xTaskCreatePinnedToCore(taskCore1, "OTA Task", 4096, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(taskCore2, "SMS Task", 2048, NULL, 1, NULL, 1);
    lcd.clear();
}

//----------------------------------------------------------------
// HÀM LOOP CHÍNH
//----------------------------------------------------------------
void loop() {
    // Luôn đọc tín hiệu nút bấm ở mỗi vòng lặp
    button = handlerButton();

    // Xử lý nhận tin nhắn từ Pi trước tiên
    String zigbee_payload = zigbee_recive();
    zigbee_payload.trim();
    if (zigbee_payload.length() > 0) {
        Serial.println("Received from Pi: " + zigbee_payload);
        if (zigbee_payload.startsWith("WIFI_SET:")) {
            String credentials = zigbee_payload.substring(9);
            int commaIndex = credentials.indexOf(',');
            if (commaIndex != -1) {
                String newSsid = credentials.substring(0, commaIndex);
                String newPassword = credentials.substring(commaIndex + 1);
                if (setWifiByCode(newSsid.c_str(), newPassword.c_str())) {
                    ESP.restart();
                }
            }
        } else { // Xử lý tin nhắn cảnh báo cấp độ cháy từ Pi
            // Chỉ cập nhật nếu có tin nhắn level mới
            if (pi_warning_message != zigbee_payload) {
                pi_warning_message = zigbee_payload;
                screen_index = screen_pi_warning; // Chuyển sang trạng thái màn hình cảnh báo
                beep_on(); // Bật còi cảnh báo
            }
            
            if (zigbee_payload != lastSentFireLevel) {
                if (millis() - lastPiSmsTime > 10000) { // 10 giây
                    if(sim_send_sms(("Fire Spread Level: " + zigbee_payload).c_str())) {
                        lastPiSmsTime = millis();
                        lastSentFireLevel = zigbee_payload;
                        Serial.println("Sent new fire level SMS: " + zigbee_payload);
                    }
                } else {
                    Serial.println("Pi SMS 10s delay: Waiting...");
                }
            }
        }
        zigbee_flush();
    }

    // Xử lý thay đổi màn hình bằng nút bấm (chỉ khi không ở màn hình cảnh báo)
    if (screen_index != screen_pi_warning) {
        if (button == BUTTON_1) screen_index = screen_1;
        else if (button == BUTTON_2) screen_index = screen_2;
        else if (button == BUTTON_3) screen_index = screen_setting;
    }

    // *** SỬA LỖI HIỂN THỊ TRẮNG: Logic quản lý màn hình mới ***
    if (last_sreen_index != screen_index) {
        // Chỉ clear LCD khi không phải màn hình Pi warning (đã tự clear)
        if (screen_index != screen_pi_warning) {
            lcd.clear();
        }
        if (screen_index != screen_setting && screen_index != screen_pi_warning) {
            last_sreen_index = screen_index;
        }
    }

    // Luồng chính điều khiển màn hình
    switch (screen_index) {
        case screen_1: screen1(); break;
        case screen_2: screen2(); break;
        case screen_setting: setting_screen(); break;
        case screen_pi_warning:
            display_pi_warning_screen(); // Tự động quay về sau 5 giây
            break;
        default: break;
    }

    // Các tác vụ nền
    long now = millis();
    if (now - sensorPrevious > SENSOR_INTERVAL) {
        sensorPrevious = now;
        sys_time = getTimeString();
        sensor_value.temp_value = dht_getTemp();
        sensor_value.humi_value = dht_getHumi();
        sensor_value.CO_value = coSensor.readPPM();
        sensor_value.gas_value = gasSensor.readPPM();
        sensor_value.mlx90614_temperature = mlx.readAmbientTempC();
        sensor_value.mlx90614_object = mlx.readObjectTempC();
    }

    handle_Reach_Limit_Value();

    if (WiFi.status() == WL_CONNECTED) {
        if (millis() - mqttPrevious > MQTT_INTERVAL) {
            mqttPrevious = millis();
            mqtt_publish(SENSOR_TOPIC, buildSensorJson().c_str());
            JsonDocument doc;
            String phone_json;
            doc["Phone"] = phone_number;
            serializeJson(doc, phone_json);
            mqtt_publish(CURRENT_PHONE_NUMBER, phone_json.c_str());
            char uptime_string[20];
            snprintf(uptime_string, sizeof(uptime_string), "%lu", uptime);
            mqtt_publish("esp32/Uptime", uptime_string);
            uptime++;
        }
        mqtt_loop();
    }
    
    if (fire_state == NO_FIRE && screen_index != screen_pi_warning) {
        beep_off();
    }
}

//----------------------------------------------------------------
// ĐỊNH NGHĨA CÁC HÀM CÒN LẠI
//----------------------------------------------------------------
void my_mqtt_callback(char *topic, byte *payload, unsigned int length) {
    String message;
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    Serial.printf("Callback on topic: %s\nMessage: %s\n", topic, message.c_str());
    if (strcmp(topic, PHONE_NUMBER_TOPIC) == 0) {
        message.toCharArray(phone_number, 12);
        sim_set_phone_number(phone_number);
        writeFile("/phone_number.bin", phone_number);
    } else if (strcmp(topic, THRESHOLD_VALUE_TOPIC) == 0) {
        handle_update_threshold(message);
    }
}

void handleGetSensors() {
    server.send(200, "application/json", buildSensorJson());
}

void handle_update_threshold(String json_payload) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, json_payload);
    if (error) {
        Serial.print("JSON parse error: ");
        Serial.println(error.f_str());
        return;
    }
    wn_value.CO_warning_value = doc["co_level_limit"];
    wn_value.gas_waring_value = doc["gas_level_limit"];
    wn_value.humi_waring_value = doc["humidity_limit"];
    wn_value.mlx90614_ambient_waring_value = doc["ambient_temp_limit"];
    wn_value.mlx90614_object_waring_value = doc["object_threshold"];
    wn_value.temp_waring_value = doc["temperature_limit"];
    mqtt_publish_retained(LIMIT_VALUE_TOPIC, buildLimitValueJson().c_str());
    writeStruct("/warning_value.bin", wn_value);
}

void handle_warning_signal_from_pi(String fire_level) {
    // Chỉ thiết lập trạng thái cảnh báo, không vẽ LCD
    pi_warning_message = fire_level;
    screen_index = screen_pi_warning;
    beep_on();
}

void display_pi_warning_screen() {
    static unsigned long warningStartTime = 0;
    static unsigned long lastUpdate = 0;
    static String last_displayed_message = "";
    static bool warningActive = false;
    
    // Lần đầu vào màn hình cảnh báo hoặc có tin nhắn mới
    if (!warningActive || last_displayed_message != pi_warning_message) {
        warningStartTime = millis();
        warningActive = true;
        last_displayed_message = pi_warning_message;
        
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("> WARNING FROM PI! <");
        lcd.setCursor(0, 1);
        lcd.print("Fire Level:");
        lcd.setCursor(0, 2);
        lcd.print(pi_warning_message);
        lcd.setCursor(0, 3);
        lcd.print("Auto return in 5s...");
        
        Serial.println("Pi warning screen started");
    }
    
    // Tính toán thời gian còn lại
    unsigned long elapsed = (millis() - warningStartTime) / 1000;
    
    // Tự động quay về sau 5 giây
    if (elapsed >= 5) {
        Serial.println("Pi warning timeout - returning to previous screen: " + String(last_sreen_index));
        
        // Bảo vệ chống vòng lặp: nếu last_sreen_index là pi_warning thì về screen_1
        screen_index_t target_screen = last_sreen_index;
        if (target_screen == screen_pi_warning) {
            target_screen = screen_1;
            Serial.println("Warning: last_screen was pi_warning, defaulting to screen_1");
        }
        
        beep_off();
        warningActive = false; // Reset state
        lcd.clear();
        screen_index = target_screen;
        Serial.println("Switched to screen: " + String(screen_index));
        return;
    }
    
    // Cập nhật countdown mỗi giây
    if (millis() - lastUpdate > 1000) {
        lastUpdate = millis();
        unsigned long remaining = 5 - elapsed;
        
        lcd.setCursor(0, 3);
        lcd.print("Auto return in " + String(remaining) + "s   ");
        Serial.println("Countdown: " + String(remaining) + "s remaining");
    }
    
    // Duy trì còi báo động
    beep_on();
}

void handle_Reach_Limit_Value() {
    if (sensor_value.CO_value > wn_value.CO_warning_value || sensor_value.gas_value > wn_value.gas_waring_value || sensor_value.humi_value < wn_value.humi_waring_value || sensor_value.temp_value > wn_value.temp_waring_value || sensor_value.mlx90614_temperature > wn_value.mlx90614_ambient_waring_value || sensor_value.mlx90614_object > wn_value.mlx90614_object_waring_value || fs_is_fire_detect() == FIRE) {
        fire_state = fs_is_fire_detect();
        sensor_value.temp_value = dht_getTemp();
        sensor_value.humi_value = dht_getHumi();
        sensor_value.CO_value = coSensor.readPPM();
        sensor_value.gas_value = gasSensor.readPPM();
        sensor_value.mlx90614_temperature = mlx.readAmbientTempC();
        sensor_value.mlx90614_object = mlx.readObjectTempC();
        beep_on();
        send_sms_request = true;
        if (WiFi.status() == WL_CONNECTED) {
            mqtt_publish(WARING_TOPIC, buildSensorJson().c_str());
        }
    }
}

button_t handlerButton() {
    button_t button_number = NO_BUTTON_PRESS;
    for (int i = 0; i < NUM_BUTTONS; i++) {
        debouncers[i].update();
        if (debouncers[i].fell()) {
            button_number = (button_t)i;
            break;
        }
    }
    return button_number;
}

void printFireState() {
    fire_state = fs_is_fire_detect();
    if (fire_state == NO_FIRE) {
        lcd.setCursor(0, 3);
        lcd.print("State: No Fire      ");
        beep_off();
    } else {
        lcd.setCursor(0, 3);
        lcd.print("State: !! FIRE !!   ");
        beep_on();
    }
}

void screen1() {
    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate < 500) return;
    lastUpdate = millis();
    printFireState();
    lcd.setCursor(0, 0);
    lcd.print(sys_time);
    lcd.setCursor(0, 1);
    lcd.print("T:" + String(sensor_value.temp_value, 1) + " H:" + String(sensor_value.humi_value, 1) + "   ");
    lcd.setCursor(0, 2);
    lcd.print("CO:" + String(sensor_value.CO_value, 1) + " GAS:" + String(sensor_value.gas_value, 1) + "   ");
}

void screen2() {
    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate < 500) return;
    lastUpdate = millis();
    printFireState();
    lcd.setCursor(0, 0);
    lcd.print(sys_time);
    lcd.setCursor(0, 1);
    lcd.print("AMB: " + String(sensor_value.mlx90614_temperature, 1) + "    ");
    lcd.setCursor(0, 2);
    lcd.print("OBJ: " + String(sensor_value.mlx90614_object, 0) + "    ");
}

void setting_screen() {
    lcd.clear();
    lcd.setCursor(3, 0);
    lcd.print("Setting Menu");
    lcd.setCursor(0, 1);
    lcd.print(" Warning Value");
    lcd.setCursor(0, 2);
    lcd.print(" WiFi");
    lcd.setCursor(0, 3);
    lcd.print(" Hard Reset");
    while (screen_index == screen_setting) {
        button = handlerButton();
        if (button == BUTTON_7) {
            pointer_index = (pointer_index + 1) % 3;
        } else if (button == BUTTON_4) {
            pointer_index = (pointer_index - 1 + 3) % 3;
        } else if (button == BUTTON_3) {
            screen_index = last_sreen_index;
        }
        for (int i=0; i<3; i++) {
            lcd.setCursor(0, i + 1);
            lcd.print(i == pointer_index ? ">" : " ");
        }
        if (button == BUTTON_1) {
            if (pointer_index == 0) screen_warning_value();
            else if (pointer_index == 1) screen_wifi();
            else if (pointer_index == 2) hard_reset();
        }
    }
    lcd.clear();
}

void hard_reset() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Reseting...");
    WiFiManager wm;
    wm.resetSettings();
    wn_value = {0};
    memcpy(phone_number, "", sizeof(phone_number));
    writeStruct("/warning_value.bin", wn_value);
    writeFile("/phone_number.bin", phone_number);
    lcd.setCursor(0, 2);
    lcd.print("Reset Done. Rebooting...");
    delay(2000);
    ESP.restart();
}

void moveUp() {
    selectedIndex = (selectedIndex - 1 + menuSize) % menuSize;
    display_warning_value_Menu();
}

void moveDown() {
    selectedIndex = (selectedIndex + 1) % menuSize;
    display_warning_value_Menu();
}

void display_warning_value_Menu() {
    int start = selectedIndex - (selectedIndex % 3);
    lcd.clear();
    lcd.setCursor(2, 0);
    lcd.print("Warning Value");
    for (int i = 0; i < 3; i++) {
        int index = (start + i) % menuSize;
        lcd.setCursor(0, i + 1);
        lcd.print(index == selectedIndex ? ">" : " ");
        lcd.print(menu_item[index]);
    }
}

void screen_warning_value() {
    lcd.clear();
    display_warning_value_Menu();
    while (1) {
        button_t button = handlerButton();
        if (button == BUTTON_4) moveUp();
        else if (button == BUTTON_7) moveDown();
        else if (button == BUTTON_3) return setting_screen();
        else if (button == BUTTON_1) return handler_selecter_warning_value(selectedIndex);
    }
}

void handler_selecter_warning_value(uint8_t selectedIndex) {
    switch (selectedIndex) {
        case 0: edit_limit(wn_value.CO_warning_value, "CO", "ppm", co_edit); break;
        case 1: edit_limit(wn_value.gas_waring_value, "GAS", "ppm", gas_edit); break;
        case 2: edit_limit(wn_value.temp_waring_value, "Temp", "oC", temp_edit); break;
        case 3: edit_limit(wn_value.humi_waring_value, "Humi", "%", humi_edit); break;
        case 4: edit_limit(wn_value.mlx90614_ambient_waring_value, "AMB", "oC", mlx_ambient_edit); break;
        case 5: edit_limit(wn_value.mlx90614_object_waring_value, "Obj", "oC", mlx_object_edit); break;
    }
    return screen_warning_value();
}

void edit_limit(float &value, const char *text, const char *unit, screen_index_t index) {
    float temp_val = value;
    screen_index = index;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Current: " + String(value));
    lcd.setCursor(0, 1);
    lcd.print("Set " + String(text) + " Limit");
    lcd.setCursor(3, 3);
    lcd.print("<-  OK  ->");
    while (screen_index == index) {
        button_t button = handlerButton();
        lcd.setCursor(6, 2);
        lcd.print(String(temp_val) + " " + unit + "   ");
        if (button == BUTTON_3) screen_index = screen_setting;
        else if (button == BUTTON_5) temp_val -= 10.0;
        else if (button == BUTTON_6) temp_val += 10.0;
        else if (button == BUTTON_7) {
            value = temp_val;
            writeStruct("/warning_value.bin", wn_value);
            if (WiFi.status() == WL_CONNECTED) {
                mqtt_publish_retained(LIMIT_VALUE_TOPIC, buildLimitValueJson().c_str());
            }
            screen_index = screen_setting;
        }
    }
}

static void wifi_infor_screen() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("SSID: " + WiFi.SSID());
    lcd.setCursor(0, 2);
    lcd.print("IP: " + WiFi.localIP().toString());
    while (1) {
        if (handlerButton() == BUTTON_3) return screen_wifi();
    }
}

void screen_wifi() {
    lcd.clear();
    lcd.setCursor(5, 0);
    lcd.print("WiFi Menu");
    lcd.setCursor(1, 1);
    lcd.print("WiFi Info");
    lcd.setCursor(1, 2);
    lcd.print("Change WiFi");
    static uint8_t wifi_menu_index = 1;
    while (1) {
        button_t button = handlerButton();
        if (button == BUTTON_3) return setting_screen();
        else if (button == BUTTON_7 || button == BUTTON_4) {
            wifi_menu_index = (wifi_menu_index == 1) ? 2 : 1;
            lcd.setCursor(0, 1); lcd.print(" ");
            lcd.setCursor(0, 2); lcd.print(" ");
            lcd.setCursor(0, wifi_menu_index); lcd.print(">");
        } else if (button == BUTTON_1) {
            if (wifi_menu_index == 1) {
                return wifi_infor_screen();
            } else {
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("Starting Config...");
                lcd.setCursor(0, 1);
                lcd.print("Please wait...");
                delay(1000);
                
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("WiFi Failed!");
                lcd.setCursor(0, 1);
                lcd.print("Enter AP Config");
                lcd.setCursor(0, 2);
                lcd.print("192.168.4.1");
                
                WiFiManager wm;
                wm.setConfigPortalTimeout(60);
                wm.startConfigPortal("ESP-WiFi-Setup");
                
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("Config Complete");
                lcd.setCursor(0, 1);
                lcd.print("Rebooting...");
                delay(2000);
                ESP.restart();
            }
        }
    }
}

void sensor_init() {
    lcd.clear();
    lcd.setCursor(5, 0);
    lcd.print("Initializing Sensors");
    fs_init();
    dht_init();
    if (!mlx.begin()) {
        lcd.clear();
        lcd.print("Error: MLX90614");
        while(1);
    }
    lcd.setCursor(0, 0);
    lcd.print("Calibrating MQ2(10s)");
    gasSensor.begin();
    gasSensor.set_A(574.25);
    gasSensor.set_B(-2.222);
    gasSensor.calibrate(10);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Calibrating MQ7(10s)");
    coSensor.begin();
    coSensor.set_A(100);
    coSensor.set_B(-1.53916);
    coSensor.calibrate(10);
    lcd.clear();
    delay(1000);
}

void get_waring_value() {
    lcd.clear();
    if (readStruct("/warning_value.bin", wn_value)) {
        lcd.print("Read Config Success");
    } else {
        lcd.print("Read Config Fail");
    }
    delay(1000);
    if (wn_value.CO_warning_value <= 0.0 || wn_value.gas_waring_value <= 0.0) {
        lcd.clear();
        lcd.print("No Data. Set Default");
        delay(1000);
        wn_value.CO_warning_value = DEFAULT_CO_LIMIT;
        wn_value.gas_waring_value = DEFAULT_GAS_LIMIT;
        wn_value.humi_waring_value = DEFAULT_HUMI_LIMIT;
        wn_value.temp_waring_value = DEFAULT_TEMP_LIMIT;
        wn_value.mlx90614_ambient_waring_value = DEFAULT_MLX90614_AMB_LIMIT;
        wn_value.mlx90614_object_waring_value = DEFAULT_MLX90614_OBJ_LIMIT;
        writeStruct("/warning_value.bin", wn_value);
    }
    if (WiFi.status() == WL_CONNECTED) {
        mqtt_publish_retained(LIMIT_VALUE_TOPIC, buildLimitValueJson().c_str());
    }
}

bool setWifiByCode(const char* ssid, const char* password) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("New WiFi received:");
    lcd.setCursor(0, 1);
    lcd.print(ssid);
    delay(1500);
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Disconnecting...");
    Serial.printf("Attempting to connect to SSID: %s\n", ssid);
    WiFi.disconnect(true);
    delay(1000);
    WiFi.begin(ssid, password);
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - startTime > 20000) { // 20 giây timeout
            Serial.println("Connection Timeout!");
            lcd.setCursor(0, 3);
            lcd.print("Connection Failed!");
            delay(2000);
            return false;
        }
        Serial.print(".");
        lcd.print(".");
        delay(500);
    }
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Connected!");
    lcd.setCursor(0, 1);
    lcd.print(WiFi.localIP());
    delay(3000);
    return true;
}
