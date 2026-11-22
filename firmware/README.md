# Alcohol Detection System (IAMC Problem Statement 7)

ESP32 + MQ-3 breath-alcohol **screening** prototype with an ignition-interlock relay.
The output is an indicative alcohol index (AI = R0/Rs), **not** a legal blood-alcohol (BAC) reading.
An MQ-3 reacts to other vapours and drifts with temperature and humidity.

## Run it in Wokwi (points 4 and 5)
1. wokwi.com > New Project > **ESP32**. Open the `diagram.json` tab and paste `diagram.json` from this repo.
2. Create files `config.h` and `sensor.h` (new file button) and paste them. Replace `sketch.ino` with the one here.
3. Add the library in the Library Manager: **LiquidCrystal I2C** (same as `libraries.txt`).
4. Check the wiring on screen against the pin table in `config.h`. If a wire shows a pin-name error, re-drag that wire in the editor.
5. Start the simulation. **Set the slider to about 10% from its left end** (clean air). The Serial Monitor `raw_mV` column should read roughly 300. If the slider is at the very end (0 V) the firmware correctly reports a sensor fault.

| Demo step | Slide the potentiometer | Expected |
|---|---|---|
| Warm-up, then calibration (about 20 s) | Keep at about 10% (raw_mV near 300) | Green blinks, LCD shows countdown, then READY |
| SAFE test | Press the button, keep slider at about 10% | SAFE, blue Engine LED ON for 60 s |
| WARNING test | Press the button, move slider to about 22% within the 5 s window | WARNING, Engine LED off, red blinks |
| DANGER test | Press the button, move slider to 35% to 60% | ALCOHOL FOUND, continuous buzzer |
| FAULT test | Move slider to the very end (above 95%) or to 0 for over 1 s | SENSOR ERROR E1 |
| Recalibrate | Hold the button for 3 s in READY | Calibrating again |

Slider positions are approximate. With a clean-air level near 9% of travel (300 mV), AI crosses 2.0 near 17% and 4.0 near 29%. Watch the Serial Monitor `ai` column and tune `AI_WARNING` / `AI_DANGER` in `config.h`.

Screenshots for point 4: circuit, code, LCD plus Serial Monitor in SAFE, WARNING, DANGER, and the FAULT case.

## Real hardware
Set `SIMULATION 0` in `config.h`. Wire MQ-3 AO through a 10 k (top) / 15 k (bottom) divider to GPIO34 and power the sensor from 5 V. Let a new sensor burn in (datasheet suggests about 24 h), and use a safe vapour source (hand sanitiser or an ethanol-dabbed swab near the sensor). Nobody should drink for a test. Check whether your relay module is active-low (`RELAY_ACTIVE_HIGH`).

## Serial log
`ms,raw_mV,rs_ohm,ai,state` at 115200 baud. Copy trials into `data/test_log.csv`, then run `python3 analyze_log.py data/test_log.csv` for the PRD success metrics.

## What was and was not verified
- The firmware logic was compiled with g++ against mock Arduino libraries (both `SIMULATION` 1 and 0) and passed scripted tests for boot lock, warm-up, calibration, SAFE, WARNING, DANGER, cool-downs, hysteresis, recalibration and FAULT. See `tests/`.
- It has **not** been run in the Wokwi editor or on real ESP32 hardware from here. Expect to fix small things (a wire name, library version) on first load.
- Thresholds 2.0 and 4.0 are starting values, not calibrated to real alcohol concentrations.

## Layout
```
sketch.ino  config.h  sensor.h  diagram.json  libraries.txt
analyze_log.py  data/test_log.csv  hardware/PCB_GUIDE.md  tests/
```
Still to do by you: the KiCad PCB and Gerber (`hardware/PCB_GUIDE.md`), the demo video, the GitHub upload and the PDF.
