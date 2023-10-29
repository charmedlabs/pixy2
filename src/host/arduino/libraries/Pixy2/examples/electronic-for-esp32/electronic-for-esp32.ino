//#include <Pixy2Esp32.h>
//Pixy2Esp32 pixy;
//#include ZumoMotorsEsp32.h

// electronic for esp32
/*
purple ->  5V
white  ->  GND
grey   ->  23
yellow ->  19
green  ->  18
blue   ->  5

purple  top left       (PU)
grey    middle left    (GY)
white   bottom left    (WH)

yellow  top right      (YL)
green   middle right   (GR)  
blue    bottom right   (BL)


 ESP32
   ^
   |
   |
   |
(PU)(YL)<-----
(GY)(GR)     |
(WH)(BL)     |
             |
 Pixy2       |
(1)(2)       |
(3)(4)--------
(5)(6)
(7)(8)
(9)(10)

(1) -> MISO      PIN->(19)
(2) -> 5V        PIN->(5V)
(3) -> SCK (CLK) PIN->(18)
(4) -> MOSI      PIN->(23)
(5) -> (NOT IMPORTANT) I2C SCL
(6) -> GND       PIN->(GND)
(7) -> SPI SS    PIN-> (5)
(8) -> (NOT IMPORTANT) Analog Out
(9) -> (NOT IMPORTANT) SDA
(10) ->(NOT IMPORTANT) Vin (6-10V)

PAN TILT
pwm0(pan) pwm1 (tilt)
Vout      Vout
GND       GND
*/