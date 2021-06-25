/*********
  Split Flap ESP Master
*********/
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "LittleFS.h"
#include <Arduino_JSON.h>
#include <Wire.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#define BAUDRATE 9600
#define ANSWERSIZE 1 //Size of units request answer
#define UNITSAMOUNT 10 //Amount of connected units
#define FLAPAMOUNT 45 //Amount of Flaps in each unit
#define MINSPEED 1 //min Speed
#define MAXSPEED 12 //max Speed
#define ESPLED 1 //Blue LED on ESP01
//#define serial //uncomment for serial debug messages, no serial messages if this whole line is a comment!

// REPLACE WITH YOUR NETWORK CREDENTIALS
const char* ssid = "SSID";
const char* password = "12345678901234567890";

//CHANGE THIS TO YOUR TIMEZONE, OFFSET IN SECONDS FROM GMT
// GMT +1 = 3600
// GMT +8 = 28800
// GMT -1 = -3600
// GMT 0 = 0
#define TIMEOFFSET 7200

const char letters[] = {' ', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '#', '$', '%', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ':', '.', '-', '?', '!'};
int displayState[UNITSAMOUNT];
String writtenLast;
unsigned long previousMillis = 0;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Search for parameter in HTTP POST request
const char* PARAM_ALIGNMENT = "alignment";
const char* PARAM_SPEEDSLIDER = "speedslider";
const char* PARAM_DEVICEMODE = "devicemode";
const char* PARAM_INPUT_1 = "input1";

//Variables to save values from HTML form
String alignment;
String speedslider;
String devicemode;
String input1;

// File paths to save input values permanently
const char* alignmentPath = "/alignment.txt";
const char* speedsliderPath = "/speedslider.txt";
const char* devicemodePath = "/devicemode.txt";

JSONVar values;

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

//Week Days
String weekDays[7] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
//Month names
String months[12] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};

void setup() {
  // Serial port for debugging purposes
#ifdef serial
  Serial.begin(BAUDRATE);
  Serial.println("master start");
#endif
#ifndef serial
  Wire.begin(1, 3); //For ESP01 only
#endif

  initWiFi();
  initFS();
  setupTime();

  loadFSValues();

  // Web Server Root URL
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(LittleFS, "/index.html", "text/html");
  });

  server.serveStatic("/", LittleFS, "/");

  server.on("/values", HTTP_GET, [](AsyncWebServerRequest * request) {
    String json = getCurrentInputValues();
    request->send(200, "application/json", json);
    json = String();
  });

  server.on("/", HTTP_POST, [](AsyncWebServerRequest * request) {
    int params = request->params();
    for (int i = 0; i < params; i++) {
      AsyncWebParameter* p = request->getParam(i);
      if (p->isPost()) {

        // HTTP POST alignment value
        if (p->name() == PARAM_ALIGNMENT) {
          alignment = p->value().c_str();
          Serial.print("Alignment set to: ");
          Serial.println(alignment);
          writeFile(LittleFS, alignmentPath, alignment.c_str());
        }

        // HTTP POST speed slider value
        if (p->name() == PARAM_SPEEDSLIDER) {
          speedslider = p->value().c_str();
          Serial.print("Speed set to: ");
          Serial.println(speedslider);
          writeFile(LittleFS, speedsliderPath, speedslider.c_str());
        }

        // HTTP POST mode value
        if (p->name() == PARAM_DEVICEMODE) {
          devicemode = p->value().c_str();
          Serial.print("Mode set to: ");
          Serial.println(devicemode);
          writeFile(LittleFS, devicemodePath, devicemode.c_str());
        }

        // HTTP POST input1 value
        if (p->name() == PARAM_INPUT_1) {
          input1 = p->value().c_str();
          Serial.print("Input 1 set to: ");
          Serial.println(input1);
        }
      }
    }
    request->send(LittleFS, "/index.html", "text/html");
  });
  server.begin();
}

void loop() {

  //Reset loop delay
  unsigned long currentMillis = millis();

  //Delay to not spam web requests
  if (currentMillis - previousMillis >= 1024) {
    previousMillis = currentMillis;

    //Mode Selection
    if (devicemode == "text") {
      showNewData(input1);
    } else if (devicemode == "date") {
      showDate();
    } else if (devicemode == "clock") {
      showClock();
    }
  }

}