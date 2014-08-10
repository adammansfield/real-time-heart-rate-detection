#ifndef QRS_H
#define QRS_H

#include <stdint.h>

/**
  @brief Return the filtered ECG signal using a linear high pass filter with
         a moving average.
  @param data      The raw ECG signal.
  @param data_hp   The high pass filtered output.
  @param size      The size of both arrays.
  @note The equation is given below: (M is the window size for low-pass)
        y[n] = y2[n] - y1[n]
  */
void qrs_filter_high_pass(uint16_t* data, uint16_t* data_hp, uint16_t size);

/**
  @brief Return the filtered ECG signal using a non-linear low pass filter.
  @param data_hp   The high pass filtered ECG signal.
  @param data_lp   The low pass filtered output.
  @param size      The size of both arrays.
  @note The equation is given below: (M is the window size for low-pass)
        z_sum[0] = data[0]^2 + data[1]^2 + data[2]^2 + ... + data[M-1]^2
        z_sum[1] = data[1]^2 + data[2]^2 + data[3]^2 + ... + data[M]^2
         .
         .  i goes to M-1
         .
        z_sum[i] = data[i]^2 + data[i+1]^2 + data[i+2]^2 + ... + data[i+M-1]^2

               M-1
        z[n] =  Σ z_sum[i]
               i=0
  */
void qrs_filter_low_pass(uint16_t* data_hp, uint16_t* data_lp, uint16_t size);

/**
  @brief Return the number of heartbeats found in the filtered ECG sample.
  @param  data_lp  The low pass filtered output.
  @param  size  The size of both arrays.
  @return The umber of heartbeats found.
  @note The equation is given below:
        threshold = alpha * gamma * peak + (1-alpha) * threshold
  */
uint16_t qrs_get_heartrate(uint16_t* data_lp, uint16_t* data_qrs, uint16_t size);

#endif // QRS_H

