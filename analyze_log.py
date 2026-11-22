#!/usr/bin/env python3
"""Compute the PRD 1.8 success metrics from data/test_log.csv.
Fill one row per trial. expected_state/result_state in {SAFE, WARNING, DANGER}.
Usage: python3 analyze_log.py data/test_log.csv"""
import csv, sys, statistics as st
from collections import defaultdict

rows = list(csv.DictReader(open(sys.argv[1] if len(sys.argv) > 1 else "data/test_log.csv")))
rows = [r for r in rows if r["expected_state"].strip() and r["result_state"].strip()]
if not rows:
    sys.exit("No completed rows yet.")

ok = sum(r["expected_state"].strip().upper() == r["result_state"].strip().upper() for r in rows)
print(f"Trials: {len(rows)}   Classification accuracy: {ok/len(rows):.0%}  (target >= 90% over >= 30 trials)")

clean = [r for r in rows if r["expected_state"].strip().upper() == "SAFE"]
if clean:
    fa = sum(r["result_state"].strip().upper() != "SAFE" for r in clean)
    print(f"Clean-air trials: {len(clean)}   false alarms: {fa}  (target <= 1 in 20)")

groups = defaultdict(list)
for r in rows:
    try:
        groups[(r["source"], r["distance_cm"])].append(float(r["peak_ai"]))
    except ValueError:
        pass
print("Repeatability (spread of peak AI per source+distance, target within +/-15% of mean):")
for k, v in groups.items():
    if len(v) >= 3:
        m = st.mean(v)
        dev = max(abs(x - m) for x in v) / m if m else float("nan")
        print(f"  {k[0]} @ {k[1]} cm: n={len(v)} mean={m:.2f} max deviation={dev:.0%}")
