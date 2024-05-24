#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"
#include <SPI.h>
#include <MFRC522.h>

// Uncomment one of the lines bellow for whatever DHT sensor type you're using!
#define DHTTYPE DHT11   // DHT 11
//#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

// Change the credentials below, so your ESP8266 connects to your router
const char* ssid = "Cyber";
const char* password = "threshold";

// Change the variable to your Raspberry Pi IP address, so it connects to your MQTT broker
const char* mqtt_server = "172.20.10.8";

#define D3 0
#define D4 2
constexpr uint8_t RST_PIN = D3;     // Configurable, see typical pin layout above
constexpr uint8_t SS_PIN = D4;     // Configurable, see typical pin layout above
constexpr uint8_t Active_buzzer = D0;

MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class
MFRC522::MIFARE_Key key;
String tag;


// Initializes the espClient. You should change the espClient name if you have multiple ESPs running in your home automation system
WiFiClient espClient;
PubSubClient client(espClient);

// DHT Sensor - GPIO 5 = D1 on ESP-12E NodeMCU board
const int DHTPin = 5;

// Lamp - LED - GPIO 4 = D2 on ESP-12E NodeMCU board
const int lamp = 4;

// Light Watcher - LED - GPIO 15 = D8 on ESP-12E NodeMCU board
const int lightOut = 15;
String lightThresholdChecker = "0";

// Initialize DHT sensor.
DHT dht(DHTPin, DHTTYPE);

// Timers auxiliar variables
long now = millis();
long lastMeasure = 0;

// Don't change the function below. This functions connects your ESP8266 to your router
void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi connected - ESP IP address: ");
  Serial.println(WiFi.localIP());
}

void buzzer(int repeat, int timeDelay) {
  for (int i = 0; i < repeat; i++) {
    digitalWrite(Active_buzzer,HIGH);
    delay (timeDelay);
    digitalWrite(Active_buzzer,LOW);
    delay (timeDelay);
  }
}

// This functions is executed when some device publishes a message to a topic that your ESP8266 is subscribed to
// Change the function below to add logic to your program, so when a device publishes a message to a topic that 
// your ESP8266 is subscribed you can actually do something
void callback(String topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  // Feel free to add more if statements to control more GPIOs with MQTT

  // If a message is received on the topic room/lamp, you check if the message is either on or off. Turns the lamp GPIO according to the message
  if(topic=="room/led"){
      Serial.print("Changing Room lamp to ");
      if(messageTemp == "on"){
        digitalWrite(lamp, HIGH);
        Serial.print("On");
      }
      else if(messageTemp == "off"){
        digitalWrite(lamp, LOW);
        Serial.print("Off");
      }
  }

  if(topic=="room/threshold"){
      Serial.print("Changed temperature threshold to ");
      Serial.print(messageTemp);
      
//      
//      if (messageTemp.toInt() > temperatureFloat){
//          Serial.println("");
//          Serial.print("Sending email...");
//          client.publish("room/send", "Do you want to turn on the fan?");
//      }
//
      // Read temperature as Celsius (the default)
      float temperatureFloat = dht.readTemperature();

      if (messageTemp.toInt() > temperatureFloat){
          Serial.println("");
          Serial.print("Sending email...");
          
          char temperatureChar[100];

          
          dtostrf(temperatureFloat,4,2,temperatureChar);
          char* firsthalf = "the current temperature is ";

          char result[5];
          String temperatureString = dtostrf(temperatureFloat, 6,2, result);
          const char* middle = temperatureString.c_str();

          
          char* secondhalf =  " degrees. Do you want to turn on the fan?";

          strcpy(temperatureChar,firsthalf);
          strcat(temperatureChar,middle);
          strcat(temperatureChar,secondhalf);
          
          client.publish("room/send", temperatureChar);
      }
  }

  if (topic=="room/light") {
      Serial.print("Changed light threshold to ");
      Serial.print(messageTemp);
      lightThresholdChecker = messageTemp;
      
  }

  if(topic=="room/email"){
      Serial.print("New email: ");
      Serial.print(messageTemp);
   }

  if(topic=="RFID/Response"){
    if (messageTemp != "[]") {

        String temp  = messageTemp.substring((messageTemp.indexOf("p") + 3), messageTemp.indexOf(","));
        String light  = messageTemp.substring((messageTemp.lastIndexOf("t") + 3), messageTemp.indexOf("}"));
        
        Serial.println("Access Granted!");
        buzzer(3, 100);
    } else {
        Serial.println("Access Denied!");
        buzzer(1, 2000);
    }
  }
  Serial.println();
}
  

// This functions reconnects your ESP8266 to your MQTT broker
// Change the function below if you want to subscribe to more topics with your ESP8266 
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    /*
     YOU MIGHT NEED TO CHANGE THIS LINE, IF YOU'RE HAVING PROBLEMS WITH MQTT MULTIPLE CONNECTIONS
     To change the ESP device ID, you will have to give a new name to the ESP8266.
     Here's how it looks:
       if (client.connect("ESP8266Client")) {
     You can do it like this:
       if (client.connect("ESP1_Office")) {
     Then, for the other ESP:
       if (client.connect("ESP2_Garage")) {
      That should solve your MQTT multiple connections problem
    */
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");  
      // Subscribe or resubscribe to a topic
      // You can subscribe to more topics (to control more LEDs in this example)
      client.subscribe("room/led");
      client.subscribe("room/threshold");
      client.subscribe("room/email");
      client.subscribe("room/send");
      client.subscribe("room/light");
      client.subscribe("RFID/Response");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

// The setup function sets your ESP GPIOs to Outputs, starts the serial communication at a baud rate of 115200
// Sets your mqtt broker and sets the callback function
// The callback function is what receives messages and actually controls the LEDs
void setup() {
  pinMode(lamp, OUTPUT);
  
  dht.begin();
  
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  SPI.begin(); // Init SPI bus
  rfid.PCD_Init(); // Init MFRC522
  pinMode(D8, OUTPUT);
  pinMode(Active_buzzer, OUTPUT);

}

// For this project, you don't need to change anything in the loop function. Basically it ensures that you ESP is connected to your broker
void loop() {

  if (!client.connected()) {
    reconnect();
  }
  
  if(!client.loop())
    client.connect("ESP8266Client");

   if (!rfid.PICC_IsNewCardPresent()) {
    return;
  }
  
  now = millis();

  if (rfid.PICC_ReadCardSerial() && now - lastMeasure > 2000) {
    for (byte i = 0; i < 4; i++) {
      tag += rfid.uid.uidByte[i];
    }
    int str_len = tag.length()+1;
    char tag_array[str_len];
    tag.toCharArray(tag_array, str_len);
    client.publish("RFID", tag_array);
    tag = "";
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  }
  
  if (now - lastMeasure > 4000) {
    // read the input on analog pin 0:
    int sensorValue = analogRead(A0);
    // Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 5V):
    float voltage = sensorValue * (5.0 / 1023.0);
    // print out the value you read:
    Serial.println(voltage);

  if (voltage < lightThresholdChecker.toInt()) {
      Serial.println("the threshold is " + lightThresholdChecker);
      // turn on led
      digitalWrite(lightOut, HIGH);
      Serial.println("The light of light intensity is On");
      client.publish("room/send", "Light for light intensity threshold is On");
    } else if (voltage > lightThresholdChecker.toInt()) {
      digitalWrite(lightOut, LOW);
      client.publish("room/send", "Light is Off");
      Serial.println("The light for light intensity threshold Off");
    }
  }
  
  // Publishes new temperature and humidity every 5 seconds
  if (now - lastMeasure > 5000) {
    lastMeasure = now;
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    float h = dht.readHumidity();
    // Read temperature as Celsius (the default)
    float t = dht.readTemperature();
    // Read temperature as Fahrenheit (isFahrenheit = true)
    float f = dht.readTemperature(true);

    // Check if any reads failed and exit early (to try again).
     if (isnan(h) || isnan(t) || isnan(f)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }

    // Computes temperature values in Celsius
    float hic = dht.computeHeatIndex(t, h, false);
    static char temperatureTemp[7];
    dtostrf(hic, 6, 2, temperatureTemp);
    
    // Uncomment to compute temperature values in Fahrenheit 
    // float hif = dht.computeHeatIndex(f, h);
    // static char temperatureTemp[7];
    // dtostrf(hif, 6, 2, temperatureTemp);
    
    static char humidityTemp[7];
    dtostrf(h, 6, 2, humidityTemp);

    // Publishes Temperature and Humidity values
    client.publish("room/temperature", temperatureTemp);
    client.publish("room/humidity", humidityTemp);
    
    Serial.print("Humidity: ");
    Serial.print(h);
    Serial.print(" %\t Temperature: ");
    Serial.print(t);
    Serial.print(" *C ");
    Serial.print(f);
    Serial.print(" *F\t Heat index: ");
    Serial.print(hic);
    Serial.println(" *C ");
    // Serial.print(hif);
    // Serial.println(" *F");
  }
} 
