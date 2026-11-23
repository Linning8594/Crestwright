// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (c) 2026 Linning

#pragma once

#include <algorithm>
#include <cmath>

/** Crestwright fixed transient shaper.

    An internal attack enhancer at the very front of the signal chain. It is
    always active, has no parameter, no UI control and no bypass. It adds up
    to 1% of the detected transient (gain 1.00 -> 1.01) and leaves the
    sustain alone.

    Detector: stereo-linked mean-square envelope.
      fast envelope: 0.5 ms attack, 30 ms release
      slow envelope: 100 ms smoothing of the fast envelope

    The positive difference (fast - slow), normalised by the slow envelope,
    drives the gain. The gain is clamped to 1.01, so the maximum boost is
    exactly 1% (about +0.086 dB). On steady material fast and slow converge,
    the difference goes to zero and the gain returns to exactly 1.00.

    Zero latency: both envelopes are causal; there is no look-ahead or delay
    line. The module has no allocation and no parameters.
*/
struct TransientShaper
{
    static constexpr double kAttackAmount = 0.01;   // fixed +1%

    void prepare (double sampleRate) noexcept
    {
        const double sr = sampleRate > 1000.0 ? sampleRate : 48000.0;

        fastAttack_  = 1.0 - std::exp (-1.0 / (0.0005 * sr));  // 0.5 ms
        fastRelease_ = 1.0 - std::exp (-1.0 / (0.030  * sr));  // 30 ms
        slowCoef_    = 1.0 - std::exp (-1.0 / (0.100  * sr));  // 100 ms

        reset();
    }

    void reset() noexcept
    {
        fastEnv_ = 0.0;
        slowEnv_ = 0.0;
    }

    inline void process (float& left, float& right) noexcept
    {
        const double l = (double) left;
        const double r = (double) right;

        // stereo-linked mean square, so a steady sine has no envelope ripple
        const double det = 0.5 * (l * l + r * r);

        fastEnv_ += (det > fastEnv_ ? fastAttack_ : fastRelease_) * (det - fastEnv_);
        slowEnv_ += slowCoef_ * (fastEnv_ - slowEnv_);

        double transient = 0.0;
        if (slowEnv_ > 1.0e-12)
            transient = std::max (0.0, fastEnv_ - slowEnv_) / slowEnv_;

        const double gain = 1.0 + kAttackAmount * std::min (1.0, transient);

        left  = (float) (l * gain);
        right = (float) (r * gain);
    }

private:
    double fastEnv_ = 0.0;
    double slowEnv_ = 0.0;
    double fastAttack_ = 0.0;
    double fastRelease_ = 0.0;
    double slowCoef_ = 0.0;
};
