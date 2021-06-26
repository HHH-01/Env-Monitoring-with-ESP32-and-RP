#include <Arduino.h>
#include <Wire.h>
#include "WiFi.h" // Enables the ESP32 to connect to the local network (via WiFi)

//Special Inlude
//#include <PubSubClient.h> // Connect and publish to the MQTT broker
#include "PubSubClient.h" // Connect and publish to the MQTT broker

//#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
//Customized Include
#include <MQTT_info.h>
#include <WIFI_info.h>


#define POTENTIOMETER_PIN 33  //Using ADC1 (GPIO32 - GPIO39), ADC2 is conflict with Wifi
#define DHT_PIN 14  //DHT11

float pot_voltage;
int pot_ADC;

//DHT sensor
#define DHTTYPE DHT11
DHT dht(DHT_PIN, DHTTYPE);

// Wifi credentials
const char *WIFI_SSID = WIFI_SSID_INFO;
const char *WIFI_PASSWORD = WIFI_PASSWORD_INFO;

// Initialise the WiFi and MQTT Client objects
WiFiClient wifiClient;
// 1883 is the listener port for the Broker
PubSubClient client(mqtt_server, 1883, wifiClient); 

  void connectToWiFi()
  {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    // Only try 15 times to connect to the WiFi
    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 15){
      delay(500);
      Serial.print(".");
      retries++;
    }

    // If we still couldn't connect to the WiFi, go to deep sleep for a minute and try again.
    if(WiFi.status() != WL_CONNECTED){
      esp_sleep_enable_timer_wakeup(1 * 60L * 1000000L);
      esp_deep_sleep_start();
    }

  // Debugging - Output the IP Address of the ESP8266
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  }

void connectToMQTT(){
  // Connect to MQTT Broker
  // client.connect returns a boolean value to let us know if the connection was successful.
  // If the connection is failing, make sure you are using the correct MQTT Username and Password (Setup Earlier in the Instructable)
  if (client.connect(clientID, mqtt_username, mqtt_password)) {
    Serial.println("Connected to MQTT Broker!");
  }
  else {
    Serial.println("Connection to MQTT Broker failed...");
  }

}



void setup() {
  // put your setup code here, to run once:
  Serial.println("Harry-IoT Application Set-up start..");
  Serial.begin(9600);
  connectToWiFi();
  dht.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.println("Harry-IoT Application Loop start......");
  connectToMQTT();

  pot_ADC = analogRead(POTENTIOMETER_PIN);
  pot_voltage = (pot_ADC * 3.3 ) / (4095);

  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  Serial.println("List of parameters to send out:");
  Serial.print("1. Pot_voltage: ");  Serial.println(pot_voltage);
  Serial.print("2. Temperature: ");  Serial.println(temperature);
  Serial.print("3. Humidity: ");  Serial.println(humidity);

  //********************************************************************
//********************************************************************
// MQTT can only transmit strings
//String hs="Hum: "+String((float)humidity_reading)+" % ";
//String ts="Temp: "+String((float)temperature_reading)+" C ";
delay(100);
Serial.println("MQTT Process:");

// PUBLISH to the MQTT Broker (topic = pot_voltage, defined at the beginning)
  if (client.publish(pot_voltage_topic, String(pot_voltage).c_str())) {
    Serial.println("Pot Voltage sent!");
  }
  // Again, client.publish will return a boolean value depending on whether it succeded or not.
  // If the message failed to send, we will try again, as the connection may have broken.
  else {
    Serial.println("Pot Voltage failed to send. Reconnecting to MQTT Broker and trying again");
    client.connect(clientID, mqtt_username, mqtt_password);
    delay(10); // This delay ensures that client.publish doesn't clash with the client.connect call
    client.publish(pot_voltage_topic, String(pot_voltage).c_str());
  }

// PUBLISH to the MQTT Broker (topic = temperature, defined at the beginning)
  if (client.publish(temperature_topic, String(temperature).c_str())) {
    Serial.println("Temperature sent!");
  }
  // Again, client.publish will return a boolean value depending on whether it succeded or not.
  // If the message failed to send, we will try again, as the connection may have broken.
  else {
    Serial.println("Temperature failed to send. Reconnecting to MQTT Broker and trying again");
    client.connect(clientID, mqtt_username, mqtt_password);
    delay(10); // This delay ensures that client.publish doesn't clash with the client.connect call
    client.publish(temperature_topic, String(temperature).c_str());
  }

  // PUBLISH to the MQTT Broker (topic = Humidity, defined at the beginning)
  if (client.publish(humidity_topic, String(humidity).c_str())) {
    Serial.println("Humidity sent!");
  }
  // Again, client.publish will return a boolean value depending on whether it succeded or not.
  // If the message failed to send, we will try again, as the connection may have broken.
  else {
    Serial.println("Humidity failed to send. Reconnecting to MQTT Broker and trying again");
    client.connect(clientID, mqtt_username, mqtt_password);
    delay(10); // This delay ensures that client.publish doesn't clash with the client.connect call
    client.publish(humidity_topic, String(humidity).c_str());
  }









  client.disconnect();  // disconnect from the MQTT broker
  delay(1*1000); //Delay every 1 second
  Serial.println("");
}