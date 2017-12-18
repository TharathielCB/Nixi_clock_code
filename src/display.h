#ifndef DISPLAY_H
#define DISPLAY_H
#include "Arduino.h"
#include "Adafruit_MCP23008.h" //port expander

class nixie_display {
private:
    uint8_t hv_pin = 4;
    uint8_t data_pin = 1;
    uint8_t clk_pin = 2;
    uint8_t strobe_pin = 3;
    uint16 shift_delay = 5;
    Adafruit_MCP23008 *mcp;
    void setup_pins();

public:
    // nixie_display(int hv_pin, int data_pin, int clk_pin);
    nixie_display(Adafruit_MCP23008 *portexpander);
    // ~nixie_display();                  // destructor
    void on();
    void off();
    void toggle();
    void clr();
    void shift_bit(uint8 value);
    void print(uint16 value);
    void set_delay(uint16 delay);
    uint16 get_delay();
};
#endif
