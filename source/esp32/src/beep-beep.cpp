#include "beep-beep.h"
#include "Arduino.h"

void beep_init(){
    pinMode(BEEP_PIN,OUTPUT);
}

void beep_on(){
    digitalWrite(BEEP_PIN,1);
}

void beep_off(){
    digitalWrite(BEEP_PIN,0);
}