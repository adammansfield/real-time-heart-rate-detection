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

Component            | Power | Voltage | Current
-------------------- | ----- | ------- | -------
MCU                  |       |         |
MCU (LowPowerMode)   |       |         |
LCD                  |       |         |
Total                | 960uW |      3V |  320 uA
Total (LowPowerMode) | 330uW |      3V |  110 uA
