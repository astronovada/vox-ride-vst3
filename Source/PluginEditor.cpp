#include "PluginEditor.h"

VoxRideLookAndFeel::VoxRideLookAndFeel()
{
    setColour (juce::Slider::thumbColourId, juce::Colour (0xfff59e0b));
    setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xfff59e0b));
    setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff272e3a));
    setColour (juce::Label::textColourId, juce::Colour (0xffcbd5e1));
}

void VoxRideLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                           float sliderPos, float startAngle, float endAngle, juce::Slider&)
{
    const auto bounds = juce::Rectangle<float> (static_cast<float> (x), static_cast<float> (y),
                                                static_cast<float> (width), static_cast<float> (height)).reduced (4.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const float angle = startAngle + sliderPos * (endAngle - startAngle);

    juce::Path bgArc;
    bgArc.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, startAngle, endAngle, true);
    g.setColour (juce::Colour (0xff272e3a));
    g.strokePath (bgArc, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path valArc;
    valArc.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, startAngle, angle, true);
    g.setColour (juce::Colour (0xfff59e0b));
    g.strokePath (valArc, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    g.setColour (juce::Colour (0xff1d222b));
    g.fillEllipse (centre.x - radius + 5.0f, centre.y - radius + 5.0f, (radius - 5.0f) * 2.0f, (radius - 5.0f) * 2.0f);

    juce::Path pointer;
    pointer.addRoundedRectangle (-1.5f, -radius + 7.0f, 3.0f, radius * 0.42f, 1.0f);
    g.setColour (juce::Colour (0xfff59e0b));
    g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));
}

VoxRideAudioProcessorEditor::VoxRideAudioProcessorEditor (VoxRideAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    setLookAndFeel (&lookAndFeel);
    setSize (920, 540);

    configureKnob (targetSlider,    targetLabel,    "Target",     " dB");
    configureKnob (vocalSensSlider, vocalSensLabel, "Vocal Sens", "");
    configureKnob (musicSensSlider, musicSensLabel, "Music Sens", "");
    configureKnob (minRangeSlider,  minLabel,       "Min Range",  " dB");
    configureKnob (maxRangeSlider,  maxLabel,       "Max Range",  " dB");
    configureKnob (outputSlider,    outputLabel,    "Output",     " dB");
    configureKnob (hpfSlider,       hpfLabel,       "High-Pass",  " Hz");
    configureKnob (deMudSlider,     deMudLabel,     "De-Mud",     " %");
    configureKnob (presenceSlider,  presenceLabel,  "Presence",   " %");
    configureKnob (deEssSlider,     deEssLabel,     "De-Esser",   " %");
    configureKnob (gateFloorSlider, gateFloorLabel, "Breath Grd", " dB");

    speedBox.addItem ("Fast", 1);
    speedBox.addItem ("Slow", 2);
    addAndMakeVisible (speedBox);

    addAndMakeVisible (clarityButton);
    addAndMakeVisible (idleReturnButton);
    addAndMakeVisible (limiterButton);
    addAndMakeVisible (bypassButton);

    auto& v = processorRef.apvts;
    targetAtt    = std::make_unique<SliderAttachment> (v, "targetDb",       targetSlider);
    vocalSensAtt = std::make_unique<SliderAttachment> (v, "vocalSens",      vocalSensSlider);
    musicSensAtt = std::make_unique<SliderAttachment> (v, "musicSens",      musicSensSlider);
    minRangeAtt  = std::make_unique<SliderAttachment> (v, "minRangeDb",     minRangeSlider);
    maxRangeAtt  = std::make_unique<SliderAttachment> (v, "maxRangeDb",     maxRangeSlider);
    outputAtt    = std::make_unique<SliderAttachment> (v, "outputDb",       outputSlider);
    hpfAtt       = std::make_unique<SliderAttachment> (v, "hpfHz",          hpfSlider);
    deMudAtt     = std::make_unique<SliderAttachment> (v, "deMudPct",       deMudSlider);
    presenceAtt  = std::make_unique<SliderAttachment> (v, "presencePct",    presenceSlider);
    deEssAtt     = std::make_unique<SliderAttachment> (v, "deEssPct",       deEssSlider);
    gateAtt      = std::make_unique<SliderAttachment> (v, "gateFloorDb",    gateFloorSlider);
    speedAtt     = std::make_unique<ComboAttachment>  (v, "speed",          speedBox);
    clarityAtt   = std::make_unique<ButtonAttachment> (v, "clarityEnabled", clarityButton);
    idleAtt      = std::make_unique<ButtonAttachment> (v, "idleReturn",     idleReturnButton);
    limiterAtt   = std::make_unique<ButtonAttachment> (v, "limiterEnabled", limiterButton);
    bypassAtt    = std::make_unique<ButtonAttachment> (v, "bypass",         bypassButton);

    startTimerHz (30);
}

VoxRideAudioProcessorEditor::~VoxRideAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void VoxRideAudioProcessorEditor::configureKnob (juce::Slider& s, juce::Label& l, const juce::String& name, const juce::String& suffix)
{
    s.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 64, 18);
    s.setTextValueSuffix (suffix);
    addAndMakeVisible (s);

    l.setText (name, juce::dontSendNotification);
    l.setJustificationType (juce::Justification::centred);
    l.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    addAndMakeVisible (l);
}

void VoxRideAudioProcessorEditor::timerCallback()
{
    uiInputDb     = processorRef.meterInputRmsDb.load (std::memory_order_relaxed);
    uiOutputDb    = processorRef.meterOutputRmsDb.load (std::memory_order_relaxed);
    uiRiderGainDb = processorRef.meterRiderGainDb.load (std::memory_order_relaxed);
    uiVocalActive = processorRef.meterVocalActive.load (std::memory_order_relaxed);

    gainHistory.push_back (uiRiderGainDb);
    if (gainHistory.size() > 160)
        gainHistory.pop_front();

    repaint();
}

void VoxRideAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff0e1116));

    // Hardware Faceplate Header
    g.setColour (juce::Colour (0xff171b23));
    g.fillRoundedRectangle (12.0f, 12.0f, getWidth() - 24.0f, 44.0f, 8.0f);
    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (19.0f, juce::Font::bold));
    g.drawText ("VoxRide  |  Automatic Vocal Rider & Clarity", 24, 18, 420, 32, juce::Justification::centredLeft);

    // Central Motorized Rider Fader Visualization
    const juce::Rectangle<float> faderBox (330.0f, 76.0f, 150.0f, 280.0f);
    g.setColour (juce::Colour (0xff12161d));
    g.fillRoundedRectangle (faderBox, 10.0f);
    g.setColour (juce::Colour (0xfff59e0b).withAlpha (0.35f));
    g.drawRoundedRectangle (faderBox, 10.0f, 1.0f);

    const float trackX = faderBox.getCentreX();
    const float trackTop = faderBox.getY() + 30.0f;
    const float trackH = faderBox.getHeight() - 78.0f;
    const float norm = juce::jlimit (0.0f, 1.0f, (uiRiderGainDb + 12.0f) / 24.0f);
    const float capY = trackTop + (1.0f - norm) * trackH;

    g.setColour (juce::Colour (0xff07090d));
    g.fillRoundedRectangle (trackX - 5.0f, trackTop, 10.0f, trackH, 4.0f);

    const float midY = trackTop + trackH * 0.5f;
    g.setColour (uiRiderGainDb >= 0.0f ? juce::Colour (0xfff59e0b) : juce::Colour (0xff22d3ee));
    g.fillRect (trackX - 4.0f, std::min (midY, capY), 8.0f, std::abs (capY - midY));

    // Machined Fader Cap
    g.setColour (juce::Colour (0xff313844));
    g.fillRoundedRectangle (trackX - 24.0f, capY - 12.0f, 48.0f, 24.0f, 3.0f);
    g.setColour (juce::Colour (0xfff59e0b));
    g.fillRect (trackX - 18.0f, capY - 1.0f, 36.0f, 2.0f);

    g.setColour (juce::Colour (0xfff59e0b));
    g.setFont (juce::FontOptions (15.0f, juce::Font::bold));
    g.drawText (juce::String (uiRiderGainDb >= 0.0f ? "+" : "") + juce::String (uiRiderGainDb, 1) + " dB",
                static_cast<int> (faderBox.getX()), static_cast<int> (faderBox.getBottom() - 34),
                static_cast<int> (faderBox.getWidth()), 24, juce::Justification::centred);

    // Bottom Gain Ride Scope
    const juce::Rectangle<float> scope (16.0f, 384.0f, getWidth() - 32.0f, 140.0f);
    g.setColour (juce::Colour (0xff080a0e));
    g.fillRoundedRectangle (scope, 8.0f);
    g.setColour (juce::Colour (0xff222834));
    g.drawRoundedRectangle (scope, 8.0f, 1.0f);

    g.setColour (juce::Colour (0xfff59e0b).withAlpha (0.25f));
    g.drawHorizontalLine (static_cast<int> (scope.getCentreY()), scope.getX() + 6.0f, scope.getRight() - 6.0f);

    if (gainHistory.size() > 1)
    {
        juce::Path trace;
        for (size_t i = 0; i < gainHistory.size(); ++i)
        {
            const float px = scope.getX() + 8.0f + (static_cast<float> (i) / 159.0f) * (scope.getWidth() - 16.0f);
            const float py = scope.getCentreY() - (juce::jlimit (-12.0f, 12.0f, gainHistory[i]) / 12.0f) * (scope.getHeight() * 0.42f);
            if (i == 0) trace.startNewSubPath (px, py);
            else trace.lineTo (px, py);
        }
        g.setColour (juce::Colour (0xfff59e0b));
        g.strokePath (trace, juce::PathStrokeType (2.0f));
    }
}

void VoxRideAudioProcessorEditor::resized()
{
    bypassButton.setBounds (getWidth() - 110, 20, 86, 28);
    limiterButton.setBounds (getWidth() - 210, 20, 92, 28);
    clarityButton.setBounds (getWidth() - 320, 20, 102, 28);

    auto placeKnob = [] (juce::Slider& s, juce::Label& l, int x, int y) {
        l.setBounds (x, y, 90, 18);
        s.setBounds (x + 6, y + 18, 78, 86);
    };

    // Left: Target & Sensitivity
    placeKnob (targetSlider,    targetLabel,    20,  76);
    placeKnob (vocalSensSlider, vocalSensLabel, 120, 76);
    placeKnob (musicSensSlider, musicSensLabel, 220, 76);
    placeKnob (minRangeSlider,  minLabel,       20,  200);
    placeKnob (maxRangeSlider,  maxLabel,       120, 200);
    speedBox.setBounds (224, 222, 86, 28);
    idleReturnButton.setBounds (220, 260, 96, 26);

    // Right: Vocal Clarity & Output
    placeKnob (hpfSlider,       hpfLabel,       500, 76);
    placeKnob (deMudSlider,     deMudLabel,     600, 76);
    placeKnob (presenceSlider,  presenceLabel,  700, 76);
    placeKnob (deEssSlider,     deEssLabel,     800, 76);
    placeKnob (gateFloorSlider, gateFloorLabel, 550, 200);
    placeKnob (outputSlider,    outputLabel,    740, 200);
}
