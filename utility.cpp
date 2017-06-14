// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <time.h>

#include "AzureIotHub.h"
#include "Arduino.h"
#include "config.h"
#include "utility.h"

static IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle;
static HTS221Sensor *ht_sensor;
static RGB_LED rgbLed;

void showC2DMessageReceived(const char * message)
{
    Screen.clean();
    Screen.print(message);
}

void blinkLED()
{
    for (int i = 0; i < 5; i++)
    {
        rgbLed.turnOff();
        delay(500);
        rgbLed.setColor(RGB_LED_BRIGHTNESS, 0, 0);
        delay(500);
    }
    rgbLed.turnOff();
}

static IOTHUBMESSAGE_DISPOSITION_RESULT c2dMessageCallback(IOTHUB_MESSAGE_HANDLE message, void *userContextCallback)
{
    const char *buffer;
    size_t size;

    if (IoTHubMessage_GetByteArray(message, (const unsigned char **)&buffer, &size) != IOTHUB_MESSAGE_OK)
    {
        LogError("IoTHubMessage_GetByteArray Failed");
        return IOTHUBMESSAGE_REJECTED;
    }
    else
    {
        char *temp = (char *)malloc(size + 1);
        if (temp == NULL)
        {
            LogError("Failed to malloc for command");
            return IOTHUBMESSAGE_REJECTED;
        }
        memcpy(temp, buffer, size);
        temp[size] = '\0';
        LogInfo("Receive C2D message: %s", temp);
        showC2DMessageReceived(temp);
        blinkLED();
        free(temp);
        return IOTHUBMESSAGE_ACCEPTED;
    }
}

void iothubInit()
{
    srand((unsigned int)time(NULL));

    if (platform_init() != 0)
    {
        LogError("Failed to initialize the platform.");
        return;
    }

    if ((iotHubClientHandle = IoTHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, MQTT_Protocol)) == NULL)
    {
        LogError("Error: iotHubClientHandle is NULL!");
        return;
    }

    if (IoTHubClient_LL_SetOption(iotHubClientHandle, "TrustedCerts", certificates) != IOTHUB_CLIENT_OK)
    {
        LogError("Failed to set option \"TrustedCerts\"");
        return;
    }

    if (IoTHubClient_LL_SetMessageCallback(iotHubClientHandle, c2dMessageCallback, NULL) != IOTHUB_CLIENT_OK)
    {
        LogError("IoTHubClient_LL_SetMessageCallback FAILED!");
        return;
    }
}

void iothubLoop(void)
{
    IoTHubClient_LL_DoWork(iotHubClientHandle);
}
