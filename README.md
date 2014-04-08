RealTimeHeartrateDetection
==========================

A low-power, real-time heart rate detector (QRS detection) on the MSP430F5529.

Part List
---------
* Ultra-Low-Power Mircontroller: [Texas Instruments MSP430F5529](http://www.ti.com/product/msp430f5529)
* Raw LCD: [Varitronix 3-Digit Reflective LCD](http://www.digikey.ca/product-detail/en/VI-321-DP-RC-S/153-1101-ND/531266)
* ECG Circuit: Instrumentation amplifier, op-amps, resistors, capacitors, and leads

Power Consumption
-----------------
*In active mode:*
Component | Current | Power @ 3V
--------- | ------- | ---------- 
MCU       |  290 μA | 870 μW
LCD       |   30 μA |  90 μW
Total     |  320 μA | 960 μW

*In low power mode:*
Component | Current | Power @ 3V
--------- | ------- | ---------- 
MCU       |   80 μA | 240 μW
LCD       |   30 μA |  90 μW
Total     |  110 μA | 330 μW

