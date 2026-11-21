#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-or-later
# Copyright (c) 2026 Linning

"""Group-delay measurement for Crestwright's phase module.

Two modes, matching the SPEC appendix:

    # create a mono impulse stimulus
    python measure_gd.py --make-impulse impulse.wav --sr 44100

    # measure wet vs dry (wet = plugin output on that impulse, COMP = 0)
    python measure_gd.py dry.wav wet_1x.wav --sr 44100

The second form writes wet_1x_gd.csv next to the wet file with columns
frequency (Hz), group delay (ms), group delay (samples), magnitude (dB).

For an impulse dry file the transfer function is taken directly. For
arbitrary program material it uses a Welch cross-spectrum estimate:
H = S(dry, wet) / S(dry, dry), then differentiates the unwrapped phase.
"""

import argparse
import csv
import os
import sys

import numpy as np
from scipy import signal
from scipy.io import wavfile


def make_impulse(path, sr, seconds=4.0):
    n = int(sr * seconds)
    x = np.zeros(n, dtype=np.float32)
    x[0] = 1.0
    wavfile.write(path, sr, x)
    print(f"wrote {path}: {n} samples, {sr} Hz, one-sample impulse")


def read_mono(path):
    sr, data = wavfile.read(path)
    if data.ndim > 1:
        data = data.mean(axis=1)
    if data.dtype.kind == "i":
        data = data.astype(np.float64) / np.iinfo(data.dtype).max
    else:
        data = data.astype(np.float64)
    return sr, data


def transfer_estimate(dry, wet, sr, is_impulse):
    if is_impulse:
        n = len(wet)
        freqs = np.fft.rfftfreq(n, 1.0 / sr)
        h = np.fft.rfft(wet, n)
        return 2.0 * np.pi * freqs / sr, h, freqs

    nper = max(256, min(8192, len(dry), len(wet)))
    f, sxy = signal.csd(dry, wet, fs=sr, nperseg=nper, noverlap=nper // 2)
    _, sxx = signal.welch(dry, fs=sr, nperseg=nper, noverlap=nper // 2)
    h = sxy / np.maximum(sxx, 1e-30)
    return 2.0 * np.pi * f / sr, h, f


def group_delay(w, h, sr):
    phase = np.unwrap(np.angle(h))
    gd = -np.gradient(phase, w)          # samples
    return gd


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("files", nargs="*", help="dry.wav wet.wav (or just wet.wav)")
    ap.add_argument("--sr", type=float, default=44100.0)
    ap.add_argument("--make-impulse", metavar="PATH")
    ap.add_argument("--max-hz", type=float, default=20000.0)
    args = ap.parse_args()

    if args.make_impulse:
        make_impulse(args.make_impulse, args.sr)
        return 0

    if len(args.files) == 1:
        dry_path = None
        wet_path = args.files[0]
    elif len(args.files) == 2:
        dry_path, wet_path = args.files
    else:
        ap.error("expected dry.wav wet.wav, or a single wet.wav")

    sr, wet = read_mono(wet_path)
    is_impulse = False

    if dry_path is None:
        dry = np.zeros_like(wet)
        dry[0] = 1.0
        is_impulse = True
    else:
        sr_dry, dry = read_mono(dry_path)
        if sr_dry != sr:
            ap.error(f"sample-rate mismatch: dry {sr_dry} Hz, wet {sr} Hz")
        is_impulse = len(dry) == len(wet) and np.sum(np.abs(dry[1:])) < 1e-9

    w, h, freqs = transfer_estimate(dry, wet, sr, is_impulse)

    gd = group_delay(w, h, sr)
    mag = 20.0 * np.log10(np.maximum(np.abs(h), 1e-30))

    out = os.path.splitext(wet_path)[0] + "_gd.csv"
    keep = (freqs > 0) & (freqs <= args.max_hz)
    with open(out, "w", newline="", encoding="utf-8") as fh:
        cw = csv.writer(fh)
        cw.writerow(["frequency_hz", "group_delay_ms", "group_delay_samples", "magnitude_db"])
        for f, g, m in zip(freqs[keep], gd[keep], mag[keep]):
            cw.writerow([f"{f:.4f}", f"{g * 1000.0 / sr:.6f}", f"{g:.4f}", f"{m:.6f}"])

    print(f"wrote {out}  ({np.count_nonzero(keep)} points)")
    for target in (20, 30, 50, 80, 100, 150, 200, 300, 500, 1000):
        if target > args.max_hz:
            continue
        idx = int(np.argmin(np.abs(freqs - target)))
        print(f"  {target:6d} Hz : {gd[idx] * 1000.0 / sr:8.3f} ms  ({mag[idx]:+.4f} dB)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
