# Gerber verification — IAMC PS-7 Alcohol Detection System PCB v1.0

Generated 2026-11-22 by `make_gerber.py` (hand-written Python generator).
**These files were NOT made in KiCad and have NOT passed KiCad DRC.**
They passed the generator's own clearance + connectivity self-check instead
(see below). Treat this as a hand-built prototype layout: get it reviewed
before ordering, and check the ESP32 pin order against your actual DevKit
board before soldering.

## Board

- Size 80 x 60 mm, 2 layers (F.Cu / B.Cu), 4 x M3 mounting holes (3.2 mm drill)
  at (3,3), (77,3), (3,57), (77,57)
- Assumed stackup (not encoded in Gerber): 1.6 mm FR-4, 1 oz copper both sides
- Track widths: signals 0.25 mm, +5V 0.50 mm, GND 0.80 mm
- Vias: 0.80 mm pad / 0.40 mm drill (20 total)
- PTH pads: 1.80 mm pad / 1.00 mm drill (53 total: 38 ESP32 + 4 MQ-3 + 4 LCD
  + 3 relay + 2 buzzer + 2 button)
- SMD: 0805 pads 1.0 x 1.25 mm, 1206 (C4) pads 1.0 x 2.0 mm

## Files

| File | Contents |
|---|---|
| IAMC-PS7-F.Cu.gbr | Top copper |
| IAMC-PS7-B.Cu.gbr | Bottom copper |
| IAMC-PS7-F.Mask.gbr | Top solder mask (pad openings +0.1 mm) |
| IAMC-PS7-B.Mask.gbr | Bottom solder mask (PTH pad openings + via openings; vias untented) |
| IAMC-PS7-F.SilkS.gbr | Top silkscreen (dot-matrix text) |
| IAMC-PS7-B.SilkS.gbr | Bottom silkscreen (project name) |
| IAMC-PS7-Edge.Cuts.gbr | Board outline + M3 hole circles |
| IAMC-PS7.drl | Excellon drill, metric, 53x 1.0 mm / 20x 0.4 mm / 4x 3.2 mm |

All files are RS-274X, format 4.4, units mm.

## Self-check results (run by make_gerber.py, 2026-11-22, re-run after fixes)

- **Clearance: 0 violations** at a 0.20 mm minimum between different nets,
  on both copper layers (pads, tracks, vias, SMD rects all checked).
- **Connectivity: every routed net is a single island** — +5V, GND, BTN,
  BUZ, LED_G, LED_R, MQ3_AO, MQ3_AO_RAW, RELAY_IN, SCL, SDA.
- **Cross-net isolation: 0 violations** (new check added 2026-11-22 after
  external review): no copper island holds features of two different
  nets. Single-island connectivity alone cannot prove this — a short
  between two nets would still leave each net one island — so isolation
  is now checked separately. GPIO34 (MQ3_AO) vs GND explicitly isolated.
- **Aperture syntax fixed 2026-11-22:** the D19 silkscreen aperture was
  emitted as `%%ADD19R,0.1800X0.1800*%%` (double `%` delimiters); all
  aperture definitions are now single-`%` RS-274X extended commands and
  no `%%` remains in any Gerber file.
- **Header pin order fixed 2026-11-22 (real PCB error, found by Claude
  review):** the first revision's right header started with VIN, shifting
  every right-header pin by one — +5V landed on a GND pin, SDA on a GND
  pad, SCL on TX0, and the module's VIN pin was left unconnected. The
  left tail was also wrong (D15/D2/D0/D4 instead of D9/D10/D11/VIN). Both
  headers now match the Espressif DevKitC V4 tables (J1/J3), verified
  against the ESP-IDF guide and the espboards 38-pin clone pinout.
  +5V now enters at the left-header VIN pad (bottom of J1) and is routed
  east on F.Cu to (20,6), north on B.Cu at x=20 to y=54, then east on
  F.Cu to the distribution at (54,54). SDA is now at J2 position 6
  (y=39.02) and SCL at J2 position 3 (y=46.64); SCL runs north around
  the divider to the x=47.5 B.Cu vertical, and the LCD-GND route was
  shortened so it no longer crosses the SDA lane. Re-check after the
  fix: clearance 0 violations, isolation 0 violations, all nets single
  island, and an independent parse of the emitted files confirms
  SDA/SCL/+5V/GND on the correct pads.
- **B-side pads fixed 2026-11-22:** plated through-hole pads now exist
  on B.Cu and have B.Mask openings (first revision had them on F.Cu
  only). All 53 PTH pads confirmed present on both copper layers and
  in B.Mask by an independent parse of the emitted files.
- **Vias moved out of LED pads 2026-11-22:** the two GND vias that sat
  inside the LEDG-2 / LEDR-2 SMD pads are now at (56,28.86) and
  (56,26.32), on the main GND B.Cu vertical, with short F.Cu stubs
  from the pads. No via now sits inside any SMD pad (checked by
  independent parse).
- Unconnected ESP32 pins (present as pads, no track): 3V3, EN, VP36, VN39,
  D35, D12, D13, D14, D9, D10, D11, D5, D18, D19, D23, TX0, RX0, TX2,
  RX2, D6, D7, D8, and the duplicate right-row pins D0B/D2B/D4B/D15B,
  plus DO (MQ-3 digital out — firmware reads AO only).

## Net list vs firmware pin map (config.h / sketch.ino)

| Net | ESP32 pin | Destination |
|---|---|---|
| MQ3_AO_RAW | — | MQ-3 AO -> R1 (10k) |
| MQ3_AO | GPIO34 | R1/R2 divider node (R1 10k, R2 15k to GND) |
| BTN | GPIO32 | push-button to GND |
| RELAY_IN | GPIO33 | relay module IN |
| BUZ | GPIO25 | buzzer + (to GND) |
| LED_G | GPIO26 | 220R -> green LED -> GND |
| LED_R | GPIO27 | 220R -> red LED -> GND |
| SDA | GPIO21 | LCD SDA |
| SCL | GPIO22 | LCD SCL |
| +5V | VIN | MQ-3 VCC, LCD VCC, relay VCC, C1/C2/C3/C4 |
| GND | GND (x3: left pos 14, right pos 1 and 7) | all module GNDs, R2, LEDs, button, caps |

ESP32 header layout follows the Espressif DevKitC V4 pin order
(2x19, 2.54 mm pitch, rows 25.4 mm apart; J1 left: 3V3, EN, VP36, VN39,
D34, D35, D32, D33, D25, D26, D27, D14, D12, GND, D13, D9, D10, D11,
VIN; J3 right: GND, D23, D22, TX0, RX0, D21, GND, D19, D18, D5, TX2,
RX2, D4, D0, D2, D15, D8, D7, D6). **Verify this order against your
physical board before ordering** — DevKit clones vary, and the first
revision of these Gerbers had the right header shifted by one position
(see above).

## Known limits / deviations from hardware/PCB_GUIDE.md

1. **Not KiCad, not DRC-checked in any EDA tool.** The only checks are the
   generator's own clearance (0.20 mm) and connectivity (single-island)
   checks. A fab's own DRC may still flag something.
2. **No GND copper pour.** GND is routed as 0.80 mm tracks plus a bus
   along y=4. Functionally equivalent for this low-current board, but
   not a pour.
3. **No 1N4007 footprint.** The guide's diode is for a motor load; this
   layout has no motor load (the relay module is self-contained with its
   own driver), so the diode was omitted rather than placed unconnected.
4. **MQ-3 DO is unconnected** (firmware reads AO only).
5. **Silkscreen is dot-matrix text** (5x7 pixel font rendered as flashes),
   not vector font — readable, but chunky.
6. **Thresholds are untuned** (same limitation as the firmware:
   test_log.csv is empty, so AI_WARNING 2.0 / AI_DANGER 4.0 are
   uncalibrated defaults).
7. **Never fabricated or electrically tested.** No hardware run exists
   for this PCB.
8. **Assembly requirements (do not skip):** (a) fit a 10 kΩ pull-down
   resistor between the relay module IN pin and GND (or use an
   active-low relay module) — GPIO33 is high-impedance during
   reset/boot, so without it the relay input floats before the
   firmware drives it low; (b) do NOT connect a 5 V LCD I2C backpack
   directly to GPIO21/22 — use a bidirectional I2C level shifter, or
   a backpack whose pull-ups go to 3.3 V; many 1602 backpacks pull
   SDA/SCL up to their 5 V supply, which can damage the ESP32;
   (c) check the active buzzer's current draw against what GPIO25 can
   safely supply — if it needs more, drive it through a transistor /
   MOSFET instead of directly from the pin.
9. **External reviews 2026-11-22 (ChatGPT and Claude):** both reviewed
   the submitted files and their comments were used as prompts for
   further checks, not as validation. Each claimed GPIO34 was shorted
   to GND in the Gerbers; that claim was checked against the emitted
   files and NOT reproduced by either review's own re-check: the
   generator's clearance check (0 violations), the cross-net isolation
   check (0 violations), and an independent parse of the emitted
   F.Cu/B.Cu files all show GPIO34 and GND on separate copper islands,
   and no continuous track exists between them (the x=14 stubs for
   LED/buzzer/relay are disjoint segments with ~1 mm gaps, not one
   vertical). The Gerbers were also opened in the Tracespace online
   Gerber viewer on 22 Nov 2026 — all 8 Gerber/drill files loaded with
   no errors, the board renders as expected, and the viewer shows no
   copper connection between the GPIO34 pad and GND on either layer.
   Real defects the reviews found and that are fixed in this revision:
   the D19 aperture syntax error, the ESP32 header pin order (see
   above), missing B-side PTH pads and B.Mask openings, and vias
   inside the LED pads. The silkscreen "5V" label was moved from the
   right-header top pad (now GND) to the +5V entry at the left-header
   VIN pad. The Gerbers are still NOT KiCad-DRC-checked
   and have NOT been fabricated — do not describe them as
   fabrication-certified.
10. **Remaining Gerber limits (not fixed):** C4 pad is 0.5 mm from the
    top board edge; the ADC trace (GPIO34) runs about 130 mm around
    the board; the mounting holes share the plated drill file
    (no separate non-plated file); no Gerber X2 layer attributes and
    no G01 commands are emitted; the relay pull-down, I2C level
    shifter and 1N4007 have no footprint on the PCB (assembly
    requirements, item 8).

## Preview

`preview.png` is a top-view render of the same geometry (green solder
mask, orange F.Cu, blue B.Cu, gold pads). It was generated from the same
data as the Gerbers and inspected visually.
