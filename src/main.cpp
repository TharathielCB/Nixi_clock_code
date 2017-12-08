#include <NtpClientLib.h>
#include <TimeLib.h>
#include <Time.h>
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

const char *ssid = "nixieClock";
WiFiUDP ntpUDP;

nixie_display display;
 int number = 0;
 int oldnumber = 0;
int i = 0;
uint8 red_value, blue_value, green_value = 0;
int8 red_step, blue_step, green_step = 0;

// define buttons
#define BTN_LEFT 16
#define BTN_CENTER 0
#define BTN_RIGHT 12

// define button_states
#define BTN_LEFT_PRESSED 0x01
#define BTN_RIGHT_PRESSED 0x02
#define BTN_CENTER_PRESSED 0x04

// define if a btn was pressed short or long
#define BTN_LEFT_SHORT 0x01
#define BTN_RIGHT_SHORT 0x02
#define BTN_CENTER_SHORT 0x04
#define BTN_LEFT_LONG 0x08
#define BTN_RIGHT_LONG 0x10
#define BTN_CENTER_LONG 0x20

uint8 btnstate = 0x00;
uint8 old_btnstate = 0x00;
unsigned long btn_starttimes[3];
unsigned long btn_endtime;

#define LED_COUNT 4
#define LED_PIN 13
#define DEFAULT_COLOR 0x0C0500
#define DEFAULT_BRIGHTNESS 255
#define DEFAULT_SPEED 200
#define DEFAULT_MODE FX_MODE_STATIC

#define BRIGHTNESS_STEP 15              // in/decrease brightness by this amount per click
#define SPEED_STEP 5

#define LED_OFF 0
#define LED_ON 1
#define LED_FADING 2

// WS2812FX ws2812fx = WS2812FX(LED_COUNT, LED_PIN, NEO_RGB + NEO_KHZ800);
Adafruit_NeoPixel strip = Adafruit_NeoPixel(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

PubSubClient mqtt_connector;
MDNSResponder mdns;
ESP8266WebServer server(80);
WiFiClient espClient;

String ntp_server;
String mqtt_server;
String password;
char hostString[16] = {0};

uint8 led_mode = 0;
uint8 input = 0;

// color variables: mix RGB (0-255) for desired yellow 250,100,10
int redPx = 0x50;
int grnHigh = 0x20;
int bluePx = 0x03;

int edit_number_oldvalue, edit_number; // Internal number which is used while in editing-menu

node menu_time, menu_year, menu_date, menu_edit_hour, menu_edit_minute, menu_edit_day, menu_edit_month, menu_edit_year;
node* menupoint = &menu_time;

void next_menupoint() {
  menupoint = menupoint->next;
}

void prev_menupoint() {
  menupoint = menupoint->prev;
}

void show_time() {
  oldnumber = number;
  number = hour() * 100 + minute();
  if (number != oldnumber ) {
    display.print(number);
    Serial.println(number);
  }
}

void show_year() {
  oldnumber = number;
  number = year();
  if (number != oldnumber) {
    display.print(number);
    Serial.println(number);
  }
}

void show_date() {
  oldnumber = number;
  number = day() * 100 + month();
  if (number != oldnumber) {
    display.print(number);
    Serial.println(number);
  }
}

void do_nothing() {
  oldnumber = number;
}

void nixies_toggle() {
    display.toggle();

}

// Start NTP only after IP network is connected
void onSTAGotIP(WiFiEventStationModeGotIP ipInfo) {
	Serial.printf("Got IP: %s\r\n", ipInfo.ip.toString().c_str());
	NTP.begin(ntp_server.c_str(), 1, true);
	NTP.setInterval(63);
}


// read Buttons as input
uint8 check_buttons() {
  // set button_state
  uint8 btn_state = 0x00;

  //  Read Input-Buttons
  if (!digitalRead(BTN_LEFT))
    btn_state += BTN_LEFT_PRESSED;

  if (!digitalRead(BTN_RIGHT))
    btn_state += BTN_RIGHT_PRESSED;

  if (!digitalRead(BTN_CENTER))
    btn_state += BTN_CENTER_PRESSED;

  return btn_state;
}

void inc_hour() {
    edit_number += 100;
    if (edit_number > 2359) {
      edit_number -= 2400;
    }
    display.print(edit_number);
    return;
}

void dec_hour() {
    edit_number -= 100;
    if (edit_number < 0) {
      edit_number += 2400;
    }
    display.print(edit_number);
    return;
}

void inc_day() {
    edit_number += 100;
    if (edit_number > 3112) {
        edit_number -= 3000;
    }
    display.print(edit_number);
}

void dec_day() {
    edit_number -= 100;
    if (edit_number < 0) {
      edit_number += 3100;
    }
    display.print(edit_number);
    return;
}

void inc_minute() {
    int minute = edit_number % 100;
    int hour = edit_number - minute;
    minute += 1;
    if (minute > 59) minute = 0;
    edit_number = hour + minute;
    display.print(edit_number);
    return;
}

void dec_minute() {
  int minute = edit_number % 100;
  int hour = edit_number - minute;
  minute -= 1;
  if (minute < 0) minute = 59;
  edit_number = hour + minute;
  display.print(edit_number);
  return;
}

void inc_month() {
    int month = edit_number % 100;
    int day = edit_number - month;
    month += 1;
    if (month > 12) month = 1;
    edit_number = day + month;
    display.print(edit_number);
    return;
}

void dec_month() {
  int month = edit_number % 100;
  int day = edit_number - month;
  month -= 1;
  if (month < 1) month= 12;
  edit_number = day + month;
  display.print(edit_number);
  return;
}

void inc_year() {
  edit_number += 1;
  if (edit_number > 2100) edit_number=1970; // assume that this clock works only till year 2100
  display.print(edit_number);
  return;
}

void dec_year() {
  edit_number -= 1;
  if (edit_number < 1970) edit_number=2100; // don't set year prior to 1970
  display.print(edit_number);
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
  edit_number = hour() * 100 + minute();
  menupoint = &menu_edit_hour;
  display.print(edit_number);
  return;
}

// finish time-edit mode,
void save_time() {
  // switch leds off
  for (int i=0; i<4; i++) strip.setPixelColor(i, strip.Color(0,0,0));
  strip.show();
  int m = edit_number % 100;
  int h = (edit_number - m) / 100;
  setTime(h,m,0,day(),month(),year());
  menupoint = &menu_time;
  for (int i=0; i<4; i++) strip.setPixelColor(i, strip.Color(0,0,0));
  strip.show();
  return;
}

// start date-edit mode
void edit_date() {
  edit_number = day() * 100 + month();
  menupoint = &menu_edit_day;
  display.print(edit_number);
  return;
}
// finish date-edit mode,
void save_date() {
  // switch leds off
  for (int i=0; i<4; i++) strip.setPixelColor(i, strip.Color(0,0,0));
  strip.show();
  int m = edit_number % 100;
  int d = (edit_number - m) / 100;
  setTime(hour(),minute(),second(),d,m,year());
  menupoint = &menu_date;
  for (int i=0; i<4; i++) strip.setPixelColor(i, strip.Color(0,0,0));
  strip.show();
  return;
}

// start year-edit Mode
void edit_year() {
  edit_number = year();
  menupoint = &menu_edit_year;
  display.print(edit_number);
  return;
}

//finish year_edit mode and save value
void save_year() {
  for (int i=0; i<4; i++) strip.setPixelColor(i, strip.Color(0,0,0));
  strip.show();
  menupoint = &menu_year;
  setTime(hour(),minute(),second(),day(),month(),edit_number);
  return;
}


void processSyncEvent(NTPSyncEvent_t ntpEvent) {
	if (ntpEvent) {
		Serial.print("Time Sync error: ");
		if (ntpEvent == noResponse)
			Serial.println("NTP server not reachable");
		else if (ntpEvent == invalidAddress)
			Serial.println("Invalid NTP server address");
	}
	else {
		Serial.print("Got NTP time: ");
		Serial.println(NTP.getTimeDateString(NTP.getLastNTPSync()));
	}
}

boolean syncEventTriggered = false; // True if a time even has been triggered
NTPSyncEvent_t ntpEvent; // Last triggered event

/**
 * Save POST-Argument from Webserver-request to EEPROM
 *
 **/
bool save_config(char* name, int start_address, int length) {
  if (server.hasArg(name)) {
      Serial.print("Save ");
      Serial.print(name);
      Serial.print(": ");
      Serial.println(server.arg(name));
      String value = server.arg(name);
      for (int i = 0; i < length; ++i) EEPROM.write(start_address + i, 0);
      for (int i=0; i < value.length(); ++i) EEPROM.write(start_address + i, value[i]);
      EEPROM.commit();
      return true;
  } else {
      return false;
  }
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
    save_config("ssid", 0, 32);
    save_config("password", 32, 64);
    save_config("ntp", 96, 64);
    save_config("mqtt_broker", 0xa0, 64);
    save_config("mqtt_user", 0xe0, 32);
    save_config("mqtt_password", 0x100, 32);
    save_config("mqtt_topic", 0x120, 32);

    EEPROM.commit();

    server.send(200,"text/html", config_form());
  });

	server.begin();
  Serial.println("HTTP server started");

  unsigned char blue=0;
  char colorstep = 1;
  unsigned int btnstate = 0x00;
  unsigned int oldbtnstate = 0x00;
  // Main-Loop of Configuration-Mode
  while(1) {
    // Check if center btn is released
    oldbtnstate = btnstate;
    btnstate = check_buttons();
    if (oldbtnstate & BTN_CENTER_PRESSED && !(btnstate & BTN_CENTER_PRESSED))
        ESP.restart();
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


String read_config(uint16 start, uint16 length) {
  String value;
  for (int i =0; i<length; ++i) {
    value += char(EEPROM.read(start + i));
  }
  return value;
}

void setup(){

  // Read configured Wifi-Settings
  static WiFiEventHandler e1, e2;
  Serial.begin(115200);
  EEPROM.begin(4096);
  display.off();
  display.print(0);
  strip.begin();
  pinMode(BTN_LEFT, INPUT);
  pinMode(BTN_CENTER, INPUT);
  pinMode(BTN_RIGHT, INPUT);

  // set startdate to 0:0 1/1/2018
  setTime(0,0,0,1,1,2018);

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


  // read eeprom for ssid and pass
  String essid = read_config(0,32);
  Serial.print("SSID: ");
  Serial.println(essid);

  Serial.println("Reading EEPROM pass");
  String epass = read_config(32,64);

  ntp_server = read_config(0x60,64);
  mqtt_server = read_config(0xa0,64);
  Serial.print("MQTT-Broker: ");
  Serial.println(mqtt_server);
  Serial.println(mqtt_server.length());
  mqtt_setup(mqtt_server.c_str(), espClient);
  strip.Color(255, 0, 0);
  strip.show(); // Initialize all pixels to 'off'
  WiFi.mode(WIFI_STA);
  WiFi.begin(essid.c_str(), epass.c_str());

	NTP.onNTPSyncEvent([](NTPSyncEvent_t event) {
		ntpEvent = event;
		syncEventTriggered = true;
	});

  e1 = WiFi.onStationModeGotIP(onSTAGotIP);// As soon WiFi is connected, start NTP Client

  server.begin();
  display.clr();
  display.print(0);
  display.on();
  led_mode = LED_ON;
  red_step = 1;
  blue_step = 1;
  green_step = 1;

  red_value = 0x0c;
  green_value = 0x05;
  blue_value = 0x00;
  for (int i=0; i< LED_COUNT; i++) {
    strip.setPixelColor(i, strip.Color(red_value, green_value, blue_value));
  }
  strip.show();
}

void loop() {


  if (WiFi.isConnected()) {
    if (!mqtt_connector.connected()) mqtt_reconnect();
    mqtt_connector.loop();
  }
  // Read Input-BTNS
  old_btnstate = btnstate;
  btnstate = check_buttons();
  if (old_btnstate != btnstate) {
    Serial.printf("Btn pressed");
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
    }
    // Center Button released
    if (!(btnstate & BTN_CENTER_PRESSED) && old_btnstate & BTN_CENTER_PRESSED) {
      btn_endtime = millis();
      if (btn_endtime - btn_starttimes[1] < 1000) {
        input = BTN_CENTER_SHORT;
      } else if (btn_endtime - btn_starttimes[1] >= 1000 && btn_endtime) {
        input = BTN_CENTER_LONG;
      }
    }
    // Right Button released
    if (!(btnstate & BTN_RIGHT_PRESSED) && old_btnstate & BTN_RIGHT_PRESSED) {
      btn_endtime = millis();
      if (btn_endtime - btn_starttimes[2] < 1000) {
        input = BTN_RIGHT_SHORT;
      } else if (btn_endtime - btn_starttimes[2] >= 1000 && btn_endtime) {
        input = BTN_RIGHT_LONG;
      }
    }

  }

  // interpret button-input
  if (input != 0x00) {
    if (input & BTN_LEFT_PRESSED) menupoint->btn_left_action();
    if (input & BTN_RIGHT_PRESSED) menupoint->btn_right_action();
    if (input & BTN_CENTER_PRESSED) menupoint->btn_center_action();
    if (input & BTN_LEFT_LONG) menupoint->btn_left_long_action();
    if (input & BTN_RIGHT_LONG) menupoint->btn_right_long_action();
    if (input & BTN_CENTER_LONG) menupoint->btn_center_long_action();

    input = 0;
  }

  menupoint->command();

}
