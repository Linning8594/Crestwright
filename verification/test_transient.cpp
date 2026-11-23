// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (c) 2026 Linning

// Standalone check of Source/TransientShaper.h.
//
//   cl /O2 /std:c++17 /EHsc /Fe:test_transient.exe test_transient.cpp
//   test_transient.exe

#include "../Source/TransientShaper.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

static constexpr double kPi = 3.14159265358979323846;

int main()
{
    const double fs = 48000.0;
    const double amp = 0.5;
    const double hz = 1000.0;

    TransientShaper ts;
    ts.prepare (fs);
    ts.reset();

    // ---- steady sine: the gain must settle back to exactly 1.0 ----------
    double lastGain = 1.0;
    double worstSteady = 0.0;
    const int total = (int) (2.0 * fs);

    for (int i = 0; i < total; ++i)
    {
        const float in = (float) (amp * std::sin (2.0 * kPi * hz * i / fs));
        float l = in, r = in;
        ts.process (l, r);

        if (i > total - (int) (0.1 * fs) && std::abs (in) > 0.1f)
        {
            const double g = std::abs ((double) l) / std::abs ((double) in);
            worstSteady = std::max (worstSteady, std::abs (g - 1.0));
            lastGain = g;
        }
    }

    printf ("steady sine: final gain %.6f, max |gain - 1| in last 100 ms %.3e\n",
            lastGain, worstSteady);

    // ---- burst: 20 ms of sine, then silence -----------------------------
    ts.reset();
    double maxGain = 1.0;
    const int burst = (int) (0.020 * fs);

    for (int i = 0; i < (int) (0.5 * fs); ++i)
    {
        const float in = i < burst
                             ? (float) (amp * std::sin (2.0 * kPi * hz * i / fs))
                             : 0.0f;
        float l = in, r = in;
        ts.process (l, r);

        if (std::abs (in) > 1.0e-6f)
            maxGain = std::max (maxGain, std::abs ((double) l) / std::abs ((double) in));
    }

    printf ("burst: max gain %.6f (limit 1.01), boost %.3f dB\n",
            maxGain, 20.0 * std::log10 (maxGain));

    // ---- silence ---------------------------------------------------------
    ts.reset();
    float l = 0.0f, r = 0.0f;
    ts.process (l, r);
    printf ("silence: output %.6f %.6f\n", (double) l, (double) r);

    const bool ok = maxGain <= 1.01 + 1.0e-6
                    && maxGain > 1.0
                    && worstSteady < 2.0e-3
                    && std::isfinite (l) && std::isfinite (r);

    printf ("result: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
