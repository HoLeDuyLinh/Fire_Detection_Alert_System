#include "NTP_module.h"

#include <WiFiUdp.h>
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "time.nist.gov", 7 * 3600);

bool syncNTPTime()
{
    timeClient.begin();
    long now = millis();
    bool status = timeClient.update();
    while (!status)
    {   
        status = timeClient.forceUpdate();
        delay(100);
    }
    return status;
}

time_t getNtpEpoch()
{
    return timeClient.getEpochTime();
}