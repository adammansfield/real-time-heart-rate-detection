RealTimeHeartrateDetection
==========================

An ultra low power, real-time heart rate detector (QRS detection) on the MSP430F5529.

Part List
---------
* Ultra Low Power Mircontroller: [Texas Instruments MSP430F5529](http://www.ti.com/product/msp430f5529)
* Raw LCD: [Varitronix 3-Digit Reflective LCD](http://www.digikey.ca/product-detail/en/VI-321-DP-RC-S/153-1101-ND/531266)
* ECG Circuit: Instrumentation amplifier, op-amps, resistors, capacitors, and leads

Dependencies
------------
* Code Composer Studio v5.5.0.00077+

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

QRS Detection Algorithm
-----------------------
A moving-average-based QRS detection algorithm is used to detect heartbeats.
This algorithm has three steps: 1) a moving average-based highpass filter,
2) a nonlinear lowpass filter and 3) threshold detection.

**Step 1 - High Pass Filter**  
This step's purpose is to suppress baseline drift and emphasize the QRS complex.

```
x: input data
M: moving average window size for high-pass
y1: moving average filter
y2: delayed input sample
y: FIR high pass filter with linear phase output

               M-1
y1[n] = 1/M  *  Σ (x[n-m])
               m=0
y2[n] = x[n - (M+1)/2]
y[n] = y2[n] - y1[n]
```

**Step 2 - Low Pass Filter**  
This step's purpose is to accentuate peaks.

```
M: moving average window size for low-pass
h: data points from low-pass filter
z: nonlinear lowpass filter output

z_sum[0] = h[0]^2 + h[1]^2 + h[2]^2 + ... + h[M-1]^2
z_sum[1] = h[1]^2 + h[2]^2 + h[3]^2 + ... + h[M]^2
z_sum[2] = h[2]^2 + h[3]^2 + h[4]^2 + ... + h[M+1]^2
 .
 .  i goes to M-1
 .
z_sum[i] = h[i]^2 + h[i+1]^2 + h[i+2]^2 + ... + h[i+M-1]^2

       M-1
z[n] =  Σ z_sum[i]
       i=0
```

**Step 3 - Threshold Detection**  
This step's purpose is to detect heartbeats. A heartbeat is detected if the
peak of a detection frame is above the calculated threshold. The next
threshold is then calculated based on the old threshold and the new peak.

```
alpha: forgetting factor (range from 0.01 to 0.10)
gamma: weighting factor for new_peak (0.15 or 0.20)
new_peak: local maximum detected in new frame

new_threshold = alpha * gamma * new_peak + (1-alpha) * old_threshold
```

**Source:** H.C. Chen and S.W. Chen, “A Moving Average based Filtering System
with its Application to Real-time QRS Detection,” IEEE Computers in
Cardiology, 2003, pp.585-588. [Link to PDF](http://cinc.org/archives/2003/pdf/585.pdf)

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

