
#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>
#include "display.h"
#include "btnmenu.h"
#include "timehandling.h"
#include "config.h"

extern PubSubClient mqtt_connector;
extern Adafruit_NeoPixel strip;
extern nixie_display display;
extern node* menupoint;
extern node menu_time, menu_year, menu_date, menu_edit_hour, menu_edit_minute, menu_edit_day, menu_edit_month, menu_edit_year;
extern uint32_t led_colors[4];
extern nixieTimer clock;
extern configuration config;

void mqtt_setup(const char* server, Client& client);
void mqtt_connect();
void mqtt_callback(char* topic, byte* payload, unsigned int length);
void mqtt_reconnect();


#endif
