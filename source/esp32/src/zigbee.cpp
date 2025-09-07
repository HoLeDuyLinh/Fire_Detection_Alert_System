#include "zigbee.h"
#include <ArduinoJson.h>
#include "MQTT.h"
HardwareSerial zigbee(1);

void zigbee_init()
{
    zigbee.begin(115200, SERIAL_8N1, UART_RX, UART_TX);
}

void zigbee_send(String payload)
{
    zigbee.println(payload);
}

String zigbee_recive()
{
    String data = "\0";
    if(zigbee.available())
    {

        while (zigbee.available())
        {
            char c = zigbee.read();
            if(c == '\n'){
                break;
            }
            data += c;
            // In ra Serial monitor
        }
    }
    return data;
}

void zigbee_flush(){
    zigbee.flush();
}