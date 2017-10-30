#include <NtpClientLib.h>
#include <TimeLib.h>

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

const char *ssid     = "nixieClock";
WiFiUDP ntpUDP;  

nixie_display display;
 int number = 0;
 int oldnumber = 0;
int i = 0;
uint8 red_value, blue_value, green_value = 0;
int8 red_step, blue_step, green_step = 0;

#define LED_COUNT 4
#define LED_PIN 13
#define DEFAULT_COLOR 0xFF5900
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

MDNSResponder mdns;
ESP8266WebServer server(80);
WiFiClient espClient;
PubSubClient mqtt_client(espClient);

String mqtt_server;
String password;
char hostString[16] = {0};

uint8 led_mode = 0;

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i=0;i<length;i++) {
   char receivedChar = (char)payload[i];
   Serial.print(receivedChar);

   if (receivedChar == '0') {
   // ESP8266 Huzzah outputs are "reversed"
      strip.setPixelColor(1, strip.Color(0,0,0));
      strip.show();
   }
   if (receivedChar == '1') {
      strip.setPixelColor(1, strip.Color(0,255,0));
   strip.show();
   }
   }
   Serial.println();
 }

// Start NTP only after IP network is connected
void onSTAGotIP(WiFiEventStationModeGotIP ipInfo) {
	Serial.printf("Got IP: %s\r\n", ipInfo.ip.toString().c_str());
	NTP.begin("pool.ntp.org", 1, true);
	NTP.setInterval(63);
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


void mqtt_reconnect() {
   // Loop until we're reconnected
  if (!mqtt_client.connected()) {
  Serial.print("Attempting MQTT connection...");
  // Attempt to connect
  if (mqtt_client.connect("NixieClock Client")) {
   Serial.println("connected");
   // ... and subscribe to topic
   mqtt_client.subscribe("nixieClock");
  } else {
   Serial.print("failed, rc=");
   Serial.print(mqtt_client.state());
   Serial.println(" try again in 5 seconds");
   // Wait 5 seconds before retrying
   delay(5000);
   }
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
    if (server.hasArg("password")) {
      Serial.print("Got password: ");
      Serial.println(server.arg("password"));
      String qpass = server.arg("password");
      for (int i = 32; i < 96; ++i) { EEPROM.write(i, 0); }
      for (int i = 0; i < qpass.length(); ++i)
      {
        EEPROM.write(32+i, qpass[i]);
        Serial.print("Wrote: ");
        Serial.println(qpass[i]); 
      }    

    }
    if (server.hasArg("ssid")) {
      Serial.print("Got SSID: ");
      Serial.println(server.arg("ssid"));
      String qssid = server.arg("ssid");
      for (int i = 0; i < 32; ++i) { EEPROM.write(i, 0); }
      for (int i=0; i < qssid.length(); ++i)
      {
        EEPROM.write( i, qssid[i]);
        Serial.print("Wrote: ");
        Serial.println(qssid[i]);
      }
    }
  EEPROM.commit();

    server.send(200,"text/html", config_form());
  });
  
	server.begin();
  Serial.println("HTTP server started");

  unsigned char blue=0;
  char colorstep = 1;

  // Main-Loop of Configuration-Mode
  while(1) {

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

  // Read configured Wifi-Settings
  /*
  String esid;
  for (int i = 0; i < 32; ++i)
    {
      esid += char(EEPROM.read(i));
  }

  String epass = "";
  for (int i = 32; i < 96; ++i)
    {
      epass += char(EEPROM.read(i));
  }

  if ( esid.length() > 1 ) {
    // test esid 
    WiFi.begin(esid.c_str(), epass.c_str());
    if ( testWifi() == 20 ) { 
        launch_configuration(0);
        return;
    }
  }
 */ 
  static WiFiEventHandler e1, e2;
  Serial.begin(115200);
  EEPROM.begin(512);
  display.off();
  display.print(0);
  strip.begin();
  pinMode(16, INPUT);      // sets the digital pin 0 as input

  // read eeprom for ssid and pass
  Serial.println("Reading EEPROM ssid");
  String essid;
  for (int i = 0; i < 32; ++i) essid += char(EEPROM.read(i));
  
  Serial.print("SSID: ");
  Serial.println(essid);

  Serial.println("Reading EEPROM pass");
  String epass = "";
  for (int i = 32; i < 96; ++i) epass += char(EEPROM.read(i));
  Serial.print("PASS: ");
  Serial.println(epass);

  mqtt_client.setServer(mqtt_server.c_str(), 1883);
  mqtt_client.setCallback(callback);
  mqtt_client.subscribe("nixieClock");
  strip.Color(255, 0, 0);
  strip.show(); // Initialize all pixels to 'off'
  WiFi.mode(WIFI_STA);
  WiFi.begin(essid.c_str(), epass.c_str());

	NTP.onNTPSyncEvent([](NTPSyncEvent_t event) {
		ntpEvent = event;
		syncEventTriggered = true;
	});

  e1 = WiFi.onStationModeGotIP(onSTAGotIP);// As soon WiFi is connected, start NTP Client

  int red_value = 0;
  int led_step = SPEED_STEP;
  while ( WiFi.status() != WL_CONNECTED ) {
    delay(50);
    for (int i=0; i< LED_COUNT; i++) {
      strip.setPixelColor(i, strip.Color(red_value,0,0)); 
    }
    red_value = red_value + led_step;

    if (red_value > 180) {
      red_value = 180;
      led_step = -led_step;
    }

    if (red_value < 0) {
      red_value = 0;
      led_step = -led_step;
    }
    strip.show();
    
    if (!digitalRead(16)) {
      config_mode();
    }
    

  }
  
  server.begin();
  display.clr();
  display.on();

  led_mode = LED_FADING;
  red_step = 1;
  blue_step = 1;
  green_step = 1;

}

void loop() {
  
 // if (!mqtt_client.connected()) 
 //   mqtt_reconnect(); 
    
  mqtt_client.loop();
  
  oldnumber = number;
  number = hour() * 100 + minute();
  if (number != oldnumber ) {
    display.print(number);
    Serial.println(number);
  }

  if (led_mode == LED_FADING) {
    red_value += red_step;
    blue_value += blue_step;
    green_value += green_step;

    if (red_value > 253 && red_step > 0) red_step = -1;
    if (red_value < 1 && red_step < 0) red_step = 1;

    if (green_value > 253 && green_step > 0) green_step = -1;
    if (green_value < 1 && green_step < 0) green_step = 1;

    if (blue_value > 253 && blue_step > 0) blue_step = -1;
    if (blue_value < 1 &&  blue_step < 0) blue_step = 1;
    
    for (int i=0; i< LED_COUNT; i++) {
      strip.setPixelColor(i, strip.Color(red_value, green_value, blue_value)); 
    }

    strip.show();
  } else if (led_mode == LED_OFF) {
    for (int i=0; i< LED_COUNT; i++) {
      strip.setPixelColor(i, strip.Color(0, 0, 0)); 
    }
    strip.show();
  }
  delay(50);
}