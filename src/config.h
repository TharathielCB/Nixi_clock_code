
#ifndef nixie_config
#define nixie_config

// define eeprom memory locations
#define SSID_MEM 0x00
#define PASSWORD_MEM 0x20
#define NTP_MEM 0x60
#define MQTT_BROKER_MEM 0xa0
#define MQTT_USER_MEM 0xe0
#define MQTT_PASSWORD_MEM 0x100
#define MQTT_TOPIC_MEM 0x120

#include "Arduino.h"
#include <EEPROM.h>
#include "PubSubClient.h"

extern PubSubClient mqtt_connector;
String read_config(uint16 start, uint16 length);
void save_config(String value, int start_address, int length);

class configuration {

private:

public:
  String essid;
  String epass;
  String ntp_server;
  String mqtt_server;
  String mqtt_password;
  String mqtt_user;
  String mqtt_topic;

  void load();
  void store();
};

#endif
