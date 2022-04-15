// all private keys
#include <credentials.h>

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
// No RTC Library
//#include "noRTC.h"
// Getting Time and Parsing

#include <ESP8266HTTPClient.h>
// Telegram Bot
#include <WiFiClientSecure.h> 
#include <UniversalTelegramBot.h>
//#include <ArduinoJson.h>

#include <NTPClient.h>
#include <WiFiUdp.h>

// MQTT
#include <PubSubClient.h>

// WebServer
#include <ESP8266WebServer.h>

// Initialize Telegram BOT 
#define BOT_TOKEN MY_BOT_TOKEN
#define CHAT_ID MY_CHAT_ID

X509List cert(TELEGRAM_CERTIFICATE_ROOT);
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);

// WebServer
ESP8266WebServer server(80);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

//Week Days
String weekDays[7]={"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

//Month names
String months[12]={"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};


// OTA
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
const char* ARDUINO_OTA_PW = OTA_PASS;
const char* kSsid = WLAN_SSID;
const char* kPassword = WLAN_PASS;

// MQTT
const char* MQTT_BROKER = AIO_SERVER;
unsigned long MQTT_PORT = AIO_SERVERPORT;
const char* MQTT_USERNAME = AIO_USERNAME;
const char* MQTT_PASSWORD = AIO_KEY;


bool wateringFlag = false;
int waterlevelflag = 0;
bool manualwateringFlag = false;

#define PUMPPIN 4
#define WATERLEVELPIN 12
#define WATERING_TIME 3000
#define HOSTNAME "FlowerRobot_ESP8266"
#define MOISTURE_VALUE 45
#define DELAY_TIME_WATERING 1200000
#define TZ_SETTING 7200
  // Set offset time in seconds to adjust for your timezone, for example:
  // GMT +1 = 3600
  // GMT +8 = 28800
  // GMT -1 = -3600
  // GMT 0 = 0


const int AirValue = 846;   //620 you need to replace this value with Value_1
const int WaterValue = 429;  //310 you need to replace this value with Value_2
int soilMoistureValue = 0;
int soilmoisturepercent=0;

//MDNSResponder mdns;
 WiFiClient espclient;
 PubSubClient client(espclient);

unsigned long previousTime = 0;

unsigned long uCurrentTime = millis();
unsigned long PrevMqttTime = millis();
unsigned long uPreviousTime = millis();
unsigned long uWIFIPreviousTime = millis();

unsigned long uWaterTimeCurrent = millis();
unsigned long uWaterTimePrev = millis();
unsigned long uWaterLevelPrev = millis();
unsigned long checkHourTime = 19;

unsigned long moist_value = 0;
unsigned int moist_value_percent = 0;
unsigned int moist_value_percent_last = 0;

int seconds = 0;
int minutes = 02;
int hours = 12;
int days = 0;

long lastMsg = 0;
char msg[50];
int value = 0;



// WebServer
void handleRoot() {
  char temp[500];
  snprintf(temp, 500,

  "<html>\
  <head>\
  <meta http-equiv='refresh' content='5'/>\
  <title>FlowerBot StatusPage</title>\
  <style>\
  body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>FlowerBot Status</h1> \
    <p>Moisture: %04d</p> \
    <p>Moisture [%%]: %03d</p> \
    <p>Time: %02d:%02d:%02d </p>  \
    <p>Check Time: %02d </p>  \
    <p>Water Level: %1d </p> \  
  </body> \
  </html>", moist_value, moist_value_percent, hours, minutes, seconds, checkHourTime, waterlevelflag);
  
  server.send(200, "text/html", temp);
}

void handlePostForms() {
  const String postForms = "<html>\
  <head>\
  <title>FlowerBot StatusPage</title>\
  <style>\
  body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
  <h1>POST form data to /postform/</h1><br>\
    <form method=\"post\" enctype=\"application/x-www-form-urlencoded\" action=\"/postform/\">\
      <input type=\"text\" name=\"hello\" value=\"0\"><br>\
      <input type=\"submit\" value=\"Submit\">\
    </form>\
  </body>\
  </html>";
  
  server.send(200, "text/html", postForms);
}

void handlePump() {
  const String temp = "{'status':'pump activated'}";
  
  server.send(200, "application/json", temp);
  manualwateringFlag = true;
}

void handleForm() {
  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Method Not Allowed");
  } else {
    //String message = "POST form was:\n";
    //for (uint8_t i = 0; i < server.args(); i++) {
    //  message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    if (server.arg(0).toInt() != 0){
    checkHourTime = server.arg(0).toInt();
    String message = "Data set:\n";
    message += " " + server.argName(0) + ": " + server.arg(0) + "\n";
    server.send(200, "text/plain", message);
    }
  }
}

void timehandle(){
    hours = timeClient.getHours();
    minutes = timeClient.getMinutes();
    seconds = timeClient.getSeconds();
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

void init_OTA(void) {
    // Port defaults to 8266
   //ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
   ArduinoOTA.setHostname(HOSTNAME);

  // No authentication by default
   ArduinoOTA.setPassword(ARDUINO_OTA_PW);

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
}

void init_wifi(){
// Init WIFI
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(kSsid, kPassword);
  secured_client.setTrustAnchors(&cert); // Add root certificate for api.telegram.org
  
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(kSsid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP().toString());

//    // Init mDNS
//  if (mdns.begin(HOSTNAME, WiFi.localIP())) {
//    Serial.println("MDNS responder started");
//  }

  // Init OTA
  init_OTA();
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    //ESP.restart();   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
  } else {
    //digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }
}

void init_time(){
  Serial.print("Retrieving time: ");
  //configTime(0, 0, "pool.ntp.org"); // get UTC time via NTP
  time_t now = time(nullptr);

// Initialize a NTPClient to get time
  timeClient.begin();
  // Set offset time in seconds to adjust for your timezone, for example:
  // GMT +1 = 3600
  // GMT +8 = 28800
  // GMT -1 = -3600
  // GMT 0 = 0
  timeClient.setTimeOffset(TZ_SETTING);
  Serial.printf("NTP Time: %d:%d:%d",timeClient.getHours(), timeClient.getMinutes(), timeClient.getSeconds());

  
  while (now < 24 * 3600)
  {
    Serial.print(".");
    delay(100);
    now = time(nullptr);
  }
  Serial.println(now);

  // hold lib
  //sync_time();
  bot.sendMessage(CHAT_ID, "FlowerBot started up", "");
}

void init_webserver(){
  server.on("/", handleRoot);
  server.on("/setchecktime", handlePostForms);
  server.on("/postform/", handleForm);
  server.on("/togglepump", handlePump);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");
}

void init_mqtt(){
    client.setServer(MQTT_BROKER, MQTT_PORT);
    client.setCallback(callback);
    //reconnect();
}

void setup() {
  // put your setup code here, to run once:

  Serial.begin(115200);
  Serial.println("");
  
  init_wifi();

  pinMode(PUMPPIN, OUTPUT);
  digitalWrite(PUMPPIN, LOW);

  pinMode(WATERLEVELPIN, INPUT);

  // OTA
  //init_OTA();
  // WebServer
  init_webserver();
  // Time
  init_time();
  // Init MQTT
  init_mqtt();
  }
 
void reconnect() {
    while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");
        String clientId = HOSTNAME;
        clientId += String(random(0xffff), HEX);
        if (client.connect(clientId.c_str(), MQTT_USERNAME, MQTT_PASSWORD)) {
          Serial.println("connected");
          client.publish("/tele/FlowerBot/Online", "connected");
          client.subscribe("/cmnd/FlowerBot/command");
        } else {
          Serial.print("failed, rc=");
          Serial.print(client.state());
          Serial.println(" retrying in 5 seconds");
          delay(5000);
        }
    }
}

void Pump(bool onoff)
{
  if (onoff){
    digitalWrite(PUMPPIN, HIGH);
  } else {
    digitalWrite(PUMPPIN, LOW);
  }
} 

void checkWatering(){
      uWaterTimeCurrent = millis();
      if (hours == checkHourTime && minutes > 0 && (moist_value_percent < MOISTURE_VALUE) && !wateringFlag && (uWaterTimeCurrent - uWaterTimePrev > DELAY_TIME_WATERING) || manualwateringFlag) {
          manualwateringFlag = false;
          wateringFlag = true;
          uWaterTimePrev = millis();
          bot.sendMessage(CHAT_ID, "FlowerBot started Watering", "");
          // check WaterLevel in Tank
          Pump(true);
    } else if ((uWaterTimeCurrent - uWaterTimePrev > WATERING_TIME) && wateringFlag) {
      // reset Flag
      wateringFlag = false;
      uWaterTimePrev = uWaterTimeCurrent;
      Pump(false);
      manualwateringFlag = false;
      bot.sendMessage(CHAT_ID, "FlowerBot stopped Watering", "");
    }
    if (hours == checkHourTime && minutes == 0 && (abs(uWaterTimeCurrent - uWaterLevelPrev) > 61000) && !waterlevelflag)
    {
      bot.sendMessage(CHAT_ID, "FlowerBot needs water!");
      uWaterLevelPrev = uWaterTimeCurrent;
    }
}

void debugMsg(){
    Serial.print("Watering Flag: ");
    Serial.println(wateringFlag);
    Serial.print("Moisture Sensor Value: ");
    Serial.println(moist_value);
    Serial.print("The time is:           ");
    //Serial.print(days);
    //Serial.print(":");
    Serial.print(hours);
    Serial.print(":");
    Serial.print(minutes);
    Serial.print(":"); 
    Serial.println(seconds); 
    //Serial.printf("NTP Time: %d:%d:%d", timeClient.getHours(),timeClient.getMinutes(),timeClient.getSeconds());
}

void loop() {
    // put your main code here, to run repeatedly:
    
    // WebServer Loop
    server.handleClient();
    //mdns.update();
    
    // OTA Loop
    ArduinoOTA.handle();
    if(!client.connected()){
      reconnect();
    }
    client.loop();
    delay(100);
    uCurrentTime = millis();
    // Time Loop
    timeClient.update();
    timehandle();
    checkWatering();

    // Watch out with to many variables before mqtt publish, creates strange reconnect errors
    int wifiretry = 0;
    if(abs(uCurrentTime - PrevMqttTime) > 60000){
      moist_value = analogRead(A0);
      moist_value_percent = map(moist_value, AirValue, WaterValue, 0, 100);
      waterlevelflag = digitalRead(WATERLEVELPIN);
    //if(client.connected()){
      snprintf (msg, 50, "%ld milliseconds", millis());
      Serial.print("Publish message: ");
      Serial.println(msg);
      client.publish("/tele/FlowerBot/Status", msg);
      //if (moist_value_percent_last != moist_value_percent){
      snprintf (msg, 50, "%ld", moist_value_percent);
      Serial.print("Publish message: ");
      Serial.println(msg);
      client.publish("/tele/FlowerBot/Humidity", msg);
      snprintf (msg, 50, "%d", waterlevelflag);
      client.publish("/tele/FlowerBot/WaterLevel", msg);
      //moist_value_percent_last = moist_value_percent;
      //}
      Serial.print("WaterFlag: ");
      Serial.println(waterlevelflag);
      PrevMqttTime = uCurrentTime;
    }
    
  if (abs(uCurrentTime - uWIFIPreviousTime) > 5000) {
    while (WiFi.status() != WL_CONNECTED && wifiretry < 5) {
      wifiretry++;
      WiFi.persistent(false);
      WiFi.disconnect();
      WiFi.mode(WIFI_OFF);
      delay(250);
      //mdns.end();
      init_wifi();
    }
    uWIFIPreviousTime = uCurrentTime;
    if(wifiretry > 5){
      Serial.println("\nReboot");
      ESP.restart();
    }
  }
    if ( uCurrentTime - uPreviousTime > 10000){  
    debugMsg();
    uPreviousTime = uCurrentTime;
    }
}
