// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (c) 2026 Linning

#include "PluginEditor.h"
#include <BinaryData.h>

namespace
{
    const juce::Colour accent      { 0xffdc9a46 };
    const juce::Colour bgTop       { 0xff26262c };
    const juce::Colour bgBottom    { 0xff131316 };
    const juce::Colour textDim     { 0xff8f8f99 };

    // Parsed once per process, not once per editor instance.
    juce::Typeface::Ptr interRegular()
    {
        static auto tf = juce::Typeface::createSystemTypefaceFor (
            BinaryData::InterRegular_ttf, (size_t) BinaryData::InterRegular_ttfSize);
        return tf;
    }

    juce::Typeface::Ptr interMedium()
    {
        static auto tf = juce::Typeface::createSystemTypefaceFor (
            BinaryData::InterMedium_ttf, (size_t) BinaryData::InterMedium_ttfSize);
        return tf;
    }

    juce::Typeface::Ptr interSemiBold()
    {
        static auto tf = juce::Typeface::createSystemTypefaceFor (
            BinaryData::InterSemiBold_ttf, (size_t) BinaryData::InterSemiBold_ttfSize);
        return tf;
    }

    juce::Typeface::Ptr interDisplaySemiBold()
    {
        static auto tf = juce::Typeface::createSystemTypefaceFor (
            BinaryData::InterDisplaySemiBold_ttf, (size_t) BinaryData::InterDisplaySemiBold_ttfSize);
        return tf;
    }
}

//==============================================================================
void CrestwrightAudioProcessorEditor::KnobLAF::drawRotarySlider (juce::Graphics& g,
                                                                int x, int y, int width, int height,
                                                                float pos, float startAngle, float endAngle,
                                                                juce::Slider& slider)
{
    auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat()
                      .reduced (&slider == compactSlider ? 3.0f : 8.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const float angle = startAngle + pos * (endAngle - startAngle);
    const float arcR = radius * 0.82f;

    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillEllipse (bounds.translated (0.0f, 3.0f));

    {
        juce::ColourGradient grad (juce::Colour (0xff6a6a74), bounds.getX(), bounds.getY(),
                                   juce::Colour (0xff24242a), bounds.getRight(), bounds.getBottom(), false);
        g.setGradientFill (grad);
        g.fillEllipse (bounds);
    }

    auto face = bounds.reduced (radius * 0.10f);
    {
        juce::ColourGradient grad (juce::Colour (0xff3d3d47), face.getCentreX(), face.getY(),
                                   juce::Colour (0xff191920), face.getCentreX(), face.getBottom(), false);
        g.setGradientFill (grad);
        g.fillEllipse (face);
    }

    g.setColour (juce::Colour (0x30ffffff));
    g.drawEllipse (face.reduced (1.0f), 1.0f);

    juce::Path track;
    track.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, startAngle, endAngle, true);
    g.setColour (juce::Colour (0xff2a2a31));
    g.strokePath (track, juce::PathStrokeType (radius * 0.085f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));

    if (pos > 0.002f)
    {
        juce::Path val;
        val.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, startAngle, angle, true);
        g.setColour (accent.withAlpha (0.10f));
        g.strokePath (val, juce::PathStrokeType (radius * 0.16f, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));
        g.setColour (accent);
        g.strokePath (val, juce::PathStrokeType (radius * 0.085f, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));
    }

    if (&slider != compactSlider)
    {
        for (int i = 0; i <= 10; ++i)
        {
            const float a = startAngle + (endAngle - startAngle) * (float) i / 10.0f;
            const float r1 = radius * 0.58f, r2 = radius * 0.65f;
            g.setColour (i % 5 == 0 ? juce::Colour (0xff77777f) : juce::Colour (0xff3a3a42));
            g.drawLine (centre.x + r1 * std::sin (a), centre.y - r1 * std::cos (a),
                        centre.x + r2 * std::sin (a), centre.y - r2 * std::cos (a), 1.4f);
        }
    }

    // stepped knob (PHASE): one clear mark per position, 1x / 2x / 3x / 4x
    if (&slider == detentSlider && detentCount > 1)
    {
        for (int i = 0; i < detentCount; ++i)
        {
            const float a = startAngle + (endAngle - startAngle) * (float) i / (float) (detentCount - 1);
            const float r1 = radius * 0.52f, r2 = radius * 0.68f;
            g.setColour (juce::Colour (0xffe6b26a).withAlpha (0.85f));
            g.drawLine (centre.x + r1 * std::sin (a), centre.y - r1 * std::cos (a),
                        centre.x + r2 * std::sin (a), centre.y - r2 * std::cos (a), 2.0f);
        }
    }

    {
        const float r1 = radius * 0.18f, r2 = radius * 0.56f;
        g.setColour (juce::Colour (0xfff4f4f8));
        g.drawLine (centre.x + r1 * std::sin (angle), centre.y - r1 * std::cos (angle),
                    centre.x + r2 * std::sin (angle), centre.y - r2 * std::cos (angle), 3.0f);
    }

    g.setColour (juce::Colour (0xff15151a));
    g.fillEllipse (juce::Rectangle<float> (radius * 0.16f, radius * 0.16f).withCentre (centre));
}

//==============================================================================
void CrestwrightAudioProcessorEditor::ToggleSwitch::paintButton (juce::Graphics& g,
                                                                bool isMouseOver, bool isMouseDown)
{
    auto b = getLocalBounds().toFloat();
    const float trackW = 42.0f, trackH = 22.0f;
    auto track = juce::Rectangle<float> (trackW, trackH)
                     .withCentre ({ b.getRight() - trackW * 0.5f - 2.0f, b.getCentreY() });

    g.setColour (getToggleState() ? juce::Colour (0xffe6b26a)
                                  : (isMouseOver ? juce::Colour (0xffb0b0ba) : textDim));
    g.setFont (font);
    g.drawText (label, b.withTrimmedRight (trackW + 10.0f).withTrimmedLeft (8.0f),
                juce::Justification::centredLeft, true);

    g.setColour (getToggleState() ? accent : juce::Colour (0xff2b2b32));
    g.fillRoundedRectangle (track, trackH * 0.5f);

    if (getToggleState())
    {
        g.setColour (accent.withAlpha (0.25f));
        g.drawRoundedRectangle (track.expanded (2.5f), trackH * 0.5f + 2.0f, 2.0f);
    }

    auto inner = track.reduced (3.0f);
    const float d = inner.getHeight();
    const float cx = getToggleState() ? inner.getRight() - d * 0.5f : inner.getX() + d * 0.5f;
    auto knob = juce::Rectangle<float> (d, d).withCentre ({ cx, inner.getCentreY() });

    g.setColour (isMouseDown ? juce::Colour (0xffd0d0d6)
                             : (getToggleState() ? juce::Colour (0xff1a1a1e) : juce::Colour (0xff8b8b94)));
    g.fillEllipse (knob);
}

//==============================================================================
CrestwrightAudioProcessorEditor::CrestwrightAudioProcessorEditor (CrestwrightAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    tfRegular  = interRegular();
    tfMedium   = interMedium();
    tfSemiBold = interSemiBold();
    tfDisplay  = interDisplaySemiBold();

    titleFont  = juce::Font (juce::FontOptions().withTypeface (tfDisplay).withHeight (24.0f));
    labelFont  = juce::Font (juce::FontOptions().withTypeface (tfMedium).withHeight (12.0f));
    valueFont  = juce::Font (juce::FontOptions().withTypeface (tfSemiBold).withHeight (19.0f));
    tinyFont   = juce::Font (juce::FontOptions().withTypeface (tfMedium).withHeight (10.0f));
    switchFont = juce::Font (juce::FontOptions().withTypeface (tfSemiBold).withHeight (12.0f));

    knobLAF.setDefaultSansSerifTypeface (tfRegular);
    knobLAF.detentSlider = &phaseSlider;
    knobLAF.detentCount  = 4;

    // close an open inline value box when the user clicks anywhere else
    addMouseListener (this, true);

    auto paramDefault = [this] (const juce::String& id) -> double
    {
        if (auto* p = processor.apvts.getParameter (id))
            return (double) p->convertFrom0to1 (p->getDefaultValue());
        return 0.0;
    };

    auto setupKnob = [this, &paramDefault] (KnobSlider& s, juce::Rectangle<int>& valueArea,
                                            const juce::String& name, const juce::String& paramId,
                                            int decimals)
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        s.setLookAndFeel (&knobLAF);
        s.setName (name);
        s.setNumDecimalPlacesToDisplay (decimals);
        s.defaultValue = paramDefault (paramId);
        s.setEditorFont (juce::Font (juce::FontOptions().withTypeface (tfSemiBold).withHeight (13.0f)));
        auto* va = &valueArea;
        s.onValueChange = [this, va] { repaint (va->expanded (0, 4)); };
        addAndMakeVisible (s);
    };

    auto setupLabel = [this] (juce::Label& l, const juce::String& text)
    {
        l.setText (text, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred);
        l.setColour (juce::Label::textColourId, textDim);
        l.setFont (labelFont);
        addAndMakeVisible (l);
    };

    setupKnob (inputSlider,  valueAreaInput,  "INPUT",    "inputgain",  1);
    setupKnob (outputSlider, valueAreaOutput, "OUTPUT",   "outputgain", 1);
    setupKnob (compSlider,   valueAreaComp,   "COMP",     "comp",       1);
    setupKnob (mixSlider,    valueAreaMix,    "COMP-MIX", "compmix",    0);
    setupKnob (phaseSlider,  valueAreaPhase,  "PHASE",    "phaseq",     0);
    setupKnob (clipSlider,   valueAreaClip,   "CLIP",     "clip",       0);

    // PHASE is a four-position switch: show and accept "1x".."4x" in the
    // inline editor (the parameter itself stays 0..3 for state compatibility)
    phaseSlider.textFromValueFunction = [] (double v)
    {
        return juce::String (juce::jlimit (1, 4, (int) std::lround (v) + 1)) + "x";
    };
    phaseSlider.valueFromTextFunction = [] (const juce::String& text)
    {
        const int n = text.retainCharacters ("0123456789").getIntValue();
        return (double) juce::jlimit (0, 3, n >= 1 && n <= 4 ? n - 1 : n);
    };

    // compact sidechain-filter knob, tucked into the COMP knob's corner
    hpfSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    hpfSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    hpfSlider.setLookAndFeel (&knobLAF);
    hpfSlider.setName ("HPF");
    hpfSlider.setNumDecimalPlacesToDisplay (0);
    hpfSlider.defaultValue = paramDefault ("schpf");
    hpfSlider.setEditorFont (juce::Font (juce::FontOptions().withTypeface (tfSemiBold).withHeight (12.0f)));
    hpfSlider.onValueChange = [this] { repaint (hpfKnobArea.expanded (2)); repaint (valueAreaComp); };
    addAndMakeVisible (hpfSlider);
    knobLAF.compactSlider = &hpfSlider;

    setupLabel (inputLabel,  "INPUT");
    setupLabel (outputLabel, "OUTPUT");
    setupLabel (compLabel,   "COMP");
    setupLabel (mixLabel,    "COMP-MIX");
    setupLabel (phaseLabel,  "PHASE");
    setupLabel (clipLabel,   "CLIP");

    autoGainButton.font = switchFont;
    linkButton.font     = switchFont;
    invertButton.font   = switchFont;
    addAndMakeVisible (autoGainButton);
    addAndMakeVisible (linkButton);
    addAndMakeVisible (invertButton);

    inputAttachment   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "inputgain", inputSlider);
    outputAttachment  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "outputgain", outputSlider);
    compAttachment    = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "comp", compSlider);
    mixAttachment     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "compmix", mixSlider);
    phaseAttachment   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "phaseq", phaseSlider);
    clipAttachment    = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "clip", clipSlider);
    hpfAttachment     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "schpf", hpfSlider);

    autoGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, "autogain", autoGainButton);
    linkAttachment     = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, "link", linkButton);
    invertAttachment   = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, "invert", invertButton);

    startTimerHz (30);
    setSize (560, 470);
}

CrestwrightAudioProcessorEditor::~CrestwrightAudioProcessorEditor()
{
    stopTimer();
    compSlider.setLookAndFeel (nullptr);
    for (auto* s : { &inputSlider, &outputSlider, &mixSlider, &phaseSlider, &clipSlider, &hpfSlider })
        s->setLookAndFeel (nullptr);
}

//==============================================================================
void CrestwrightAudioProcessorEditor::paint (juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();

    juce::ColourGradient bg (bgTop, area.getCentreX(), 0.0f,
                             bgBottom, area.getCentreX(), area.getBottom(), false);
    g.setGradientFill (bg);
    g.fillAll();

    {
        const auto c = juce::Point<float> (getWidth() * 0.5f, 170.0f);
        juce::ColourGradient glow (accent.withAlpha (0.06f), c,
                                   accent.withAlpha (0.0f), c.translated (180.0f, 0.0f), true);
        g.setGradientFill (glow);
        g.fillRect (getLocalBounds());
    }

    g.setColour (juce::Colour (0xfff0e6d8));
    g.setFont (titleFont);
    g.drawText ("CRESTWRIGHT", getLocalBounds().removeFromTop (48),
                juce::Justification::centred, true);

    g.setColour (juce::Colour (0x22ffffff));
    g.drawHorizontalLine (48, 30.0f, (float) getWidth() - 30.0f);

    // ---- gain reduction meter -------------------------------------------
    g.setColour (textDim);
    g.setFont (tinyFont);
    g.drawText ("GAIN REDUCTION", meterLabelArea, juce::Justification::centred, true);

    auto r = meterArea.toFloat();
    g.setColour (juce::Colour (0xff1c1c21));
    g.fillRoundedRectangle (r, 3.0f);

    for (float db : { 3.0f, 6.0f, 12.0f, 20.0f })
    {
        const float x = r.getRight() - r.getWidth() * (db / 20.0f);
        g.setColour (juce::Colour (0x18ffffff));
        g.drawVerticalLine ((int) x, r.getY() + 1.0f, r.getBottom() - 1.0f);

        g.setColour (textDim);
        g.setFont (tinyFont);
        g.drawText (juce::String ((int) db),
                    juce::Rectangle<int> ((int) x - 12, meterTickArea.getY(),
                                          24, meterTickArea.getHeight()),
                    juce::Justification::centred, false);
    }

    const float prop = juce::jlimit (0.0f, 1.0f, meterDb / 20.0f);
    if (prop > 0.005f)
    {
        juce::Rectangle<float> bar (r.getRight() - r.getWidth() * prop, r.getY(),
                                    r.getWidth() * prop, r.getHeight());
        juce::ColourGradient grad (accent, bar.getX(), bar.getCentreY(),
                                   juce::Colour (0xffe07a4a), bar.getRight(), bar.getCentreY(), false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (bar, 3.0f);
    }

    // ---- knob value readouts --------------------------------------------
    g.setColour (juce::Colour (0xffe8e8ee));

    auto drawValue = [&] (const juce::String& text, juce::Rectangle<int> box, float height)
    {
        g.setFont (juce::Font (juce::FontOptions().withTypeface (tfSemiBold).withHeight (height)));
        g.drawText (text, box, juce::Justification::centred, true);
    };

    drawValue (juce::String (inputSlider.getValue(), 1) + " dB", valueAreaInput, 15.0f);
    drawValue (juce::String (outputSlider.getValue(), 1) + " dB", valueAreaOutput, 15.0f);

    const float m = juce::jlimit (0.0f, 100.0f, (float) compSlider.getValue()) / 100.0f;
    const int atk = juce::roundToInt (40.0f * std::pow (0.25f, m));
    const int rel = juce::roundToInt (200.0f * std::pow (0.4f, m));

    auto compValueArea = valueAreaComp;
    drawValue ("A " + juce::String (atk) + "ms  R " + juce::String (rel) + "ms",
               compValueArea.removeFromTop (14), 12.0f);

    g.setColour (textDim);
    g.setFont (juce::Font (juce::FontOptions().withTypeface (tfSemiBold).withHeight (9.5f)));
    g.drawText ("HPF " + juce::String (juce::roundToInt (hpfSlider.getValue())) + " Hz",
                compValueArea, juce::Justification::centred, false);

    drawValue (juce::String ((int) mixSlider.getValue()), valueAreaMix, 19.0f);

    const int depth = juce::jlimit (1, 4, 1 + (int) std::lround ((double) phaseSlider.getValue()));
    drawValue (juce::String (depth) + "x", valueAreaPhase, 19.0f);

    drawValue (clipSlider.getValue() < 0.5 ? "OFF"
                                           : juce::String ((int) clipSlider.getValue()),
               valueAreaClip, 19.0f);
}

void CrestwrightAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    area.removeFromTop (54);

    meterLabelArea = area.removeFromTop (12);
    meterArea = area.removeFromTop (12).reduced (60, 0);
    meterTickArea = area.removeFromTop (10);

    area.removeFromTop (6);

    auto row1 = area.removeFromTop (158);
    auto row2 = area.removeFromTop (158);
    area.removeFromTop (6);
    switchesArea = area.removeFromTop (30);

    auto placeColumn = [] (juce::Rectangle<int> col, juce::Label& lab,
                           juce::Slider& s, juce::Rectangle<int>& va) -> juce::Rectangle<int>
    {
        lab.setBounds (col.removeFromTop (18));
        auto knob = col.removeFromTop (116).withSizeKeepingCentre (116, 116);
        s.setBounds (knob);
        va = col.removeFromTop (24);
        return knob;
    };

    const int colW = getWidth() / 3;

    auto c = row1;
    placeColumn (c.removeFromLeft (colW), inputLabel,  inputSlider,  valueAreaInput);
    auto compKnob = placeColumn (c.removeFromLeft (colW), compLabel, compSlider, valueAreaComp);
    placeColumn (c,                       phaseLabel,  phaseSlider,  valueAreaPhase);

    c = row2;
    placeColumn (c.removeFromLeft (colW), outputLabel, outputSlider, valueAreaOutput);
    placeColumn (c.removeFromLeft (colW), mixLabel,    mixSlider,    valueAreaMix);
    placeColumn (c,                       clipLabel,   clipSlider,   valueAreaClip);

    // small sidechain-HPF knob in the COMP knob's bottom-right corner
    hpfKnobArea = juce::Rectangle<int> (48, 48)
                      .withPosition (compKnob.getRight() - 40, compKnob.getBottom() - 48);
    hpfSlider.setBounds (hpfKnobArea);

    auto s = switchesArea;
    linkButton.setBounds     (s.removeFromLeft (colW).reduced (6, 0));
    autoGainButton.setBounds (s.removeFromLeft (colW).reduced (6, 0));
    invertButton.setBounds   (s.reduced (6, 0));
}

void CrestwrightAudioProcessorEditor::mouseDown (const juce::MouseEvent& e)
{
    for (auto* k : { &inputSlider, &outputSlider, &compSlider, &mixSlider,
                     &phaseSlider, &clipSlider, &hpfSlider })
    {
        if (k->hasOpenEditor() && ! k->ownsEditorComponent (e.originalComponent))
            k->closeEditor (true);
    }
}

void CrestwrightAudioProcessorEditor::timerCallback()
{
    const float target = juce::jlimit (0.0f, 20.0f, processor.grDbOut.load());
    meterDb = target > meterDb ? target : juce::jmax (target, meterDb * 0.90f);
    repaint (meterArea.expanded (0, 35));
}
