#include <WiFi.h>
#include <ArduinoOTA.h>
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <time.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <stdlib.h>

const char* wifiSSID     = "";
const char* wifiPassword = "";

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 1 * 3600;
const int   daylightOffset_sec = 3600;

const long utcOffsetInSeconds = 3600;

const unsigned int IN1 = 26; //GPIO13 - IN1;
const unsigned int IN2 = 25; //GPIO14 - IN2;
const unsigned int IN3 = 33; //GPIO15 - IN3;
const unsigned int IN4 = 32; //GPIO4  - IN4

long STOP_AFTER = 0;
long STOP_AFTER_MILLIS;

unsigned long BOOT_TIME;
esp_reset_reason_t REBOOT_REASON;
char* REBOOT_REASON_TEXT;

bool TIMER_MINUTE_ACTIVE = false;

Preferences preferences;

AsyncWebServer server(80);
WebSocketsServer webSocket(81);

WiFiClient espClient;
PubSubClient client(espClient);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);

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

  server.on("/favicon-16x16.png", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/favicon-16x16.png", "text/javascript");
  });

  server.on("/favicon-32x32.png", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/favicon-32x32.png", "text/javascript");
  });

  server.on("/site.webmanifest", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/site.webmanifest", "text/javascript");
  });
  
  server.on("/android-chrome-512x512.png", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/android-chrome-512x512.png", "text/javascript");
  });
  
  server.on("/android-chrome-192x192.png", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/android-chrome-192x192.png", "text/javascript");
  });
  
  server.on("/apple-touch-icon.png", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/apple-touch-icon.png", "text/javascript");
  });
  
  server.begin();
 
  //configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  timeClient.begin();

  timeClient.forceUpdate();

  //save boot time and reboot reason to preferences
  BOOT_TIME = timeClient.getEpochTime();
  REBOOT_REASON = esp_reset_reason();

  switch(REBOOT_REASON) {
    case ESP_RST_SW: REBOOT_REASON_TEXT = "ESP_RST_SW"; break;
    case ESP_RST_POWERON: REBOOT_REASON_TEXT = "ESP_RST_POWERON"; break;
    case ESP_RST_UNKNOWN: REBOOT_REASON_TEXT = "ESP_RST_UNKNOWN"; break;
    case ESP_RST_PANIC: REBOOT_REASON_TEXT = "ESP_RST_PANIC"; break;
  }

  preferences.begin("dcf", false);
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
     rebootESP("WIFI");
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
  boolean timerActive = preferences.getBool("timerActive");
  if ( timerActive == true) {
    checkTimerAction();
  }
}

void checkTimerAction() {
  timeClient.update();
  
  //check if timer time is now
  unsigned long minus = timeClient.getEpochTime() - preferences.getInt("timerTimestamp");
  
  if ( minus%86400 == 0 ){
    if ( TIMER_MINUTE_ACTIVE == false ) {
      //start for specified time
      long forSeconds = preferences.getInt("timerForSeconds");
      bothMotorsForwards(forSeconds * 1000);
  
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
  preferences.putInt("lastFeedTime", timeClient.getEpochTime());
  preferences.putInt("lastDuration", forMS);

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

void saveTimerNew(boolean active, int timestamp, int forSeconds) {

  preferences.putInt("timerTimestamp", timestamp);
  
  preferences.putInt("timerForSeconds", forSeconds);
  preferences.putBool("timerActive", active);
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) { // When a WebSocket message is received
  switch (type) {
    case WStype_DISCONNECTED:             // if the websocket is disconnected
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED: {              // if a new websocket connection is established      
        //Send initial status
        const size_t capacity = JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(9) + 180;
        DynamicJsonDocument doc(capacity);

        unsigned long minus = timeClient.getEpochTime() - preferences.getInt("timerTimestamp");
        
        doc["type"] = "initial";
        doc["boottime"] = BOOT_TIME;
        doc["rebootreason"] = REBOOT_REASON;
        doc["rebootreasontext"] = REBOOT_REASON_TEXT;
        doc["rebootsource"] = preferences.getString("rebootReason");
        doc["freeram"] = esp_get_free_heap_size();
        doc["lastfeedtime"] = preferences.getInt("lastFeedTime");
        doc["lastfeedduration"] = preferences.getInt("lastDuration");

        JsonObject timer = doc.createNestedObject("timer");

        JsonObject timer_1 = timer.createNestedObject("1");
        timer_1["active"]    = preferences.getBool("timerActive");
        timer_1["timestamp"] = preferences.getInt("timerTimestamp");
        timer_1["seconds"]   = preferences.getInt("timerForSeconds");
         
        char connectionMessage[370];
        serializeJson(doc, connectionMessage);

        webSocket.sendTXT(num, connectionMessage);
      }
      break;
    case WStype_TEXT:                     // if new text data is received
      const size_t capacity = JSON_OBJECT_SIZE(5) + 60;
      DynamicJsonDocument doc(capacity);

      DeserializationError err = deserializeJson(doc, payload);
      if (err) {
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
          bool active = doc["activated"];
          int forSeconds = doc["forSeconds"];
          int selectedTimestamp = doc["timestamp"];
          
          saveTimerNew(active, selectedTimestamp, forSeconds);
          webSocket.sendTXT(num, "{\"type\":\"answer\",\"action\":\"saveTimer\"}");
        } else if (strcmp(action, "reboot") == 0) {
          webSocket.sendTXT(num, "{\"type\":\"answer\",\"action\":\"reboot\"}");
          rebootESP("WEBSOCKET");
        } else if (strcmp(action, "flush") == 0 ) {
          webSocket.sendTXT(num, "{\"type\":\"answer\",\"action\":\"flush\"}");
          flushPreferences();
        }
      }
      break;
  }
}

void flushPreferences() {
  preferences.clear();
}

void startWebSocket() { // Start a WebSocket server
  webSocket.begin();                          // start the websocket server
  webSocket.onEvent(webSocketEvent);          // if there's an incomming websocket message, go to function 'webSocketEvent'
  Serial.println("WebSocket server started.");
}

void rebootESP(char* rebootReason) {
  preferences.putString("rebootReason", rebootReason);
  ESP.restart();
}