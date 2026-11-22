# IAMC Problem Statement 7 — Alcohol Detection System

ESP32 + MQ-3 alcohol detection system with an I2C LCD, LEDs, buzzer and a
fail-safe relay. Built for the IAMC evaluation; simulation-verified in
Wokwi. **Not yet tested on real hardware.**


![Demo video](https://github.com/user-attachments/assets/b64c7515-185b-489f-a044-7a369ffbc8ef)
Demo video (108 s) — press play to watch the system run through SAFE, WARNING, DANGER and FAULT.



- **Live simulation:** https://wokwi.com/projects/481328204846969857
- **Full report (points 1–5):** [`docs/IAMC-PS7-Alcohol-Detection-System.pdf`](docs/IAMC-PS7-Alcohol-Detection-System.pdf)
- **Demo video (90 s):** [`video/IAMC-PS7-Alcohol-Detection-Demo.mp4`](video/IAMC-PS7-Alcohol-Detection-Demo.mp4)

## How it works

The MQ-3 sensor output goes through a 10k/15k divider into GPIO34. The
firmware calibrates a clean-air baseline R0, then reports an alcohol index
AI = R0/Rs (indicative only — **not** a legal BAC reading):

| State | AI | Indication |
|---|---|---|
| SAFE | < 2.0 | Green LED, relay released |
| WARNING | 2.0 – 4.0 | Red LED + buzzer |
| DANGER | ≥ 4.0 | Red LED + buzzer, relay locked |
| FAULT | sensor out of range | Latching E1, relay locked |

State machine: WARMUP → CALIBRATING → READY → SAMPLING → SAFE / WARNING /
DANGER / FAULT. The relay is **locked by default** and is released only in
SAFE, through a single `changeState` gate.

## Wokwi-verified results (measured, 22 Nov 2026)

| State | Pot slider | Sensor mV | Peak AI |
|---|---|---|---|
| SAFE | 10 | 176 | 1.00 |
| WARNING | 65 | 361 | 2.19 |
| DANGER | 145 | 630 | 4.20 |
| FAULT E1 | 1023 | 3171 | — |

Calibration R0 = 178,208 Ω (serial `# R0=178208`); 177,500 Ω is the clean-air Rs at 176 mV, not R0. Screenshots of every state are in
[`screenshots/`](screenshots/). The buzzer tone was **not** verified by ear
(browser audio limit). `test_log.csv` is empty (header only), so the
thresholds are uncalibrated defaults.

## Repo layout

- `firmware/` — `sketch.ino`, `config.h`, `sensor.h` (Arduino/ESP32)
- `wokwi/` — `diagram.json`, `libraries.txt` (open in Wokwi to simulate)
- `pcb/` — `gerber.zip` (RS-274X Gerbers + drill), `VERIFICATION.md`,
  `PCB_GUIDE.md`
- `docs/` — full PDF report
- `video/` — demo video
- `screenshots/` — Wokwi state captures
- `tests/test_main.cpp` — host unit tests (g++, all passed)
- `analyze_log.py`, `test_log.csv` — log analysis (log currently empty)

## PCB

80×60 mm, 2 layers, hand-built Gerbers (Python generator, **not KiCad**).
Self-checked: 0 clearance violations at 0.20 mm, every routed net a single
island. See [`pcb/VERIFICATION.md`](pcb/VERIFICATION.md) for the honest
limits — including that it has never been fabricated or tested, has no GND
pour, and the ESP32 header follows the Espressif DevKitC V4 pin order
(J1/J3, verified against the ESP-IDF guide).

## Pins

MQ-3 GPIO34 · button GPIO32 · green LED 26 · red LED 27 · buzzer 25 ·
relay 33 · I2C SDA 21 / SCL 22.
