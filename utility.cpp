// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <time.h>

#include "AzureIotHub.h"
#include "Arduino.h"
#include "config.h"
#include "utility.h"
#include "json.h"
#include "http_client.h"

static IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle;
static RGB_LED rgbLed;
static int msgStart;
static char msgBody[2048];
static int msgBodyLen;

//DigiCert High Assurance EV Root CA
const char GITHUB_CERT[] = 
"-----BEGIN CERTIFICATE-----\r\n"
"MIIDxTCCAq2gAwIBAgIQAqxcJmoLQJuPC3nyrkYldzANBgkqhkiG9w0BAQUFADBs\r\n"
"MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\r\n"
"d3cuZGlnaWNlcnQuY29tMSswKQYDVQQDEyJEaWdpQ2VydCBIaWdoIEFzc3VyYW5j\r\n"
"ZSBFViBSb290IENBMB4XDTA2MTExMDAwMDAwMFoXDTMxMTExMDAwMDAwMFowbDEL\r\n"
"MAkGA1UEBhMCVVMxFTATBgNVBAoTDERpZ2lDZXJ0IEluYzEZMBcGA1UECxMQd3d3\r\n"
"LmRpZ2ljZXJ0LmNvbTErMCkGA1UEAxMiRGlnaUNlcnQgSGlnaCBBc3N1cmFuY2Ug\r\n"
"RVYgUm9vdCBDQTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAMbM5XPm\r\n"
"+9S75S0tMqbf5YE/yc0lSbZxKsPVlDRnogocsF9ppkCxxLeyj9CYpKlBWTrT3JTW\r\n"
"PNt0OKRKzE0lgvdKpVMSOO7zSW1xkX5jtqumX8OkhPhPYlG++MXs2ziS4wblCJEM\r\n"
"xChBVfvLWokVfnHoNb9Ncgk9vjo4UFt3MRuNs8ckRZqnrG0AFFoEt7oT61EKmEFB\r\n"
"Ik5lYYeBQVCmeVyJ3hlKV9Uu5l0cUyx+mM0aBhakaHPQNAQTXKFx01p8VdteZOE3\r\n"
"hzBWBOURtCmAEvF5OYiiAhF8J2a3iLd48soKqDirCmTCv2ZdlYTBoSUeh10aUAsg\r\n"
"EsxBu24LUTi4S8sCAwEAAaNjMGEwDgYDVR0PAQH/BAQDAgGGMA8GA1UdEwEB/wQF\r\n"
"MAMBAf8wHQYDVR0OBBYEFLE+w2kD+L9HAdSYJhoIAu9jZCvDMB8GA1UdIwQYMBaA\r\n"
"FLE+w2kD+L9HAdSYJhoIAu9jZCvDMA0GCSqGSIb3DQEBBQUAA4IBAQAcGgaX3Nec\r\n"
"nzyIZgYIVyHbIUf4KmeqvxgydkAQV8GK83rZEWWONfqe/EW1ntlMMUu4kehDLI6z\r\n"
"eM7b41N5cdblIZQB2lWHmiRk9opmzN6cN82oNLFpmyPInngiK3BD41VHMWEZ71jF\r\n"
"hS9OMPagMRYjyOfiZRYzy78aG6A9+MpeizGLYAiJLQwGXFK3xPkKmNEVX58Svnw2\r\n"
"Yzi9RKR/5CYrCsSXaQ3pjOLAEFe4yHYSkVXySGnYvCoCWw9E1CAx2/S6cCZdkGCe\r\n"
"vEsXCS+0yx5DaMkHJ8HSXPfqIbloEpw8nL+e/IBcm2PN7EeqJSdnoDfzAIJ9VNep\r\n"
"+OkuE6N36B9K\r\n"
"-----END CERTIFICATE-----\r\n";


void displayIssue()
{
    Screen.clean();
    Screen.print(0, msgBody+msgStart, true);
}

void scrollIssue()
{
    if(msgBodyLen != 0 && digitalRead(USER_BUTTON_B) == LOW)
    {
        msgStart += 16;
        if(msgStart >=  msgBodyLen)
        {
            msgStart = 0;
        }
        displayIssue();
    }
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

void getRepoIssues()
{
    Screen.clean();
    Screen.print("Requesting...");
    Serial.println("Requesting...");

    bool success = false;

    HTTPClient *client = new HTTPClient(GITHUB_CERT, HTTP_GET, GITHUB_REPO_API_URL);
    client->set_header("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64)");
    const Http_Response *response = client->send();
    if(response != NULL)
    {
        json_object *jsonObject, *countObject;
        int issueCount;
        Serial.println(response->body);
        if((jsonObject = json_tokener_parse(response->body)) != NULL &&
            (countObject = json_object_object_get(jsonObject, "open_issues_count")) != NULL &&
            (issueCount = json_object_get_int(countObject)) >= 0)
        {
            sprintf(msgBody, "There're total %d open issues in repo %s", issueCount, GITHUB_REPO_URL);
            msgBodyLen = strlen(msgBody);
            msgStart = 0;
            displayIssue();
            success = true;
        }
    }
    if(!success)
    {
        Screen.clean();
        Screen.print("Failed to get repo's issues.", true);
    }
    delete client;
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
        //TODO: handle if message length >= 2048
        msgStart = 0;
        msgBody[size] = '\0';
        memcpy(msgBody, buffer, size);
        msgBodyLen = size;
        LogInfo("Receive C2D message: %s", msgBody);
        displayIssue();
        blinkLED();
        return IOTHUBMESSAGE_ACCEPTED;
    }
}

void iothubInit()
{
    msgStart = msgBodyLen = 0;
    msgBody[0] = 0;

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

void iothubLoop()
{
    if(digitalRead(USER_BUTTON_A) == LOW)
    {
        getRepoIssues();
    }
    else
    {
        IoTHubClient_LL_DoWork(iotHubClientHandle);
        scrollIssue();
    }
}
