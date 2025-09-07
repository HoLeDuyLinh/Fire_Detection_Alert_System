#include "OTA.h"
#include "LiquidCrystal_I2C.h"
WebServer server(80);
unsigned long ota_progress_millis = 0;
void onOTAStart()
{

    Serial.println("OTA update started!");

}

void onOTAProgress(size_t current, size_t final)
{
    if (millis() - ota_progress_millis > 1000)
    {
        ota_progress_millis = millis();
        // int percent = (current/final*1.0)*100;
        Serial.printf("OTA Progress Current: %u bytes, Final: %u bytes\n", current, final);
    }
}

void onOTAEnd(bool success)
{
  
    if (success)
    {
        Serial.println("OTA update finished successfully!");
    }
    else
    {
        Serial.println("There was an error during OTA update!");
    }

}

void OTA_init()
{
    ElegantOTA.begin(&server); 

    ElegantOTA.onStart(onOTAStart);
    ElegantOTA.onProgress(onOTAProgress);
    ElegantOTA.onEnd(onOTAEnd);

    server.begin();
}

void OTA_loop()
{
    server.handleClient();
    ElegantOTA.loop();
}