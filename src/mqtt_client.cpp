#include "mqtt_client.h"

void mqtt_setup(const char* server, Client &client) {
  mqtt_connector.setClient(client);
  mqtt_connector.setServer(server, 1883);
  mqtt_connector.setCallback(mqtt_callback);
  mqtt_connector.subscribe("nixieClock");
}


void mqtt_reconnect() {
   // Loop until we're reconnected
  if (!mqtt_connector.connected()) {
  Serial.print("Attempting MQTT connection...");
  // Attempt to connect
  if (mqtt_connector.connect("NixieClock Client")) {
    Serial.println("connected");
    // ... and subscribe to topic
    mqtt_connector.subscribe("nixieClock");
    mqtt_connector.subscribe("nixieClock/display");
    mqtt_connector.subscribe("nixieClock/power");
    mqtt_connector.subscribe("nixieClock/leds");
  //  mqtt_connector.subscribe("nixieClock/leds/0");
  //  mqtt_connector.subscribe("nixieClock/leds/1");
  //  mqtt_connector.subscribe("nixieClock/leds/2");
  //  mqtt_connector.subscribe("nixieClock/leds/3");
  } else {
   Serial.print("failed, rc=");
   Serial.print(mqtt_connector.state());
   Serial.println(" try again in 5 seconds");
   // Wait 5 seconds before retrying
   delay(5000);
   }
  }
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  String message = "";
  char red_h[3];
  char green_h[3];
  char blue_h[3];

  for (int i=0;i<length;i++) {
   char receivedChar = (char)payload[i];
   message += receivedChar;
   Serial.print(receivedChar);

   }
   Serial.println();

   if (!strcmp(topic,"nixieClock/display")) {
        // Print given numbers on display
        Serial.println("Displaying message");
        display.print(message.toInt());
   }

   if (!strcmp(topic, "nixieClock/power")) {
        // send 0 to poweroff tubes, 1 to switch them on
        char received_char = (char)payload[0];
        if (received_char == '0') {
            display.off();
        } else if (received_char == '1') {
            display.on();
        }
    }

    if (!strcmp(topic,"nixieClock/leds")) {
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

            for (uint8 i=0; i<4; i++) strip.setPixelColor(i, strip.Color(red,green,blue));
            strip.show();

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

            for (uint8 i=0; i<4; i++) strip.setPixelColor(i, strip.Color(red,green,blue));
            strip.show();

        } else if (length==12) {
            // read rgb values for all 4 leds interpret every byte as uint8
            for (int i=0; i<4; i++)
                strip.setPixelColor(i, strip.Color((uint8) payload[i*3+0], (uint8) payload[i*3+1], (uint8) payload[i*3+2]));
            strip.show();
        } else if (length==1){
            // if '0' is send switch all leds off
            if ((char) payload[0] == '0') {
                for (int i=0; i<4; i++) {
                    strip.setPixelColor(i, strip.Color(0,0,0));
                }
                strip.show();
            }
        }
        else {
            Serial.println("Wrong number of bytes for LED-Colors");
        }
    }



    if (!strcmp(topic,"nixieClock/leds/0")) {
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
            Serial.print("Setting LED ");
            Serial.println(red);
            strip.setPixelColor(0, strip.Color(red,green,blue));
            strip.show();
        }
    }

    if (!strcmp(topic,"nixieClock/leds/1")) {
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

            strip.setPixelColor(1, strip.Color(red,green,blue));
            strip.show();
        }
    }

    if (!strcmp(topic,"nixieClock/leds/2")) {
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

    if (!strcmp(topic,"nixieClock/leds/3")) {
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
