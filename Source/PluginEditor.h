#pragma once

#include "PluginProcessor.h"
#include <deque>

class VoxRideLookAndFeel : public juce::LookAndFeel_V4
{
public:
    VoxRideLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider&) override;
};

class VoxRideAudioProcessorEditor : public juce::AudioProcessorEditor,
                                    private juce::Timer
{
public:
    explicit VoxRideAudioProcessorEditor (VoxRideAudioProcessor&);
    ~VoxRideAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void configureKnob (juce::Slider& s, juce::Label& l, const juce::String& name, const juce::String& suffix);

    VoxRideAudioProcessor& processorRef;
    VoxRideLookAndFeel lookAndFeel;

    juce::Slider targetSlider, vocalSensSlider, musicSensSlider;
    juce::Slider minRangeSlider, maxRangeSlider, outputSlider;
    juce::Slider hpfSlider, deMudSlider, presenceSlider, deEssSlider, gateFloorSlider;

    juce::Label targetLabel, vocalSensLabel, musicSensLabel, minLabel, maxLabel, outputLabel;
    juce::Label hpfLabel, deMudLabel, presenceLabel, deEssLabel, gateFloorLabel;

    juce::ComboBox speedBox;
    juce::ToggleButton clarityButton { "Clarity In" };
    juce::ToggleButton idleReturnButton { "Idle Return" };
    juce::ToggleButton limiterButton { "Limiter" };
    juce::ToggleButton bypassButton { "Bypass" };

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<SliderAttachment> targetAtt, vocalSensAtt, musicSensAtt;
    std::unique_ptr<SliderAttachment> minRangeAtt, maxRangeAtt, outputAtt;
    std::unique_ptr<SliderAttachment> hpfAtt, deMudAtt, presenceAtt, deEssAtt, gateAtt;
    std::unique_ptr<ComboAttachment>  speedAtt;
    std::unique_ptr<ButtonAttachment> clarityAtt, idleAtt, limiterAtt, bypassAtt;

    std::deque<float> gainHistory;
    float uiInputDb = -96.0f;
    float uiOutputDb = -96.0f;
    float uiRiderGainDb = 0.0f;
    bool  uiVocalActive = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoxRideAudioProcessorEditor)
};
