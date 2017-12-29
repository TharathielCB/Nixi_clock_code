


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
    for (uint8 i=0; i<50; i++) shift_bit(LOW);
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

void nixie_display::shift_bit(uint8 value) {
    mcp->digitalWrite(strobe_pin, LOW);

    if ( value==0) {
        mcp->digitalWrite(data_pin, HIGH);
    } else {
        mcp->digitalWrite(data_pin,LOW);
    };

    mcp->digitalWrite(clk_pin, HIGH);
    delay(shift_delay);
    mcp->digitalWrite(clk_pin, LOW);
    delay(shift_delay);

    mcp->digitalWrite(strobe_pin, LOW);
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
    uint8 i;
    uint8 digit[4];
    uint8 sr_bits[64] = {0};
    uint8 bitlist_lsb[10] = {7,11,12,13,14,10,8,9,5,6};
    uint8 bitlist_msb[10] = {18,2,3,4,19,0,1,15,16,17};
  // Pin-Mapping of Shift-Register ->Nixie-Tubes
	uint8 bitlist_tube0[10] = {10,0,3,4,5,6,7,8,9,11};
	uint8 bitlist_tube1[10] = {19,12,11,10,13,14,15,16,17,18};
	uint8 bitlist_tube2[10] = {32+15,32+10,32+11,32+12,32+13,32+14,32+19,32+18,32+17,32+16};
	uint8 bitlist_tube3[10] = {32+30,32+21,32+22,32+23,32+24,32+25,32+26,32+27,32+28,32+29};

	// 5 6,1,2,3                     7 8 9 0 4
    //           8 9 0 6 7 5 1 2 3 4


    // extract digits from value
    for (i = 0; i < 4; i++){
        digit[3-i] = value % 10;
        value = value / 10;
    }

    sr_bits[bitlist_tube0[digit[0]]] = HIGH;
    sr_bits[bitlist_tube1[digit[1]]] = HIGH;
    sr_bits[bitlist_tube2[digit[2]]] = HIGH;
    sr_bits[bitlist_tube3[digit[3]]] = HIGH;

    for (i = 0; i < 64; i++) shift_bit(sr_bits[63-i]);
    // shift_bit(0);
}
