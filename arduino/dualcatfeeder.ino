#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <FS.h>

#include <OneButton.h>
#include <L298N.h>

const char* wifiSSID     = "";
const char* wifiPassword = "";

const char* mqttServer   = "";
const int   mqttPort     = 1883;
const char* mqttUser     = "";
const char* mqttPassword = "";
const char* mqttPath     = "";

const int   OTAPort     = 8266;
const char* OTAHostname = "";
//const char* OTAPassword = "password";

const unsigned int IN1 = 13; //GPIO13 - IN1;
const unsigned int IN2 = 14; //GPIO14 - IN2;
const unsigned int IN3 = 15; //GPIO15 - IN3;
const unsigned int IN4 = 4;  //GPIO4  - IN4

int BUTTON1 = 12; //GPIO12 - Black button
int BUTTON2 = 1;  //GPIO16 - Blue button
int BUTTON3 = 5;  //GPIO5  - White button

int MOTOR_STATUS = 0;

long STOP_AFTER = 0;
long STOP_AFTER_MILLIS;

WiFiClient espClient;
PubSubClient client(espClient);
WebSocketsServer webSocket(81);

ESP8266WebServer webServer(80);

OneButton button1(BUTTON1, true);
OneButton button2(BUTTON2, true);
OneButton button3(BUTTON3, true);

L298N leftMotor(99, IN1, IN2);
L298N rightMotor(99, IN3, IN4);

void setup() {
  //Convert TX to GPIO1
  pinMode(1, FUNCTION_3);

  Serial.begin(115200);
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifiSSID, wifiPassword);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  ArduinoOTA.setPort(OTAPort);
  ArduinoOTA.setHostname(OTAHostname);
  //ArduinoOTA.setPassword(OTAPassword);

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  //MQTT connection
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);

  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client.connect("ESP8266Client", mqttUser, mqttPassword )) {
      Serial.println("connected");
    } else {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }

  client.publish("home/kueche/futterautomat/log", "Here we go again.");
  client.subscribe("home/kueche/futterautomat/#");

  //Motor
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  //Turn off motors
  stopBothMotors();

  //Button
  pinMode(BUTTON1, INPUT_PULLUP);
  pinMode(BUTTON2, INPUT_PULLUP);
  pinMode(BUTTON3, INPUT_PULLUP);

  //WebSocket
  startWebSocket();

  button1.attachDoubleClick(doubleClickButton1);
  button2.attachLongPressStart(clickButton2);
  button3.attachDoubleClick(doubleClickButton3);

  SPIFFS.begin();
  
  //WebServer
  webServer.on("/", handleRoot);
  webServer.onNotFound(handleWebRequests); 
  webServer.begin();

  
}

void handleRoot(){
  webServer.sendHeader("Location", "/index.html", true);
  webServer.send(302, "text/plain","");
}

void handleWebRequests(){
  client.publish("home/kueche/futterautomat/log", "handleWebRequests");
  
  if(loadFromSpiffs(webServer.uri())) return;
  String message = "File Not Detected\n\n";
  message += "URI: ";
  message += webServer.uri();
  message += "\nMethod: ";
  message += (webServer.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += webServer.args();
  message += "\n";
  for (uint8_t i=0; i<webServer.args(); i++){
    message += " NAME:"+webServer.argName(i) + "\n VALUE:" + webServer.arg(i) + "\n";
  }
  webServer.send(404, "text/plain", message);
  Serial.println(message);
}

bool loadFromSpiffs(String path){
  client.publish("home/kueche/futterautomat/log", "loadFromSpiffs");
  
  String dataType = "text/plain";

  if(path.endsWith("/")) path += "index.htm";
 
  if(path.endsWith(".src")) path = path.substring(0, path.lastIndexOf("."));
  else if(path.endsWith(".html")) dataType = "text/html";
  else if(path.endsWith(".htm")) dataType = "text/html";
  else if(path.endsWith(".css")) dataType = "text/css";
  else if(path.endsWith(".js")) dataType = "application/javascript";
  else if(path.endsWith(".png")) dataType = "image/png";
  else if(path.endsWith(".gif")) dataType = "image/gif";
  else if(path.endsWith(".jpg")) dataType = "image/jpeg";
  else if(path.endsWith(".ico")) dataType = "image/x-icon";
  else if(path.endsWith(".xml")) dataType = "text/xml";
  else if(path.endsWith(".pdf")) dataType = "application/pdf";
  else if(path.endsWith(".zip")) dataType = "application/zip";
  File dataFile = SPIFFS.open(path.c_str(), "r");
  if (webServer.hasArg("download")) dataType = "application/octet-stream";
  if (webServer.streamFile(dataFile, dataType) != dataFile.size()) {
  }
 
  dataFile.close();
  return true;
}

void startWebSocket() { // Start a WebSocket server
  webSocket.begin();                          // start the websocket server
  webSocket.onEvent(webSocketEvent);          // if there's an incomming websocket message, go to function 'webSocketEvent'
  Serial.println("WebSocket server started.");
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) { // When a WebSocket message is received
  switch (type) {
    case WStype_DISCONNECTED:             // if the websocket is disconnected
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED: {              // if a new websocket connection is established
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
      }
      break;
    case WStype_TEXT:                     // if new text data is received
      const size_t capacity = JSON_OBJECT_SIZE(2) + 30;
      DynamicJsonDocument doc(capacity);

      DeserializationError err = deserializeJson(doc, payload);
      if (err) {
        client.publish("home/kueche/futterautomat/log", (char*) err.c_str());
      } else {
        const char* action = doc["action"];
        int time_ms        = doc["time"];
        
        if ( strcmp(action, "forwards") == 0) {
            bothMotorsForwards(time_ms);
        } else if ( strcmp(action, "backwards") == 0) {
          bothMotorsBackwards(time_ms);
        } else if ( strcmp(action, "stop") == 0) {
          stopBothMotors();
        }
      }
      break;
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  char msg[length + 1];
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    msg[i] = (char)payload[i];
  }

  msg[length] = '\0';

  //Motorstatus kontrollieren
  if (strcmp(topic, "home/kueche/futterautomat/motorstatus") == 0) {
    //controlMotorStatus(msg, 1);
    //controlMotorStatus(msg, 2);
  } else if (strcmp(topic, "home/kueche/futterautomat/motordirection") == 0) {
    //changeMotorDirection(msg, 1);
    //changeMotorDirection(msg, 2);
  }
}

void loop() {
  ArduinoOTA.handle();

  //MQTT
  client.loop();

  //WebSocket
  webSocket.loop();
  
  //WebServer
  webServer.handleClient();

  //Buttons
  button1.tick();
  button2.tick();
  button3.tick();

  //check for active STOP_AFTER timer
  if ( STOP_AFTER > 0 ){
    if ( millis() - STOP_AFTER_MILLIS >= STOP_AFTER ){
      stopBothMotors();
    }
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
  rightMotor.stop();
  leftMotor.stop();

  STOP_AFTER = 0;
}

void bothMotorsForwards(long forMS) {
  rightMotor.forward();
  leftMotor.forward();

  if ( forMS > 0){
    STOP_AFTER_MILLIS = millis();
    STOP_AFTER = forMS;
  } else {
    STOP_AFTER = 0;
  }
}

void bothMotorsBackwards(long forMS) {
  rightMotor.backward();
  leftMotor.backward();

  if ( forMS > 0){
    STOP_AFTER_MILLIS = millis();
    STOP_AFTER = forMS;
  } else {
    STOP_AFTER = 0;
  }
}