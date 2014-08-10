#include "qrs.h"

#define square(x) ((x)*(x))

// Below are the customizable paramaters for the QRS detection algorithm.
// These change depending on the sampling frequency.
// These parameters are tuned for a 256 Hz sampling frequency.
// Also windows were selected as powers of two for processing efficiency.

/**
  @brief Width of the moving summation for the low pass filter.
  @note Suggested low-pass width should correspond to 150 ms in real-time.
        32 / 250 = 128ms.
  */
const int kLowPassWindowSize = 32;

/**
  @brief Moving average window for the high pass filter.
  */
const uint16_t kHighPassWindowSize = 16;
const uint16_t kHighPassWindowSizePowerOfTwo = 4;

/**
  @brief Width of the frame to decide if it contains a QRS.
  @note Size of the decision frame to determine if frame contains a QRS. The
        width of this frame should be a slightly wider than the width of the
        average QRS based on the ECG sampling rate.
        Average resting heart rate = 250 Hz * 60 / 75HBpm = 200 samp
  */
const uint16_t kQrsDecideFrameSize = 200;

/**
  @brief Width of the frame to get the initial threshold.
  @note Size of the frame to get the the initial peak.
        Min resting heart rate = 250Hz * 60 / 350 = 43 HBpm
  */
const uint16_t kQrsInitialFrameSize = 350;

/**
  @brief The minimum number of samples in between heartbeats.
  @note Max heart rate = (250Hz * 60) / 75 = 200 HBpm.
  */
const uint16_t kMinSamplesBetweenBeats = 75;

/**
  @brief Used to get the heart beats per minute.
  @note beatsPerMin = (sampFreq * 60) / numOfSampBtwnBeats
        sampFreq=256Hz so 60*256 = 15360
        Set here to avoid recalculation.
  */
const uint16_t kSecondsTimesSampFreq = 15360;

/**
  @brief Calculate y1[n] for the moving average high pass filter.
  @param  data  The raw ECG signal.
  @param  n  The index of the signal to calculate y1 for.
  @return The calculation of y1 for the given n.
  @note The equation is given below: (M is the window size for the high pass)
                       M-1
        y1[n] = 1/M  *  Σ (data[n-m])
                       m=0
  */
static inline uint16_t CalculateHighPassY1(uint16_t n, uint16_t* data, uint16_t size)
{
  int16_t index;
  uint16_t sum;
  uint16_t m;

  sum = 0;

  for (m = 0; m < kHighPassWindowSize; ++m)
  {
    index = n - m;

    // Reuse first term if index will be negative.
    if (0 > index)
    {
      index = 0;
    }

    sum += data[index];
  }

  return sum >> kHighPassWindowSizePowerOfTwo;
}

/**
  @brief Calculate y2[n] for the moving average high pass filter.
  @param  data  The raw ECG signal.
  @param  n  The index of the signal to calculate y2 for.
  @return The calculation of y2 for the given n.
  @note The equation is given below: (M is the window size for the high pass)
        y2[n] = data[n - (M+1)/2]
  */
static inline uint16_t CalculateHighPassY2(uint16_t n, uint16_t* data, uint16_t size)
{
  int16_t index;

  index = n - (kHighPassWindowSize + 1) / 2;

  // Reuse first term if index will be negative.
  if (index < 0)
  {
    index = 0;
  }

  return data[index];
}

/**
  @brief Calculate the new threshold based on old threshold and the max value from last sample.
  @param  old_threshold  The old threshold value.
  @param  new_peak  The new max peak of the current frame.
  @return The new threshold
  @note The equation is given below:
        0.01 <= alpha <= 0.10
        gamma=0.15 or gamma=0.20
        NewThreshold = alpha * gamma * new_peak + (1-alpha) * old_threshold
  */
static inline uint16_t CalculateNewThreshold(uint16_t old_threshold, uint16_t new_peak)
{
  uint32_t new_threshold;
  uint32_t retval;
  uint32_t one_minus_alpha;
  uint32_t alpha_times_gamma;

  // Scaled by 2^10.
  alpha_times_gamma = 8; // 0.05 * 0.15 = 0.0075 * 1024 = 8
  one_minus_alpha = 973; // 1 - 0.05 = 0.95 * 1024 = 973

  new_threshold = alpha_times_gamma * new_peak + one_minus_alpha * old_threshold;

  retval = new_threshold >> 10;
  return retval;
}

/**
  @brief Returns the highest value in a given range.
  @param  data  The filtered ECG signal.
  @param  size  The size of the array.
  @return The largest value from the first index to the last index.
  */
static inline uint16_t GetPeak(uint16_t first_index, uint16_t last_index, uint16_t* data, uint16_t size)
{
  uint16_t peak;
  uint16_t i;

  if (last_index > size)
  {
    last_index = size;
  }

  peak = 0;

  for (i = first_index; i < last_index; ++i)
  {
    if (data[i] > peak)
    {
      peak = data[i];
    }
  }

  return peak;
}

void qrs_filter_high_pass(uint16_t* data, uint16_t* data_hp, uint16_t size)
{
  uint16_t y1_n;
  uint16_t y2_n;
  uint16_t n;

  for (n = 0; n < size; ++n)
  {
    y1_n = CalculateHighPassY1(n, data, size);
    y2_n = CalculateHighPassY2(n, data, size);

    if (y2_n > y1_n)
    {
      data_hp[n] = y2_n - y1_n;
    }
    else
    {
      data_hp[n] = 0;
    }
  }
}

void qrs_filter_low_pass(uint16_t* data_hp, uint16_t* data_lp, uint16_t size)
{
  uint32_t z_n;
  uint16_t n;
  uint16_t i;
  uint16_t index;

  for (n = 0; n < size; n++)
  {
    z_n = 0;

    // Sum up the kLowPassWindowSize squared terms.
    for (i = n; i < n + kLowPassWindowSize; ++i)
    {
      index = i;

      // For the last several terms, reuse the last term for the moving average.
      if (index >= size)
      {
        index = size - 1;
      }

      z_n += square(data_hp[index]);
    }

    if (z_n > 0xFFFF)
    {
      data_lp[n] = 0xFFFF;
    }
    else
    {
      data_lp[n] = z_n;
    }
  }
}

uint16_t qrs_get_heartrate(uint16_t* data_lp, uint16_t* data_qrs, uint16_t size)
{
  uint16_t heartbeat_count;
  uint16_t heartbeat_rate;
  uint16_t threshold;
  uint16_t new_peak;
  uint16_t first_frame_index;
  uint16_t last_frame_index;
  uint16_t cur_num_samp_btwn_beats;
  uint16_t num_samp_btwn_beats[16];
  uint16_t i;

  heartbeat_count = 0;
  heartbeat_rate = 0;
  cur_num_samp_btwn_beats = 0;

  // The initial threshold is the largest value of the first frame.
  first_frame_index = 0;
  last_frame_index  = kQrsInitialFrameSize;
  threshold = GetPeak(first_frame_index, last_frame_index, data_lp, size);

  // Detect heartbeats.
  for (first_frame_index = 0;
       first_frame_index < size;
       first_frame_index += kQrsDecideFrameSize)
  {
    // Ensure the frame does not go out of array bounds.
    last_frame_index = first_frame_index + kQrsDecideFrameSize;
    if (last_frame_index > size)
    {
      last_frame_index = size;
    }

    for (i = first_frame_index; i < last_frame_index; ++i)
    {
      if (cur_num_samp_btwn_beats > kMinSamplesBetweenBeats)
      {
        if (data_lp[i] >= threshold)
        {
          data_qrs[i] = 1;
          num_samp_btwn_beats[heartbeat_count] = cur_num_samp_btwn_beats;
          cur_num_samp_btwn_beats = 0;
          heartbeat_count++;
        }
        else
        {
          cur_num_samp_btwn_beats++;
          data_qrs[i] = 0;
        }
      }
      else
      {
        data_qrs[i] = 0;
        cur_num_samp_btwn_beats++;
      }
    }

    new_peak = GetPeak(first_frame_index, last_frame_index, data_lp, size);
    threshold = CalculateNewThreshold(threshold, new_peak);
  }

  // Do not use the first number of samples between heartbeats
  // because it is actually the number of samples from the starting
  // to the first heartbeat. Therefore unreliable to use.
  for (i = 1; i < heartbeat_count; ++i)
  {
    heartbeat_rate += kSecondsTimesSampFreq / num_samp_btwn_beats[i];
  }

  // Get the average heartrate.
  heartbeat_rate /= heartbeat_count - 1;

  return heartbeat_rate;
}

