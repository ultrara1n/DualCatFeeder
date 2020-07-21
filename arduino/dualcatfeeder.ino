#include <WiFi.h>
#include <ArduinoOTA.h>
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <time.h>
#include <string>

const char* wifiSSID     = "";
const char* wifiPassword = "";

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 1 * 3600;
const int   daylightOffset_sec = 3600;

const unsigned int IN1 = 26; //GPIO13 - IN1;
const unsigned int IN2 = 25; //GPIO14 - IN2;
const unsigned int IN3 = 33; //GPIO15 - IN3;
const unsigned int IN4 = 32;  //GPIO4  - IN4

long STOP_AFTER = 0;
long STOP_AFTER_MILLIS;

bool TIMER_MINUTE_ACTIVE = false;

Preferences preferences;

AsyncWebServer server(80);
WebSocketsServer webSocket(81);

void setup() {
  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  WiFi.begin(wifiSSID, wifiPassword);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }

  ArduinoOTA.begin();

  //Motor
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  //Turn off motors
  stopBothMotors();

  //WebSocket
  startWebSocket();

  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/index.html", String());
  });

  server.on("/bootstrap.min.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/bootstrap.min.css", "text/css");
  });

  server.on("/dualcatfeeder.js", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/dualcatfeeder.js", "text/javascript");
  });

  server.begin();

  preferences.begin("dcf", false);

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

void loop() {
  int wifi_retry = 0;
  while(WiFi.status() != WL_CONNECTED && wifi_retry < 5 ) {
      wifi_retry++;
      Serial.println("WiFi not connected. Try to reconnect");
      WiFi.disconnect();
      WiFi.mode(WIFI_OFF);
      WiFi.mode(WIFI_STA);
      WiFi.begin(wifiSSID, wifiPassword);
      delay(100);
  }
  if(wifi_retry >= 5) {
      Serial.println("\nReboot");
      ESP.restart();
  }
  
  ArduinoOTA.handle();

  //WebSocket
  webSocket.loop();

  //check for active STOP_AFTER timer
  if ( STOP_AFTER > 0 ) {
    if ( millis() - STOP_AFTER_MILLIS >= STOP_AFTER ) {
      stopBothMotors();
    }
  }

  //timer check, if activated
  if (preferences.getBool("timerActive") == true) {
    checkTimerAction();
  }
}

void checkTimerAction() {
  struct tm timeinfo;
  getLocalTime(&timeinfo);
  
  char timeHour[3];
  strftime(timeHour,3, "%H", &timeinfo);

  char timeMinute[3];
  strftime(timeMinute,3, "%M", &timeinfo);

  int actualHour   = atoi(timeHour);
  int actualMinute = atoi(timeMinute);
  
  //check if timer time is now
   int selectedHour    = preferences.getInt("selectedHour");
   int selectedMinute  = preferences.getInt("selectedMinute");
  if ( selectedHour == actualHour && selectedMinute == actualMinute ){
    if ( TIMER_MINUTE_ACTIVE == false ) {
    //start for specified time
    int forSeconds = preferences.getInt("timerForSeconds");
    bothMotorsForwards(forSeconds);
  
    //set flag to true
    TIMER_MINUTE_ACTIVE = true;
    }
  } else {
    TIMER_MINUTE_ACTIVE = false;
  }
}

void doubleClickButton1() {
  bothMotorsBackwards(0);
}

void clickButton2() {
  stopBothMotors();
}

void doubleClickButton3() {
  bothMotorsForwards(0);
}

void stopBothMotors() {
  //rightMotor.stop();
  //leftMotor.stop();
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);

  STOP_AFTER = 0;
}

void bothMotorsForwards(long forMS) {
  //rightMotor.forward();
  //leftMotor.forward();

  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);

  if ( forMS > 0) {
    STOP_AFTER_MILLIS = millis();
    STOP_AFTER = forMS;
  } else {
    STOP_AFTER = 0;
  }
}

void bothMotorsBackwards(long forMS) {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);

  if ( forMS > 0) {
    STOP_AFTER_MILLIS = millis();
    STOP_AFTER = forMS;
  } else {
    STOP_AFTER = 0;
  }
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) { // When a WebSocket message is received
  switch (type) {
    case WStype_DISCONNECTED:             // if the websocket is disconnected
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED: {              // if a new websocket connection is established      
        //Send initial status
        const size_t capacity = JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(4);
        DynamicJsonDocument doc(capacity);

        doc["type"] = "initial";
        doc["uptime"] = (long) esp_timer_get_time() / 1000000;

        JsonObject timer = doc.createNestedObject("timer");

        JsonObject timer_1 = timer.createNestedObject("1");

        timer_1["active"]  = preferences.getBool("timerActive");
        timer_1["hour"]    = preferences.getInt("selectedHour");
        timer_1["minute"]  = preferences.getInt("selectedMinute");
        timer_1["seconds"] = preferences.getInt("timerForSeconds");

        char connectionMessage[200];
        serializeJson(doc, connectionMessage);

        webSocket.sendTXT(num, connectionMessage);
      }
      break;
    case WStype_TEXT:                     // if new text data is received
      const size_t capacity = JSON_OBJECT_SIZE(5) + 60;
      DynamicJsonDocument doc(capacity);

      DeserializationError err = deserializeJson(doc, payload);
      if (err) {
        //client.publish("home/kueche/futterautomat/log", (char*) err.c_str());
      } else {
        const char* action = doc["action"];

        if ( strcmp(action, "forwards") == 0) {
          int time_ms = doc["time"];

          //webSocket.sendTXT(num, "{\"type\":\"answer\",\"action\":\"forwards\",\"\":" + std::to_string(time_ms) + "}" );
          
          bothMotorsForwards(time_ms);
        } else if ( strcmp(action, "backwards") == 0) {
          int time_ms = doc["time"];
          bothMotorsBackwards(time_ms);
        } else if ( strcmp(action, "stop") == 0) {
          webSocket.sendTXT(num, "{\"type\":\"answer\",\"action\":\"stop\"}");
          stopBothMotors();
        } else if (strcmp(action, "saveTimer") == 0) {
          int selectedHour = doc["hour"];
          int selectedMinute = doc["minute"];
          int forSeconds = doc["forSeconds"];
          bool active = doc["activated"];
          saveTimer(active, selectedHour, selectedMinute, forSeconds);
        } else if (strcmp(action, "reboot") == 0) {
          webSocket.sendTXT(num, "{\"type\":\"answer\",\"action\":\"reboot\"}");
          rebootESP();
        }
      }
      break;
  }
}

void saveTimer(boolean active, int selectedHour, int selectedMinute, int forSeconds) {
  //time
  preferences.putInt("selectedHour", selectedHour);
  preferences.putInt("selectedMinute", selectedMinute);
  //seconds
  preferences.putInt("timerForSeconds", forSeconds);
  //active
  preferences.putBool("timerActive", active);
}

void startWebSocket() { // Start a WebSocket server
  webSocket.begin();                          // start the websocket server
  webSocket.onEvent(webSocketEvent);          // if there's an incomming websocket message, go to function 'webSocketEvent'
  Serial.println("WebSocket server started.");
}

void rebootESP() {
  ESP.restart();
}