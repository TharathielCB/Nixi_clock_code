#include "mqtt_client.h"

String mqtt_topics[8];


void mqtt_subscribe(uint8 id, String topic) {
	char *full_topic = (char*)calloc(strlen(config.mqtt_topic.c_str())+ strlen(topic.c_str()) + 1, sizeof(char));
	strcpy(full_topic, config.mqtt_topic.c_str());
    strcat(full_topic, topic.c_str());
	String str = String(config.mqtt_topic + topic);
	Serial.print("Subscribing to topic ");
	Serial.println(full_topic);
	mqtt_connector.subscribe(full_topic);
	mqtt_topics[id] = String(full_topic);
}


void mqtt_setup(const char* server, Client &client) {
  Serial.print("Setting up mqtt");
  Serial.println(server);
  mqtt_connector.setClient(client);
  mqtt_connector.setServer(server, 1883);
  mqtt_connector.setCallback(mqtt_callback);
  

  mqtt_subscribe(0, String("/display")); 
  mqtt_subscribe(1, String("/power")); 
  mqtt_subscribe(2, String("/leds"));
  mqtt_subscribe(3, String("/mode"));
  mqtt_subscribe(4, String("/ntp"));
  mqtt_subscribe(5, String("/ntp/broker"));
  mqtt_subscribe(6, String("/led/"));


  publish_config();
}



void mqtt_reconnect() {
   // Loop until we're reconnected
  if (!mqtt_connector.connected()) {
  Serial.print("Attempting MQTT connection...");
  // Attempt to connect
  if (mqtt_connector.connect("NixieClock Client")) {
    Serial.println("connected");
    // ... and subscribe to topic
	  mqtt_subscribe(0, String("/display")); 
	  mqtt_subscribe(1, String("/power")); 
	  mqtt_subscribe(2, String("/leds"));
	  mqtt_subscribe(3, String("/mode"));
	  mqtt_subscribe(4, String("/ntp"));
	  mqtt_subscribe(5, String("/ntp/broker"));
	  mqtt_subscribe(6, String("/led/"));
  } else {
    Serial.print("failed, rc=");
    Serial.print(mqtt_connector.state());
    Serial.println(" try again in 5 seconds");
   }
  }
}



void mqtt_publish(const char* topic, const char* payload, bool retained) {
	if (mqtt_connector.connected()) {

		char *full_topic = (char*)calloc(strlen((config.mqtt_topic).c_str()) + strlen(topic) + 1, sizeof(char));
		strcpy(full_topic, config.mqtt_topic.c_str());
        strcat(full_topic, topic);	
		Serial.print("Publishing ");
		Serial.print(full_topic);
		Serial.print(" : ");
		Serial.print(payload);
		if (mqtt_connector.publish(full_topic, payload, retained)) {
			Serial.println(" OK");
		} else {
			Serial.println(" Failed");
		}

		free(full_topic);
	}
}


void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  String message = "";
  String topic_str(topic);
  char red_h[3];
  char green_h[3];
  char blue_h[3];
  for (int i=0;i<length;i++) {
   char receivedChar = (char)payload[i];
   message += receivedChar;
  }
  Serial.print(message);
   Serial.println();

    // TOPIC: display
    if (topic_str == mqtt_topics[0]) {
        // Print given numbers on display
        Serial.println("Displaying message");
		display.print(message.toInt());
    }
	
	// TOPIC: mode
    if (topic_str == mqtt_topics[3]) {
        // Switch mode between "time", "date", "year"
        if (strcmp(message.c_str(),"time")==0) menupoint = &menu_time;
        if (strcmp(message.c_str(),"date")==0) menupoint = &menu_date;
        if (strcmp(message.c_str(),"year")==0) menupoint = &menu_year;
    }

	// TOPIC: power
    if (topic_str ==  mqtt_topics[1]) {
        // send 0 to poweroff tubes, 1 to switch them on
        char received_char = (char)payload[0];
        if (received_char == '0') {
            display.off();
        } else if (received_char == '1') {
            display.on();
        }
    }

	// TOPIC: ntp
    if (topic_str == mqtt_topics[4]) {
        //
        char received_char = (char)payload[0];
        if (received_char == '0') {
          nixieclock.fetch_ntptime();
        }
    }

    // TOPIC: ntp/broker
    if (topic_str, mqtt_topics[5]) {
        //
        char received_char = (char)payload[0];
        if (received_char == '0') {
          nixieclock.fetch_ntptime();
        }
    }

	// TOPIC: leds
    if (!strcmp(topic, mqtt_topics[2].c_str())) {
        // receive rgb_values for all leds
        if (length==6) {

            red_h[0] = (char)payload[0];
            red_h[1] = (char)payload[1];
            red_h[2] = '\0';
            green_h[0] = (char)payload[2];
            green_h[1] = (char)payload[3];
            green_h[2] = '\0';
            blue_h[0] = (char)payload[4];
            blue_h[1] = (char)payload[5];
            blue_h[2] = '\0';

            uint8 red = (uint8)strtol(red_h, NULL, 16);
            uint8 green = (uint8)strtol(green_h, NULL, 16);
            uint8 blue = (uint8)strtol(blue_h, NULL, 16);

            for (uint8 i=0; i<4; i++) led_colors[i] = strip.Color(red,green,blue);

        } else if (length==7 && (char)payload[0] == '#') {

            red_h[0] = (char)payload[1];
            red_h[1] = (char)payload[2];
            red_h[2] = '\0';
            green_h[0] = (char)payload[3];
            green_h[1] = (char)payload[4];
            green_h[2] = '\0';
            blue_h[0] = (char)payload[5];
            blue_h[1] = (char)payload[6];
            blue_h[2] = '\0';

            uint8 red = (uint8)strtol(red_h, NULL, 16);
            uint8 green = (uint8)strtol(green_h, NULL, 16);
            uint8 blue = (uint8)strtol(blue_h, NULL, 16);

            for (uint8 i=0; i<4; i++) led_colors[i] = strip.Color(red,green,blue);

        } else if (length==12) {
            // read rgb values for all 4 leds interpret every byte as uint8
            // for (uint8 i=0; i<4; i++) led_colors[i] = strip.Color(red,green,blue);
        } else if (length==1){
            // if '0' is send switch all leds off
            if ((char) payload[0] == '0') {
                for (uint8 i=0; i<4; i++) led_colors[i] = strip.Color(0,0,0);
            }
        }
        else {
            Serial.println("Wrong number of bytes for LED-Colors");
        }
    }



    if (!strcmp(topic, (config.mqtt_topic + "/led/0").c_str())) {
        // receive rgb_values for first led (hex) e.g. ffffff
        if (length==6) {

            red_h[0] = (char)payload[0];
            red_h[1] = (char)payload[1];
            green_h[0] = (char)payload[2];
            green_h[1] = (char)payload[3];
            blue_h[0] = (char)payload[4];
            blue_h[1] = (char)payload[5];

            uint8 red = (uint8)strtol(red_h, NULL, 16);
            uint8 green = (uint8)strtol(green_h, NULL, 16);
            uint8 blue = (uint8)strtol(blue_h, NULL, 16);
            led_colors[0] = strip.Color(red,green,blue);
          }
    }

    if (!strcmp(topic, (config.mqtt_topic + "/led/1").c_str())) {
        // receive rgb_values for first led (hex) e.g. ffffff
        if (length==6) {

            red_h[0] = (char)payload[0];
            red_h[1] = (char)payload[1];
            green_h[0] = (char)payload[2];
            green_h[1] = (char)payload[3];
            blue_h[0] = (char)payload[4];
            blue_h[1] = (char)payload[5];

            uint8 red = (uint8)strtol(red_h, NULL, 16);
            uint8 green = (uint8)strtol(green_h, NULL, 16);
            uint8 blue = (uint8)strtol(blue_h, NULL, 16);
            led_colors[1] = strip.Color(red,green,blue);

        }
    }

    if (!strcmp(topic,(config.mqtt_topic + "/led/2").c_str())) {
        // receive rgb_values for first led (hex) e.g. ffffff
        if (length==6) {


            red_h[0] = (char)payload[0];
            red_h[1] = (char)payload[1];
            green_h[0] = (char)payload[2];
            green_h[1] = (char)payload[3];
            blue_h[0] = (char)payload[4];
            blue_h[1] = (char)payload[5];

            uint8 red = (uint8)strtol(red_h, NULL, 16);
            uint8 green = (uint8)strtol(green_h, NULL, 16);
            uint8 blue = (uint8)strtol(blue_h, NULL, 16);

            strip.setPixelColor(2, strip.Color(red,green,blue));
            strip.show();
        }
    }

    if (!strcmp(topic, (config.mqtt_topic + "/led/3").c_str())) {
        // receive rgb_values for first led (hex) e.g. ffffff
        if (length==6) {

            red_h[0] = (char)payload[0];
            red_h[1] = (char)payload[1];
            green_h[0] = (char)payload[2];
            green_h[1] = (char)payload[3];
            blue_h[0] = (char)payload[4];
            blue_h[1] = (char)payload[5];

            uint8 red = (uint8)strtol(red_h, NULL, 16);
            uint8 green = (uint8)strtol(green_h, NULL, 16);
            uint8 blue = (uint8)strtol(blue_h, NULL, 16);

            strip.setPixelColor(3, strip.Color(red,green,blue));
            strip.show();
        }
    }
 }

void publish_config() {
	if (mqtt_connector.connected()) {
		// Send Config as retained messages
		mqtt_publish("/config/essid", config.essid.c_str(), true);
		mqtt_publish("/config/ntp_server", config.ntp_server.c_str(), true);
	}
}
