// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (c) 2026 Linning

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PhaseNetwork.h"

class CrestwrightAudioProcessor : public juce::AudioProcessor,
                                 private juce::AudioProcessorValueTreeState::Listener
{
public:
    CrestwrightAudioProcessor();
    ~CrestwrightAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Crestwright"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 1.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;

    // Live gain-reduction readout for the editor's meter (dB, >= 0).
    std::atomic<float> grDbOut { 0.0f };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    // ---- phase module ------------------------------------------------------
    // Six second-order allpass sections from the approved reference preset,
    // repeated for 1x..4x. See PhaseNetwork.h for the design and the exact
    // f0/Q values.
    PhaseNetwork phase;

    // ---- 1176-flavoured compressor -----------------------------------------
    struct Compressor
    {
        float env = 0.0f;
        float threshDb = -6.0f, ratio = 1.0f;
        float attackCoef = 0.0f, releaseCoef = 0.0f;
        float makeupDb = 0.0f;
        float lastGrDb = 0.0f;

        // 12 dB/oct Butterworth high-pass that feeds the detector only; the
        // audio path itself is untouched
        struct DetectorHPF
        {
            double b0 = 1.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0;
            double z1 = 0.0, z2 = 0.0;

            void setCoeffs (double hz, double sr) noexcept
            {
                const double w0 = 6.283185307179586 * juce::jlimit (10.0, 0.45 * sr, hz) / sr;
                const double cw = std::cos (w0), sw = std::sin (w0);
                const double alpha = sw / (2.0 * 0.7071067811865476);
                const double a0 = 1.0 + alpha;

                b0 = (1.0 + cw) * 0.5 / a0;
                b1 = -(1.0 + cw) / a0;
                b2 = b0;
                a1 = -2.0 * cw / a0;
                a2 = (1.0 - alpha) / a0;
            }

            inline double process (double x) noexcept
            {
                const double y = b0 * x + z1;
                z1 = b1 * x - a1 * y + z2;
                z2 = b2 * x - a2 * y;
                return y;
            }

            void reset() noexcept { z1 = z2 = 0.0; }
        };

        DetectorHPF hpfL, hpfR;
        float hpfHz = 80.0f, hpfTargetHz = 80.0f;
        double hpfSampleRate = 48000.0;

        void setParams (float macro, double sr);
        void setSidechainHPF (float hz, double sr) noexcept
        {
            hpfTargetHz = juce::jlimit (20.0f, 400.0f, hz);
            hpfSampleRate = sr;
        }
        void updateSidechainHPF() noexcept;
        void reset() noexcept;

        inline void process (float& l, float& r) noexcept
        {
            // detector sees the high-passed signal only
            const float detL = (float) std::abs (hpfL.process ((double) l));
            const float detR = (float) std::abs (hpfR.process ((double) r));
            const float x = juce::jmax (detL, detR);
            const float coef = x > env ? attackCoef : releaseCoef;
            env = coef * env + (1.0f - coef) * x;

            const float levelDb = 20.0f * std::log10 (env + 1.0e-9f);
            const float over = levelDb - threshDb;
            float gr = 0.0f;

            constexpr float knee = 3.0f;
            if (over > -0.5f * knee)
            {
                if (over < 0.5f * knee)
                {
                    const float t = over + 0.5f * knee;
                    gr = (1.0f - 1.0f / ratio) * t * t / (2.0f * knee);
                }
                else
                {
                    gr = (1.0f - 1.0f / ratio) * over;
                }
            }

            lastGrDb = gr;

            // Make-up rises with the actual gain reduction instead of being
            // applied at full value the moment a transient arrives. Because
            // makeupDbEff < gr for every gr > 0, the overall gain never goes
            // above unity, so full-scale peaks cannot be pushed into
            // clipping before the attack has caught up.
            const float makeupDbEff = makeupDb * (1.0f - std::exp (-gr / 12.0f));
            const float g = std::pow (10.0f, (makeupDbEff - gr) / 20.0f);
            l *= g;
            r *= g;
        }
    };

    Compressor comp;

    std::atomic<float>* pComp = nullptr;
    std::atomic<float>* pCompMix = nullptr;
    std::atomic<float>* pPhaseQ = nullptr;
    std::atomic<float>* pInvert = nullptr;
    std::atomic<float>* pAutoGain = nullptr;
    std::atomic<float>* pInputGain = nullptr;
    std::atomic<float>* pOutputGain = nullptr;
    std::atomic<float>* pLink = nullptr;
    std::atomic<float>* pClip = nullptr;
    std::atomic<float>* pSchpf = nullptr;

    void parameterChanged (const juce::String& parameterID, float newValue) override;

    // guards the Link pair so an induced change cannot bounce back
    std::atomic<bool> linkUpdating { false };

    juce::SmoothedValue<float> compMixSm;

    double currentSampleRate = 48000.0;

    // per-sample input/output RMS matcher for the auto-gain switch
    float msIn = 1.0e-6f, msOut = 1.0e-6f;
    float autoGainLin = 1.0f, agTarget = 1.0f;
    float rmsCoef = 0.0f, agCoef = 0.0f;
    int agCounter = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CrestwrightAudioProcessor)
};
