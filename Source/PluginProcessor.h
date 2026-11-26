#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>

class VoxRideAudioProcessor : public juce::AudioProcessor
{
public:
    VoxRideAudioProcessor();
    ~VoxRideAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "VoxRide"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // Lock-free telemetry for the hardware UI editor (60 Hz)
    std::atomic<float> meterInputRmsDb  { -96.0f };
    std::atomic<float> meterOutputRmsDb { -96.0f };
    std::atomic<float> meterMusicRmsDb  { -96.0f };
    std::atomic<float> meterRiderGainDb { 0.0f };
    std::atomic<float> meterDeEssGrDb   { 0.0f };
    std::atomic<bool>  meterVocalActive { false };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void updateClarityFilters (float deEssCutDb);

    double currentSampleRate = 44100.0;

    // Vocal Rider DSP state
    float currentGainDb = 0.0f;
    float smoothedInputDb = -96.0f;
    float smoothedMusicDb = -96.0f;
    float holdTimerSec = 0.0f;
    bool  isVocalActive = false;
    float deEssReductionDb = 0.0f;

    // Clarity DSP chain (stereo)
    using IIRFilter = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>;
    IIRFilter hpfFilter;
    IIRFilter deMudFilter;
    IIRFilter presenceFilter;
    IIRFilter airFilter;
    IIRFilter deEssFilter;
    juce::dsp::IIR::Filter<float> sibDetectorFilter;

    juce::dsp::Limiter<float> safetyLimiter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoxRideAudioProcessor)
};
