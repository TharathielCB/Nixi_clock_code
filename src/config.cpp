#include "config.h"

String read_config(uint16 start, uint16 length) {
  String value;
  for (int i =0; i<length; ++i) {
    value += char(EEPROM.read(start + i));
  }
  return value;
}

/**
 * Save POST-Argument from Webserver-request to EEPROM
 *
 **/
void save_config(String value, int start_address, int length) {
  for (int i = 0; i < length; ++i) EEPROM.write(start_address + i, 0);
  for (int i=0; i < value.length(); ++i) EEPROM.write(start_address + i, value[i]);
  EEPROM.commit();
}



void configuration::load() {
// read eeprom for ssid and pass
  essid = read_config(SSID_MEM,32);
  Serial.print("SSID: ");
  Serial.println(essid);
  Serial.println("Reading Wifi pass");
  String epass = read_config(PASSWORD_MEM,64);
  ntp_server = read_config(NTP_MEM, 64);
  mqtt_server = read_config(MQTT_BROKER_MEM, 64);
}

void configuration::store() {
  //save configuration to eeprom
  save_config(essid, SSID_MEM, 32);
  save_config(epass, PASSWORD_MEM, 64);
  save_config(ntp_server, NTP_MEM, 64);
  save_config(mqtt_server, MQTT_BROKER_MEM, 64);
  save_config(mqtt_user, MQTT_USER_MEM, 32);
  save_config(mqtt_password, MQTT_PASSWORD_MEM, 32);
  save_config(mqtt_topic, MQTT_TOPIC_MEM, 32);
  EEPROM.commit();
}
