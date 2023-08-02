// #include <ESP8266WiFi.h>
// #include <ESP8266mDNS.h>
// #include <ArduinoOTA.h>
// #include <ESP8266WebServer.h>
// #include <WiFiManager.h>
// #include <ESP8266HTTPClient.h>
// #include <WiFiClient.h>
// #include <IRremoteESP8266.h>
// #include <IRac.h>
// #include <IRutils.h>
// #include <ArduinoJson.h>

// const uint16_t kIrLed = 4; // The ESP GPIO pin to use that controls the IR LED.
// IRac ac(kIrLed);           // Create a A/C object using GPIO to sending messages with.

// ESP8266WebServer server(80);
// String serverName = "http://192.168.1.106:1880/update-sensor";

// void handleRoot()
// {
//     server.send(200, "text/html", "<h1>Hello</h1>");
// }

// void handleNotFound()
// {
//     server.send(404, "text/plain", "Not found");
// }

// void sendRegistrationData()
// {
//     if (WiFi.status() == WL_CONNECTED)
//     {
//         WiFiClient client;
//         HTTPClient http;

//         // Your Domain name with URL path or IP address with path
//         http.begin(client, serverName);

//         // If you need Node-RED/server authentication, insert user and password below
//         // http.setAuthorization("REPLACE_WITH_SERVER_USERNAME", "REPLACE_WITH_SERVER_PASSWORD");

//         // Specify content-type header
//         http.addHeader("Content-Type", "application/json");

//         StaticJsonDocument<200> resBody;
//         resBody["device_id"] = "tPmAT5Ab3j7F9";
//         resBody["device_type"] = "ir_remote";
//         resBody["ip_address"] = WiFi.localIP().toString();
//         String resBodyString;
//         serializeJson(resBody, resBodyString);

//         int httpResponseCode = http.POST(resBodyString);

//         Serial.print("HTTP Response code: ");
//         Serial.println(httpResponseCode);

//         // Free resources
//         http.end();
//     }
// }

// void setModel()
// // A JSON object containing protocol_no and model_no needs to be passed into this endpoint
// {
//     StaticJsonDocument<200> reqBody;
//     // Deserialize the JSON document
//     DeserializationError error = deserializeJson(reqBody, server.arg("plain"));

//     // Test if parsing succeeds.
//     if (error)
//     {
//         Serial.print(F("deserializeJson() failed: "));
//         Serial.println(error.f_str());
//         server.send(400, "text/html", "Invalid input");
//         return;
//     }

//     decode_type_t protocol = (decode_type_t)reqBody["protocol_no"];
//     // If the protocol is supported by the IRac class ...
//     if (ac.isProtocolSupported(protocol))
//     {
//         ac.next.protocol = protocol; // Change the protocol used.
//         ac.next.model = reqBody["model_no"];
//         server.send(200, "text/html", "Protocol set");
//     }
//     else
//     {
//         server.send(400, "text/html", "Protocol unknown");
//     }
// }

// void turnOn()
// {
//     ac.next.power = true; // We want to turn on the A/C unit.
//     Serial.println("Sending a message to turn ON the A/C unit.");
//     ac.sendAc();
//     server.send(200, "text/html", "Command sent");
// }

// void turnOff()
// {
//     ac.next.power = false; // Now we want to turn the A/C off.
//     Serial.println("Send a message to turn OFF the A/C unit.");
//     ac.sendAc(); // Send the message.
//     server.send(200, "text/html", "Command sent");
// }

// void setup()
// {
//     Serial.begin(115200);
//     WiFi.mode(WIFI_STA);

//     // WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
//     WiFiManager wm;

//     bool res = wm.autoConnect("AutoConnectAP", "password"); // password protected AP

//     if (!res)
//     {
//         Serial.println("Failed to connect to WiFi");
//         ESP.restart();
//     }
//     else
//     {
//         Serial.println("Connected to WiFi");
//     }

//     // Port defaults to 8266
//     ArduinoOTA.setPort(8266);
//     // Hostname defaults to esp8266-[ChipID]
//     ArduinoOTA.setHostname("esp8266");
//     // No authentication by default
//     // ArduinoOTA.setPassword((const char *)"123");
//     ArduinoOTA.begin();

//     if (MDNS.begin("esp8266"))
//     {
//         Serial.println("MDNS responder started");
//     }

//     server.on("/", HTTP_GET, handleRoot);
//     server.on("/setModel", HTTP_POST, setModel);
//     server.on("/turnOn", HTTP_GET, turnOn);
//     server.on("/turnOff", HTTP_GET, turnOff);
//     server.onNotFound(handleNotFound);

//     server.begin();
//     Serial.println("HTTP server started");

//     sendRegistrationData();
// }

// void loop()
// {
//     ArduinoOTA.handle();
//     server.handleClient();
//     MDNS.update();
// }