


/** Control Nixie_Displays */

#include "display.h"

void serial_publisher(const char* topic, const char *message, bool persistence){
	Serial.print(topic);
	Serial.println(message);
}


void nixie_display::set_publish_callback(PublishFunctionPtr callback) {
	publisher = callback;
}



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
    mcp->digitalWrite(hv_pin, HIGH);
    power = 1;
	mpublish("/status/power", "1");	
    Serial.println("HV on!!!");
}

void nixie_display::off(){
    mcp->digitalWrite(hv_pin, LOW);
    power = 0;
	mpublish("/status/power", "0");	

    Serial.println("HV off!");
}

void nixie_display::toggle() {
    if (power == 1) {
      off();
    } else {
      on();
    }
}

void nixie_display::mpublish(String topic, String message) {
		// send all messages from display as retained message
		publisher((topic).c_str(), (message).c_str(), true);
	}

void nixie_display::shift_bit(uint8 value, uint16 delay_value) {
    mcp->digitalWrite(clk_pin, HIGH);
    delayMicroseconds(delay_value);

    if ( value==0) {
        mcp->digitalWrite(data_pin, LOW);
    } else {
        mcp->digitalWrite(data_pin, HIGH);
    }
    mcp->digitalWrite(clk_pin, LOW);
    delayMicroseconds(delay_value);
}

nixie_display::nixie_display(Adafruit_MCP23008 *portexpander, configuration *config) {
    mcp = portexpander;
	conf = config;
	set_publish_callback(&serial_publisher); // will be set in main
    setup_pins();
}

void nixie_display::set_delay(uint16 delay) {
    shift_delay = delay;
}

uint16 nixie_display::get_delay() {
    return shift_delay;
}

void nixie_display::print(uint16 value) {
    mcp->digitalWrite(strobe_pin, HIGH);
    print(value, shift_delay * 1000);
}


/**
* Print immediately without visible shift
*/
void nixie_display::printh(uint16 value) {
    mcp->digitalWrite(strobe_pin, LOW);
    print(value, 1);
    mcp->digitalWrite(strobe_pin, HIGH);
}


void nixie_display::print_comma(uint8 value) {
	commas_values = value;
	printh(display_value);
}

uint8 nixie_display::get_commas() {
	return commas_values;
}

void nixie_display::print(uint16 value, uint16 delay_us) {
	char *value_str = new char[5];
    sprintf(value_str, "%4d", value);
    
    display_value = value;
	
	uint8 i;
    uint8 digit[4];
    uint8 sr_bits[64] = { 0 };
	Serial.println("preparing bits");
    // Pin-Mapping of Shift-Register ->Nixie-Tubes
  	uint8 bitlist_tube0[10] = {10,0,2,3,4,5,6,7,9,11};
  	uint8 bitlist_tube1[10] = {22,16,15,14,13,12,18,19,20,21};
  	uint8 bitlist_tube2[10] = {46,39,41,42,43,44,50,49,48,47};
  	uint8 bitlist_tube3[10] = {61,52,51,54,55,56,57,58,59,62};

	uint8 commas[8] = {1,8,17,23,40,45,53,60};

    // extract digits from value
    for (i = 0; i < 4; i++){
        digit[3-i] = value % 10;
        value = value / 10;
    }
	Serial.println("create sr_bits");

	// display commas
	for (i = 0; i < 7; i++) {
		if (commas_values & (0x01 << i)) {
			sr_bits[commas[7-i]] = HIGH;
		}
	}	

    sr_bits[bitlist_tube0[digit[0]]] = HIGH;
    sr_bits[bitlist_tube1[digit[1]]] = HIGH;
    sr_bits[bitlist_tube2[digit[2]]] = HIGH;
    sr_bits[bitlist_tube3[digit[3]]] = HIGH;
    // additionally fill space between tubes with values
    // -> looks better while shifting
   // sr_bits[bitlist_tube2[digit[2]]-10] = HIGH;
   // sr_bits[bitlist_tube1[digit[1]]+10] = HIGH;

	// for a prettier looking of the shifting-effect
	// set some of the not connected shift-register ouput to HIGH
	sr_bits[24] = HIGH;
	sr_bits[30] = HIGH;
	sr_bits[38] = HIGH;



	Serial.println("shifting bits");
    shift_bit(0,1);
    for (i = 0; i < 64; i++) shift_bit(sr_bits[63-i], delay_us);
	Serial.println("Publishing");
	Serial.println("converting value into string");
	// itoa(value, value_str, 10);
	Serial.println("sending to mqtt");
	Serial.println(value_str);
	mpublish("/status/display", value_str);
	free(value_str);
}
