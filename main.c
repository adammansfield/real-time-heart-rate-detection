#include <msp430.h>
#include <stdlib.h>
#include <stdint.h>
#include "main.h"
#include "printf.h"
#include "qrs.h"

/**
  @brief Disables the USB component.
  */
static void disable_usb(void)
{
  USBKEYPID = USBKEY;    // Enable access to USB config registers.
  USBPWRCTL &= ~VUSBEN;  // Disable the VUSB LDO.
  USBPWRCTL &= ~SLDOEN;  // Disable the VUSB SLDO.
  USBKEYPID = 0x9600;    // Disable access to USB config registers.
}

/**
  @brief Initializes the ADC converter.
  */
static void init_adc(void)
{
  P7DIR &= ~BIT0;                 // Set P7.0 as input.
  P7SEL |= BIT0;                  // ADC option select A12 (P7.0).

  ADC12CTL0 = ADC12SHT02 |        // 64 CLK cycles sampling time.
              ADC12ON;            // ADC12 on.
  ADC12CTL1 = ADC12CSTARTADD_0 |  // ADC12 Conversion Start Address 0.
              ADC12SSEL_3      |  // SMCLK.
              ADC12SHP;           // Sample/Hold Pulse Mode.
  ADC12MCTL0 = ADC12INCH_12;      // Use A12 (P7.0) as input

  ADC12IE = ADC12IE0;             // Enable interrupt on ADC12IE0.
  ADC12CTL0 |= ADC12ENC;          // Enable conversion.
}

/**
  @brief Initializes the timer that changes state to start processing ECG.
  */
static void init_detector_timer(void)
{
  TA0CCR0 = (32768 >> 3) * QRS_DETECTING_PERIOD;

  TA0CTL = TASSEL_1 |  // ACLK (32768 Hz)
           ID_3     |  // Clock divider. Divide by 8 (2^3).
           MC_1     |  // Up to TA0CCR0.
           TACLR;      // Clear timer.

  TA0CCTL0 = CCIE;     // Enable interrupt on TA0CCR0.
}

/**
  @brief Initializes the pins used to drive the display.
  */
static void init_display_driver(void)
{
  // Set pins to output direction.
  P4DIR |= 0x01;  // P4.0.
  P3DIR |= 0x7F;  // P3.0 to  P3.6 (First Digit).
  P6DIR |= 0x7F;  // P6.0 to  P6.6 (Second Digit).
  P2DIR |= 0x05;  // P2.0 and P2.2 (Third Digit).
}

/**
  @brief Initializes the timer used to refresh the display.
  */
static void init_display_timer(void)
{
  TA2CCR0 = (32768 >> 3) / DISPLAY_REFRESH_FREQUENCY;

  TA2CTL = TASSEL_1 |  // ACLK (32768 Hz)
           ID_3     |  // Clock divider. Divide by 8 (2^3).
           MC_1     |  // Up to TA2CCR0.
           TACLR;      // Clear timer.

  TA2CCTL0 = CCIE;     // Enable interrupt on TA2CCR0.
}

/**
  @brief Initializes the timer that does the sampling.
  @notes ECG has a bandwidth up to 150 Hz.
         Spectrum analysis shows that the majority of the component in below 30 Hz.
  */
static void init_sampler_timer(void)
{
#if TEST_SAMPLE == 0
  TA1CCR0 = (32768 >> 3) / SAMPLING_FREQUENCY;

  TA1CTL = TASSEL_1 |  // ACLK (32768 Hz)
           ID_3     |  // Clock divider. Divide by 8 (2^3).
           MC_1     |  // Up to TA1CCR0.
           TACLR;      // Clear timer.

  TA1CCTL0 = CCIE;     // Enable interrupt on TA1CCR0.
#endif
}

/**
  @brief Copies value into each of the first size characters of the object pointer.
  */
void memset(void* pointer, int8_t value, uint16_t size)
{
  int8_t* int8_pointer = (int8_t*)pointer;

  while (size)
  {
    *int8_pointer = value;
    int8_pointer++;
    size--;
  }
}

/**
  @brief Initialize all pins as output and output low.
  @notes This is done to ensure that there are no floating inputs.
         This should be called before setting any other pins.
  */
static void set_pins_to_output_low(void)
{
  P1DIR = 0xFF;
  P2DIR = 0xFF;
  P3DIR = 0xFF;
  P4DIR = 0xFF;
  P5DIR = 0xFF;
  P6DIR = 0xFF;
  P7DIR = 0xFF;
  P8DIR = 0xFF;
  PJDIR = 0xFF;

  P1OUT = 0x00;
  P2OUT = 0x00;
  P3OUT = 0x00;
  P4OUT = 0x00;
  P5OUT = 0x00;
  P6OUT = 0x00;
  P7OUT = 0x00;
  P8OUT = 0x00;
  PJOUT = 0x00;
}

/**
  @brief Parses a number into three digits to display on the LCD.
  @param  value  Number to display on LCD.
  */
static void set_display_number(uint16_t value)
{
  static const uint8_t digit_to_seven_seg[] = {
      0x3F, // Zero  (0).
      0x0C, // One   (1).
      0x5B, // Two   (2).
      0x5E, // Three (3).
      0x6C, // Four  (4).
      0x76, // Five  (5).
      0x77, // Six   (6).
      0x1C, // Seven (7).
      0x7F, // Eight (8).
      0x7E  // Nine  (9).
  };

  static const uint8_t digit_to_off_or_one[] = {
      0x00,  // Zero (0).
      0x05,  // One  (1).
      0x04   // Error (3B Top right vertical).
  };

  uint8_t digit[3];

  digit[0] = value % 10;
  value /= 10;
  digit[1] = value % 10;
  digit[2] = value / 10;

  // Third digit can only display one(1).
  if (1 < digit[2])
  {
    digit[2] = 2;  // Digit error.
  }

  // Drive common to ground.
  P4OUT = 0x00;

  // Drive desired segments to high.
  P2OUT = digit_to_off_or_one[digit[2]];
  P6OUT = digit_to_seven_seg[digit[1]];
  P3OUT = digit_to_seven_seg[digit[0]];
}

/**
  @brief Copy source uint16_t array into destination for len.
  @param  dst    The destination array to copy into.
  @param  src    The source array to copy from.
  @param  index  The starting index of the source to start copying from.
  @param  len    The number of elements to copy.
  @note This will unroll the source array so that the index will correspond with dst[0].
  */
static void unroll_array(uint16_t* dst, uint16_t* src, uint16_t index, uint16_t len)
{
  uint16_t count = len;

  while (count)
  {
    *dst = src[index];
    dst++;
    index++;
    count--;

    if (index == len)
    {
      index = 0;
    }
  }
}

/**
  @brief Timer A2 interrupt service routine to refresh the LCD display.
  @note The LCD is alternatively biased by driving common to ground and
        the desired pins to high or driving common to high and the desired
        pins to low.
  */
#pragma vector=TIMER2_A0_VECTOR
__interrupt void refresh_display(void)
{
  P4OUT ^= 0x01;
  P2OUT ^= 0xFF;
  P6OUT ^= 0xFF;
  P3OUT ^= 0xFF;
}

/**
  @brief Timer A1 interrupt service routine to start the ADC conversion.
  */
#pragma vector=TIMER1_A0_VECTOR
__interrupt void start_adc_conversion(void)
{
#if TEST_SAMPLE == 0
  if (kStateSnapshotSample != state)
  {
    ADC12CTL0 |= ADC12SC; // Start conversion.
  }
#endif
}

/**
  @brief Timer A0 interrupt service routine to start the processor.
  */
#pragma vector=TIMER0_A0_VECTOR
__interrupt void start_qrs_detector(void)
{
  if (kStateIdle == state)
  {
    state = kStateSnapshotSample;
  }

  // Clear low power mode to wake up CPU.
  __bic_SR_register_on_exit(LOW_POWER_MODE);
}

/**
  @brief ADC interrupt routine to store sampled ADC value into an array.
  */
#pragma vector=ADC12_VECTOR
__interrupt void store_adc_value(void)
{
#if TEST_SAMPLE == 0
  if (kStateSnapshotSample != state)
  {
    sample_array_pointer[sample_index] = ADC12MEM0;
    ++sample_index;

    if (SAMPLE_LEN <= sample_index)
    {
      sample_index = 0;
    }
  }
#endif
}

int main(void)
{
  uint16_t array_a[SAMPLE_LEN];
  uint16_t array_b[SAMPLE_LEN];
  uint16_t heartrate;

#if TEST_SAMPLE == 1
  uint16_t sample_array[SAMPLE_LEN] = {
    865, 904, 904, 888, 902, 914, 919, 940, 968, 945, 926, 894, 915, 954, 839, 989,
    972, 904, 915, 881, 943, 944, 932, 921, 931, 930, 870, 814, 915, 905, 843, 912,
    895, 883, 935, 965, 943, 924, 883, 820, 936, 931, 934, 915, 910, 900, 854, 900,
    779, 939, 931, 934, 911, 952, 966, 895, 951, 908, 944, 935, 897, 936, 881, 905,
    927, 923, 871, 918, 926, 935, 911, 892, 872, 905, 907, 902, 887, 919, 920, 931,
    903, 817, 945, 894, 872, 866, 937, 857, 878, 886, 913, 905, 919, 828, 958, 936,
    925, 918, 920, 912, 915, 889, 897, 913, 920, 933, 868, 899, 899, 891, 929, 887,
    920, 931, 921, 937, 925, 908, 905, 904, 931, 840, 876, 935, 873, 888, 880, 878,
    899, 904, 886, 903, 883, 846, 900, 903, 845, 873, 855, 900, 881, 923, 936, 870,
    939, 914, 919, 864, 919, 907, 883, 899, 832, 885, 881, 801, 931, 891, 857, 860,
    866, 912, 848, 925, 832, 902, 871, 936, 887, 875, 875, 880, 880, 899, 924, 988,
    999, 1113, 1110, 1097, 1055, 884, 948, 852, 897, 907, 835, 873, 770, 893, 908, 914,
    924, 901, 894, 913, 937, 912, 887, 929, 903, 942, 897, 904, 925, 883, 923, 890,
    907, 915, 936, 908, 929, 908, 929, 864, 895, 904, 918, 872, 915, 867, 939, 900,
    920, 932, 932, 899, 876, 895, 963, 871, 958, 867, 947, 919, 944, 904, 916, 947,
    952, 945, 919, 957, 905, 881, 841, 931, 913, 950, 908, 921, 892, 943, 925, 921,
    943, 825, 908, 929, 889, 906, 855, 879, 884, 892, 833, 907, 784, 921, 937, 903,
    880, 913, 905, 1012, 887, 876, 911, 944, 900, 883, 902, 926, 912, 880, 930, 929,
    891, 913, 939, 875, 918, 916, 929, 938, 841, 928, 916, 933, 895, 816, 811, 891,
    902, 935, 927, 891, 876, 921, 902, 873, 918, 915, 908, 817, 914, 919, 899, 912,
    851, 913, 913, 864, 898, 932, 909, 894, 774, 876, 851, 867, 899, 908, 895, 743,
    882, 912, 890, 900, 830, 792, 896, 892, 873, 918, 867, 883, 896, 900, 898, 884,
    929, 912, 897, 863, 864, 899, 913, 913, 836, 936, 860, 914, 844, 856, 872, 905,
    883, 907, 859, 857, 846, 894, 1003, 798, 797, 911, 856, 881, 887, 921, 967, 982,
    1049, 1027, 1078, 991, 972, 936, 913, 870, 804, 855, 827, 836, 856, 900, 851, 841,
    902, 839, 897, 788, 886, 912, 807, 902, 838, 868, 872, 894, 892, 881, 873, 907,
    897, 931, 891, 923, 916, 897, 884, 921, 928, 915, 892, 878, 896, 884, 940, 937,
    873, 910, 936, 878, 955, 1014, 957, 897, 924, 928, 850, 927, 937, 927, 896, 902,
    823, 912, 950, 916, 886, 926, 919, 873, 884, 863, 794, 927, 873, 887, 884, 916,
    845, 896, 888, 896, 825, 856, 862, 896, 868, 865, 846, 887, 912, 851, 918, 887,
    875, 895, 823, 944, 859, 929, 919, 848, 914, 794, 884, 828, 857, 923, 888, 936,
    915, 878, 881, 902, 877, 887, 883, 835, 862, 910, 818, 913, 887, 854, 914, 870,
    851, 837, 904, 846, 876, 917, 888, 871, 802, 907, 876, 886, 875, 921, 855, 908,
    866, 870, 907, 880, 917, 862, 871, 873, 889, 920, 795, 910, 886, 867, 867, 856,
    892, 846, 906, 916, 827, 897, 891, 884, 928, 890, 897, 908, 888, 856, 862, 888,
    908, 827, 885, 879, 835, 817, 801, 798, 832, 834, 855, 896, 875, 891, 910, 835,
    865, 829, 931, 855, 857, 892, 919, 925, 1025, 1040, 1073, 1040, 995, 928, 887, 887,
    872, 738, 807, 868, 885, 912, 875, 891, 893, 855, 855, 875, 825, 910, 848, 913,
    879, 865, 869, 883, 899, 926, 894, 926, 947, 896, 867, 903, 967, 867, 912, 935,
    899, 931, 926, 920, 904, 883, 861, 883, 908, 912, 931, 959, 909, 940, 895, 936,
    880, 943, 955, 823, 902, 923, 961, 955, 863, 867, 889, 915, 900, 875, 852, 905,
    868, 899, 844, 889, 936, 910, 885, 906, 877, 895, 908, 882, 879, 891, 929, 921,
    902, 878, 877, 873, 936, 888, 862, 868, 902, 878, 892, 960, 855, 903, 931, 907,
    895, 878, 872, 939, 878, 892, 854, 913, 913, 883, 924, 899, 924, 878, 878, 875,
    875, 865, 918, 899, 851, 881, 937, 919, 884, 907, 878, 915, 900, 840, 891, 897,
    878, 914, 844, 903, 833, 914, 879, 852, 892, 900, 910, 936, 860, 910, 904, 925,
    732, 914, 829, 930, 881, 862, 884, 915, 871, 904, 900, 871, 876, 924, 920, 874,
    879, 894, 892, 916, 832, 878, 897, 918, 901, 831, 891, 924, 932, 841, 883, 904,
    822, 895, 844, 861, 873, 878, 817, 823, 815, 879, 893, 953, 988, 974, 1081, 1046,
    1020, 940, 868, 902, 836, 867, 796, 791, 876, 836, 891, 844, 876, 898, 884, 896,
    835, 753, 892, 896, 841, 864, 749, 913, 916, 878, 829, 843, 892, 882, 951, 834,
    825, 852, 900, 910, 816, 887, 930, 844, 905, 758, 903, 932, 928, 874, 873, 910,
    859, 924, 913, 918, 939, 937, 928, 915, 914, 904, 937, 963, 817, 861, 920, 910,
    836, 924, 830, 870, 905, 888, 814, 882, 744, 884, 753, 834, 871, 871, 868, 950,
    838, 857, 867, 879, 881, 878, 825, 760, 863, 795, 918, 836, 862, 852, 907, 855,
    887, 827, 860, 880, 924, 870, 839, 891, 838, 803, 900, 841, 889, 856, 747, 881,
    871, 872, 818, 860, 918, 822, 820, 824, 835, 868, 840, 873, 880, 887, 848, 849,
    873, 891, 852, 867, 856, 878, 854, 834, 868, 846, 880, 900, 870, 883, 866, 882,
    879, 867, 827, 860, 867, 876, 895, 839, 913, 862, 895, 932, 788, 915, 872, 865,
    902, 886, 896, 872, 848, 872, 886, 827, 891, 856, 878, 921, 880, 838, 844, 860,
    840, 873, 740, 878, 877, 894, 859, 980, 957, 1000, 1012, 1031, 1065, 1022, 908, 891,
    896, 804, 863, 743, 846, 859, 903, 816, 883, 891, 809, 888, 836, 900, 882, 839,
    907, 905, 926, 864, 823, 866, 910, 910, 887, 894, 869, 895, 1003, 905, 863, 897,
    918, 920, 843, 908, 872, 934, 866, 1020, 953, 859, 950, 925, 840, 904, 793, 921,
    940, 929, 937, 854, 939, 887, 904, 908, 915, 889, 828, 926, 937, 895, 860, 878,
    899, 926, 880, 888, 892, 846, 883, 822, 847, 865, 856, 899, 875, 884, 900, 884,
    889, 861, 864, 913, 946, 892, 847, 882, 913, 909, 887, 945, 912, 926, 843, 919,
    892, 884, 871, 861, 860, 879, 908, 882, 892, 913, 902, 876, 875, 929, 889, 889,
    850, 888, 914, 862, 895, 884, 860, 878, 897, 891, 891, 866, 888, 836, 899, 911,
    859, 868, 897, 819, 870, 972, 867, 823, 853, 876, 926, 883, 820, 893, 861, 932,
    888, 878, 891, 899, 911, 921, 913, 955, 952, 927, 899, 881, 889, 903, 896, 892,
    879, 920, 854, 867, 937, 861, 915, 883, 916, 784, 866, 900, 889, 901, 791, 852,
    916, 884, 900, 1016, 1016, 1079, 1105, 1076, 1046, 902, 955, 868, 873, 819, 907, 916,
    892, 907, 903, 867, 913, 894, 851, 917, 848, 863, 917, 884, 923, 809, 936, 934,
    924, 915, 873, 921, 914, 905, 900, 866, 971, 901, 910, 918, 1016, 963, 872, 948,
    912, 940, 859, 942, 951, 912, 911, 971, 966, 976, 942, 891, 940, 945, 953, 955,
    959, 943, 942, 936, 1004, 924, 892, 941, 923, 884, 865, 879, 943, 867, 915, 905,
    944, 886, 888, 932, 863, 883, 856, 859, 912, 903, 889, 936, 930, 919, 769, 857,
    876, 908
  };
#else
  uint16_t sample_array[SAMPLE_LEN];
#endif

#if ENABLE_LOGGING == 1
  uint16_t log_idx;
#endif

  // Stop watchdog timer.
  WDTCTL = WDTPW | WDTHOLD;

  state = kStateIdle;
  sample_index = 0;
  sample_array_pointer = &sample_array[0];
  heartrate = 0;

#if TEST_SAMPLE == 0
  memset(sample_array, 0, sizeof(sample_array));
#endif

  // Setting pins to low must be before any setup of other pins.
  set_pins_to_output_low();
  disable_usb();

  init_adc();
  init_detector_timer();
  init_display_driver();
  init_display_timer();
  init_sampler_timer();

  set_display_number(0);

  // Enable interrupts.
  __bis_SR_register(GIE);

  // Enter low power mode.
  __bis_SR_register(LOW_POWER_MODE);

  for(;;)
  {
    switch(state)
    {
      case kStateIdle:
      {
        break;
      }

      case kStateSnapshotSample:
      {
        unroll_array(array_a, sample_array, sample_index, SAMPLE_LEN);

#if ENABLE_LOGGING == 1
        printf("STEP ORIGINAL\n");
        for (log_idx = 0; log_idx < SAMPLE_LEN; log_idx++)
        {
          printf("%u\n", array_a[log_idx]);
        }
#endif

        state = kStateQrsDetect;
        break;
      }

      case kStateQrsDetect:
      {
        qrs_filter_high_pass(array_a, array_b, SAMPLE_LEN);

#if ENABLE_LOGGING == 1
        printf("STEP HIGH PASS\n");
        for (log_idx = 0; log_idx < SAMPLE_LEN; log_idx++)
        {
          printf("%u\n", array_b[log_idx]);
        }
#endif

        qrs_filter_low_pass(array_b, array_a, SAMPLE_LEN);

#if ENABLE_LOGGING == 1
        printf("STEP LOW PASS\n");
        for (log_idx = 0; log_idx < SAMPLE_LEN; log_idx++)
        {
          printf("%u\n", array_a[log_idx]);
        }
#endif

        heartrate = qrs_get_heartrate(array_a, array_b, SAMPLE_LEN);

#if ENABLE_LOGGING == 1
        printf("STEP QRS\n");
        for (log_idx = 0; log_idx < SAMPLE_LEN; log_idx++)
        {
          printf("%u\n", array_b[log_idx]);
        }
#endif

        state = kStateSetDisplay;
        break;
      }

      case kStateSetDisplay:
      {
        if (MAX_HEARTRATE < heartrate)
        {
          set_display_number(MAX_HEARTRATE);
        }
        else
        {
          set_display_number(heartrate);
        }

        state = kStateIdle;
        break;
      }

      default:
      {
        state = kStateIdle;
        break;
      }
    }

    // Enter low power mode.
    __bis_SR_register(LOW_POWER_MODE);
  }
}

