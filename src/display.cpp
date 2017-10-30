


/** Control Nixie_Displays */

#include "display.h"


void nixie_display::setup_pins() {
    pinMode(hv_pin, OUTPUT);
    pinMode(data_pin, OUTPUT);
    pinMode(clk_pin, OUTPUT);
}

void nixie_display::clr() {
    for (uint8 i=0; i<50; i++) shift_bit(LOW);
}

void nixie_display::on() {
    digitalWrite(hv_pin, LOW);
}

void nixie_display::off(){
    digitalWrite(hv_pin, HIGH);
}

void nixie_display::shift_bit(uint8 value) {

    if ( value==0) { 
        digitalWrite(data_pin, HIGH);
    } else {
        digitalWrite(data_pin,LOW);
    };

    digitalWrite(clk_pin, HIGH);
    delay(shift_delay);
    digitalWrite(clk_pin, LOW);
    delay(shift_delay);
}

nixie_display::nixie_display() {
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
    uint8 minutes[20] = {0};
    uint8 hour[20] = {0};
    uint8 bitlist_lsb[10] = {7,11,12,13,14,10,8,9,5,6};
    uint8 bitlist_msb[10] = {18,2,3,4,19,0,1,15,16,17};

    // 5 6,1,2,3                     7 8 9 0 4
    //           8 9 0 6 7 5 1 2 3 4


    // extract digits from value
    for (i = 0; i < 4; i++){
        digit[3-i] = value % 10; 
        value = value / 10;
    }

    hour[bitlist_msb[digit[0]]] = HIGH;
    hour[bitlist_lsb[digit[1]]] = HIGH;
    minutes[bitlist_msb[digit[2]]] = HIGH;
    minutes[bitlist_lsb[digit[3]]] = HIGH;

    for (i = 0; i < 20; i++) shift_bit(minutes[19-i]);
    for (i = 0; i < 20; i++) shift_bit(hour[19-i]);
    // shift_bit(0);
}
                                                                                                                                                              