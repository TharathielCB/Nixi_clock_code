
#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>
#include "display.h"

extern PubSubClient mqtt_connector;
extern Adafruit_NeoPixel strip;
extern nixie_display display;

void mqtt_setup(const char* server, Client& client);
void mqtt_connect();
void mqtt_callback(char* topic, byte* payload, unsigned int length);
void mqtt_reconnect();


#endif