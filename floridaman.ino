/*
  WS2812FX Webinterface.
  
  Harm Aldick - 2016
  www.aldick.org

  FEATURES
    * Webinterface with mode, color, speed and brightness selectors

  LICENSE

  The MIT License (MIT)

  Copyright (c) 2016  Harm Aldick 

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.

  
  CHANGELOG
  2016-11-26 initial version
  
*/

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WS2812FX.h>

#include "index.html.h"
#include "main.js.h"

// CHANGE ME FOR SUPER FUN TIME
#define WIFI_SSID "****SEKRET****"
#define WIFI_PASSWORD "****SEKRET****"

#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))

#define LED_PIN D2
#define LED_COUNT 6

// checks WiFi every 30s
#define WIFI_TIMEOUT 30000

// Thats right, a port is open
#define HTTP_PORT 80

#define DEFAULT_COLOR 0x00FF00
#define DEFAULT_BRIGHTNESS 31
#define DEFAULT_SPEED 50
#define DEFAULT_MODE FX_MODE_TWINKLE_FADE_RANDOM

unsigned long last_wifi_check_time = 0;
String modes = "";

WS2812FX ws2812fx = WS2812FX(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
ESP8266WebServer server = ESP8266WebServer(HTTP_PORT);

void setup(){
  Serial.begin(115200);
  banner();

  modes.reserve(5000);
  modes_setup();

  Serial.println("WS2812FX setup");
  ws2812fx.init();
  ws2812fx.setMode(DEFAULT_MODE);
  ws2812fx.setColor(DEFAULT_COLOR);
  ws2812fx.setSpeed(DEFAULT_SPEED);
  ws2812fx.setBrightness(DEFAULT_BRIGHTNESS);
  ws2812fx.start();

  Serial.println("Wifi setup");
  wifi_setup();
 
  Serial.println("HTTP server setup");
  server.on("/", srv_handle_index_html);
  server.on("/main.js", srv_handle_main_js);
  server.on("/modes", srv_handle_modes);
  server.on("/set", srv_handle_set);
  server.onNotFound(srv_handle_not_found);
  server.begin();
  Serial.println("HTTP server started.");

  Serial.println("ready!");
}

void loop() {
  unsigned long now = millis();

  server.handleClient();
  ws2812fx.service();

  if(now - last_wifi_check_time > WIFI_TIMEOUT) {
    Serial.print("Checking WiFi... ");
    if(WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi connection lost. Reconnecting...");
      wifi_setup();
    } else {
      Serial.println("OK");
    }
    last_wifi_check_time = now;
  }
}

void wifi_setup() {
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  WiFi.mode(WIFI_STA);

  unsigned long connect_start = millis();
  while(WiFi.status() != WL_CONNECTED) {
    delay(50);
    ws2812fx.service();

    if(millis() - connect_start > WIFI_TIMEOUT) {
      Serial.println();
      Serial.print("Tried ");
      Serial.print(WIFI_TIMEOUT);
      Serial.print("ms. Resetting ESP now.");
      ESP.reset();
    }
  }

  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();
}

void banner() {
  Serial.println("");
  Serial.println(" █████▒██▓     ▒█████   ██▀███   ██▓▓█████▄  ▄▄▄          ███▄ ▄███▓  ▄▄▄       ███▄    █ ");
  Serial.println("▓██   ▒▓██▒    ▒██▒  ██▒▓██ ▒ ██▒▓██▒▒██▀ ██▌ ▒████▄       ▓██▒▀█▀ ██▒▒ ████▄     ██ ▀█   █ ");
  Serial.println("▒████ ░▒██░    ▒██░  ██▒▓██ ░▄█ ▒▒██▒░██   █▌ ▒██  ▀█▄     ▓██    ▓██░▒██  ▀█▄  ▓██  ▀█ ██▒");
  Serial.println("░▓█▒  ░▒██░    ▒██   ██░▒██▀▀█▄  ░██░░▓█▄   ▌░██▄▄▄▄██     ▒██    ▒██ ░██▄▄▄▄██ ▓██▒  ▐▌██▒");
  Serial.println("░▒█░   ░██████▒░ ████▓▒░░██▓ ▒██▒░██░░▒████▓  ▓█   ▓██▒    ▒██▒   ░██▒ ▓█   ▓██▒▒██░   ▓██░");
  Serial.println(" ▒ ░   ░ ▒░▓  ░░ ▒░▒░▒░ ░ ▒▓ ░▒▓░░▓   ▒▒▓  ▒  ▒▒   ▓▒█░   ░ ▒░   ░  ░ ▒▒   ▓▒█░░ ▒░   ▒ ▒ ");
  Serial.println(" ░     ░ ░ ▒  ░  ░ ▒ ▒░   ░▒ ░ ▒░ ▒ ░ ░ ▒  ▒   ▒   ▒▒ ░   ░  ░      ░  ▒   ▒▒ ░░ ░░   ░ ▒░");
  Serial.println(" ░ ░     ░ ░   ░ ░ ░ ▒    ░░   ░  ▒ ░ ░ ░  ░   ░   ▒      ░      ░     ░   ▒      ░   ░ ░ ");
  Serial.println("           ░  ░    ░ ░     ░      ░     ░          ░  ░          ░         ░  ░         ░ ");
  Serial.println("                                      ░                                                   ");
  Serial.println("");
  Serial.println("Looking for Florida Man Party Access Point...");                                         
}  

/*
 * Build <li> string for all modes.
 */
void modes_setup() {
  modes = "";
  for(uint8_t i=0; i < ws2812fx.getModeCount(); i++) {
    modes += "<li><a href='#' class='m' id='";
    modes += i;
    modes += "'>";
    modes += ws2812fx.getModeName(i);
    modes += "</a></li>";
  }
}

/* #####################################################
#  Webserver Functions
##################################################### */

void srv_handle_not_found() {
  server.send(404, "text/plain", "File Not Found");
}

void srv_handle_index_html() {
  server.send_P(200,"text/html", index_html);
}

void srv_handle_main_js() {
  server.send_P(200,"application/javascript", main_js);
}

void srv_handle_modes() {
  server.send(200,"text/plain", modes);
}

void srv_handle_set() {
  for (uint8_t i = 0; i < server.args(); i++) {
    if (server.argName(i) == "c") {
      uint32_t tmp = (uint32_t) strtol(&server.arg(i)[0], NULL, 16);
      if (tmp >= 0x000000 && tmp <= 0xFFFFFF) {
        ws2812fx.setColor(tmp);
      }
    }

    if (server.argName(i) == "m") {
      uint8_t tmp = (uint8_t) strtol(&server.arg(i)[0], NULL, 10);
      ws2812fx.setMode(tmp % ws2812fx.getModeCount());
    }

    if (server.argName(i) == "b") {
      uint8_t tmp = (uint8_t) strtol(&server.arg(i)[0], NULL, 10);
      ws2812fx.setBrightness(tmp);
    }

    if (server.argName(i) == "s") {
      uint8_t tmp = (uint8_t) strtol(&server.arg(i)[0], NULL, 10);
      ws2812fx.setSpeed(tmp);
    }
  }
  server.send(200, "text/plain", "OK");
}
