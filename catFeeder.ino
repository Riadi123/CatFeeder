// Look for all "REPLACEME" before uploading the code.
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoOTA.h>
#include <Servo.h>
// wifi
ESP8266WiFiMulti wifiMulti;
WiFiClientSecure client;

//Servo
int servoPin=14;//D5
Servo servo;

// ultrasonic
long t;
int trigger = 4;//D2
int echo = 5;//D1
float distance;
float percentageFood;
float max_food = 27.00;

// telegram
#define BOTtoken "632546814:AAHTB7A82XsaqN6xZFlP4Ao6ixhBF49ocG8"
UniversalTelegramBot bot(BOTtoken, client);
int Bot_mtbs = 1000;
long Bot_lasttime;
bool Start = false;

void setup() {
  // Serial setup
  Serial.begin(115200);

  // Wifi connection setup
  wifiMulti.addAP("Jancuk", "bokirkimping");
  
  while (wifiMulti.run() != WL_CONNECTED) {         // Wait for the Wi-Fi to connect: scan for Wi-Fi networks, and connect to the strongest of the networks above
    delay(1000);
    Serial.print('.');
  }
  Serial.print(WiFi.localIP());

  // pins setup

  pinMode(trigger, OUTPUT);
  pinMode(echo, INPUT);

  //servo
  servo.attach(servoPin);

  // OTA setup
  ArduinoOTA.setHostname("catFeeder");
  ArduinoOTA.begin();
}

// calc remaining food in %
void calcRemainingFood() {
  digitalWrite(trigger, LOW);
  delayMicroseconds(2);
  digitalWrite(trigger, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigger, LOW);
  t = (pulseIn(echo, HIGH) / 2);
  if (t == 0.00) {
    Serial.println("Failed to read from SR02");
    delay(1000);
    return;
  }
  distance = float(t * 0.0343);
  Serial.print("Distance:\t");
  Serial.println(distance);
//  Serial.println(t);
  percentageFood = (100 - ((100 / max_food) * distance));
  if (percentageFood < 0.00) {
    percentageFood = 0.00;
  }
  Serial.print("Remaining food:\t");
  Serial.print(percentageFood);
  Serial.println(" %");
  delay(500);
}

// feeds cats
void feedCats() {
    servo.write(0);
    delay(1000);
    servo.write(90);
    delay(1000);
}


// telegram message handler
void handleNewMessages(int numNewMessages) {
  Serial.println("handleNewMessages");
  Serial.println(String(numNewMessages));

  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    String text = bot.messages[i].text;

    String from_name = bot.messages[i].from_name;
      if (text == "/feed") {
        if (percentageFood == 0.00) {
          bot.sendMessage(chat_id, "There's no food! (Ultrasonic measured distance: " + String(distance) + " cm).", "");
        }
        else {
          feedCats();
          bot.sendMessage(chat_id, "Cats feeded! Remaining food: " + String(percentageFood) + " %. Ultrasonic measured distance: " + String(distance) + " cm.", "");
        }
      }
      if (text == "/status") {
        calcRemainingFood();
        char buffer[5];
        bot.sendMessage(chat_id, "Remaining food: " + String(percentageFood) + " % (Ultrasonic measured distance: " + String(distance) + " cm).", "");
      }
      if (text == "/help" || text == "/start") {
        //String welcome = "Welcome to the most awesome ESP8266 catFeeder, " + from_name + "!\n";
        String welcome = "Welcome to the most awesome ESP8266 catFeeder!\n";
        welcome += "/feed : Delivers one dose of feed.\n";
        welcome += "/help : Outputs this help message.\n";
        welcome += "/status : Returns remaining feed quantity.\n";
        bot.sendMessage(chat_id, welcome, "Markdown");
      }
    }
}



void loop() {
  ArduinoOTA.handle();
  calcRemainingFood();
  Serial.println(WiFi.localIP());
  if (millis() > Bot_lasttime + Bot_mtbs)  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while (numNewMessages) {
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }

    Bot_lasttime = millis();
  }
  //delay(1000);
}
