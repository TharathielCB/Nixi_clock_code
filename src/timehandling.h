#ifndef TIMEHANDLING
    #define TIMEHANDLING
    #include <Wire.h>       // i²c library
    #include <WiFiUdp.h>
    #include "Arduino.h"
    #include <Time.h>
    #include <NTPClient.h>

    // #include <TimeLib.h>
    // #include "uRTCLib.h"

    /*
     * The nixieTimer-class is used to read and save time within the
     * nixie-clock. The class is written to support MAX-DS3231 or
     * no RTC for this the constructor asks I²C-BUS wheater the
     * rtc-device is available.
     */

    #define DS3231_I2C_ADDRESS 0x68
    #define NTP_PACKET_SIZE 48
    #define byte uint8_t
    #define dow(t) (int ((t / 86400) + 4) % 7)
    extern String ntp_server;


    void setDS3231time(byte second, byte minute, byte hour, byte
        dayOfMonth, byte month, byte year);
    void readDS3231time(byte *second, byte *minute, byte *hour, byte *dayOfWeek,
        byte *dayOfMonth, byte *month, byte *year);

    class nixieTimer {
    private:
      WiFiUDP* ntp_udp;
      NTPClient* time_client;

      bool hasRTC = false;
      byte _sec, _min, _hour, _dayOfWeek, _dayOfMonth, _month, _year;
      // uRTCLib hw_rtc;
      void read_time();


    public:
      nixieTimer();
      void begin();
      uint8_t get_hour();
      uint8_t get_minute();
      uint8_t get_day();
      uint8_t get_month();
      uint16_t get_year();

      void store_time();
      void set_hour(uint8_t h);
      void set_minute(uint8_t m);
      void set_second(uint8_t s);
      void set_month(uint8_t month);
      void set_day(uint8_t day);
      void set_year(uint16_t year);
      void set_time(time_t t);
      void fetch_ntptime();
    };
#endif
