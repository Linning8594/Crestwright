// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (c) 2026 Linning

#pragma once

#include <algorithm>
#include <array>
#include <cmath>

//==============================================================================
/** Crestwright phase module.

    A cascade of six second-order allpass sections, repeated up to four times
    for the 1x/2x/3x/4x depth switch.

    The six (f0, Q) pairs are taken verbatim from the revised reference
    preset (v2; kept locally as verification/reference-preset-v2.ffp, not
    part of the public repository), bands 1, 4, 7, 2, 3 and 5 sorted by
    frequency:

        f0 =  21.272 Hz   Q = 0.479995
        f0 = 169.033 Hz   Q = 0.469874
        f0 = 447.819 Hz   Q = 0.477733
        f0 = 892.054 Hz   Q = 0.376757
        f0 = 1873.367 Hz  Q = 0.471898
        f0 = 6367.351 Hz  Q = 0.475947

    Every section is the standard bilinear (RBJ) second-order allpass:

        w0    = 2*pi*f0/fs
        alpha = sin(w0) / (2*Q)
        a1    = -2*cos(w0) / (1 + alpha)
        a2    =  (1 - alpha) / (1 + alpha)
        H(z)  = (a2 + a1 z^-1 + z^-2) / (1 + a1 z^-1 + a2 z^-2)

    Properties:
      * zero latency (plain IIR, no look-ahead or delay line)
      * |H(e^jw)| == 1 for every section, so the chain is pure allpass
      * depth 2x/3x/4x runs the same cascade two/three/four times
      * a 30 ms linear output crossfade joins the depth taps
      * double precision state and coefficients, pole radius capped at 0.9995
      * no allocation after prepare()

    All four depth taps are always evaluated so the state of every tap stays
    valid; a depth change is then a pure output crossfade with no state
    discontinuity. The cost is 4 * 6 = 24 biquads per channel, which is still
    far below one percent of a modern core at 48 kHz.

    This header deliberately has no JUCE dependency so it can be compiled and
    measured on its own by verification/test_phase.cpp.
*/
struct PhaseNetwork
{
    static constexpr int kNumSections = 6;
    static constexpr int kMaxPasses   = 4;   // 1x .. 4x
    static constexpr double kCrossfadeSeconds = 0.030;
    static constexpr double kMaxPoleRadius    = 0.9995;

    struct Section
    {
        double b0 = 1.0, b1 = 0.0;   // b2 is always 1 for this allpass form
        double a1 = 0.0, a2 = 0.0;
    };

    // ---- reference preset data ------------------------------------------
    static constexpr double kF0[kNumSections] =
    {
        21.272339342794353, 169.033344043676, 447.819093846016,
        892.054005153722, 1873.367204745139, 6367.350518082420
    };

    static constexpr double kQ[kNumSections] =
    {
        0.479995399714, 0.469873934984, 0.477732777596,
        0.376756578684, 0.471898287535, 0.475946873426
    };

    // ---- lifecycle -------------------------------------------------------
    void prepare (double sampleRate) noexcept
    {
        sampleRate_ = sampleRate > 1000.0 ? sampleRate : 48000.0;
        fadeLength_ = std::max (1, (int) std::lround (kCrossfadeSeconds * sampleRate_));

        // alpha that puts the pole radius exactly at kMaxPoleRadius
        constexpr double r2 = kMaxPoleRadius * kMaxPoleRadius;
        const double alphaMin = (1.0 - r2) / (1.0 + r2);

        for (int i = 0; i < kNumSections; ++i)
        {
            const double f0 = std::clamp (kF0[i], 1.0, 0.45 * sampleRate_);
            const double q  = std::clamp (kQ[i], 0.05, 60.0);

            const double w0 = 2.0 * kPi * f0 / sampleRate_;
            const double alpha = std::max (std::sin (w0) / (2.0 * q), alphaMin);
            const double a0 = 1.0 + alpha;

            auto& s = sections_[i];
            s.b0 = (1.0 - alpha) / a0;          // == a2
            s.b1 = -2.0 * std::cos (w0) / a0;   // == a1
            s.a1 = s.b1;
            s.a2 = s.b0;
        }

        reset();
    }

    void reset() noexcept
    {
        for (auto& pass : z1_) for (auto& ch : pass) ch.fill (0.0);
        for (auto& pass : z2_) for (auto& ch : pass) ch.fill (0.0);

        for (int i = 0; i < kMaxPasses; ++i)
            weights_[i] = i == targetDepth_ - 1 ? 1.0 : 0.0;

        fadeRemaining_ = 0;
    }

    /** Select 1x .. 4x. Idempotent; the change is crossfaded over 30 ms. */
    void setDepth (int depth) noexcept
    {
        depth = std::clamp (depth, 1, kMaxPasses);
        if (depth == targetDepth_)
            return;

        targetDepth_ = depth;
        fadeRemaining_ = fadeLength_;

        for (int i = 0; i < kMaxPasses; ++i)
        {
            const double target = i == depth - 1 ? 1.0 : 0.0;
            fadeStep_[i] = (target - weights_[i]) / (double) fadeLength_;
        }
    }

    int getDepth() const noexcept { return targetDepth_; }

    // ---- audio -----------------------------------------------------------
    inline void process (float& left, float& right) noexcept
    {
        const double inL = (double) left;
        const double inR = (double) right;

        double vL = inL, vR = inR;
        double outL = 0.0, outR = 0.0;

        for (int pass = 0; pass < kMaxPasses; ++pass)
        {
            vL = runPass (pass, 0, vL);
            vR = runPass (pass, 1, vR);
            outL += weights_[pass] * vL;
            outR += weights_[pass] * vR;
        }

        advanceFade();

        // automatic recovery: never latch into Inf/NaN
        if (! std::isfinite (outL) || ! std::isfinite (outR))
        {
            reset();
            outL = outR = 0.0;
        }

        left  = (float) outL;
        right = (float) outR;
    }

private:
    static constexpr double kPi = 3.14159265358979323846;

    inline double runPass (int pass, int channel, double x) noexcept
    {
        for (int i = 0; i < kNumSections; ++i)
        {
            const Section& s = sections_[i];
            const double y = s.b0 * x + z1_[pass][channel][i];
            z1_[pass][channel][i] = s.b1 * x - s.a1 * y + z2_[pass][channel][i];
            z2_[pass][channel][i] = x - s.a2 * y;
            x = y;
        }
        return x;
    }

    inline void advanceFade() noexcept
    {
        if (fadeRemaining_ <= 0)
            return;

        for (int i = 0; i < kMaxPasses; ++i)
            weights_[i] += fadeStep_[i];

        if (--fadeRemaining_ == 0)
            for (int i = 0; i < kMaxPasses; ++i)
                weights_[i] = i == targetDepth_ - 1 ? 1.0 : 0.0;
    }

    Section sections_[kNumSections] {};
    std::array<std::array<std::array<double, kNumSections>, 2>, kMaxPasses> z1_ {};
    std::array<std::array<std::array<double, kNumSections>, 2>, kMaxPasses> z2_ {};

    double weights_[kMaxPasses] { 1.0, 0.0, 0.0, 0.0 };
    double fadeStep_[kMaxPasses] {};
    int fadeRemaining_ = 0;
    int fadeLength_ = 1;
    int targetDepth_ = 1;
    double sampleRate_ = 48000.0;
};
