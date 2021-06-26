#include <Arduino.h>

float readLux(int LIGHT_SENSOR_PIN){
  float mV = (analogRead(LIGHT_SENSOR_PIN) * 3300.0) / 1023.0;
  float lux_value = (mV / 2.0); // Readings are in Lux scale
  return lux_value;
}