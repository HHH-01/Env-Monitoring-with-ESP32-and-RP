//Notice: 
/*
Library Notice: 
1. Modified Adafruit_LPS2X.h to use the default address on the MKR ENV board (from 0x5D to 0x5C) Feb-16-2020
2. ADC for the Lux need to keep it as Pin#33, otherwise the wifi/ADC conflict/Ability to connect COM will be disrupted
*/


#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_HTS221.h>
#include <LPS22HBSensor.h>
#include "Adafruit_VEML6075.h"
#include <Adafruit_Sensor.h>
#include <ArduinoJson.h>
#include "WiFi.h" // Enables the ESP32 to connect to the local network (via WiFi)

//Special Inlude
#include <LuxReading.h> //Created your own one
#include <Adafruit_LPS2X.h> //Harry Modified the default address from 0x5D to 0x5C Feb-16-2020
#include "PubSubClient.h" // Connect and publish to the MQTT broker

//Customized Include
#include <MQTT_info.h>
#include <WIFI_info.h>

#define LIGHT_SENSOR_PIN 33  //#define Ambient Light Sensor //Need to keep it at Pin #33
#define SCL 22              //default SCL ESP32
#define SDA 21              //default SDA on ESP32


Adafruit_HTS221 hts; 
Adafruit_Sensor *hts_humidity, *hts_temp, *lps_temp, *lps_pressure;
Adafruit_VEML6075 uv = Adafruit_VEML6075();
Adafruit_LPS22 lps;


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




//Function Declaration
float readLux();



void setup() {
  Serial.println("Harry-IoT Application Set-up start..");
  // put your setup code here, to run once:
  //Serial.begin(115200);
  Serial.begin(9600);
  connectToWiFi();
  

  while (!Serial)
  delay(10); // will pause Zero, Leonardo, etc until serial console opens
  pinMode(LIGHT_SENSOR_PIN, OUTPUT);
  
  //=============HTS221 Set up Begin============================================
  while (!Serial)
    delay(10); // will pause Zero, Leonardo, etc until serial console opens

  Serial.println("Adafruit HTS221 test!");

  if (!hts.begin_I2C()) {
    Serial.println("Failed to find HTS221 chip");
    while (1) {
      delay(10);
    }
  }

  Serial.println("HTS221 Found!");
  hts_temp = hts.getTemperatureSensor();
  hts_temp->printSensorDetails();

  hts_humidity = hts.getHumiditySensor();
  hts_humidity->printSensorDetails();

  //=============HTS221 Set up End============================================
  
  //=============LTS Set up Begin=============================================
  Serial.println("Adafruit LPS22 test!");

  // Try to initialize!
  if (!lps.begin_I2C()) {
  //if (!lps.begin_SPI(LPS_CS)) {
  //if (!lps.begin_SPI(LPS_CS, LPS_SCK, LPS_MISO, LPS_MOSI)) {
    Serial.println("Failed to find LPS22 chip");
    while (1) { delay(10); }
  }
  Serial.println("LPS22 Found!");

  lps.setDataRate(LPS22_RATE_10_HZ);
  Serial.print("Data rate set to: ");
  switch (lps.getDataRate()) {
    case LPS22_RATE_ONE_SHOT: Serial.println("One Shot / Power Down"); break;
    case LPS22_RATE_1_HZ: Serial.println("1 Hz"); break;
    case LPS22_RATE_10_HZ: Serial.println("10 Hz"); break;
    case LPS22_RATE_25_HZ: Serial.println("25 Hz"); break;
    case LPS22_RATE_50_HZ: Serial.println("50 Hz"); break;

  }
  //=============LTS Set up End===============================================

  //=============VEML6075 Set up Begin===============================================
  Serial.println("VEML6075 Full Test");
  
  if (! uv.begin()) {
    Serial.println("Failed to communicate with VEML6075 sensor, check wiring?");
  }
  Serial.println("Found VEML6075 sensor");

  // Set the integration constant
  uv.setIntegrationTime(VEML6075_100MS);
  // Get the integration constant and print it!
  Serial.print("Integration time set to ");
  switch (uv.getIntegrationTime()) {
    case VEML6075_50MS: Serial.print("50"); break;
    case VEML6075_100MS: Serial.print("100"); break;
    case VEML6075_200MS: Serial.print("200"); break;
    case VEML6075_400MS: Serial.print("400"); break;
    case VEML6075_800MS: Serial.print("800"); break;
  }
  Serial.println("ms");

  // Set the high dynamic mode
  uv.setHighDynamic(true);
  // Get the mode
  if (uv.getHighDynamic()) {
    Serial.println("High dynamic reading mode");
  } else {
    Serial.println("Normal dynamic reading mode");
  }

  // Set the mode
  uv.setForcedMode(false);
  // Get the mode
  if (uv.getForcedMode()) {
    Serial.println("Forced reading mode");
  } else {
    Serial.println("Continuous reading mode");
  }

  // Set the calibration coefficients
  uv.setCoefficients(2.22, 1.33,  // UVA_A and UVA_B coefficients
                     2.95, 1.74,  // UVB_C and UVB_D coefficients
                     0.001461, 0.002591); // UVA and UVB responses
  //=============VEML6075 Set up End===============================================

}


void loop() {
  Serial.println("Harry-IoT Application Loop start......");
  connectToMQTT();
  
  //Lux Reading Begin==========================================================
  float lux;
  lux = readLux(LIGHT_SENSOR_PIN);
  //Serial.print("1. Ambient light: ");Serial.print(lux);Serial.println(" lux");
  //delay(100);
  //Lux Reading End==========================================================

  //Humidity+Temperature Reading Begin=======================================
  sensors_event_t humidity;
  sensors_event_t temp;
  hts_humidity->getEvent(&humidity);
  hts_temp->getEvent(&temp);
  
  /* Display the results (humidity is measured in % relative humidity (% rH) */
  /*
  Serial.print("2. HTS-Humidity: ");Serial.print(humidity.relative_humidity);Serial.println(" % rH");
  Serial.print("3. HTS-Temperature: ");Serial.print(temp.temperature);Serial.println(" degrees C");
  
  delay(100);
  */
  //Humidity+Temperature Reading End=======================================


  //Pressure Reading Begin=======================================
  sensors_event_t temp2;
  sensors_event_t pressure;
  lps.getEvent(&pressure, &temp2);// get pressure
  /*
  Serial.print("4. LPS-Temperature: ");Serial.print(temp2.temperature);Serial.println(" degrees C");
  Serial.print("5. LPS-Pressure: ");Serial.print(pressure.pressure);Serial.println(" hPa");
  delay(100);
  */
  //Pressure Reading End=======================================


  //UV Reading Begin=======================================
  /*
  Serial.print("6. Raw UVA reading:  "); Serial.println(uv.readUVA());
  Serial.print("7. Raw UVB reading:  "); Serial.println(uv.readUVB());
  Serial.print("8. UV Index reading: "); Serial.println(uv.readUVI());
  Serial.println("");
  delay(100);
  */
  //UV Reading End=======================================

  //Prepare list of variable to send to AWS:
  //1. Lux as lux
  Serial.println("List of parameters to send out:");
  Serial.print("Lux: ");  Serial.println(lux);
  float humidity_reading = humidity.relative_humidity; Serial.print("humidity_reading: ");  Serial.println(humidity_reading);
  float temperature_reading = temp.temperature;         Serial.print("temperature_reading: ");  Serial.println(temperature_reading);
  float pressure_reading = pressure.pressure;         Serial.print("pressure_reading: ");  Serial.println(pressure_reading);
  float UVA_reading = uv.readUVA(); Serial.print("UVA_reading: ");  Serial.println(UVA_reading);
  float UVB_reading = uv.readUVB(); Serial.print("UVB_reading: ");  Serial.println(UVB_reading);
  float UV_Index_reading = uv.readUVI(); Serial.print("UV_Index_reading: ");  Serial.println(UV_Index_reading);

//********************************************************************
//********************************************************************
// MQTT can only transmit strings
//String hs="Hum: "+String((float)humidity_reading)+" % ";
//String ts="Temp: "+String((float)temperature_reading)+" C ";
delay(100);
Serial.println("MQTT Process:");
// PUBLISH to the MQTT Broker (topic = Temperature, defined at the beginning)
  if (client.publish(temperature_topic, String(temperature_reading).c_str())) {
    Serial.println("Temperature sent!");
  }
  // Again, client.publish will return a boolean value depending on whether it succeded or not.
  // If the message failed to send, we will try again, as the connection may have broken.
  else {
    Serial.println("Temperature failed to send. Reconnecting to MQTT Broker and trying again");
    client.connect(clientID, mqtt_username, mqtt_password);
    delay(10); // This delay ensures that client.publish doesn't clash with the client.connect call
    client.publish(temperature_topic, String(temperature_reading).c_str());
  }

// PUBLISH to the MQTT Broker (topic = Humidity, defined at the beginning)
//******************************  
  if (client.publish(humidity_topic, String(humidity_reading).c_str())) {
    Serial.println("Humidity sent!");
  }
  // Again, client.publish will return a boolean value depending on whether it succeded or not.
  // If the message failed to send, we will try again, as the connection may have broken.
  else {
    delay(100);
    Serial.println("Humidity failed to send. Reconnecting to MQTT Broker and trying again");
    client.connect(clientID, mqtt_username, mqtt_password);
    delay(10); // This delay ensures that client.publish doesn't clash with the client.connect call
    client.publish(humidity_topic, String(humidity_reading).c_str());
  }

// PUBLISH to the MQTT Broker (topic = Pressure, defined at the beginning)
//******************************  
  if (client.publish(pressure_topic, String(pressure_reading).c_str())) {
    Serial.println("Pressure sent!");
  }
  // Again, client.publish will return a boolean value depending on whether it succeded or not.
  // If the message failed to send, we will try again, as the connection may have broken.
  else {
    delay(100);
    Serial.println("Pressure failed to send. Reconnecting to MQTT Broker and trying again");
    client.connect(clientID, mqtt_username, mqtt_password);
    delay(10); // This delay ensures that client.publish doesn't clash with the client.connect call
    client.publish(pressure_topic, String(pressure_reading).c_str());
  }
// PUBLISH to the MQTT Broker (topic = UVA, defined at the beginning)
//******************************  
  if (client.publish(UVA_topic, String(UVA_reading).c_str())) {
    Serial.println("UVA sent!");
  }
  // Again, client.publish will return a boolean value depending on whether it succeded or not.
  // If the message failed to send, we will try again, as the connection may have broken.
  else {
    delay(100);
    Serial.println("UVA failed to send. Reconnecting to MQTT Broker and trying again");
    client.connect(clientID, mqtt_username, mqtt_password);
    delay(10); // This delay ensures that client.publish doesn't clash with the client.connect call
    client.publish(UVA_topic, String(UVA_reading).c_str());
  }
// PUBLISH to the MQTT Broker (topic = UVB, defined at the beginning)
//******************************  
  if (client.publish(UVB_topic, String(UVB_reading).c_str())) {
    Serial.println("UVB sent!");
  }
  // Again, client.publish will return a boolean value depending on whether it succeded or not.
  // If the message failed to send, we will try again, as the connection may have broken.
  else {
    delay(100);
    Serial.println("UVB failed to send. Reconnecting to MQTT Broker and trying again");
    client.connect(clientID, mqtt_username, mqtt_password);
    delay(10); // This delay ensures that client.publish doesn't clash with the client.connect call
    client.publish(UVB_topic, String(UVB_reading).c_str());
  }
// PUBLISH to the MQTT Broker (topic = UV_Index, defined at the beginning)
//******************************  
  if (client.publish(UV_topic, String(UV_Index_reading).c_str())) {
    Serial.println("UV Index sent!");
  }
  // Again, client.publish will return a boolean value depending on whether it succeded or not.
  // If the message failed to send, we will try again, as the connection may have broken.
  else {
    delay(100);
    Serial.println("UV Index failed to send. Reconnecting to MQTT Broker and trying again");
    client.connect(clientID, mqtt_username, mqtt_password);
    delay(10); // This delay ensures that client.publish doesn't clash with the client.connect call
    client.publish(UV_topic, String(UV_Index_reading).c_str());
  }
// PUBLISH to the MQTT Broker (topic = Lux, defined at the beginning)
//******************************  
  if (client.publish(lux_topic, String(lux).c_str())) {
    Serial.println("Lux sent!");
  }
  // Again, client.publish will return a boolean value depending on whether it succeded or not.
  // If the message failed to send, we will try again, as the connection may have broken.
  else {
    delay(100);
    Serial.println("Lux failed to send. Reconnecting to MQTT Broker and trying again");
    client.connect(clientID, mqtt_username, mqtt_password);
    delay(10); // This delay ensures that client.publish doesn't clash with the client.connect call
    client.publish(lux_topic, String(lux).c_str());
  }
client.disconnect();  // disconnect from the MQTT broker
delay(1*1000); //Delay every 1 second
Serial.println("");
}


