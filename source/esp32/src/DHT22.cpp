#include "DHT22.h"

#include <Arduino.h>

DHT dht(DHTPIN, DHTTYPE);

void dht_init()
{
    dht.begin();
}

float dht_getTemp(){
    return dht.readTemperature();
}

float dht_getHumi(){
    return dht.readHumidity();
}