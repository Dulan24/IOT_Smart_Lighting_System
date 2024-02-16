#include <WiFi.h>
#include <PubSubClient.h>
#include <FastLED.h>
#include "fauxmoESP.h"
#include <Arduino.h>
#include <PubSubClient.h>
#include "Mqtt.h"
#include "Led.h"
#include "Wifi_credentials.h"
#include <ArduinoJson.h>

void Wifi_init();
void pinInit();
void callback(char* topic, byte* payload, unsigned int length);
CRGB light1[NUM_LEDS];
CRGB light2[NUM_LEDS];
static unsigned long last = millis();
int pirState=LOW;

WiFiClient espClient;
PubSubClient client(espClient);
fauxmoESP fauxmo;


void setup() {
  Serial.begin(115200);
  pinInit();
  Wifi_init();

  client.setServer(MQTTSERVER, MQTTPORT);
  client.setCallback(callback); // For Subscription

  while (!client.connected()) {
    Serial.println("Connecting to MQTT..");
    if (client.connect("ESP32SAHRKs")) {
      Serial.print("Connected to MQTT");
      client.subscribe(TOPIC1); // For Subscription
      //client.subscribe(TOPIC2);
    } else {
      Serial.println("MQTT Failed to connect");
      delay(5000);
    }
  }
  fauxmo.createServer(true); // not needed, this is the default value
  fauxmo.setPort(80); // This is required for gen3 devices
  fauxmo.enable(true);
  fauxmo.addDevice("light1");
  fauxmo.addDevice("light2");

  FastLED.addLeds<WS2812B, PIN_ROOM1, GRB>(light1, NUM_LEDS);
  FastLED.addLeds<WS2812B, PIN_ROOM2, GRB>(light2, NUM_LEDS);
  fauxmo.onSetState([](unsigned char device_id, const char * device_name, bool state, unsigned char value) {

         Serial.printf("[MAIN] Device #%d (%s) state: %s value: %d\n", device_id, device_name, state ? "ON" : "OFF", value);

        StaticJsonDocument<200> doc;
        const char* output;
        doc["color"]="#ff0000";
        if (strcmp(device_name, "light1")==0) {
          if (state){
            fill_solid(light1,NUM_LEDS, CRGB::Red);
            FastLED.setBrightness(value);
            doc["state"]="on";
            doc["brightness"]=value;}
          else{
            fill_solid(light1,NUM_LEDS, CRGB::Black);
            doc["state"]="off";}
          FastLED.show();
          //serializeJson(doc,output);
          //client.publish("Sharks/room1pub",output);
        }
        
        else if (strcmp(device_name, "room2")==0) {
          if (state){
            fill_solid(light2,NUM_LEDS, CRGB::Red);
            FastLED.setBrightness(value);
            doc["state"]="on";
            doc["brightness"]=value;
          }
          else{
            fill_solid(light2,NUM_LEDS, CRGB::Black);
            doc["state"]="off";}
          FastLED.show();
          //serializeJson(doc,output);
          //client.publish("Sharks/room2pub",output);
        }
    });
}

void loop() {
  client.loop();
  fauxmo.handle();
  
        
  int val = digitalRead(PIN_PIRSENSOR);  
  if (val == HIGH) {           
    digitalWrite(PIN_PIRLIGHT, HIGH);  
    if (pirState == LOW) {
      Serial.println("Motion detected!");
      pirState = HIGH;
    }
  } else {
    if (millis() - last > 5000) {
        last = millis();
      if (pirState == HIGH){
          digitalWrite(PIN_PIRLIGHT, LOW); 
          Serial.println("Motion ended!");
          pirState = LOW;
      } 
    }
  }  
}

void pinInit(){
  pinMode(PIN_ROOM1,OUTPUT);
  pinMode(PIN_ROOM2,OUTPUT);
  pinMode(PIN_PIRLIGHT,OUTPUT);
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message received on topic: ");
  Serial.println(topic);

  char payloadStr[length + 1];
  memcpy(payloadStr, payload, length);
  payloadStr[length] = '\0';

  Serial.print("Payload: ");
  Serial.println(payloadStr);
  
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, payload);
  
  // Test if parsing succeeds
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }

  int brightness=doc["brightness"];
  
  // Extract color string from JSON
  const char *colorHex = doc["color"];

  // Convert hexadecimal string to RGB values
  long colorValue = strtol(colorHex + 1, NULL, 16); // Skip the '#' character
  uint8_t red = (colorValue >> 16) & 0xFF;
  uint8_t green = (colorValue >> 8) & 0xFF;
  uint8_t blue = colorValue & 0xFF;
  
  if(strcmp(topic,"Sharks/room1sub")==0){
    if(strcmp(doc["state"],"off")==0) {
      fill_solid(light1,NUM_LEDS, CRGB::Black);
      FastLED.show();
      fauxmo.setState("light1",false,0);
      client.publish("Sharks/room1pub",payloadStr);}
    else if(strcmp(doc["state"],"on")==0){
      fill_solid(light1,NUM_LEDS, CRGB(red,green,blue));
      FastLED.setBrightness(brightness);
      FastLED.show();
      fauxmo.setState("light1",true,brightness);
      client.publish("Sharks/room1pub",payloadStr);}
  }

  if(strcmp(topic,"Sharks/room2sub")==0){
    if(strcmp(doc["state"],"off")==0) {
      fill_solid(light2,NUM_LEDS, CRGB::Black);
      FastLED.show();
      fauxmo.setState("light2",false,0);
      client.publish("Sharks/room2pub",payloadStr);}
    else if(strcmp(doc["state"],"on")==0){
      fill_solid(light2,NUM_LEDS, CRGB(red,green,blue));
      FastLED.setBrightness(brightness);
      FastLED.show();
      fauxmo.setState("light2",true,brightness);
      client.publish("Sharks/room1pub",payloadStr);}
  }
  
  if(strcmp(topic,"Sharks/room3sub")==0){
    if(strcmp(doc["state"],"off")==0) {
      digitalWrite(PIN_PIRLIGHT,LOW);
      fauxmo.setState("room1",false,0);
      client.publish("Sharks/room3pub",payloadStr);}
    else if(strcmp(doc["state"],"on")==0){
      digitalWrite(PIN_PIRLIGHT,HIGH);
      fauxmo.setState("room1",true,brightness);
      client.publish("Sharks/room3pub",payloadStr);}
  }
}

