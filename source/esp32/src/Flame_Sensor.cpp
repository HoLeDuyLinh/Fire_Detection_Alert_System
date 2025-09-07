#include "Flame_Sensor.h"
#include "Arduino.h"
void fs_init(){
    pinMode(FS_PIN,INPUT);
}

bool fs_is_fire_detect(){
    bool status = NO_FIRE;
    if(digitalRead(FS_PIN) == FIRE){
        // Serial.println("Fire");
        status = FIRE;
    }

    return status;
}