// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (c) 2026 Linning

#pragma once

#include "PluginProcessor.h"

class CrestwrightAudioProcessorEditor : public juce::AudioProcessorEditor,
                                      private juce::Timer
{
public:
    explicit CrestwrightAudioProcessorEditor (CrestwrightAudioProcessor&);
    ~CrestwrightAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    struct KnobLAF : public juce::LookAndFeel_V4
    {
        void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                               float pos, float startAngle, float endAngle,
                               juce::Slider&) override;

        // optional detent marks for a stepped knob (the PHASE switch)
        juce::Slider* detentSlider = nullptr;
        int detentCount = 0;

        // the small sidechain-HPF knob skips the tick ring to stay readable
        juce::Slider* compactSlider = nullptr;
    };

    // A rotary knob with the interaction details of a hardware-style control:
    //  - double click  -> an inline text box opens over the knob
    //  - ctrl+click    -> back to the parameter default
    //  - shift + drag  -> fine movement (the wheel is already fine with shift)
    struct KnobSlider : public juce::Slider
    {
        double defaultValue = 0.0;
        juce::Font editorFont { juce::FontOptions (13.0f) };
        std::unique_ptr<juce::TextEditor> inlineEditor;
        bool closingEditor = false;

        void setEditorFont (juce::Font f) { editorFont = std::move (f); }

        bool hasOpenEditor() const
        {
            return inlineEditor != nullptr && inlineEditor->isVisible();
        }

        bool ownsEditorComponent (juce::Component* c) const
        {
            return inlineEditor != nullptr
                && (c == inlineEditor.get() || (c != nullptr && inlineEditor->isParentOf (c)));
        }

        void showEditor()
        {
            if (inlineEditor == nullptr)
            {
                inlineEditor = std::make_unique<juce::TextEditor>();
                inlineEditor->setJustification (juce::Justification::centred);
                inlineEditor->setFont (editorFont);
                inlineEditor->setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff14141a));
                inlineEditor->setColour (juce::TextEditor::textColourId, juce::Colour (0xfff0f0f5));
                inlineEditor->setColour (juce::TextEditor::outlineColourId, juce::Colour (0x66dc9a46));
                inlineEditor->setColour (juce::TextEditor::focusedOutlineColourId, juce::Colour (0xffdc9a46));
                inlineEditor->setColour (juce::TextEditor::highlightColourId, juce::Colour (0x55dc9a46));
                inlineEditor->setColour (juce::TextEditor::highlightedTextColourId, juce::Colours::white);
                inlineEditor->setBorder (juce::BorderSize<int> (2));
                inlineEditor->setIndents (4, 0);
                inlineEditor->onReturnKey = [this] { closeEditor (true); };
                inlineEditor->onEscapeKey = [this] { closeEditor (false); };
                inlineEditor->onFocusLost  = [this] { closeEditor (true); };
                addAndMakeVisible (*inlineEditor);
            }

            inlineEditor->setText (getTextFromValue (getValue()), juce::dontSendNotification);

            const int w = juce::jmin (getWidth() - 6, 84);
            const int h = juce::jlimit (18, 24, getHeight() / 4);
            inlineEditor->setBounds (getLocalBounds().withSizeKeepingCentre (w, h));
            inlineEditor->setVisible (true);
            inlineEditor->selectAll();
            inlineEditor->grabKeyboardFocus();
        }

        void closeEditor (bool apply)
        {
            if (inlineEditor == nullptr || ! inlineEditor->isVisible() || closingEditor)
                return;

            closingEditor = true;

            if (apply)
                setValue (getValueFromText (inlineEditor->getText()), juce::sendNotificationSync);

            inlineEditor->setVisible (false);
            closingEditor = false;
        }

        void mouseDown (const juce::MouseEvent& e) override
        {
            if (hasOpenEditor())
                closeEditor (true);

            if (e.mods.isLeftButtonDown() && e.mods.isCtrlDown())
            {
                setValue (defaultValue, juce::sendNotificationSync);
                return;
            }

            if (e.mods.isShiftDown())
                setMouseDragSensitivity (1000);

            juce::Slider::mouseDown (e);
        }

        void mouseUp (const juce::MouseEvent& e) override
        {
            setMouseDragSensitivity (250);
            juce::Slider::mouseUp (e);
        }

        void mouseDoubleClick (const juce::MouseEvent&) override
        {
            showEditor();
        }

        void mouseWheelMove (const juce::MouseEvent& e,
                             const juce::MouseWheelDetails& wheel) override
        {
            if (e.mods.isShiftDown() && wheel.deltaY != 0.0f)
            {
                const double range = getMaximum() - getMinimum();
                const double step  = getInterval() > 0.0
                                         ? getInterval()
                                         : juce::jmax (range / 1000.0, 0.0001);

                setValue (getValue() + (wheel.deltaY > 0.0f ? step : -step),
                          juce::sendNotificationSync);
                return;
            }

            juce::Slider::mouseWheelMove (e, wheel);
        }
    };

    struct ToggleSwitch : public juce::Button
    {
        explicit ToggleSwitch (const juce::String& text)
            : Button (text), label (text) { setClickingTogglesState (true); }

        void paintButton (juce::Graphics&, bool isMouseOver, bool isMouseDown) override;

        juce::String label;
        juce::Font font { juce::FontOptions (12.0f) };
    };

    void timerCallback() override;

    CrestwrightAudioProcessor& processor;

    juce::Typeface::Ptr tfRegular, tfMedium, tfSemiBold, tfDisplay;
    juce::Font titleFont, labelFont, valueFont, tinyFont, switchFont;

    KnobLAF knobLAF;
    KnobSlider inputSlider, outputSlider, compSlider, mixSlider, phaseSlider, clipSlider;
    KnobSlider hpfSlider;   // small sidechain-filter knob at the COMP corner
    juce::Label  inputLabel, outputLabel, compLabel, mixLabel, phaseLabel, clipLabel;

    ToggleSwitch autoGainButton { "AUTO GAIN" };
    ToggleSwitch linkButton     { "LINK" };
    ToggleSwitch invertButton   { "PHASE INVERT" };

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        inputAttachment, outputAttachment, compAttachment, mixAttachment, phaseAttachment, clipAttachment,
        hpfAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
        autoGainAttachment, linkAttachment, invertAttachment;

    juce::Rectangle<int> meterArea, meterLabelArea, meterTickArea;
    juce::Rectangle<int> valueAreaInput, valueAreaOutput, valueAreaComp,
                         valueAreaMix, valueAreaPhase, valueAreaClip;
    juce::Rectangle<int> hpfKnobArea;
    juce::Rectangle<int> switchesArea;
    float meterDb = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CrestwrightAudioProcessorEditor)
};
