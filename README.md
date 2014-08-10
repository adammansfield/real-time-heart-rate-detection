RealTimeHeartrateDetection
==========================

A low-power, real-time heart rate detector (QRS detection) on the MSP430F5529.

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

Part List
---------
* Ultra-Low-Power Mircontroller: [Texas Instruments MSP430F5529](http://www.ti.com/product/msp430f5529)
* Raw LCD: [Varitronix 3-Digit Reflective LCD](http://www.digikey.ca/product-detail/en/VI-321-DP-RC-S/153-1101-ND/531266)
* ECG Circuit: Instrumentation amplifier, op-amps, resistors, capacitors, and leads

Dependencies
------------
* Code Composer Studio v5.5.0.00077+


Pin Map from MSP430 to LCD
--------------------------
MSP430 | 7SEG | LCD
-------|------|----
  P4.0 |  COM |  24
  P3.0 |   1E |   9
  P3.1 |   1D |  10
  P3.2 |   1C |  11
  P3.3 |   1B |  12
  P3.4 |   1A |  13
  P3.5 |   1F |  14
  P3.6 |   1G |  15
  P6.0 |   2E |   5
  P6.1 |   2D |   6
  P6.2 |   2C |   7
  P6.3 |   2B |  16
  P6.4 |   2A |  17
  P6.5 |   2F |  18
  P6.6 |   2G |  19
  P2.0 |   3C |   3
  P2.2 |   3B |  20

Third Party Sources
-------------------
* consoleio.h/c
    - Original Author: oPossum
    - Source: http://forum.43oh.com/topic/2354-printing-to-the-ccs-debugger-console/
* printf.h/c
    - Original Author: oPossum
    - Source: http://forum.43oh.com/topic/1289-tiny-printf-c-version/

