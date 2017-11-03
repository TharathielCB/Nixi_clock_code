## nixie-uhr-bausatz/nixieuhr_firmware

EEPROM Usage
------------

Starting at 512
------- --------------------- ------------
Address Content               Length[byte]
------- --------------------- ------------
0x00    SSID                   32
0x20    SSID-Password          64
0x60    NTP-Server             64
0xa0    MQTT-Broker            64
0xe0    MQTT-username          32
0x100   MQTT-Password          32
0x120   MQTT-Topic             32
