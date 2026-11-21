// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (c) 2026 Linning

// Standalone verification of Source/PhaseNetwork.h.
//
//   cl /O2 /std:c++17 /EHsc /Fe:test_phase.exe test_phase.cpp
//
// Measures the actual shipped DSP code (not a Python model):
//   * impulse response and zero-latency check
//   * group delay vs the SPEC table at 44.1 kHz and 48 kHz
//   * magnitude flatness (allpass)
//   * 1x..4x depth scaling
//   * click-free depth switching
//   * NaN/Inf recovery
//   * CPU cost at 48 kHz stereo

#include "../Source/PhaseNetwork.h"

#include <chrono>
#include <cmath>
#include <complex>
#include <cstdio>
#include <random>
#include <vector>

static constexpr double kPi = 3.14159265358979323846;

struct Anchor { double hz, ms; };

static const Anchor kSpec[] =
{
    { 20.0, 18.601 }, { 30.0, 17.668 }, { 50.0, 13.618 }, { 80.0, 9.348 },
    { 100.0, 7.918 }, { 150.0, 5.946 }, { 200.0, 4.750 }, { 300.0, 3.322 },
    { 500.0, 1.967 }, { 1000.0, 0.0 }, { 2000.0, 0.0 }, { 4000.0, 0.0 },
    { 8000.0, 0.0 }, { 16000.0, 0.0 }
};

static std::vector<double> impulseResponse (double fs, int depth, int n = 1 << 17)
{
    PhaseNetwork net;
    net.prepare (fs);
    net.setDepth (depth);
    net.reset();                 // snap the depth weights, no fade in the IR

    std::vector<double> h ((size_t) n, 0.0);
    for (int i = 0; i < n; ++i)
    {
        float l = i == 0 ? 1.0f : 0.0f;
        float r = 0.0f;
        net.process (l, r);
        h[(size_t) i] = (double) l;
    }
    return h;
}

static std::complex<double> hAt (const std::vector<double>& h, double w)
{
    std::complex<double> acc (0.0, 0.0);
    const std::complex<double> step = std::polar (1.0, -w);
    std::complex<double> rot (1.0, 0.0);
    for (double x : h)
    {
        acc += x * rot;
        rot *= step;
    }
    return acc;
}

// group delay in samples via central difference of the unwrapped phase
static double groupDelay (const std::vector<double>& h, double fs, double hz)
{
    const double dw = 2.0 * kPi * 0.25 / fs;
    const double w = 2.0 * kPi * hz / fs;
    const std::complex<double> a = hAt (h, w + dw);
    const std::complex<double> b = hAt (h, w - dw);
    return -std::arg (a / b) / (2.0 * dw);
}

static double magDb (const std::vector<double>& h, double fs, double hz)
{
    const double w = 2.0 * kPi * hz / fs;
    return 20.0 * std::log10 (std::abs (hAt (h, w)));
}

static void reportCurve (double fs, int depth)
{
    const auto h = impulseResponse (fs, depth);

    printf ("\n  %s @ %.1f kHz, depth %dx\n", "group delay", fs / 1000.0, depth);
    printf ("  %8s %10s %10s %9s %8s %s\n", "Hz", "spec ms", "measured", "err ms", "tol ms", "");
    double worst = 0.0;
    for (const auto& a : kSpec)
    {
        const double gdMs = groupDelay (h, fs, a.hz) * 1000.0 / fs;
        const double tol = a.hz <= 300.0 ? 0.10 * a.ms : 0.05;
        const double err = gdMs - a.ms;
        const bool ok = std::abs (err) <= tol + 1e-9;
        if (a.hz <= 300.0)
            worst = std::max (worst, std::abs (err) / tol);
        else
            worst = std::max (worst, std::abs (err) / 0.05);
        printf ("  %8.1f %10.3f %10.3f %+9.3f %8.3f  %s\n",
                a.hz, a.ms, gdMs, err, tol, ok ? "ok" : "OUT");
    }
    printf ("  worst / tolerance: %.2f\n", worst);

    double worstMag = 0.0;
    for (int i = 0; i <= 400; ++i)
    {
        const double hz = 10.0 * std::pow (2000.0, i / 400.0); // 10 Hz .. 20 kHz
        worstMag = std::max (worstMag, std::abs (magDb (h, fs, hz)));
    }
    printf ("  max |magnitude| deviation: %.3e dB\n", worstMag);

    // first non-zero sample of the impulse response == latency
    int first = -1;
    for (size_t i = 0; i < h.size(); ++i)
        if (std::abs (h[i]) > 1e-15) { first = (int) i; break; }
    printf ("  first non-zero IR sample: %d (zero latency = 0), h[0] = %.6f\n",
            first, h[0]);
}

static void reportDepthScaling()
{
    const double fs = 48000.0;
    printf ("\n  depth scaling (GD in ms, 48 kHz)\n");
    printf ("  %8s", "Hz");
    for (int d = 1; d <= 4; ++d) printf (" %10s", (std::to_string (d) + "x").c_str());
    printf ("\n");
    for (double hz : { 20.0, 50.0, 100.0, 300.0, 500.0, 1000.0 })
    {
        printf ("  %8.1f", hz);
        for (int d = 1; d <= 4; ++d)
        {
            const auto h = impulseResponse (fs, d);
            printf (" %10.3f", groupDelay (h, fs, hz) * 1000.0 / fs);
        }
        printf ("\n");
    }
}

static void reportSwitching()
{
    const double fs = 48000.0;
    const double hz = 1000.0;
    const int n = (int) fs;
    const int switchAt = (int) (0.25 * fs);

    PhaseNetwork net;
    net.prepare (fs);
    net.reset();
    net.setDepth (1);

    std::vector<double> y ((size_t) n, 0.0);
    double maxBefore = 0.0, maxDuring = 0.0, maxAfter = 0.0;
    double prev = 0.0;

    for (int i = 0; i < n; ++i)
    {
        if (i == switchAt) net.setDepth (4);
        float l = (float) std::sin (2.0 * kPi * hz * i / fs);
        float r = l;
        net.process (l, r);
        y[(size_t) i] = l;
        const double d = std::abs ((double) l - prev);
        prev = l;
        if (i < switchAt) maxBefore = std::max (maxBefore, d);
        else if (i < switchAt + (int) (0.030 * fs)) maxDuring = std::max (maxDuring, d);
        else if (i > switchAt + (int) (0.060 * fs)) maxAfter = std::max (maxAfter, d);
    }

    const double natural = 2.0 * std::sin (kPi * hz / fs);
    printf ("\n  depth switch 1x -> 4x at 1 kHz, 48 kHz\n");
    printf ("  max sample step before %.6f, during fade %.6f, after %.6f (sine natural step %.6f)\n",
            maxBefore, maxDuring, maxAfter, natural);

    // short-window RMS through the fade to expose level jumps
    auto rms = [&] (int a, int b)
    {
        double s = 0.0;
        for (int i = a; i < b; ++i) s += y[(size_t) i] * y[(size_t) i];
        return std::sqrt (s / std::max (1, b - a));
    };
    const int win = (int) (0.005 * fs);
    double prevRms = rms (switchAt - 3 * win, switchAt);
    double worstDb = 0.0;
    printf ("  RMS every 5 ms around the switch:");
    for (int k = 0; k < 10; ++k)
    {
        const double cur = rms (switchAt + k * win, switchAt + (k + 1) * win);
        const double db = 20.0 * std::log10 ((cur + 1e-12) / (prevRms + 1e-12));
        worstDb = std::max (worstDb, std::abs (db));
        printf (" %+.1f", db);
        prevRms = cur;
    }
    printf ("  dB (max |step| %.1f dB)\n", worstDb);
}

static void reportRecovery()
{
    const double fs = 48000.0;
    PhaseNetwork net;
    net.prepare (fs);
    net.reset();
    net.setDepth (2);

    float l = 1.0f, r = 1.0f;
    for (int i = 0; i < 1000; ++i) { l = 0.1f; r = 0.1f; net.process (l, r); }

    l = std::nanf (""); r = std::nanf ("");
    net.process (l, r);
    const bool finiteImmediately = std::isfinite (l) && std::isfinite (r);

    bool finiteAfter = true;
    for (int i = 0; i < 5000; ++i)
    {
        l = (float) std::sin (0.05 * i); r = l;
        net.process (l, r);
        if (! std::isfinite (l) || ! std::isfinite (r)) finiteAfter = false;
    }
    printf ("\n  NaN injection: output finite immediately: %s, stays finite: %s\n",
            finiteImmediately ? "yes" : "NO", finiteAfter ? "yes" : "NO");
}

static void reportCpu()
{
    const double fs = 48000.0;
    const int seconds = 10;
    const int n = (int) (fs * seconds);

    PhaseNetwork net;
    net.prepare (fs);
    net.reset();
    net.setDepth (4); // worst case: all four passes

    std::mt19937 rng (7);
    std::uniform_real_distribution<float> dist (-0.5f, 0.5f);

    std::vector<float> buf ((size_t) n);
    for (auto& v : buf) v = dist (rng);

    const auto t0 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < n; ++i)
    {
        float l = buf[(size_t) i], r = buf[(size_t) i];
        net.process (l, r);
        buf[(size_t) i] = l;
    }
    const auto t1 = std::chrono::high_resolution_clock::now();
    const double sec = std::chrono::duration<double> (t1 - t0).count();

    printf ("\n  CPU: %.3f s for %.1f s of 48 kHz stereo (depth 4x, 4 passes) = %.3f %% of one core\n",
            sec, (double) seconds, 100.0 * sec / seconds);
    printf ("       %.1f ns per stereo sample, %.1f M biquads/s\n",
            sec * 1e9 / n, 4.0 * PhaseNetwork::kNumSections * 2.0 * fs / 1e6);
}

int main()
{
    printf ("Crestwright phase network verification\n");
    printf ("sections: %d, passes: %d, crossfade: %.0f ms\n",
            PhaseNetwork::kNumSections, PhaseNetwork::kMaxPasses,
            PhaseNetwork::kCrossfadeSeconds * 1000.0);
    for (int i = 0; i < PhaseNetwork::kNumSections; ++i)
        printf ("  section %d: f0 = %10.4f Hz  Q = %.6f\n",
                i + 1, PhaseNetwork::kF0[i], PhaseNetwork::kQ[i]);

    reportCurve (44100.0, 1);
    reportCurve (48000.0, 1);
    reportDepthScaling();
    reportSwitching();
    reportRecovery();
    reportCpu();
    return 0;
}
