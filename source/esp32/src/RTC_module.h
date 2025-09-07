#pragma once
#include <RTClib.h>
#include "NTP_module.h"


bool updateRTC();

DateTime getRTC();

String getDateString();

String getTimeString();
