#include <TimeLib.h>
#include <Time.h>
#include <string>
// change next line to use with another board/shield
#include <ESP8266mDNS.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
//#include <WiFi.h> // for WiFi shield
//#include <WiFi101.h> // for WiFi 101 shield or MKR1000
#include <WiFiUdp.h>
#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>
#include <PubSubClient.h>
#include <ESP8266WebServer.h>

#include "display.h"
#include "webserving.h"
#include "mqtt_client.h"
#include "btnmenu.h"
#include "timehandling.h"
#include "config.h"
#include <Wire.h>       // i²c library
#include "Adafruit_MCP23008.h" //port expander

const char *ssid = "nixieClock";

 int number = 0;
 int oldnumber = 0;
int i = 0;
uint8 red_value, blue_value, green_value = 0;
int8 red_step, blue_step, green_step = 0;
uint16_t blink_time = 0;
boolean hasRTC;

// define buttons
#define BTN_LEFT 0
#define BTN_CENTER 7
#define BTN_RIGHT 6
#define BTN_PROG 0

// define button_states
#define BTN_LEFT_PRESSED 0x01
#define BTN_RIGHT_PRESSED 0x02
#define BTN_CENTER_PRESSED 0x04
#define BTN_PROG_PRESSED 0x08
// define if a btn was pressed short or long
#define BTN_LEFT_SHORT 0x01
#define BTN_RIGHT_SHORT 0x02
#define BTN_CENTER_SHORT 0x04
#define BTN_LEFT_LONG 0x08
#define BTN_RIGHT_LONG 0x10
#define BTN_CENTER_LONG 0x20
#define BTN_PROG_SHORT 0x40

WiFiUDP udp; // A UDP instance to let us send and receive packets over UDP
uint8 btnstate = 0x00;
uint8 old_btnstate = 0x00;
unsigned long btn_starttimes[3];
unsigned long btn_endtime;


#define LED_COUNT 4                   // define number of ws2812 leds
#define LED_PIN 13                    // define Pin where WS2812 connected to

#define DATA_PIN 1
#define CLK_PIN 2
#define STROBE_PIN 3
#define HV_PIN 4

#define BRIGHTNESS_STEP 15              // in/decrease brightness by this amount per click
#define SPEED_STEP 5

#define LED_OFF 0
#define LED_ON 1
#define LED_FADING 2


Adafruit_NeoPixel strip = Adafruit_NeoPixel(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
uint32_t led_colors[LED_COUNT] = {0};

Adafruit_MCP23008 mcp; // create port-expander-session
PubSubClient mqtt_connector;
MDNSResponder mdns;
ESP8266WebServer server(80);
WiFiClient espClient;
configuration config;       //create configuration object
nixie_display display(&mcp, &config);
String ntp_server;
String mqtt_server;
String password;
char hostString[16] = {0};
uint8 led_mode = 0;
uint8 input = 0;
uint8 oldsec = 0;
nixieTimer nixieclock;
bool last_wifistate = false;

uint32 mqtt_connection_time = 0;

int edit_number_oldvalue, edit_number; // Internal number which is used while in editing-menu

char incomming_byte;
String serial_command = "";

node menu_time, menu_year, menu_date, menu_edit_hour, menu_edit_minute, menu_edit_day, menu_edit_month, menu_edit_year;
node* menupoint = &menu_time;


void run_serial_command(String command){ 
	if (command == "wifi") {
		Serial.println("WiFi Status");
		Serial.print("Essid: ");
		Serial.println(config.essid);
		Serial.print("Password: ");
		Serial.println(config.epass);
		if (WiFi.isConnected()) { 
			Serial.println("connected");
		} else {
			
			Serial.println("not connected");
		}
	}

	if (command == "mqtt") {
		Serial.println("MQTT Status");
		Serial.print("Broker: ");
		Serial.println(config.mqtt_server);
		Serial.print("Topic: ");
		Serial.println(config.mqtt_topic);
		Serial.print("User: ");
		Serial.println(config.mqtt_user);
		if (mqtt_connector.connected()) {
			Serial.println("Connected");
		} else {
			Serial.println("Not Connected");
		}
	}
}

void next_menupoint() {
  menupoint = menupoint->next;
}
void prev_menupoint() {
  menupoint = menupoint->prev;
}


void show_time() {
  oldnumber = number;
//  number = clock.hour() * 100 + clock.minute();
  number = nixieclock.get_hour() * 100 + nixieclock.get_minute();

  if (number != oldnumber ) {
    display.print(number);
    Serial.println(number);
  }
}

void show_year() {
  oldnumber = number;
  number = nixieclock.get_year();
  if (number != oldnumber) {
    display.print(number);
    Serial.println(number);
  }
}


void show_date() {
  oldnumber = number;
  number = nixieclock.get_day() * 100 + nixieclock.get_month();
  if (number != oldnumber) {
    display.print(number);
    Serial.println(number);
  }
}


/** Show stopwatch timer
 *
 **/
void show_timer(){
	oldnumber = number;
	
}

void do_nothing() {
  oldnumber = number;
}

void nixies_toggle() {
    display.toggle();

}

// Start NTP only after IPnetwork is connected
void onSTAGotIP(WiFiEventStationModeGotIP ipInfo) {
	Serial.printf("Got IP: %s\r\n", ipInfo.ip.toString().c_str());
  nixieclock.fetch_ntptime();
  // NTP.begin(ntp_server.c_str(), 1, true);
	// NTP.setInterval(63);
}


// read Buttons as input
uint8 check_buttons() {
  // set button_state
  uint8 btn_state = 0x00;

  //  Read Input-Buttons
  if (!mcp.digitalRead(BTN_LEFT))
    btn_state += BTN_LEFT_PRESSED;

  if (!mcp.digitalRead(BTN_RIGHT))
    btn_state += BTN_RIGHT_PRESSED;

  if (!mcp.digitalRead(BTN_CENTER))
    btn_state += BTN_CENTER_PRESSED;

  if (!digitalRead(BTN_PROG))
    btn_state += BTN_PROG_PRESSED;

  return btn_state;
}

void inc_hour() {
    edit_number += 100;
    if (edit_number > 2359) {
      edit_number -= 2400;
    }
    display.printh(edit_number);
    return;
}

void dec_hour() {
    edit_number -= 100;
    if (edit_number < 0) {
      edit_number += 2400;
    }
    display.printh(edit_number);
    return;
}

void inc_day() {
    edit_number += 100;
    if (edit_number > 3112) {
        edit_number -= 3000;
    }
    display.printh(edit_number);
}

void dec_day() {
    edit_number -= 100;
    if (edit_number < 0) {
      edit_number += 3100;
    }
    display.printh(edit_number);
    return;
}

void inc_minute() {
    int minute = edit_number % 100;
    int hour = edit_number - minute;
    minute += 1;
    if (minute > 59) minute = 0;
    edit_number = hour + minute;
    display.printh(edit_number);
    return;
}

void dec_minute() {
  int minute = edit_number % 100;
  int hour = edit_number - minute;
  minute -= 1;
  if (minute < 0) minute = 59;
  edit_number = hour + minute;
  display.printh(edit_number);
  return;
}

void inc_month() {
    int month = edit_number % 100;
    int day = edit_number - month;
    month += 1;
    if (month > 12) month = 1;
    edit_number = day + month;
    display.printh(edit_number);
    return;
}

void dec_month() {
  int month = edit_number % 100;
  int day = edit_number - month;
  month -= 1;
  if (month < 1) month= 12;
  edit_number = day + month;
  display.printh(edit_number);
  return;
}

void inc_year() {
  edit_number += 1;
  if (edit_number > 2100) edit_number=1970; // assume that this clock works only till year 2100
  display.printh(edit_number);
  return;
}

void dec_year() {
  edit_number -= 1;
  if (edit_number < 1970) edit_number=2100; // don't set year prior to 1970
  display.printh(edit_number);
  return;
}

void edit_hour() {
    strip.setPixelColor(0, strip.Color(0,0,0));
    strip.setPixelColor(1, strip.Color(0,0,0));
    if (second() % 2) {
      strip.setPixelColor(2, strip.Color(100,50,0));
      strip.setPixelColor(3, strip.Color(100,50,0));
    } else {
      strip.setPixelColor(2, strip.Color(0,0,0));
      strip.setPixelColor(3, strip.Color(0,0,0));
    }
    strip.show();
}

void edit_minute() {
  strip.setPixelColor(2, strip.Color(0,0,0));
  strip.setPixelColor(3, strip.Color(0,0,0));
  if (second() % 2) {
    strip.setPixelColor(0, strip.Color(100,50,0));
    strip.setPixelColor(1, strip.Color(100,50,0));
  } else {
    strip.setPixelColor(0, strip.Color(0,0,0));
    strip.setPixelColor(1, strip.Color(0,0,0));
  }
  strip.show();
}

void edit_day() {
  strip.setPixelColor(0, strip.Color(0,0,0));
  strip.setPixelColor(1, strip.Color(0,0,0));
  if (second() % 2) {
    strip.setPixelColor(2, strip.Color(100,50,0));
    strip.setPixelColor(3, strip.Color(100,50,0));
  } else {
    strip.setPixelColor(2, strip.Color(0,0,0));
    strip.setPixelColor(3, strip.Color(0,0,0));
  }
  strip.show();
}

void edit_month() {
  strip.setPixelColor(2, strip.Color(0,0,0));
  strip.setPixelColor(3, strip.Color(0,0,0));
  if (second() % 2) {
    strip.setPixelColor(0, strip.Color(100,50,0));
    strip.setPixelColor(1, strip.Color(100,50,0));
  } else {
    strip.setPixelColor(0, strip.Color(0,0,0));
    strip.setPixelColor(1, strip.Color(0,0,0));
  }
  strip.show();
}

void editing_year() {
    if (second() % 2) {
      for (i=0; i<4; i++) strip.setPixelColor(i, strip.Color(100,50,0));
    } else {
      for (i=0; i<4; i++) strip.setPixelColor(i, strip.Color(0,0,0));
    }
    strip.show();
}

// start time-edit mode,
void edit_time() {
  edit_number = nixieclock.get_hour() * 100 + nixieclock.get_minute();
  menupoint = &menu_edit_hour;
  display.print(edit_number);
  return;
}

// finish time-edit mode,
void save_time() {
  // switch leds off
  for (int i=0; i<4; i++) strip.setPixelColor(i, strip.Color(0,0,0));
  strip.show();
  uint8_t m = edit_number % 100;
  uint8_t h = (edit_number - m) / 100;
  nixieclock.set_minute(m);
  nixieclock.set_hour(h);
  menupoint = &menu_time;
  for (int i=0; i<4; i++) strip.setPixelColor(i, strip.Color(0,0,0));
  strip.show();
  return;
}

// start date-edit mode
void edit_date() {
  edit_number = nixieclock.get_day() * 100 + nixieclock.get_month();
  menupoint = &menu_edit_day;
  display.print(edit_number);
  return;
}
// finish date-edit mode,
void save_date() {
  // switch leds off
  for (int i=0; i<4; i++) strip.setPixelColor(i, strip.Color(0,0,0));
  strip.show();
  uint8_t m = edit_number % 100;
  uint8_t d = (edit_number - m) / 100;
  nixieclock.set_month(m);
  nixieclock.set_day(d);
  menupoint = &menu_date;
  for (int i=0; i<4; i++) strip.setPixelColor(i, strip.Color(0,0,0));
  strip.show();
  return;
}

// start year-edit Mode
void edit_year() {
  edit_number = nixieclock.get_year();
  menupoint = &menu_edit_year;
  display.print(edit_number);
  return;
}

//finish year_edit mode and save value
void save_year() {
  for (int i=0; i<4; i++) strip.setPixelColor(i, strip.Color(0,0,0));
  strip.show();
  menupoint = &menu_year;
  nixieclock.set_year(edit_number);
  return;
}




/** Configuration Mode
 *  Start web-server to configure via webserver
 */
void config_mode() {
  Serial.print("Configuring access point...");
  WiFi.mode(WIFI_AP);
  WiFi.softAP("nixieClock");    // first without pw

  IPAddress myIP = WiFi.softAPIP();
	Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());
  Serial.println(WiFi.localIP());

  if (!mdns.begin("nixieclock", WiFi.localIP())) {
    Serial.println("Error setting up MDNS responder!");
    return;
  }

  Serial.println("mDNS responder started");
  server.begin();
  // main Configform
  server.on("/", [](){
    Serial.println("Serving Config-Form");
    server.send(200,"text/html", config_form());
  });

  // restart ESP
  server.on("/restart", []() {
    ESP.restart();
  });

  server.on("/save", HTTP_POST, [](){
    if (server.hasArg("ssid")) config.essid = server.arg("ssid");
    if (server.hasArg("password")) {
		// if( !strcmp(server.arg("password").c_str(),""))
			config.epass = server.arg("password");
			Serial.println(config.epass);
	}
    if (server.hasArg("ntp")) config.ntp_server = server.arg("ntp");
    if (server.hasArg("mqtt_broker")) config.mqtt_server = server.arg("mqtt_broker");
    if (server.hasArg("mqtt_user")) config.mqtt_user = server.arg("mqtt_user");
    if (server.hasArg("mqtt_password")) config.mqtt_password = server.arg("mqtt_password");
    if (server.hasArg("mqtt_topic")) config.mqtt_topic = server.arg("mqtt_topic");
    config.store();
    server.send(200,"text/html", config_form());
  });

	server.begin();
  Serial.println("HTTP server started");

  unsigned char blue=0;
  char colorstep = 1;
  unsigned int btnstate = 0x00;
  unsigned int oldbtnstate = 0x00;
  uint16_t value = 0;
  uint16_t oldvalue = 0;
  uint8_t oldsec = 0;
  bool nixie_test = false;
  // Main-Loop of Configuration-Mode
  while(1) {
    // Check if center btn is released
    oldbtnstate = btnstate;
    btnstate = check_buttons();
    if (oldbtnstate & BTN_CENTER_PRESSED && !(btnstate & BTN_CENTER_PRESSED))
        ESP.restart();

    // Nixie-Test Mode: press left BUTTON in CONFIG-MODE
    if (oldbtnstate & BTN_LEFT_PRESSED && !(btnstate & BTN_LEFT_PRESSED))
        if (nixie_test) {
          nixie_test = false;
          display.off();
        } else {
          nixie_test = true;
          display.on();
          value = 0;
        }


    if (nixie_test && oldsec != second()) {
      value += 1111;
      if (value > 9999) value = 0;
      oldsec = second();
    }

    if (oldvalue != value) {
      display.printh(value);
      oldvalue = value;
    }

    // While in config-mode blue leds are fading
    blue += colorstep;
    for (int i=0; i< LED_COUNT; i++)
      strip.setPixelColor(i, strip.Color(0,0,blue));
    if (colorstep > 0 && blue > 250) colorstep = -1;
    if (colorstep < 0 && blue < 1) colorstep = 1;
    strip.show();
    delay(30);
    server.handleClient();
  }
}

void setup(){
  
  Serial.begin(115200);

  Serial.println("Setup publish callback ");
  display.set_publish_callback(&mqtt_publish);
  Serial.println("Sarting WIre");
  Wire.begin();
  Serial.println("Statring mcp");
  mcp.begin();
  
  nixieclock.begin();

  // Read configured Wifi-Settings
  static WiFiEventHandler e1, e2;
  EEPROM.begin(4096);
  strip.begin();
  mcp.pinMode(BTN_LEFT, INPUT);
  mcp.pinMode(BTN_CENTER, INPUT);
  mcp.pinMode(BTN_RIGHT, INPUT);
  mcp.pullUp(BTN_LEFT, HIGH);  // turn on a 100K pullup internally
  mcp.pullUp(BTN_RIGHT, HIGH);  // turn on a 100K pullup internally
  mcp.pullUp(BTN_CENTER, HIGH);  // turn on a 100K pullup internally
  mcp.pinMode(HV_PIN, OUTPUT);
  mcp.pinMode(CLK_PIN, OUTPUT);
  mcp.pinMode(DATA_PIN, OUTPUT);
  mcp.pinMode(STROBE_PIN, OUTPUT);
  Serial.println("Switching Display off");
  display.off();

  pinMode(BTN_PROG,INPUT_PULLUP);
  // set startdate to 0:0 1/1/2018
  Serial.println("Setting System Timer");
  setTime(0,0,0,1,1,2018);

  // Enter Config_mode if prog_btn is pressed during startup
  if (!digitalRead(BTN_PROG)) {
    config_mode();
  }

  // Define MENUs
  menu_time.command = &show_time;
  menu_time.next = &menu_date;
  menu_time.prev = &menu_year;
  menu_time.btn_left_action = &next_menupoint;
  menu_time.btn_center_action = &nixies_toggle;
  menu_time.btn_right_action = &prev_menupoint;
  menu_time.btn_left_long_action = &edit_time;
  menu_time.btn_center_long_action = &config_mode;
  menu_time.btn_right_long_action = &do_nothing;
  menu_year.command = &show_year;
  menu_year.next = &menu_time;
  menu_year.btn_left_action = &next_menupoint;
  menu_year.prev = &menu_date;
  menu_year.btn_right_action = &prev_menupoint;
  menu_year.btn_center_action = &do_nothing;
  menu_year.btn_left_long_action = &edit_year;
  menu_year.btn_center_long_action = &do_nothing;
  menu_year.btn_right_long_action = &do_nothing;
  menu_date.command = &show_date;
  menu_date.next = &menu_year;
  menu_date.btn_left_action = &next_menupoint;
  menu_date.prev = &menu_time;
  menu_date.btn_right_action = &prev_menupoint;
  menu_date.btn_center_action = &do_nothing;
  menu_date.btn_left_long_action = &edit_date;
  menu_date.btn_center_long_action = &do_nothing;
  menu_date.btn_right_long_action = &do_nothing;

  menu_edit_hour.command = &edit_hour;
  menu_edit_hour.prev = &menu_time;
  menu_edit_hour.next = &menu_edit_minute;
  menu_edit_hour.btn_left_action = &inc_hour;
  menu_edit_hour.btn_right_action = &dec_hour;
  menu_edit_hour.btn_center_action = &next_menupoint;
  menu_edit_hour.btn_left_long_action = &do_nothing;
  menu_edit_hour.btn_center_long_action = &save_time;
  menu_edit_hour.btn_right_long_action = &do_nothing;
  menu_edit_minute.command = &edit_minute;
  menu_edit_minute.prev = &menu_edit_hour;
  menu_edit_minute.next = &menu_edit_hour;
  menu_edit_minute.btn_left_action = &inc_minute;
  menu_edit_minute.btn_right_action = &dec_minute;
  menu_edit_minute.btn_center_action = &next_menupoint;
  menu_edit_minute.btn_left_long_action = &do_nothing;
  menu_edit_minute.btn_center_long_action = &save_time;
  menu_edit_minute.btn_right_long_action = &do_nothing;
  menu_edit_day.command = &edit_day;
  menu_edit_day.prev = &menu_date;
  menu_edit_day.next = &menu_edit_month;
  menu_edit_day.btn_left_action = &inc_day;
  menu_edit_day.btn_right_action = &dec_day;
  menu_edit_day.btn_center_action = &next_menupoint;
  menu_edit_day.btn_left_long_action = &do_nothing;
  menu_edit_day.btn_center_long_action = &save_date;
  menu_edit_day.btn_right_long_action = &do_nothing;
  menu_edit_month.command = &edit_month;
  menu_edit_month.prev = &menu_edit_day;
  menu_edit_month.next = &menu_edit_day;
  menu_edit_month.btn_left_action = &inc_month;
  menu_edit_month.btn_right_action = &dec_month;
  menu_edit_month.btn_center_action = &next_menupoint;
  menu_edit_month.btn_left_long_action = &do_nothing;
  menu_edit_month.btn_center_long_action = &save_date;
  menu_edit_month.btn_right_long_action = &do_nothing;
  menu_edit_year.command = &editing_year;
  menu_edit_year.prev = &menu_year;
  menu_edit_year.next = &menu_year;
  menu_edit_year.btn_left_action = &inc_year;
  menu_edit_year.btn_right_action = &dec_year;
  menu_edit_year.btn_center_action = &do_nothing;
  menu_edit_year.btn_left_long_action = &do_nothing;
  menu_edit_year.btn_center_long_action = &save_year;
  menu_edit_year.btn_right_long_action = &do_nothing;

  Serial.print("Loading config");
  config.load(); // load configuration from eeprom
  Serial.println(" OK");
  Serial.print("Setup mqtt ");
  mqtt_setup(config.mqtt_server.c_str(), espClient);
  Serial.println(" OK");
  Serial.print("Setup ntpserver for clockmodule");
  nixieclock.set_ntp_server(config.ntp_server);
  Serial.println(" OK");
  strip.Color(255, 0, 0);
  Serial.print("Setup Strip");
  strip.show(); // Initialize all pixels to 'off'
  Serial.println(" OK");

  WiFi.mode(WIFI_STA);
  WiFi.begin(config.essid.c_str(), config.epass.c_str());
  Serial.println("Wifi Setup ready");
  e1 = WiFi.onStationModeGotIP(onSTAGotIP);// As soon WiFi is connected, start NTP Client
  Serial.println("Got IP");

  display.clr();
  Serial.println("Empty display");
  display.print(0);
  Serial.println("Started with 0000");
  display.on();

  led_mode = LED_ON;
  red_step = 1;
  blue_step = 1;
  green_step = 1;

  red_value = 0x0c;
  green_value = 0x05;
  blue_value = 0x00;
  for (int i=0; i< LED_COUNT; i++)
    led_colors[i]=strip.Color(red_value, green_value, blue_value);

  strip.show();
  Serial.println("Trying to publish config");
  publish_config();
  Serial.println("OK");
}

void loop() {

  // 
  if (Serial.available() > 0) {
	  incomming_byte = Serial.read();
	  if (incomming_byte == '\n' or incomming_byte == '\r') {
		  Serial.println(" executing command ");
		  run_serial_command(serial_command);
		  // clear command after command was executed
		  serial_command = "";
		  Serial.print("$ ");
		  
	  } else {
		serial_command += incomming_byte;
		Serial.print(incomming_byte);
	  }
  }

  // Redraw LEDs once in a sec
  if (oldsec != second()) {
    for (int i=0; i<LED_COUNT; i++) {
      strip.setPixelColor(i, led_colors[i]);
    }
    strip.show();
  }
  oldsec = second();


  if (WiFi.isConnected()) {

	  if (!mqtt_connector.connected()) {
      if (now() - mqtt_connection_time > 5) {
        mqtt_reconnect();
        mqtt_connection_time = now();
      }
    }
    mqtt_connector.loop();
  } 
  // Read Input-BTNS

  old_btnstate = btnstate;
  btnstate = check_buttons();
  if (old_btnstate != btnstate) {
    Serial.printf("Button pressed %i", btnstate);

    if (btnstate & BTN_LEFT_PRESSED) Serial.printf("BTN_LEFT");
    if (btnstate & BTN_CENTER_PRESSED) Serial.printf("BTN_CENTER");
    if (btnstate & BTN_RIGHT_PRESSED) Serial.printf("BTN_RIGHT");

    // Left Button pressed, wasn't before
    if (btnstate & BTN_LEFT_PRESSED && !(old_btnstate & BTN_LEFT_PRESSED)) {
      btn_starttimes[0] = millis();
    }
    // Center_Button pressed, wasn't before
    if (btnstate & BTN_CENTER_PRESSED && !(old_btnstate & BTN_CENTER_PRESSED)) {
      btn_starttimes[1] = millis();
    }
    // Center_Button pressed, wasn't before
    if (btnstate & BTN_RIGHT_PRESSED && !(old_btnstate & BTN_RIGHT_PRESSED)) {
      btn_starttimes[2] = millis();
    }
    // Left Button released
    if (!(btnstate & BTN_LEFT_PRESSED) && old_btnstate & BTN_LEFT_PRESSED) {
      btn_endtime = millis();
      if (btn_endtime - btn_starttimes[0] < 1000) {
        input = BTN_LEFT_SHORT;
      } else if (btn_endtime - btn_starttimes[0] >= 1000 && btn_endtime) {
        input = BTN_LEFT_LONG;
      }
      Serial.printf("LeftButton released after %ds", btn_endtime-btn_starttimes[0]);
    }
    // Center Button released
    if (!(btnstate & BTN_CENTER_PRESSED) && old_btnstate & BTN_CENTER_PRESSED) {
      btn_endtime = millis();
      if (btn_endtime - btn_starttimes[1] < 1000) {
        input = BTN_CENTER_SHORT;
      } else if (btn_endtime - btn_starttimes[1] >= 1000 && btn_endtime) {
        input = BTN_CENTER_LONG;
      }
      Serial.printf("CENTER Button released after %ds", btn_endtime-btn_starttimes[1]);
    }
    // Right Button released
    if (!(btnstate & BTN_RIGHT_PRESSED) && old_btnstate & BTN_RIGHT_PRESSED) {
      btn_endtime = millis();
      if (btn_endtime - btn_starttimes[2] < 1000) {
        input = BTN_RIGHT_SHORT;
      } else if (btn_endtime - btn_starttimes[2] >= 1000 && btn_endtime) {
        input = BTN_RIGHT_LONG;
      }
      Serial.printf("RIGHT Button released after %ds", btn_endtime-btn_starttimes[2]);
    }

    // Prg Button released
    if ((!btnstate & BTN_PROG_PRESSED) && (old_btnstate & BTN_PROG_PRESSED)) {
      input = BTN_PROG_SHORT;
    }

  }

  // interpret button-input
  if (input != 0x00) {
    if (input & BTN_PROG_SHORT) config_mode();
    if (input & BTN_LEFT_SHORT) menupoint->btn_left_action();
    if (input & BTN_RIGHT_SHORT) menupoint->btn_right_action();
    if (input & BTN_CENTER_SHORT) menupoint->btn_center_action();
    if (input & BTN_LEFT_LONG) menupoint->btn_left_long_action();
    if (input & BTN_RIGHT_LONG) menupoint->btn_right_long_action();
    if (input & BTN_CENTER_LONG) menupoint->btn_center_long_action();

    input = 0;
  }

  menupoint->command();



}
