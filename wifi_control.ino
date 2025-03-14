/*
MIT License

Copyright (c) 2025 controllercustom@myyahoo.com

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#define DEBUG_ON  0

#if DEBUG_ON
#define DBG_begin(...)    Serial.begin(__VA_ARGS__)
#define DBG_print(...)    Serial.print(__VA_ARGS__); Serial.flush()
#define DBG_println(...)  Serial.println(__VA_ARGS__); Serial.flush()
#define DBG_printf(...)   Serial.printf(__VA_ARGS__); Serial.flush()
#else
#define DBG_begin(...)
#define DBG_print(...)
#define DBG_println(...)
#define DBG_printf(...)
#endif

#include <WiFi.h>
#if defined(ESP32)
#include <ESPmDNS.h>
#elif defined(ARDUINO_ARCH_RP2040)
#include <LEAmDNS.h>
#endif
#include <WiFiClient.h>
#include <WebSocketsServer.h> // Install WebSockets by Markus Sattler from IDE Library manager
#include <WebServer.h>
#include "index_html.h"
#include "secrets.h"

#ifndef STASSID
#define STASSID "myssid"
#define STAPSK "mypassphrase"
#endif

WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

#ifdef LED_BUILTIN
const int led = LED_BUILTIN;
#else
const int led = 5;  // Choose a pin and hook up an LED and resistor
#endif

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length)
{
  switch(type) {
    case WStype_DISCONNECTED:
      DBG_printf("[%u] Disconnected!\r\n", num);
      break;
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        DBG_printf("[%u] Connected from %d.%d.%d.%d url: %s\r\n", num, ip[0], ip[1], ip[2], ip[3], payload);
      }
      break;
    case WStype_TEXT:
      {
        DBG_printf("[%u] get Text: [%d] %s \r\n", num, length, payload);

        char *event = (char *)malloc(length + 1);  // get space for NUL terminated string
        if (event != NULL) {
          memcpy(event, payload, length);
          event[length] = '\0';
          if (strcmp("LED_on", event) == 0) {
            digitalWrite(led, HIGH);
          } else if (strcmp("LED_off", event) == 0) {
            digitalWrite(led, LOW);
          } else {
            DBG_print("Ignoring event: ");
            DBG_println(event);
          }
          free(event);
        }
      }
      break;
    case WStype_BIN:
      DBG_printf("[%u] get binary length: %u\r\n", num, length);
      break;
    default:
      DBG_printf("Invalid WStype [%d]\r\n", type);
      break;
  }
}

void handleRoot() {
  server.send(200, "text/html", INDEX_HTML);
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void setup_server() {
  server.on("/", handleRoot);

  server.onNotFound(handleNotFound);

  server.begin();
  DBG_println("HTTP server started");
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  DBG_println("websocket server started");
}

void setup() {
  pinMode(led, OUTPUT);
  digitalWrite(led, LOW);

#if DEBUG_ON
  Serial.begin(115200);
  while (!Serial && millis() < 4000) {
    delay(10);
  }
  Serial.println("wifi_control starting");
#else
  Serial.end();
#endif
  WiFi.mode(WIFI_STA);
  WiFi.begin(STASSID, STAPSK);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    DBG_println("Connection Failed! Rebooting...");
    delay(5000);
#if defined(ESP32)
    ESP.restart();
#elif defined(ARDUINO_ARCH_RP2040)
    rp2040.restart();
#endif
  }

  DBG_println("Ready");
  DBG_print("IP address: ");
  DBG_println(WiFi.localIP());

  if (MDNS.begin("wificontrol")) {
    DBG_println("MDNS responder started");
    DBG_println("Connect to http://wificontrol.local");
    MDNS.addService("http", "tcp", 80);
    MDNS.addService("ws", "tcp", 81);
  }
  setup_server();
}

void loop() {
  webSocket.loop();
  server.handleClient();
#ifndef ESP32
  MDNS.update();
#endif
}
