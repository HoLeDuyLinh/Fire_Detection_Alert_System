#pragma once


#include <DHT.h>

#define DHTPIN 4

#define DHTTYPE DHT22

void dht_init();

float dht_getTemp();

float dht_getHumi();