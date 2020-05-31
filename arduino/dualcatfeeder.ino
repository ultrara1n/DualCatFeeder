#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <WebSocketsServer.h>

#include "OneButton.h"
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
const char* OTAPassword = "";

const unsigned int IN1 = 13; //GPIO13 - IN1;
const unsigned int IN2 = 14; //GPIO14 - IN2;
const unsigned int IN3 = 15; //GPIO15 - IN3;
const unsigned int IN4 = 4;  //GPIO4  - IN4

int BUTTON1 = 12; //GPIO12 - Black button
int BUTTON2 = 1;  //GPIO16 - Blue button
int BUTTON3 = 5;  //GPIO5  - White button 

int MOTOR_STATUS = 0;

WiFiClient espClient;
PubSubClient client(espClient);
WebSocketsServer webSocket(81);

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
  ArduinoOTA.setPassword(OTAPassword);

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
  //controlMotorStatus("off", 1);
  //controlMotorStatus("off", 2);

  rightMotor.stop();
  leftMotor.stop();
    
  //Button
  pinMode(BUTTON1, INPUT_PULLUP);
  pinMode(BUTTON2, INPUT_PULLUP);
  pinMode(BUTTON3, INPUT_PULLUP);

  //WebSocket
  startWebSocket();

  button1.attachDoubleClick(doubleClickButton1);
  button2.attachLongPressStart(clickButton2);
  button3.attachDoubleClick(doubleClickButton3);
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
      Serial.printf("[%u] get Text: %s\n", num, payload);
      if (payload[0] == 'M') {            //Motorsteuerung
        if(payload[1] == 'S') {
          int motor = (int)payload[2];
          controlMotorStatus("on", 1);
          controlMotorStatus("on", 2);
          client.publish("home/kueche/futterautomat/log", "Motor an");
        }
        client.publish("home/kueche/futterautomat/log", "trollolo");
      }
      break;
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  char msg[length+1];
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    msg[i] = (char)payload[i];
  }

  msg[length] = '\0';
  
  //Motorstatus kontrollieren
  if(strcmp(topic, "home/kueche/futterautomat/motorstatus")==0){
    controlMotorStatus(msg, 1);
    controlMotorStatus(msg, 2);
  } else if (strcmp(topic, "home/kueche/futterautomat/motordirection")==0){
    changeMotorDirection(msg, 1);
    changeMotorDirection(msg, 2);
  }
}

void controlMotorStatus(char* msg, int motor) {
  int gpioIN1;
  int gpioIN2;
  
  if (motor == 1) {
    gpioIN1 = IN1;
    gpioIN2 = IN2;
  } else if (motor == 2) {
    gpioIN1 = IN3;
    gpioIN2 = IN4;
  }
  
  if(strcmp(msg, "on")==0){
    // turn on motor
    //digitalWrite(ENA, HIGH); // set speed to 200 out of possible range 0~255
    digitalWrite(gpioIN1, HIGH);
    digitalWrite(gpioIN2, LOW);
    MOTOR_STATUS = 1;
  } else if (strcmp(msg, "off")==0) {
    digitalWrite(gpioIN1, LOW);
    digitalWrite(gpioIN2, LOW);
    MOTOR_STATUS = 0;
  }
}

void changeMotorDirection(char* msg, int motor){
  int gpioIN1;
  int gpioIN2;
  
  if (motor == 1) {
    gpioIN1 = IN1;
    gpioIN2 = IN2;
  } else if (motor == 2) {
    gpioIN1 = IN3;
    gpioIN2 = IN4;
  }
  
  if(strcmp(msg, "left")==0){
    //Motor 1
    digitalWrite(gpioIN1, LOW);
    digitalWrite(gpioIN2, HIGH);   
    MOTOR_STATUS = 1;
  } else if (strcmp(msg, "right")==0){
    digitalWrite(gpioIN1, HIGH);
    digitalWrite(gpioIN2, LOW);
    MOTOR_STATUS = 1;
  }
}

void toggleMotors() {
  if (MOTOR_STATUS == 0) {
    controlMotorStatus("on", 1);
    controlMotorStatus("on", 2); 
  } else if (MOTOR_STATUS == 1) {
    controlMotorStatus("off", 1);
    controlMotorStatus("off", 2);
  }
}

void loop() {
  ArduinoOTA.handle();

  client.loop();

  webSocket.loop(); 

  button1.tick();
  button2.tick();
  button3.tick();
}

void doubleClickButton1(){
  //changeMotorDirection("left", 1);
  //changeMotorDirection("left", 2);
  rightMotor.backward();
  leftMotor.backward();
}

void clickButton2(){
  controlMotorStatus("off", 1);
  controlMotorStatus("off", 2);
  rightMotor.stop();
  leftMotor.stop();
}

void doubleClickButton3(){
  changeMotorDirection("right", 1);
  changeMotorDirection("right", 2);
}