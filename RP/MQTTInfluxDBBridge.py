import RPi.GPIO as GPIO	#Import Raspberry Pi GPIO lib
from time import sleep	#Import the sleep function from time module
import time
import requests
GPIO.setwarnings(False)	#Ignore warning for now
GPIO.setmode(GPIO.BOARD)	#Use Physical pin numbering
GPIO.setup(3, GPIO.OUT, initial=GPIO.LOW)
GPIO.setup(11, GPIO.OUT, initial=GPIO.LOW)
GPIO.setup(12, GPIO.OUT, initial=GPIO.LOW)
GPIO.setup(13, GPIO.OUT, initial=GPIO.LOW)
GPIO.setup(15, GPIO.OUT, initial=GPIO.LOW)
#Threshold for data
temperature_max = 30
humidity_max = 50
pressure_max = 0
UVA_max = 0
UVB_max = 0
UVIndex_max =0
lux_max =3000
voltage_max=1.5
import re
from typing import NamedTuple

import paho.mqtt.client as mqtt
from influxdb import InfluxDBClient

INFLUXDB_ADDRESS = '10.0.0.175'
INFLUXDB_USER = 'ENTER YOUR INFLUXDB USERNAME HERE'
INFLUXDB_PASSWORD = 'ENTER YOUR INFLUXDB PWD HERE'
INFLUXDB_DATABASE = 'harryspace'

MQTT_ADDRESS = '10.0.0.175'
MQTT_USER = 'ENTER YOUR MQTT USERNAME HERE'
MQTT_PASSWORD = 'ENTER YOUR MQTT PASSWORD HERE'

MQTT_TOPIC = 'harryspace/+/+'
MQTT_REGEX = 'harryspace/([^/]+)/([^/]+)'
MQTT_CLIENT_ID = 'MQTTInfluxDBBridge'

influxdb_client = InfluxDBClient(INFLUXDB_ADDRESS, 8086, INFLUXDB_USER, INFLUXDB_PASSWORD, None)

class SensorData(NamedTuple):
    location: str
    measurement: str
    value: float

def on_connect(client, userdata, flags, rc):
    """ The callback for when the client receives a CONNACK response from the server."""
    print('Connected with result code ' + str(rc))
    client.subscribe(MQTT_TOPIC)

def _parse_mqtt_message(topic, payload):
    match = re.match(MQTT_REGEX, topic)
    if match:
        location = match.group(1)
        measurement = match.group(2)
        if measurement == 'status':
            return None
        return SensorData(location, measurement, float(payload))
    else:
        return None

def _send_sensor_data_to_influxdb(sensor_data):
    json_body = [
        {
            'measurement': sensor_data.measurement,
            'tags': {
                'location': sensor_data.location
            },
            'fields': {
                'value': sensor_data.value
            }
        }
    ]
    influxdb_client.write_points(json_body)

def on_message(client, userdata, msg):
    """The callback for when a PUBLISH message is received from the server."""
    print(msg.topic + ' ' + str(msg.payload))
    sensor_data = _parse_mqtt_message(msg.topic, msg.payload.decode('utf-8'))
    if sensor_data is not None:
        _send_sensor_data_to_influxdb(sensor_data)



    
  #Zone2: temperature  alert
    if "temperature" in msg.topic and sensor_data.value >humidity_max and "02" in sensor_data.location :
        print('Humidity is greater than its max threshold')
        GPIO.output(13, GPIO.HIGH)
        requests.post('https://maker.ifttt.com/trigger/temperature_outofrange/with/key/bubRTZD1QqCUexGmtZWTxu', 
         params={"value1":temperature_max,"value2":sensor_data.value,"value3":"Zone#2"})
    elif "humidity" in msg.topic and sensor_data.value<humidity_max: 
        GPIO.output(13, GPIO.LOW)
   
 #Zone2: voltage alert
    if "voltage" in msg.topic and sensor_data.value >voltage_max and "02" in sensor_data.location:
        print('Pot of Voltage  Zone#2  is greater than its max threshold')
        GPIO.output(15, GPIO.HIGH)
        requests.post('https://maker.ifttt.com/trigger/voltage_outofrange/with/key/bubRTZD1QqCUexGmtZWTxu', 
         params={"value1":voltage_max,"value2":sensor_data.value,"value3":"Zone#2"})
    elif "voltage" in msg.topic and sensor_data.value <voltage_max:
        GPIO.output(15, GPIO.LOW)


 #Zone1: temperature alert
    if "temperature" in msg.topic and sensor_data.value >temperature_max and "01" in sensor_data.location:
        print('Temperature of Zone#1  is greater than its max threshold')
        GPIO.output(11, GPIO.HIGH)
        requests.post('https://maker.ifttt.com/trigger/temperature_outofrange/with/key/bubRTZD1QqCUexGmtZWTxu', 
         params={"value1":temperature_max,"value2":sensor_data.value,"value3":"Zone#1"})

    elif "temperature" in msg.topic and sensor_data.value<temperature_max: 
        GPIO.output(11, GPIO.LOW)
    
    #lux checking
    if "lux" in msg.topic and sensor_data.value >lux_max:
        print('Lux is greater than its max threshold')
        GPIO.output(12, GPIO.HIGH)
    elif "lux" in msg.topic and sensor_data.value<lux_max: 
        GPIO.output(12, GPIO.LOW)

def _init_influxdb_database():
    databases = influxdb_client.get_list_database()
    if len(list(filter(lambda x: x['name'] == INFLUXDB_DATABASE, databases))) == 0:
        influxdb_client.create_database(INFLUXDB_DATABASE)
    influxdb_client.switch_database(INFLUXDB_DATABASE)

def main():
    _init_influxdb_database()

    mqtt_client = mqtt.Client(MQTT_CLIENT_ID)
    mqtt_client.username_pw_set(MQTT_USER, MQTT_PASSWORD)
    mqtt_client.on_connect = on_connect
    mqtt_client.on_message = on_message

    mqtt_client.connect(MQTT_ADDRESS, 1883)
   # GPIO.output(3, GPIO.LOW)
   # sleep(1)
   # GPIO.output(3, GPIO.HIGH)
   # print('Harry is here') 
   # mqtt_client.loop_forever()
    n=0
    while True:  				
       mqtt_client.loop_start()
       sleep(0.1)
       GPIO.output(3, GPIO.HIGH)
       sleep(0.1)
       GPIO.output(3, GPIO.LOW)
       sleep(0.1)
       n=n+1
       print(n)	


if __name__ == '__main__':
    print('MQTT to InfluxDB bridge')
    main()

