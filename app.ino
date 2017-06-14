#include "AzureIotHub.h"
#include "AZ3166WiFi.h"
#include "config.h"
#include "utility.h"

static bool hasWifi = false;

void initWifi()
{
    Screen.print("Azure IoT DevKit\r\n \r\nConnecting...\r\n");

    if (WiFi.begin() == WL_CONNECTED)
    {
        IPAddress ip = WiFi.localIP();
        Screen.print(1, ip.get_address());
        hasWifi = true;
        Screen.print(2, "Running... \r\n");
    }
    else
    {
        Screen.print(1, "No Wi-Fi\r\n ");
    }
}

void setup()
{
    hasWifi = false;
    initWifi();
    if (!hasWifi)
    {
        LogInfo("Please make sure the wifi connected!");
        return;
    }

    Serial.begin(9600);
    iothubInit();
    Serial.println("Listening cloud to device message:");
}

void loop()
{
    iothubLoop();
    delay(10);
}
