/*
  Alcohol Detection System - IAMC Problem Statement 7
  ESP32 + MQ-3 (or potentiometer in Wokwi) + I2C LCD + LEDs + buzzer + relay interlock.

  Output is an INDICATIVE alcohol index (AI = R0/Rs), NOT a legal blood-alcohol reading.
  Relay is default-LOCKED and is released only in RESULT_SAFE (fail-safe, ARD ADR-5).
*/
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Preferences.h>
#include <esp_task_wdt.h>
#include "config.h"
#include "sensor.h"

enum State { ST_WARMUP, ST_CALIBRATING, ST_READY, ST_SAMPLING,
             ST_RESULT_SAFE, ST_RESULT_WARNING, ST_RESULT_DANGER, ST_FAULT };
enum Level { LV_SAFE = 0, LV_WARNING = 1, LV_DANGER = 2 };

LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);
Preferences prefs;

State    state = ST_WARMUP;
uint32_t stateSince = 0;
float    r0 = 0.0f;                 // clean-air baseline resistance
float    lastAI = 0.0f, peakAI = 0.0f;
Level    liveLevel = LV_SAFE, heldLevel = LV_SAFE;
uint32_t lastTick = 0, lastLcd = 0, lastLog = 0;
int      faultCode = 0;

// calibration accumulators
double   calSum = 0, calSumSq = 0;
uint32_t calN = 0;
int      calTries = 0;

// fault debounce
bool     badActive = false;
uint32_t badSince = 0;

// button
bool     btnPrev = false;
uint32_t btnDownAt = 0;
uint32_t btnLastChange = 0;

const char* stateName(State s) {
  switch (s) {
    case ST_WARMUP:         return "WARMUP";
    case ST_CALIBRATING:    return "CALIBRATING";
    case ST_READY:          return "READY";
    case ST_SAMPLING:       return "SAMPLING";
    case ST_RESULT_SAFE:    return "SAFE";
    case ST_RESULT_WARNING: return "WARNING";
    case ST_RESULT_DANGER:  return "DANGER";
    default:                return "FAULT";
  }
}

// ---------- outputs ----------
void setRelay(bool unlocked) {
  bool level = RELAY_ACTIVE_HIGH ? unlocked : !unlocked;
  digitalWrite(PIN_RELAY, level ? HIGH : LOW);
}

void changeState(State s, uint32_t now) {
  state = s;
  stateSince = now;
  setRelay(s == ST_RESULT_SAFE);          // single place that can unlock the vehicle
  Serial.print("# state -> ");
  Serial.println(stateName(s));
}

void enterFault(int code, uint32_t now) {
  faultCode = code;
  changeState(ST_FAULT, now);
}

void startCalibration(uint32_t now) {
  calSum = 0; calSumSq = 0; calN = 0;
  changeState(ST_CALIBRATING, now);
}

void lcdPrint(uint8_t row, const char* text) {
  char buf[17];
  snprintf(buf, sizeof(buf), "%-16s", text);   // pad to 16 chars, avoids lcd.clear() flicker
  lcd.setCursor(0, row);
  lcd.print(buf);
}

void drawLcd(uint32_t now) {
  char l1[24], l2[24];
  switch (state) {
    case ST_WARMUP: {
      uint32_t left = (WARMUP_MS - (now - stateSince)) / 1000UL + 1;
      snprintf(l1, sizeof l1, "Warming up...");
      snprintf(l2, sizeof l2, "%lu s left", (unsigned long)left);
      break; }
    case ST_CALIBRATING:
      snprintf(l1, sizeof l1, "Calibrating");
      snprintf(l2, sizeof l2, "Keep air clean");
      break;
    case ST_READY:
      snprintf(l1, sizeof l1, "Ready");
      snprintf(l2, sizeof l2, "Press to test");
      break;
    case ST_SAMPLING:
      snprintf(l1, sizeof l1, "BLOW NOW");
      snprintf(l2, sizeof l2, "AI: %.1f", lastAI);
      break;
    case ST_RESULT_SAFE:
      snprintf(l1, sizeof l1, "SAFE   pk %.1f", peakAI);
      snprintf(l2, sizeof l2, "Engine ON");
      break;
    case ST_RESULT_WARNING:
      snprintf(l1, sizeof l1, "WARNING pk %.1f", peakAI);
      snprintf(l2, sizeof l2, "Engine LOCKED");
      break;
    case ST_RESULT_DANGER:
      snprintf(l1, sizeof l1, "ALCOHOL FOUND");
      snprintf(l2, sizeof l2, "LOCKED pk %.1f", peakAI);
      break;
    default:
      snprintf(l1, sizeof l1, "SENSOR ERROR");
      snprintf(l2, sizeof l2, "Code E%d", faultCode);
      break;
  }
  lcdPrint(0, l1);
  lcdPrint(1, l2);
}

void updateIndicators(uint32_t now) {
  bool green = false, red = false, buzz = false;
  uint32_t t = now - stateSince;
  switch (state) {
    case ST_WARMUP:         green = (now / 500) % 2; break;               // slow blink
    case ST_CALIBRATING:
    case ST_SAMPLING:       green = (now / 125) % 2; break;               // fast blink
    case ST_READY:          green = true; break;
    case ST_RESULT_SAFE:    green = true; buzz = (t < 150); break;        // one short beep
    case ST_RESULT_WARNING: red = (now / 500) % 2; buzz = (t % 2000) < 100; break;
    case ST_RESULT_DANGER:  red = true; buzz = true; break;               // continuous
    case ST_FAULT: {
      red = (now / 100) % 2;
      uint32_t p = now % 2000;                                            // 3 beeps, repeating
      buzz = (p < 100) || (p >= 200 && p < 300) || (p >= 400 && p < 500);
      break; }
  }
  digitalWrite(PIN_LED_GREEN, green);
  digitalWrite(PIN_LED_RED, red);
  digitalWrite(PIN_BUZZER, buzz);
}

// ---------- decision logic ----------
Level classify(float ai, Level prev) {
  Level l = LV_SAFE;
  if (ai >= AI_DANGER) l = LV_DANGER;
  else if (ai >= AI_WARNING) l = LV_WARNING;
  // hysteresis: do not drop a level until AI falls below 80 % of that level's threshold
  if (prev == LV_DANGER && ai >= AI_DANGER * HYSTERESIS && l < LV_DANGER) l = LV_DANGER;
  if (prev >= LV_WARNING && ai >= AI_WARNING * HYSTERESIS && l < LV_WARNING) l = LV_WARNING;
  return l;
}

void startSampling(uint32_t now) {
  peakAI = 0; liveLevel = LV_SAFE; heldLevel = LV_SAFE;
  changeState(ST_SAMPLING, now);
}

void finishSampling(uint32_t now) {
  Serial.print("# result peakAI=");
  Serial.println(peakAI, 2);
  if (heldLevel == LV_DANGER)       changeState(ST_RESULT_DANGER, now);
  else if (heldLevel == LV_WARNING) changeState(ST_RESULT_WARNING, now);
  else                              changeState(ST_RESULT_SAFE, now);
}

void logLine(uint32_t now, float pinMv, float rs, float ai) {
  Serial.print(now);            Serial.print(',');
  Serial.print(pinMv, 0);       Serial.print(',');
  Serial.print(rs, 0);          Serial.print(',');
  Serial.print(ai, 2);          Serial.print(',');
  Serial.println(stateName(state));
}

void tick(uint32_t now) {
  float pinMv = readPinMv();
  float vs = sensorMv(pinMv);
  float rs = calcRs(vs);
  float ai = calcAI(r0, rs);
  lastAI = ai;

  // sensor fault: pin stuck at 0 or at the rail for FAULT_MS (ignored during warm-up)
  if (state != ST_WARMUP && state != ST_FAULT) {
    bool bad = (pinMv < FAULT_LOW_MV) || (pinMv > FAULT_HIGH_MV);
    if (bad) {
      if (!badActive) { badActive = true; badSince = now; }
      else if (now - badSince >= FAULT_MS) { enterFault(1, now); badActive = false; return; }
    } else {
      badActive = false;
    }
  }

  if (state == ST_SAMPLING || now - lastLog >= LOG_PERIOD_MS) {
    lastLog = now;
    logLine(now, pinMv, rs, ai);
  }

  switch (state) {
    case ST_WARMUP:
      if (now - stateSince >= WARMUP_MS) startCalibration(now);
      break;

    case ST_CALIBRATING: {
      calSum += vs; calSumSq += (double)vs * vs; calN++;
      if (now - stateSince >= CAL_MS) {
        double mean = calSum / calN;
        double var = calSumSq / calN - mean * mean;
        double sd = var > 0 ? sqrt(var) : 0;
        bool ok = (sd / mean <= CAL_MAX_SPREAD) && mean >= CAL_MIN_MV && mean <= CAL_MAX_FRAC * VCC_SENSOR_MV;
        if (ok) {
          r0 = calcRs((float)mean);
          float last = prefs.getFloat("r0", 0.0f);
          Serial.print("# R0="); Serial.print(r0, 0);
          if (last > 0) { Serial.print(" (last boot "); Serial.print(last, 0); Serial.print(")"); }
          Serial.println();
          prefs.putFloat("r0", r0);
          calTries = 0;
          changeState(ST_READY, now);
        } else {
          calTries++;
          Serial.println("# calibration rejected (unstable or out of range)");
          if (calTries >= CAL_TRIES) enterFault(2, now);
          else startCalibration(now);
        }
      }
      break; }

    case ST_SAMPLING:
      if (ai > peakAI) peakAI = ai;
      liveLevel = classify(ai, liveLevel);
      if (liveLevel > heldLevel) heldLevel = liveLevel;
      if (now - stateSince >= BREATH_WINDOW_MS) finishSampling(now);
      break;

    case ST_RESULT_SAFE:
      if (now - stateSince >= SAFE_UNLOCK_MS) changeState(ST_READY, now);          // re-lock
      break;
    case ST_RESULT_WARNING:
      if (now - stateSince >= COOLDOWN_WARNING_MS) changeState(ST_READY, now);
      break;
    case ST_RESULT_DANGER:
      if (now - stateSince >= COOLDOWN_DANGER_MS) changeState(ST_READY, now);
      break;
    default: break;                                                              // READY, FAULT
  }
}

// short press in READY = start test, long press in READY = recalibrate
void handleButton(uint32_t now) {
  bool down = (digitalRead(PIN_BTN) == LOW);
  if (down != btnPrev && now - btnLastChange >= DEBOUNCE_MS) {
    btnLastChange = now;
    btnPrev = down;
    if (down) {
      btnDownAt = now;
    } else if (state == ST_READY) {
      if (now - btnDownAt >= LONGPRESS_MS) { calTries = 0; startCalibration(now); }
      else startSampling(now);
    }
  }
}

void startWatchdog() {
#if USE_WATCHDOG
  #if ESP_ARDUINO_VERSION_MAJOR >= 3
    esp_task_wdt_config_t cfg = { 5000, 0, true };   // timeout_ms, idle_core_mask, trigger_panic
    esp_task_wdt_reconfigure(&cfg);
  #else
    esp_task_wdt_init(5, true);
  #endif
  esp_task_wdt_add(NULL);
#endif
}

void setup() {
  // relay first: boot must always be LOCKED
  pinMode(PIN_RELAY, OUTPUT);
  setRelay(false);
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_BTN, INPUT_PULLUP);

  Serial.begin(115200);
  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();
  analogSetPinAttenuation(PIN_MQ3, ADC_11db);
  prefs.begin("ps7", false);

  Serial.println("# Alcohol Detection System - IAMC PS-7 (indicative index, not legal BAC)");
  Serial.println("ms,raw_mV,rs_ohm,ai,state");
  startWatchdog();
  changeState(ST_WARMUP, millis());
}

void loop() {
  esp_task_wdt_reset();
  uint32_t now = millis();
  handleButton(now);
  if (now - lastTick >= SAMPLE_PERIOD_MS) { lastTick = now; tick(now); }
  updateIndicators(now);
  if (now - lastLcd >= 250) { lastLcd = now; drawLcd(now); }
}
