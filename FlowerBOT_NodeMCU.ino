
/*******************************************************************
*  An example of bot that echos back any messages received         *
*                                                                  *
*  written by Giacarlo Bacchio (Gianbacchio on Github)             *
*  adapted by Brian Lough                                          *
*******************************************************************/
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

// Initialize Wifi connection to the router
char ssid[] = "ssid";     // your network SSID (name)
char password[] = "yourpassword"; // your network key

// Initialize Telegram BOT
#define BOTtoken "username:password"  // your Bot Token (Get from Botfather)
#define ChatID "yourchatID"

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

int Bot_mtbs = 100000; //mean time between scan messages
long Bot_lasttime;   //last time messages' scan has been done
bool b_lastThreshold = false;

void setup() {
  Serial.begin(115200);

  // Set WiFi to station mode and disconnect from an AP if it was Previously
  // connected
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  // Attempt to connect to Wifi network:
  Serial.print("Connecting Wifi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Inputconfig for Moisture sensor
  pinMode(D0, INPUT);
}

void loop() {
   
  if (millis() > Bot_lasttime + Bot_mtbs)  {
    // checks if new messages has been sent to bot
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    
    // get latest value of moisture sensor
    bool b_thresholdReached = digitalRead(D0);
    int i_moistureValue = map(analogRead(A0), 0, 1023, 0, 100);
    
    // reads moisture Sensor
    Serial.println(b_thresholdReached);
    Serial.println(i_moistureValue);

    while(b_lastThreshold != b_thresholdReached)
    {
      Serial.println("Moisture has changed!");
      if(b_thresholdReached == true)
      {
         bot.sendMessage(ChatID,"Flower X needs water!", "");
      }
      else
      {
         bot.sendMessage(ChatID, "Flower X: Thank you !", "");
      }

      b_lastThreshold = b_thresholdReached;
      Bot_lasttime = millis();
    }
  }
}
