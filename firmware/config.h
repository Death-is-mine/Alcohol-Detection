// config.h - all pins, thresholds and timings in one place (IAMC PS-7, Alcohol Detection System)
#pragma once

// 1 = Wokwi simulation (slide potentiometer on GPIO34, 3.3 V reference)
// 0 = real hardware (MQ-3 module AO -> 10k/15k divider -> GPIO34, sensor on 5 V)
#define SIMULATION 1

// ---------- Pins (ESP32 DevKit V1) ----------
#define PIN_MQ3        34   // ADC1_CH6, input only
#define PIN_BTN        32   // start test / long-press = recalibrate (INPUT_PULLUP)
#define PIN_LED_GREEN  26
#define PIN_LED_RED    27
#define PIN_BUZZER     25
#define PIN_RELAY      33
#define LCD_ADDR       0x27
#define RELAY_ACTIVE_HIGH 1 // set 0 if your relay module is active-low

// ---------- Electrical model ----------
#if SIMULATION
  #define VCC_SENSOR_MV  3300.0f   // pot is powered from 3V3
  #define DIVIDER_GAIN   1.0f      // no divider in simulation
  #define WARMUP_MS      10000UL   // shortened so the demo video stays short
#else
  #define VCC_SENSOR_MV  5000.0f   // MQ-3 module runs on 5 V
  #define DIVIDER_GAIN   1.6667f   // (10k + 15k) / 15k : undo the divider
  #define WARMUP_MS      60000UL   // minimum warm-up per TRD 2.4
#endif
#define RL_OHM           10000.0f  // module load resistor; only needed for absolute Rs in the log
                                   // (it cancels out of the alcohol index AI = R0/Rs)

// ---------- Calibration ----------
#define CAL_MS           10000UL
#define CAL_MAX_SPREAD   0.05f     // reject if std/mean of clean-air samples > 5 %
#define CAL_MIN_MV       50.0f
#define CAL_MAX_FRAC     0.60f     // clean-air output must be below 60 % of Vcc
#define CAL_TRIES        3

// ---------- Sampling and decision ----------
#define SAMPLE_PERIOD_MS 100UL     // 10 Hz
#define BREATH_WINDOW_MS 5000UL
#define AI_WARNING       2.0f      // starting values: TUNE with your own test log
#define AI_DANGER        4.0f
#define HYSTERESIS       0.8f      // leave a level only when AI < 80 % of its entry threshold

// ---------- Interlock timing ----------
#define SAFE_UNLOCK_MS        60000UL
#define COOLDOWN_WARNING_MS   30000UL
#define COOLDOWN_DANGER_MS    60000UL

// ---------- Fault detection (pin millivolts) ----------
#define FAULT_LOW_MV     10.0f
#define FAULT_HIGH_MV    3150.0f   // ESP32 ADC saturates near here at 11 dB attenuation
#define FAULT_MS         1000UL

// ---------- Misc ----------
#define DEBOUNCE_MS      30UL
#define LONGPRESS_MS     3000UL
#define LOG_PERIOD_MS    500UL     // idle log rate (every tick while SAMPLING)
#define USE_WATCHDOG     1
