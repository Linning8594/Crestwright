// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (c) 2026 Linning

#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout CrestwrightAudioProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { "comp", 1 }, "Comp", NormalisableRange<float> (0.0f, 100.0f, 0.1f), 0.0f));
    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { "compmix", 1 }, "Comp Mix", NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f));
    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { "inputgain", 1 }, "Input", NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f));
    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { "outputgain", 1 }, "Output", NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f));
    layout.add (std::make_unique<AudioParameterBool> (
        ParameterID { "link", 1 }, "Link", true));
    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { "phaseq", 1 }, "Phase Depth", NormalisableRange<float> (0.0f, 3.0f, 1.0f), 0.0f));
    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { "clip", 1 }, "Clip", NormalisableRange<float> (0.0f, 100.0f, 1.0f), 0.0f));
    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { "schpf", 1 }, "SC HPF",
        NormalisableRange<float> (20.0f, 400.0f, 1.0f, 0.4f), 80.0f));
    layout.add (std::make_unique<AudioParameterBool> (
        ParameterID { "autogain", 1 }, "Auto Gain", false));
    layout.add (std::make_unique<AudioParameterBool> (
        ParameterID { "invert", 1 }, "Phase Invert", false));

    return layout;
}

CrestwrightAudioProcessor::CrestwrightAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    pComp       = apvts.getRawParameterValue ("comp");
    pCompMix    = apvts.getRawParameterValue ("compmix");
    pPhaseQ     = apvts.getRawParameterValue ("phaseq");
    pInvert     = apvts.getRawParameterValue ("invert");
    pAutoGain   = apvts.getRawParameterValue ("autogain");
    pInputGain  = apvts.getRawParameterValue ("inputgain");
    pOutputGain = apvts.getRawParameterValue ("outputgain");
    pLink       = apvts.getRawParameterValue ("link");
    pClip       = apvts.getRawParameterValue ("clip");
    pSchpf      = apvts.getRawParameterValue ("schpf");

    apvts.addParameterListener ("inputgain", this);
    apvts.addParameterListener ("outputgain", this);
    apvts.addParameterListener ("link", this);
}

CrestwrightAudioProcessor::~CrestwrightAudioProcessor()
{
    apvts.removeParameterListener ("inputgain", this);
    apvts.removeParameterListener ("outputgain", this);
    apvts.removeParameterListener ("link", this);
}

void CrestwrightAudioProcessor::parameterChanged (const juce::String& parameterID, float newValue)
{
    // Link mirrors the two trims: whichever knob is moved, the other one
    // takes the opposite value. It also applies the balance the moment Link
    // is switched on, so the pair starts out matched.
    if (pLink == nullptr)
        return;

    const bool linkOn = parameterID == "link" ? newValue > 0.5f : pLink->load() > 0.5f;
    if (! linkOn)
        return;

    if (parameterID != "inputgain" && parameterID != "outputgain" && parameterID != "link")
        return;

    // the partner parameter write below notifies us synchronously; ignore that
    if (linkUpdating.exchange (true))
        return;

    juce::String partnerId;
    float target = 0.0f;

    if (parameterID == "inputgain")
    {
        partnerId = "outputgain";
        target = -newValue;
    }
    else if (parameterID == "outputgain")
    {
        partnerId = "inputgain";
        target = -newValue;
    }
    else
    {
        partnerId = "outputgain";
        target = -(pInputGain != nullptr ? pInputGain->load() : 0.0f);
    }

    if (auto* partner = apvts.getParameter (partnerId))
    {
        const float clamped = juce::jlimit (-24.0f, 24.0f, target);
        const float current = partner->convertFrom0to1 (partner->getValue());

        if (std::abs (current - clamped) > 0.05f)
            partner->setValueNotifyingHost (partner->convertTo0to1 (clamped));
    }

    linkUpdating.store (false);
}

//==============================================================================
//==============================================================================
void CrestwrightAudioProcessor::Compressor::setParams (float macro01, double sr)
{
    const float m = juce::jlimit (0.0f, 1.0f, macro01);

    // the detector runs 14 dB hotter than the audio, so unity-level sources
    // already sit well above the threshold (the old curve needed +14 dB of
    // input gain to feel the same amount of compression)
    threshDb = -20.0f - 14.0f * m;                // -20 dB .. -34 dB
    ratio    = 1.0f + 7.0f * m;                   // 1:1 .. 8:1

    const float attackMs  = 40.0f * std::pow (0.25f, m);    // 40 ms .. 10 ms
    const float releaseMs = 200.0f * std::pow (0.4f, m);    // 200 ms .. 80 ms

    attackCoef  = (float) std::exp (-1.0 / (attackMs * 0.001 * sr));
    releaseCoef = (float) std::exp (-1.0 / (releaseMs * 0.001 * sr));

    makeupDb = 10.0f * m;                                   // 0 .. +10 dB, gain-aware
}

void CrestwrightAudioProcessor::Compressor::updateSidechainHPF() noexcept
{
    const float target = juce::jlimit (20.0f, 400.0f, hpfTargetHz);

    if (std::abs (target - hpfHz) < 0.01f)
        return;

    hpfHz += (target - hpfHz) * 0.25f;
    if (std::abs (target - hpfHz) < 0.05f)
        hpfHz = target;

    hpfL.setCoeffs (hpfHz, hpfSampleRate);
    hpfR.setCoeffs (hpfHz, hpfSampleRate);
}

void CrestwrightAudioProcessor::Compressor::reset() noexcept
{
    env = 0.0f;
    hpfHz = juce::jlimit (20.0f, 400.0f, hpfTargetHz);
    hpfL.setCoeffs (hpfHz, hpfSampleRate);
    hpfR.setCoeffs (hpfHz, hpfSampleRate);
    hpfL.reset();
    hpfR.reset();
}

void CrestwrightAudioProcessor::prepareToPlay (double sampleRate, int)
{
    currentSampleRate = sampleRate > 0.0 ? sampleRate : 48000.0;

    phase.prepare (currentSampleRate);
    phase.setDepth (juce::jlimit (1, 4, 1 + (int) std::lround (pPhaseQ->load())));

    comp.setParams (pComp->load() / 100.0f, currentSampleRate);
    comp.setSidechainHPF (pSchpf->load(), currentSampleRate);
    comp.reset();

    // 0.5 s RMS window, 0.8 s gain glide; both per sample, so the behaviour
    // does not depend on the host block size
    rmsCoef = 1.0f - (float) std::exp (-1.0 / (0.5 * currentSampleRate));
    agCoef  = 1.0f - (float) std::exp (-1.0 / (0.8 * currentSampleRate));
    msIn = msOut = 1.0e-6f;
    autoGainLin = agTarget = 1.0f;
    agCounter = 0;

    compMixSm.reset (currentSampleRate, 0.02);
    compMixSm.setCurrentAndTargetValue (pCompMix->load() / 100.0f);

    phase.reset();
}

bool CrestwrightAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& in  = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();
    return in == out && (in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo());
}

void CrestwrightAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numIn = getTotalNumInputChannels();
    const int numOut = getTotalNumOutputChannels();
    for (int ch = numIn; ch < numOut; ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    const int n = buffer.getNumSamples();
    if (n <= 0)
        return;

    const float inGain  = std::pow (10.0f, pInputGain->load() / 20.0f);
    const float outGain = std::pow (10.0f, pOutputGain->load() / 20.0f);

    // clip: 0 = OFF (bypass), 1..100 = soft-clip amount. The threshold moves
    // exponentially from +16 dBFS down to -16.5 dBFS so that low settings
    // stay out of the way of normal full-scale drum peaks.
    const float clipAmt = juce::jlimit (0.0f, 1.0f, pClip->load() / 100.0f);
    const bool  clipOn  = clipAmt > 0.0f;
    const float clipThreshold = 6.3f * std::pow (0.15f / 6.3f, clipAmt);

    // new phase module: depth comes from the existing PHASE control
    phase.setDepth (juce::jlimit (1, 4, 1 + (int) std::lround (pPhaseQ->load())));

    comp.setParams (pComp->load() / 100.0f, currentSampleRate);
    comp.setSidechainHPF (pSchpf->load(), currentSampleRate);
    comp.updateSidechainHPF();
    compMixSm.setTargetValue (pCompMix->load() / 100.0f);
    const bool invert   = pInvert->load() > 0.5f;
    const bool autoGain = pAutoGain->load() > 0.5f;
    const float sign = invert ? -1.0f : 1.0f;

    auto* left  = buffer.getWritePointer (0);
    auto* right = numOut > 1 ? buffer.getWritePointer (1) : nullptr;

    constexpr float eps = 1.0e-9f;

    for (int i = 0; i < n; ++i)
    {
        float l = left[i];
        float r = right != nullptr ? right[i] : l;

        l *= inGain;
        r *= inGain;

        // input level (after input gain)
        msIn += rmsCoef * (0.5f * (l * l + r * r) - msIn);

        // phase module: six allpass sections, 1x..4x with a 30 ms crossfade
        phase.process (l, r);

        // numerical safety net: never latch into Inf/NaN and silence the plugin
        if (!std::isfinite (l) || !std::isfinite (r)
            || std::abs (l) > 1.0e4f || std::abs (r) > 1.0e4f)
        {
            phase.reset();
            comp.reset();
            l = 0.0f;
            r = 0.0f;
        }

        // 1176-flavoured, stereo-linked compressor
        const float preL = l, preR = r;
        comp.process (l, r);

        // parallel compressor blend
        const float cm = compMixSm.getNextValue();
        l = cm * l + (1.0f - cm) * preL;
        r = cm * r + (1.0f - cm) * preR;

        // soft clip stage (CLIP = 1 is a true bypass)
        if (clipOn)
        {
            l = clipThreshold * std::tanh (l / clipThreshold);
            r = clipThreshold * std::tanh (r / clipThreshold);
        }

        // processed level is measured before the auto gain, so it cannot self-cancel
        msOut += rmsCoef * (0.5f * (l * l + r * r) - msOut);

        if (++agCounter >= 32)
        {
            agCounter = 0;

            if (! autoGain)
                agTarget = 1.0f;
            else if (msIn > 1.0e-7f)
                agTarget = juce::jlimit (0.063f, 15.85f, std::sqrt ((msIn + eps) / (msOut + eps)));
            // else: hold the current gain through silence
        }

        autoGainLin += (agTarget - autoGainLin) * agCoef;
        l *= autoGainLin * outGain;
        r *= autoGainLin * outGain;

        left[i] = sign * l;
        if (right != nullptr)
            right[i] = sign * r;
    }

    grDbOut.store (comp.lastGrDb);
}

//==============================================================================
juce::AudioProcessorEditor* CrestwrightAudioProcessor::createEditor()
{
    return new CrestwrightAudioProcessorEditor (*this);
}

void CrestwrightAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void CrestwrightAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CrestwrightAudioProcessor();
}
