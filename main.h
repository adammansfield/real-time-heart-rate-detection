#ifndef MAIN_H_
#define MAIN_H_

// How often the heartbeat is updated (in seconds).
#define QRS_DETECTING_PERIOD 2

// How often the display is refreshed (in Hz).
#define DISPLAY_REFRESH_FREQUENCY 1

// How often the input is sampled (in Hz).
#define SAMPLING_FREQUENCY 256

// Print debugging information to console.
#define ENABLE_LOGGING 0

// Test QRS detection with a preset sample array.
#define TEST_SAMPLE 0

// The power mode that will be entered and exited.
#define LOW_POWER_MODE LPM1_bits

// The maximum heartrate that can be shown on the display.
#define MAX_HEARTRATE 199

// Length of sample array.
#define SAMPLE_LEN 1250

// The states of the  finite state machine.
typedef enum {
  kStateIdle,
  kStateSnapshotSample, // Copy the current sample array.
  kStateQrsDetect,      // Process sample array to get heart rate and update the display.
  kStateSetDisplay      // Update the display.
} state_t;

// The current state.
state_t state = kStateIdle;

// The current index in the sample array.
int16_t sample_index = 0;

// Pointer to sample array in main. The sample array is not global
// because if it is then the CPU will hang on init_zero.
uint16_t* sample_array_pointer = NULL;

/**
  @brief Sample, detect and show heartrate.
  */
int main(void);

#endif // MAIN_H_

