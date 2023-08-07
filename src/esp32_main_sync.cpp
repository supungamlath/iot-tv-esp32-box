#include <Arduino.h>
#include "BluetoothSerial.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <EEPROM.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

#define EEPROM_SIZE 384

const int ledPins[] = {14, 27, 12, 25};
const uint16_t kRecvPin = 26;

const String deviceName = "ESP32TVBox";
const char *serverName = "https://server-uyxt.onrender.com/api/data/64ccb31544b32dea1157b5bd";

String bufferReceive = "", connID = "", wifi_ssid = "", wifi_pass = "";
long previousLedTime = 0, previousPostTime = 0;
int ledRiderCounter = 1;
int ledStates[] = {LOW, LOW, LOW, LOW};
enum LedStatus
{
    OFF,
    ON,
    BLINK,
    RIDER,
    STATEFUL
} ledStatus;

const uint16_t kCaptureBufferSize = 256; // 1024 == ~511 bits
// kTimeout is the Nr. of milli-Seconds of no-more-data before we consider a message ended.
const uint8_t kTimeout = 50; // Milli-Seconds

BluetoothSerial SerialBT;
IRrecv irrecv(kRecvPin, kCaptureBufferSize, kTimeout, false);
decode_results results;

void setLeds();
void indicateWiFiStatus();
void handleBluetooth();
void handleIRCommands();
void postDataToServer();

void setup()
{
    for (int i = 0; i < 4; i++)
    {
        pinMode(ledPins[i], OUTPUT);
    }

    Serial.begin(115200);
    Serial.println("Device started");

    WiFi.mode(WIFI_STA);
    SerialBT.begin(deviceName);
    EEPROM.begin(EEPROM_SIZE);
    irrecv.enableIRIn(); // Start up the IR receiver.

    wifi_ssid = EEPROM.readString(0);
    wifi_pass = EEPROM.readString(128);
    connID = EEPROM.readString(256);
    Serial.println("CONN: " + connID);
    Serial.println("SSID: " + wifi_ssid);
    Serial.println("PASS: " + wifi_pass);

    WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
    WiFi.setHostname(deviceName.c_str());
    WiFi.disconnect();
    WiFi.begin(wifi_ssid.c_str(), wifi_pass.c_str());

    ledStatus = ON;
    setLeds();
    delay(1000);
    ledStatus = OFF;
    setLeds();
}

void loop()
{
    indicateWiFiStatus();
    handleBluetooth();
    handleIRCommands();
    postDataToServer();
    setLeds();

    delay(10);
}

void indicateWiFiStatus()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        ledStatus = RIDER;
    }
    else
    {
        ledStatus = STATEFUL;
    }
}

void handleBluetooth()
{
    if (SerialBT.available())
    {
        char c = SerialBT.read();
        bufferReceive += c;

        if (bufferReceive == "CONNECT:")
        {
            String parameter[4];
            int i = 0;
            while (1)
            {
                if (SerialBT.available())
                {
                    char c2 = SerialBT.read();
                    if (c2 == '\t')
                    {
                        i++;
                    }
                    else if (c2 == '\n')
                    {
                        break;
                    }
                    else
                    {
                        parameter[i] += c2;
                    }
                }
                delay(0); // nop
            }

            if (!connID.equals(parameter[0]))
            {
                connID = parameter[0];
                EEPROM.writeString(256, connID);
            }

            if (!wifi_ssid.equals(parameter[1]))
            {
                wifi_ssid = parameter[1];
                EEPROM.writeString(0, wifi_ssid);
            }

            if (!wifi_pass.equals(parameter[2]))
            {
                wifi_pass = parameter[2];
                EEPROM.writeString(128, wifi_pass);
            }

            Serial.println("WiFi Credentials Updated");
            bufferReceive = "";

            if (WiFi.begin(wifi_ssid.c_str(), wifi_pass.c_str()))
            {
                EEPROM.commit();
                Serial.println("WiFi Connected");
                ledStatus = ON;
                setLeds();
                delay(1000);
                ledStatus = OFF;
                setLeds();
            };
        }
    }
}

void handleIRCommands()
{

    // Check if an IR message has been received.
    if (irrecv.decode(&results))
    { // We have captured something.
      // The capture has stopped at this point.
        Serial.print("IR command received: ");
        Serial.println(results.command);

        if (results.command == 70)
        {
            ledStates[0] = !ledStates[0];
        }
        else if (results.command == 71)
        {
            ledStates[1] = !ledStates[1];
        }
        else if (results.command == 72)
        {
            ledStates[2] = !ledStates[2];
        }
        else if (results.command == 73)
        {
            ledStates[3] = !ledStates[3];
        }

        irrecv.resume();
    }
}

void postDataToServer()
{
    if ((WiFi.status() == WL_CONNECTED) && (millis() - previousPostTime > 15000))
    {
        previousPostTime = millis();
        HTTPClient http;
        http.begin(serverName);

        http.addHeader("Content-Type", "application/json");

        char postRequestData[256];
        sprintf(postRequestData, "{\"deviceId\":\"%s\",\"connectionID\":\"%s\",\"person1\":\"%d\",\"person2\":\"%d\",\"person3\":\"%d\",\"person4\":\"%d\"}", connID.c_str(), connID.c_str(), ledStates[0], ledStates[1], ledStates[2], ledStates[3]);

        int httpResponseCode = http.PUT(postRequestData);

        Serial.print("HTTP POST Request: ");
        Serial.println(postRequestData);
        Serial.print("HTTP POST Request Response: ");
        Serial.println(httpResponseCode);

        // Free resources
        http.end();
    }
}

void setLeds()
{
    if (ledStatus == OFF)
    {
        for (int i = 0; i < 4; i++)
        {
            digitalWrite(ledPins[i], LOW);
        }
    }
    else if (ledStatus == ON)
    {
        for (int i = 0; i < 4; i++)
        {
            digitalWrite(ledPins[i], HIGH);
        }
    }
    else if (ledStatus == BLINK)
    {
        if (millis() - previousLedTime > 300)
        {
            previousLedTime = millis();
            for (int i = 0; i < 4; i++)
            {
                digitalWrite(ledPins[i], !digitalRead(ledPins[i]));
            }
        }
    }
    else if (ledStatus == RIDER)
    {
        if (millis() - previousLedTime > 250)
        {
            previousLedTime = millis();

            if (ledRiderCounter <= 3)
                digitalWrite(ledPins[ledRiderCounter], HIGH);

            ledRiderCounter++;

            if (ledRiderCounter == 5)
            {
                ledRiderCounter = 0;
                for (int i = 0; i < 4; i++)
                {
                    digitalWrite(ledPins[i], LOW);
                }
            }
        }
    }
    else if (ledStatus == STATEFUL)
    {
        for (int i = 0; i < 4; i++)
        {
            digitalWrite(ledPins[i], ledStates[i]);
        }
    }
}