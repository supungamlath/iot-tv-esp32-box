#include <Arduino.h>
#include "BluetoothSerial.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <EEPROM.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <asyncHTTPrequest.h>
#include <Ticker.h>
#include <EasyButton.h>

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

#if !(defined(ESP32))
#error This code is intended to run on the ESP8266 or ESP32 platform! Please check your Tools->Board setting.
#endif

#define EEPROM_SIZE 384

const int ledPins[] = {26, 27, 14, 13, 12};
const int buttonPins[] = {22, 21, 19, 18, 4};
const uint16_t irReceiverPin = 23;

const String deviceName = "ESP32TVBox";
const char *serverName = "http://192.168.1.101:3002/add-watch-data/device_004";

String bufferReceive = "", connID = "", wifi_ssid = "", wifi_pass = "";
long previousLedTime = 0, previousSelectedTime = 0;
int ledRiderCounter = 1;
int profileStates[] = {LOW, LOW, LOW, LOW, LOW};
int selectedState = 0;
enum LedStatus
{
    OFF,
    ON,
    BLINK,
    RIDER,
    STATEFUL
} ledStatus;

const uint16_t irCaptureBufferSize = 256; // 1024 == ~511 bits
const uint8_t irTimeout = 50;             // No. of milli-Seconds of no-more-data before we consider a message ended.

BluetoothSerial SerialBT;
IRrecv irReceiver(irReceiverPin, irCaptureBufferSize, irTimeout, false);
decode_results results;
asyncHTTPrequest request;
Ticker ticker;
EasyButton button1(buttonPins[0], 40, true, false), button2(buttonPins[1], 40, true, false), button3(buttonPins[2], 40, true, false), button4(buttonPins[3], 40, true, false), button5(buttonPins[4], 40, true, false);

void setLeds();
void indicateWiFiStatus();
void handleBluetooth();
void handleIRCommands();
void handleButtonPresses();
void postDataToServer();
void requestCallback(void *, asyncHTTPrequest *, int);

void onButton1Pressed()
{
    profileStates[0] = !profileStates[0];
}

void onButton2Pressed()
{
    profileStates[1] = !profileStates[1];
}

void onButton3Pressed()
{
    profileStates[2] = !profileStates[2];
}

void onButton4Pressed()
{
    profileStates[3] = !profileStates[3];
}

void onButton5Pressed()
{
    profileStates[4] = !profileStates[4];
}

void setup()
{
    for (int i = 0; i < 5; i++)
    {
        pinMode(ledPins[i], OUTPUT);
    }

    Serial.begin(115200);
    Serial.println("Device started");

    WiFi.mode(WIFI_STA);
    SerialBT.begin(deviceName);
    EEPROM.begin(EEPROM_SIZE);
    irReceiver.enableIRIn(); // Start up the IR receiver.

    wifi_ssid = EEPROM.readString(0);
    wifi_pass = EEPROM.readString(128);
    connID = EEPROM.readString(256);
    // Serial.println("CONN: " + connID);
    // Serial.println("SSID: " + wifi_ssid);
    // Serial.println("PASS: " + wifi_pass);

    WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
    WiFi.setHostname(deviceName.c_str());
    WiFi.disconnect();
    WiFi.begin(wifi_ssid.c_str(), wifi_pass.c_str());

    ledStatus = ON;
    setLeds();
    delay(1000);
    ledStatus = OFF;
    setLeds();

    request.setDebug(false);
    request.setReqHeader("Content-Type", "application/json");
    // request.onReadyStateChange(requestCallback);
    ticker.attach(10, postDataToServer);

    button1.begin();
    button2.begin();
    button3.begin();
    button4.begin();
    button5.begin();

    button1.onPressed(onButton1Pressed);
    button2.onPressed(onButton2Pressed);
    button3.onPressed(onButton3Pressed);
    button4.onPressed(onButton4Pressed);
    button5.onPressed(onButton5Pressed);
}

void loop()
{
    indicateWiFiStatus();
    handleBluetooth();
    handleIRCommands();
    handleButtonPresses();
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
    if (irReceiver.decode(&results))
    { // We have captured something.
      // The capture has stopped at this point.
        Serial.print("IR command received: ");
        Serial.println(results.command);

        if (results.command == 71)
        {
            selectedState--;
            if (selectedState < 0)
            {
                selectedState = 4;
            }
            previousSelectedTime = millis();
        }
        else if (results.command == 72)
        {
            profileStates[selectedState] = !profileStates[selectedState];
            previousSelectedTime = 0;
        }
        else if (results.command == 73)
        {
            selectedState++;
            if (selectedState > 4)
            {
                selectedState = 0;
            }
            previousSelectedTime = millis();
        }

        irReceiver.resume();
    }
}

void requestCallback(void *optParm, asyncHTTPrequest *request, int readyState)
{
    if (readyState == 4)
    {
        Serial.println(request->responseText());
        Serial.println();
        request->setDebug(false);
    }
}

void handleButtonPresses()
{
    button1.read();
    button2.read();
    button3.read();
    button4.read();
    button5.read();
}

void postDataToServer()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        if (request.readyState() == 0 || request.readyState() == 4)
        {
            char postRequestData[256];
            sprintf(postRequestData, "{\"person1\":\"%d\",\"person2\":\"%d\",\"person3\":\"%d\",\"person4\":\"%d\",\"person5\":\"%d\"}", profileStates[0], profileStates[1], profileStates[2], profileStates[3], profileStates[4]);

            request.open("POST", serverName);
            request.send(postRequestData);
        }
    }
}

void setLeds()
{
    if (ledStatus == OFF)
    {
        for (int i = 0; i < 5; i++)
        {
            digitalWrite(ledPins[i], LOW);
        }
    }
    else if (ledStatus == ON)
    {
        for (int i = 0; i < 5; i++)
        {
            digitalWrite(ledPins[i], HIGH);
        }
    }
    else if (ledStatus == BLINK)
    {
        if (millis() - previousLedTime > 300)
        {
            previousLedTime = millis();
            for (int i = 0; i < 5; i++)
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

            if (ledRiderCounter <= 4)
                digitalWrite(ledPins[ledRiderCounter], HIGH);

            ledRiderCounter++;

            if (ledRiderCounter == 6)
            {
                ledRiderCounter = 0;
                for (int i = 0; i < 5; i++)
                {
                    digitalWrite(ledPins[i], LOW);
                }
            }
        }
    }
    else if (ledStatus == STATEFUL)
    {
        for (int i = 0; i < 5; i++)
        {
            if (i != selectedState)
                digitalWrite(ledPins[i], profileStates[i]);
        }
        if (millis() - previousSelectedTime < 3000)
        {
            if (millis() - previousLedTime > 250)
            {
                previousLedTime = millis();
                digitalWrite(ledPins[selectedState], !digitalRead(ledPins[selectedState]));
            }
        }
        else
        {
            digitalWrite(ledPins[selectedState], profileStates[selectedState]);
        }
    }
}