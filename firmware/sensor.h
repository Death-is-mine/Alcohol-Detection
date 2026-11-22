// sensor.h - ADC read and MQ-3 maths
#pragma once
#include <Arduino.h>
#include "config.h"

// Average 16 ADC readings; returns millivolts at the ESP32 pin.
inline float readPinMv() {
  uint32_t sum = 0;
  for (int i = 0; i < 16; i++) sum += analogReadMilliVolts(PIN_MQ3);
  return sum / 16.0f;
}

// Undo the resistor divider to get the sensor module's output voltage.
inline float sensorMv(float pinMv) { return pinMv * DIVIDER_GAIN; }

// Sensor resistance from the module output voltage: Rs = RL * (Vcc - Vs) / Vs
// Vs is clamped away from 0 and Vcc so we never divide by zero.
inline float calcRs(float vsMv) {
  if (vsMv < 20.0f) vsMv = 20.0f;
  if (vsMv > VCC_SENSOR_MV - 20.0f) vsMv = VCC_SENSOR_MV - 20.0f;
  return RL_OHM * (VCC_SENSOR_MV - vsMv) / vsMv;
}

// Alcohol index AI = R0 / Rs. About 1.0 in clean air, rises with alcohol (Rs falls).
inline float calcAI(float r0, float rs) { return (r0 > 0.0f && rs > 0.0f) ? r0 / rs : 0.0f; }
