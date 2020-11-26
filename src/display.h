#ifndef DISPLAY_H
#define DISPLAY_H
#include <Time.h>
#include "Arduino.h"
#include "Adafruit_MCP23008.h" //port expander
#include "config.h"
#include <string>
// #include "mqtt_client.h"


typedef void(*PublishFunctionPtr)(const char*, const char*, bool);

void serial_publisher(const char* topic, const char* message, bool persistence);

class nixie_display {
private:
	configuration *conf; 
	uint8_t power = 0;
    uint8_t hv_pin = 6;
    uint8_t data_pin = 3;
    uint8_t clk_pin = 4;
    uint8_t strobe_pin = 5;
    uint16 shift_delay = 5; // make change of value look fancy
    Adafruit_MCP23008 *mcp;
	void setup_pins();
    PublishFunctionPtr publisher;
	void mpublish(String topic, String message);


public:
    // nixie_display(int hv_pin, int data_pin, int clk_pin);
    nixie_display(Adafruit_MCP23008 *portexpander, configuration *config);
    // ~nixie_display();                  // destructor
    void on();
    void off();
    void toggle();
    void clr();
    void shift_bit(uint8 value, uint16 delay_us);
    void print(uint16 value);
    void printh(uint16 value);
    void print(uint16 value, uint16 delay_us);
	void set_publish_callback(PublishFunctionPtr callback);
    void set_delay(uint16 delay);
    uint16 get_delay();
};
#endif
