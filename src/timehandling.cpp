#include "timehandling.h"

byte ntp_buffer[ NTP_PACKET_SIZE];
WiFiUDP udp; // A UDP instance to let us send and receive packets over UDP

// Convert normal decimal numbers to binary coded decimal
byte decToBcd(byte val)
{
  return( (val/10*16) + (val%10) );
}
// Convert binary coded decimal to normal decimal numbers
byte bcdToDec(byte val)
{
  return( (val/16*10) + (val%16) );
}

void readDS3231time(byte *second, byte *minute, byte *hour,
  byte *dayOfWeek, byte *dayOfMonth, byte *month, byte *year)
{
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0); // set DS3231 register pointer to 00h
  Wire.endTransmission();
  Wire.requestFrom(DS3231_I2C_ADDRESS, 7);
  // request seven bytes of data from DS3231 starting from register 00h
  *second = bcdToDec(Wire.read() & 0x7f);
  *minute = bcdToDec(Wire.read());
  *hour = bcdToDec(Wire.read() & 0x3f);
  *dayOfWeek = bcdToDec(Wire.read());
  *dayOfMonth = bcdToDec(Wire.read());
  *month = bcdToDec(Wire.read());
  *year = bcdToDec(Wire.read());
}

void setDS3231time(byte second, byte minute, byte hour, byte dayOfWeek, byte
dayOfMonth, byte month, byte year)
{
  // sets time and date data to DS3231
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0); // set next input to start at the seconds register
  Wire.write(decToBcd(second)); // set seconds
  Wire.write(decToBcd(minute)); // set minutes
  Wire.write(decToBcd(hour)); // set hours
  Wire.write(decToBcd(dayOfWeek)); // set day of week (1=Sunday, 7=Saturday)
  Wire.write(decToBcd(dayOfMonth)); // set date (1 to 31)
  Wire.write(decToBcd(month)); // set month
  Wire.write(decToBcd(year)); // set year (0 to 99)
  Wire.endTransmission();
}


// send an NTP request to the time server at the given address
unsigned long sendNTPpacket(String& address)
{
  Serial.println("sending NTP packet...");
  // set all bytes in the buffer to 0
  memset(ntp_buffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  ntp_buffer[0] = 0b11100011;   // LI, Version, Mode
  ntp_buffer[1] = 0;     // Stratum, or type of clock
  ntp_buffer[2] = 6;     // Polling Interval
  ntp_buffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  ntp_buffer[12]  = 49;
  ntp_buffer[13]  = 0x4E;
  ntp_buffer[14]  = 49;
  ntp_buffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address.c_str() , 123); //NTP requests are to port 123
  udp.write(ntp_buffer, NTP_PACKET_SIZE);
  udp.endPacket();
}


nixieTimer::nixieTimer(){

}

void nixieTimer::begin() {
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  if (Wire.endTransmission () == 0) {
    Serial.println("Found MAXDS3231 RTC");
    hasRTC = true;
  } else {
    hasRTC = false;
    Serial.println("No RTC found.");

  }
}


/* Update Time. read either from rtc if installed or local time-storage.
*
*/
void nixieTimer::read_time() {
  if (hasRTC) {
    readDS3231time(&_sec, &_min, &_hour, &_dayOfWeek, &_dayOfMonth, &_month, &_year);
  } else {
    _sec = second();
    _min = minute();
    _hour = hour();
    _dayOfMonth = day();
    _month = month();
    _year = year() - 2000;
  }
}

void nixieTimer::store_time() {
  setTime(_hour, _min, _sec, _dayOfMonth, _month, _year);
  if (hasRTC) {
    byte dow = 1;
    byte y = _year - 2000;
    setDS3231time(_sec, _min, _hour, dow, _dayOfMonth, _month, y);
  }

}

void nixieTimer::fetch_ntptime() {
  sendNTPpacket(ntp_server);
  delay(1000);
  int cb = udp.parsePacket();
  if (!cb) {
    Serial.println("got no answer from NTP-Serer");
  } else {
    Serial.print("Got answer from NTP-Server ");
    Serial.print(ntp_server);
    udp.read(ntp_buffer, NTP_PACKET_SIZE);


    // NTP contains four timestamps with an integer part and a fraction part
    // we only use the integer part here
    unsigned long t1, t2, t3, t4;
    t1 = t2 = t3 = t4 = 0;
    for (int i = 0; i < 4; i++)
    {
      t1 = t1 << 8 | ntp_buffer[16 + i];
      t2 = t2 << 8 | ntp_buffer[24 + i];
      t3 = t3 << 8 | ntp_buffer[32 + i];
      t4 = t4 << 8 | ntp_buffer[40 + i];
    }

    // part of the fractional part
    // could be 4 bytes but this is more precise than the 1307 RTC
    // which has a precision of ONE second
    // in fact one byte is sufficient for 1307
    float f1, f2, f3, f4;
    f1 = ((long)ntp_buffer[20] * 256 + ntp_buffer[21]) / 65536.0;
    f2 = ((long)ntp_buffer[28] * 256 + ntp_buffer[29]) / 65536.0;
    f3 = ((long)ntp_buffer[36] * 256 + ntp_buffer[37]) / 65536.0;
    f4 = ((long)ntp_buffer[44] * 256 + ntp_buffer[45]) / 65536.0;

    // convert NTP to regular unix timestamp, differs seventy years = 2208988800 seconds
    // ntp starts  1900
    // unix time starts 1970
#define SECONDS_FROM_1970_TO_2000 946684800
    const unsigned long seventyYears = 2208988800UL + 946684800UL; //library differences, it wants seconds since 2000 not 1970
    t1 -= seventyYears;
    t2 -= seventyYears;
    t3 -= seventyYears;
    t4 -= seventyYears;
    t4 += 1;               // adjust the delay(1000) at begin of loop!
    if (f4 > 0.4) t4++;    // adjust fractional part, see above
    set_time(t4);

  }

}


uint8_t nixieTimer::get_hour() {
  read_time();
  return _hour;
}

uint8_t nixieTimer::get_minute() {
  read_time();
  return _min;
}

uint8_t nixieTimer::get_month() {
  return month();
}

uint8_t nixieTimer::get_day() {
  return day();
}

uint16_t nixieTimer::get_year() {
  return year();
}

void nixieTimer::set_time(time_t t) {
  setTime(t);
  set_year(year());
  set_month(month());
  set_day(day());
  set_hour(hour());
  set_minute(minute());
  set_second(second());
}

void nixieTimer::set_month(uint8_t m) {
  read_time();
  _month = m;
  store_time();
}

void nixieTimer::set_day(uint8_t d) {
  read_time();
  _dayOfMonth = d;
  store_time();
}


void nixieTimer::set_year(uint16_t y) {
  read_time();
  _year = y;
  store_time();
}


void nixieTimer::set_hour(uint8_t h) {
  read_time();
  _hour = h;
  store_time();
}


void nixieTimer::set_minute(uint8_t m) {
  read_time();
  _min = m;
  store_time();
}

void nixieTimer::set_second(uint8_t s) {
  read_time();
  _sec = s;
  store_time();
}
