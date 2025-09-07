
#include "FileSystemManager.h"

bool initFS()
{
    if (SPIFFS.begin() != true)
    {
        SPIFFS_LOG("Init SPIFFS Fail");
        return false;
    }
    else
    {
        SPIFFS_LOG("Init SPIFFS Success");
        return true;
    }
}

void writeFile(const char *path, const char *message)
{
    SPIFFS.begin();
    File file = SPIFFS.open(path, FILE_WRITE);

    if (!file)
    {
        SPIFFS_LOG("Failed to open file for writing");
        return;
    }

    if (file.print(message))
    {
        SPIFFS_LOG("File written successfully");
    }
    else
    {
        SPIFFS_LOG("Failed to write file");
    }
    file.close(); // Don't forget to close the file
}

String readFile(fs::SPIFFSFS &fs, const char *path)
{
    SPIFFS_LOG("Reading file: %s\r\n", path);

    File file = fs.open(path);
    if (!file || file.isDirectory())
    {
        SPIFFS_LOG("- failed to open file for reading");
        return String();
    }

    String fileContent;
    while (file.available())
    {
        fileContent = file.readStringUntil('\n').c_str();
        break;
    }
    file.close();
    return fileContent;
}
