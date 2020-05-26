# DualCatFeeder
Before starting, most of the credits are going to Thomas Krichbaumer from https://www.machs-selbst.net/ who provided me with his CAD files for the basic cat feeder design.

![Image of nearly finished models](/media/dualcatfeeder.png)
![GIF of electronical setup](/media/esp8266motor.gif)

The electronics are based on an ESP8266 microcontroller, a L298N driver (evaluating this atm, maybe replacing with a TB6612) and two DC-Motors (relativeley high torque and low rpm). 

Beside standard tinker equipment (soldering iron, breadboard, cable etc.) there are some components I especially bought for this project:

Component | Amount | Source | Price
------------ | ------------- | -------------| -------------
ESP8266 ESP-12F 4MB       | 1 | [AliExpress](https://www.aliexpress.com/item/33020743322.html)   | 1,80$
ESP8266 Breakout Board    | 1 | [AliExpress](https://www.aliexpress.com/item/32860694356.html)   | 0,40$
DC12V 18RPM Motor        | 2 | [AliExpress](https://www.aliexpress.com/item/32867070357.html)   | 16,12$
L298N motor driver        | 1 | [AliExpress](https://www.aliexpress.com/item/33012645746.html)   | 1,40$
6mm rigid flange coupler  | 2 | [AliExpress](https://www.aliexpress.com/item/4000317773964.html) | 2,02$
LF33CV voltage regulator  | 1 | [Conrad](https://www.conrad.de/de/p/stmicroelectronics-lf33cv-spannungsregler-linear-to-220ab-positiv-fest-500-1185795.html) | 0,92€
Push Buttons              | 3 | [AliExpress](https://www.aliexpress.com/item/33010781184.html) | 1,20$
DC Power Socker Screwable | 1 | [AliExpress](https://www.aliexpress.com/item/1987966589.html) [Amazon](https://www.amazon.de/gp/product/B00FWP5EYK/) [Banggood](https://www.banggood.com/10pcs-DC-022-5_5-2_1mm-Round-Hole-Screw-Nut-DC-Power-Socket-ROHS-Internal-Diameter-5_5mm-p-1200831.html) | 1,83€

Make sure that the size of your power socket and the provided plug of the power supply are matching! In this case both are 5,5mm*2,1mm.

Power consumption
Situation | Ampere
------------ | -------------
Both motors off, ESP8266 connected to WiFi | 0,021A
Both motors on, no load, ESP8266 connected to WiFi | 0,031A
Both motors on, highest load I can achieve with my bare hands, ESP8266 connected to WiFI | 0,075A

So we are around 18 Watts in high load enviroments - real life measurements to come.
This means the system should be powered by an 12V 2A power supply.

To save space and minimize cables flying around I designed a PCB (and a case for it):
![Image of nearly finished models](/media/pcb.png)