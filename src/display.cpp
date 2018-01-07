


/** Control Nixie_Displays */

#include "display.h"


void nixie_display::setup_pins() {
    mcp->begin();
    mcp->pinMode(hv_pin, OUTPUT);
    mcp->pinMode(data_pin, OUTPUT);
    mcp->pinMode(clk_pin, OUTPUT);
    mcp->pinMode(strobe_pin, OUTPUT);
}

void nixie_display::clr() {
    for (uint8 i=0; i<64; i++) shift_bit(LOW,1);
}

void nixie_display::on() {
    mcp->digitalWrite(hv_pin, LOW);
    Serial.println("HV on!!!");
}

void nixie_display::off(){
    mcp->digitalWrite(hv_pin, HIGH);
    Serial.println("HV off!");
}

void nixie_display::toggle() {
    if (mcp->digitalRead(hv_pin)) {
      on();
    } else {
      off();
    }
}

void nixie_display::shift_bit(uint8 value, uint16 delay_value) {
    mcp->digitalWrite(strobe_pin, HIGH);
    mcp->digitalWrite(clk_pin, HIGH);
    delayMicroseconds(delay_value);

    if ( value==0) {
        mcp->digitalWrite(data_pin, LOW);
    } else {
        mcp->digitalWrite(data_pin,HIGH);
    };
    mcp->digitalWrite(clk_pin, LOW);
    delayMicroseconds(delay_value);
}

nixie_display::nixie_display(Adafruit_MCP23008 *portexpander) {
    mcp = portexpander;
    setup_pins();
}

void nixie_display::set_delay(uint16 delay) {
    shift_delay = delay;
}

uint16 nixie_display::get_delay() {
    return shift_delay;
}

void nixie_display::print(uint16 value) {
    print(value, shift_delay * 1000);
}

void nixie_display::print(uint16 value, uint16 delay_us) {
    uint8 i;
    uint8 digit[4];
    uint8 sr_bits[64] = { 0 };
    // Pin-Mapping of Shift-Register ->Nixie-Tubes
  	uint8 bitlist_tube0[10] = { 8, 0, 1, 2, 3, 4, 5, 6, 7, 9};
  	uint8 bitlist_tube1[10] = {19,12,11,10,13,14,15,16,17,18};
  	uint8 bitlist_tube2[10] = {47,42,43,44,45,46,51,50,49,48};
  	uint8 bitlist_tube3[10] = {62,53,54,55,56,57,58,59,60,61};

    // extract digits from value
    for (i = 0; i < 4; i++){
        digit[3-i] = value % 10;
        value = value / 10;
    }

    sr_bits[bitlist_tube0[digit[0]]] = HIGH;
    sr_bits[bitlist_tube1[digit[1]]] = HIGH;
    sr_bits[bitlist_tube2[digit[2]]] = HIGH;
    sr_bits[bitlist_tube3[digit[3]]] = HIGH;
    // additionally fill space between tubes with values
    // -> looks better while shifting
    sr_bits[bitlist_tube2[digit[2]]-10] = HIGH;
    sr_bits[bitlist_tube1[digit[1]]+10] = HIGH;

    shift_bit(0,1);
    for (i = 0; i < 64; i++) shift_bit(sr_bits[63-i], delay_us);
}
