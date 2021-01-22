# DualCatFeeder
Before starting, most of the credits are going to Thomas Krichbaumer who provide me the CAD files for his cat feeder design (https://www.thingiverse.com/thing:3623148).

![Front of the DualCatFeeder](/media/dual_cat_feeder.jpg)
![GIF of electronical setup](/media/esp8266motor.gif)
![GIF of web app](/media/web_app.webp)

## Where is the code?
For some real code to use with this device, head over to https://github.com/ultrara1n/dualcatfeeder-esp32

## BOM
s
The electronics are based on an ESP32 microcontroller, a L298N driver and two DC-Motors (relativeley high torque and low rpm). 

Beside standard tinker equipment (soldering iron, breadboard, cable etc.) there are some components I especially bought for this project:

Component | Amount | Source | Price
------------ | ------------- | -------------| -------------
ESP32       | 1 | [AliExpress](https://www.aliexpress.com/item/1005001635370174.html)   | 5$
DC12V 18RPM Motor        | 2 | [AliExpress](https://www.aliexpress.com/item/32867070357.html)   | 16,12$
L298N motor driver        | 1 | [AliExpress](https://www.aliexpress.com/item/33012645746.html)   | 1,40$
6mm rigid flange coupler  | 2 | [AliExpress](https://www.aliexpress.com/item/4000317773964.html) | 2,02$
DC Power Socket Screwable | 1 | [AliExpress](https://www.aliexpress.com/item/1987966589.html), [Amazon](https://www.amazon.de/gp/product/B00FWP5EYK/), [Banggood](https://www.banggood.com/10pcs-DC-022-5_5-2_1mm-Round-Hole-Screw-Nut-DC-Power-Socket-ROHS-Internal-Diameter-5_5mm-p-1200831.html) | 1,83€
DC12V 2A power supply | 1 |

Make sure that the size of your power socket and the provided plug of the power supply are matching! In this case both are 5,5mm*2,1mm.

<!-- ## Power consumption
Situation | Ampere
------------ | -------------
Both motors off, ESP8266 connected to WiFi | 0,021A
Both motors on, no load, ESP8266 connected to WiFi | 0,031A
Both motors on, highest load I can achieve with my bare hands, ESP8266 connected to WiFI | 0,075A

So we are around 18 Watts in high load enviroments - real life measurements to come.
This means the system should be powered by an 12V 2A power supply.

To save space and minimize cables flying around I designed a PCB (and a case for it): -->
![Image of nearly finished models](/media/pcb.png)

