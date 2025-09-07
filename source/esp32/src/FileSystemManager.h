#pragma once
#include <SPIFFS.h>
#if DEBUG_SPIFFS
#define SPIFFS_LOG(format, ...) Serial.printf("[SPIFFS] " format "\n", ##__VA_ARGS__)
#else
#define SPIFFS_LOG(format, ...) // Không làm gì nếu debug bị tắt
#endif
template <typename T>
bool writeStruct(const char *path, const T &data)
{
    File file = SPIFFS.open(path, FILE_WRITE);
    if (!file)
    {
        // Serial.println("Failed to open file for writing");
        return false;
    }

    file.write((uint8_t *)&data, sizeof(data)); 
    file.close();
    return true;
}
template <typename T>
bool readStruct(const char *path, T &data)
{
    File file = SPIFFS.open(path, FILE_READ);
    if (!file)
    {
        // Serial.println("Failed to open file for reading");
        return false;
    }

    file.read((uint8_t *)&data, sizeof(T)); // read sizeof data
    file.close();

    // if (bytesRead != sizeof(T)) {  //  compare with sizeof struct
    //     Serial.println("Failed to read complete struct");
    //     return false;
    // }

    return true;
}

bool initFS();
void writeFile(const char *path, const char *message);
String readFile(fs::SPIFFSFS &fs, const char *path);