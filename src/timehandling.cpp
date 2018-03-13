#include "timehandling.h"

byte ntp_buffer[ NTP_PACKET_SIZE];
WiFiUDP udp; // A UDP instance to let us send and receive packets over UDP
NTPClient tc(udp, ntp_server.c_str());


bool is_dst(int day, int month, int year, int dow) {
        if (month < 3 || month > 10)  return false;
        if (month > 3 && month < 10)  return true;

        int previousSunday = day - dow;

        if (month == 3) return previousSunday >= 25;
        if (month == 10) return previousSunday < 25;

        return false; // this line never gonna happend
    }

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
unsigned long sendNTPpacket(const char* address)
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
  // 8 bytes of zero for Root De   Udp.write(packetBuffer, NTP_PACKET_SIZE);lay & Root Dispersion
  ntp_buffer[12]  = 49;
  ntp_buffer[13]  = 0x4E;
  ntp_buffer[14]  = 49;
  ntp_buffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(ntp_buffer, NTP_PACKET_SIZE);
  udp.endPacket();
}


nixieTimer::nixieTimer(){
    time_client = &tc;
}

void nixieTimer::begin() {
  udp.begin(1337);
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
  Serial.println("Update Time");
  // time_client->update();
  // set_time(time_client->getEpochTime());

  Serial.println("Sending ntp package.");
  sendNTPpacket(ntp_server.c_str());
  Serial.println(ntp_server);
  // wait to see if a reply is available
  delay(1000);
  if (udp.parsePacket()) {
    // We've received a packet, read the data from it
    udp.read(ntp_buffer, NTP_PACKET_SIZE); // read the packet into the buffer

    // the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, extract the two words:

    unsigned long highWord = word(ntp_buffer[40], ntp_buffer[41]);
    unsigned long lowWord = word(ntp_buffer[42], ntp_buffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    // now convert NTP time into everyday time:
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;
    // subtract seventy years:
    unsigned long epoch = secsSince1900 - seventyYears + 3600;
    Serial.print(epoch);
    set_time(epoch);

    // Finally adjust daylight_saving time
    int day_of_week = dow(epoch);
    if (is_dst(_dayOfMonth, _month, _year, day_of_week)) {
      Serial.println("Daylight saving time");
      set_time(epoch + 3600);
    }
  } else {
    Serial.println("No Answer from ntp_server");
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
  setDS3231time(second(), minute(), hour(), day(), dow(t), month(), year());
  _year = year();
  _month = month();
  _dayOfMonth = day();
  _hour = hour();
  _min = minute();
  _sec = second();
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
