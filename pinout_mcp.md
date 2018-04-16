         0 |
		 1 | - HV-Data
	     2 | - HV-CLK
MCP23008 3 | - Latch
    GPIO 4 | - !HV-Power
		 5 | 
		 6 |
		 7 |
-----------/

import time
from machine import Pin, I2C

MCP23008_ADDRESS = 0x20
i2caddr = MCP23008_ADDRESS | 0x0
i2c = I2C(scl=Pin(5), sda=Pin(4), freq=100000)
i2c.writeto(i2caddr, b'\x00\xe1')
i2c.writeto(i2caddr, b'\x09\x00') # PowerON

for i in range(64):
	i2c.writeto(i2caddr, b'\x09\x04')
	i2c.writeto(i2caddr, b'\x09\x00')

i2c.writeto(i2caddr, b'\x09\x0e')
i2c.writeto(i2caddr, b'\x09\x0a')

i2c.writeto(i2caddr, b'\x09\x06') # Data + CLK
DATA = 0x02
CLK = 0x04
LATCH = 0x08
POWER = 0x10

0  Tube0 1
1  Tube0 2 
2  Tube0 3
3  Tube0 4
4  Tube0 5
5  Tube0 6
6  Tube0 7
7  Tube0 8
8  Tube0 0
9  Tube0 9
10 Tube1 3
11 Tube1 2
12 Tube1 1
13 Tube1 4
14 Tube1 5
15 Tube1 6
16 Tube1 7
17 Tube1 8
18 Tube1 9
19 Tube1 0
20 
21
22
23
24 -
25 -
26 -
27 -
28 -
29 -
30 -
31 -
32 -
33 -
34 -
35 -
36 -
37 -
38 -
39 -
40 -
41 -
42 -
43 Tube2 1
44 Tube2 2
45 Tube2 3
46 Tube2 4
47 Tube2 5
48 -
49 Tube2 9
50 Tube2 8
51 - 
52 Tube2 6
53 - 
54 Tube3 1
55 Tube3 2
56 Tube3 3
57 Tube3 4
58 Tube3 5
59 Tube3 6
60 Tube3 7
61 Tube3 8
62 Tube3 9
63 Tube3 0
