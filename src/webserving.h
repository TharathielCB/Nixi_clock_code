#include <EEPROM.h>
#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#ifndef WEBSERvING
  #include "config.h"
    extern configuration config;
    #define WEBSERVING
    String html_header();
    String config_body();

    String config_form();
#endif
