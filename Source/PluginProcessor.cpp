#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
#include <algorithm>

namespace
{
    constexpr float kMinDb = -96.0f;

    inline float dbToGain (float db)
    {
        return db <= kMinDb ? 0.0f : std::pow (10.0f, db / 20.0f);
    }

    inline float gainToDb (float g)
    {
        return g <= 1e-5f ? kMinDb : std::max (kMinDb, 20.0f * std::log10 (g));
    }
}

VoxRideAudioProcessor::VoxRideAudioProcessor()
    : AudioProcessor (BusesProperties()
          .withInput  ("Input",     juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output",    juce::AudioChannelSet::stereo(), true)
          .withInput  ("Sidechain", juce::AudioChannelSet::stereo(), false)),
      apvts (*this, nullptr, "VOXRIDE_PARAMS", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout VoxRideAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

        p.push_back (std::make_unique<juce::AudioParameterFloat> ("targetDb",       "Target Level",      juce::NormalisableRange<float> (-40.0f, 0.0f, 0.1f),   -18.0f));
    // ... (baris 35 biarkan jika sudah ada .0f)
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("musicSens",      "Music Sensitivity", juce::NormalisableRange<float> (-10.0f, 10.0f, 0.1f),  2.0f));
    // ... (baris 37 biarkan jika sudah ada .0f)
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("minRangeDb",     "Min Range",         juce::NormalisableRange<float> (-12.0f, 0.0f, 0.1f),   -6.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("maxRangeDb",     "Max Range",         juce::NormalisableRange<float> (0.0f, 12.0f, 0.1f),    6.0f));
    // ... (baris 40 biarkan jika sudah ada .0f)
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("gateFloorDb",    "Breath Guard",      juce::NormalisableRange<float> (-60.0f, -20.0f, 0.5f), -40.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("outputDb",       "Output Trim",       juce::NormalisableRange<float> (-18.0f, 12.0f, 0.1f),  0.0f));
    // ... (baris 43 biarkan jika sudah ada .0f)
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("hpfHz",          "High-Pass Hz",      juce::NormalisableRange<float> (20.0f, 200.0f, 1.0f),  85.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("deMudPct",       "De-Mud %",          juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f),   30.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("presencePct",    "Presence / Air %",  juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f),   38.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("deEssPct",       "De-Esser %",        juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f),   32.0f));
    p.push_back (std::make_unique<juce::AudioParameterBool>  ("limiterEnabled", "True-Peak Limiter", true));
    p.push_back (std::make_unique<juce::AudioParameterBool>  ("bypass",         "Bypass",            false));

    return { p.begin(), p.end() };
}

bool VoxRideAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto mainIn = layouts.getMainInputChannelSet();
    const auto mainOut = layouts.getMainOutputChannelSet();
    if (mainOut != juce::AudioChannelSet::mono() && mainOut != juce::AudioChannelSet::stereo())
        return false;
    if (mainIn != mainOut)
        return false;
    return true;
}

void VoxRideAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = std::max (8000.0, sampleRate);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = currentSampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32> (getTotalNumOutputChannels());

    hpfFilter.prepare (spec);
    deMudFilter.prepare (spec);
    presenceFilter.prepare (spec);
    airFilter.prepare (spec);
    deEssFilter.prepare (spec);

    sibDetectorFilter.reset();
    sibDetectorFilter.coefficients = juce::dsp::IIR::Coefficients<float>::makeBandPass (currentSampleRate, 6500.0f, 2.4f);

    safetyLimiter.prepare (spec);
    safetyLimiter.setThreshold (-0.3f);
    safetyLimiter.setRelease (50.0f);

    currentGainDb = 0.0f;
    smoothedInputDb = kMinDb;
    smoothedMusicDb = kMinDb;
    holdTimerSec = 0.0f;
    isVocalActive = false;
    deEssReductionDb = 0.0f;
}

void VoxRideAudioProcessor::updateClarityFilters (float deEssCutDb)
{
    const bool clarityOn = apvts.getRawParameterValue ("clarityEnabled")->load() > 0.5f;
    const float hpfHz    = clarityOn ? apvts.getRawParameterValue ("hpfHz")->load() : 10.0f;
    const float deMudDb  = clarityOn ? -(apvts.getRawParameterValue ("deMudPct")->load() / 100.0f) * 5.5f : 0.0f;
    const float presPct  = clarityOn ? apvts.getRawParameterValue ("presencePct")->load() / 100.0f : 0.0f;
    const float presDb   = presPct * 5.0f;
    const float airDb    = presPct * 3.2f;

    *hpfFilter.state      = *juce::dsp::IIR::Coefficients<float>::makeHighPass  (currentSampleRate, std::clamp (hpfHz, 10.0f, 220.0f), 0.707f);
    *deMudFilter.state    = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(currentSampleRate, 280.0f,   1.10f, dbToGain (deMudDb));
    *presenceFilter.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(currentSampleRate, 3200.0f,  1.15f, dbToGain (presDb));
    *airFilter.state      = *juce::dsp::IIR::Coefficients<float>::makeHighShelf (currentSampleRate, 10500.0f, 0.707f, dbToGain (airDb));
    *deEssFilter.state    = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(currentSampleRate, 6500.0f,  2.00f, dbToGain (deEssCutDb));
}

void VoxRideAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    auto mainBus = getBusBuffer (buffer, true, 0);
    auto scBus   = getBusCount (true) > 1 ? getBusBuffer (buffer, true, 1) : juce::AudioBuffer<float>();

    const int numSamples  = mainBus.getNumSamples();
    const int numChannels = mainBus.getNumChannels();
    if (numSamples == 0 || numChannels == 0)
        return;

    // 1. Measure Input Vocal RMS & 6.5 kHz Sibilance RMS
    double vocalSumSq = 0.0;
    double sibSumSq   = 0.0;
    for (int i = 0; i < numSamples; ++i)
    {
        float mono = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
            mono += mainBus.getSample (ch, i);
        mono /= static_cast<float> (numChannels);

        vocalSumSq += static_cast<double> (mono) * mono;
        const float sib = sibDetectorFilter.processSample (mono);
        sibSumSq += static_cast<double> (sib) * sib;
    }

    const float inputRmsDb = gainToDb (std::sqrt (static_cast<float> (vocalSumSq / numSamples)));
    const float sibRmsDb   = gainToDb (std::sqrt (static_cast<float> (sibSumSq / numSamples)));
    meterInputRmsDb.store (inputRmsDb, std::memory_order_relaxed);

    // 2. Measure Optional Sidechain Music RMS
    float musicRmsDb = kMinDb;
    if (scBus.getNumChannels() > 0 && scBus.getNumSamples() == numSamples)
    {
        double mSumSq = 0.0;
        for (int i = 0; i < numSamples; ++i)
        {
            float m = 0.0f;
            for (int ch = 0; ch < scBus.getNumChannels(); ++ch)
                m += scBus.getSample (ch, i);
            m /= static_cast<float> (scBus.getNumChannels());
            mSumSq += static_cast<double> (m) * m;
        }
        musicRmsDb = gainToDb (std::sqrt (static_cast<float> (mSumSq / numSamples)));
    }
    meterMusicRmsDb.store (musicRmsDb, std::memory_order_relaxed);

    // True Bypass check
    if (apvts.getRawParameterValue ("bypass")->load() > 0.5f)
    {
        meterRiderGainDb.store (0.0f, std::memory_order_relaxed);
        meterOutputRmsDb.store (inputRmsDb, std::memory_order_relaxed);
        meterVocalActive.store (false, std::memory_order_relaxed);
        return;
    }

    // 3. Step Real-Time Vocal Rider Ballistics
    const float dt = std::clamp (static_cast<float> (numSamples / currentSampleRate), 0.001f, 0.1f);
    const float targetDb    = apvts.getRawParameterValue ("targetDb")->load();
    const float vocalSens   = apvts.getRawParameterValue ("vocalSens")->load();
    const float musicSens   = apvts.getRawParameterValue ("musicSens")->load();
    const bool  isFast      = apvts.getRawParameterValue ("speed")->load() < 0.5f;
    const float minRangeDb  = apvts.getRawParameterValue ("minRangeDb")->load();
    const float maxRangeDb  = apvts.getRawParameterValue ("maxRangeDb")->load();
    const bool  idleReturn  = apvts.getRawParameterValue ("idleReturn")->load() > 0.5f;
    const float gateFloorDb = apvts.getRawParameterValue ("gateFloorDb")->load();
    const float outputDb    = apvts.getRawParameterValue ("outputDb")->load();
    const bool  clarityOn   = apvts.getRawParameterValue ("clarityEnabled")->load() > 0.5f;
    const float deEssPct    = apvts.getRawParameterValue ("deEssPct")->load();
    const bool  limiterOn   = apvts.getRawParameterValue ("limiterEnabled")->load() > 0.5f;

    const float detTau   = isFast ? 0.014f : 0.045f;
    const float detAlpha = 1.0f - std::exp (-dt / detTau);
    smoothedInputDb = (smoothedInputDb <= kMinDb + 1.0f) ? inputRmsDb : smoothedInputDb + (inputRmsDb - smoothedInputDb) * detAlpha;

    const float musAlpha = 1.0f - std::exp (-dt / 0.08f);
    smoothedMusicDb = (smoothedMusicDb <= kMinDb + 1.0f) ? musicRmsDb : smoothedMusicDb + (musicRmsDb - smoothedMusicDb) * musAlpha;

    float musicOffsetDb = 0.0f;
    if (musicRmsDb > -60.0f && std::abs (musicSens) > 0.01f)
    {
        const float musicDelta = std::clamp (smoothedMusicDb - (-24.0f), -12.0f, 12.0f);
        musicOffsetDb = std::clamp (musicDelta * (musicSens / 10.0f) * 0.5f, -6.0f, 6.0f);
    }
    const float effectiveTargetDb = std::clamp (targetDb + musicOffsetDb, -42.0f, 0.0f);

    const float effectiveFloorDb = std::clamp (gateFloorDb - vocalSens * 0.65f, -68.0f, -14.0f);
    const float holdDurationSec  = isFast ? 0.095f : 0.18f;

    if (smoothedInputDb >= effectiveFloorDb)
    {
        isVocalActive = true;
        holdTimerSec = holdDurationSec;
    }
    else if (isVocalActive)
    {
        if (smoothedInputDb >= effectiveFloorDb - 2.5f)
            holdTimerSec = holdDurationSec * 0.5f;
        else
        {
            holdTimerSec = std::max (0.0f, holdTimerSec - dt);
            if (holdTimerSec <= 0.0f)
                isVocalActive = false;
        }
    }

    float desiredGainDb = currentGainDb;
    if (isVocalActive)
    {
        const float trackingRatio = 1.0f + vocalSens * 0.025f;
        desiredGainDb = std::clamp ((effectiveTargetDb - smoothedInputDb) * trackingRatio, minRangeDb, maxRangeDb);
    }
    else if (idleReturn)
    {
        desiredGainDb = 0.0f;
    }

    const float attackSec  = isFast ? 0.018f : 0.085f;
    const float releaseSec = isFast ? 0.065f : 0.260f;
    const float tau = !isVocalActive ? 0.35f : (desiredGainDb < currentGainDb ? attackSec : releaseSec);
    const float gainAlpha = 1.0f - std::exp (-dt / tau);
    const float prevGainLinear = dbToGain (currentGainDb + outputDb);
    currentGainDb += (desiredGainDb - currentGainDb) * gainAlpha;
    const float nextGainLinear = dbToGain (currentGainDb + outputDb);

    // Dynamic De-Esser attenuation
    float targetDeEssDb = 0.0f;
    if (clarityOn && deEssPct > 0.0f && sibRmsDb > -48.0f)
    {
        const float threshDb = -30.0f - (deEssPct / 100.0f) * 10.0f;
        const float excessDb = std::max (0.0f, sibRmsDb - threshDb);
        const float maxCut   = (deEssPct / 100.0f) * 8.5f;
        targetDeEssDb = -std::min (maxCut, excessDb * 0.75f);
    }
    const float deEssTau = targetDeEssDb < deEssReductionDb ? 0.008f : 0.060f;
    deEssReductionDb += (targetDeEssDb - deEssReductionDb) * (1.0f - std::exp (-dt / deEssTau));

    // 4. Process Clarity EQ Chain
    updateClarityFilters (deEssReductionDb);
    juce::dsp::AudioBlock<float> block (mainBus);
    juce::dsp::ProcessContextReplacing<float> ctx (block);

    if (clarityOn)
    {
        hpfFilter.process (ctx);
        deMudFilter.process (ctx);
        presenceFilter.process (ctx);
        airFilter.process (ctx);
        deEssFilter.process (ctx);
    }

    // 5. Apply Zipper-Free Ramped Rider + Output Trim Gain
    mainBus.applyGainRamp (0, numSamples, prevGainLinear, nextGainLinear);

    // 6. True-Peak Safety Limiter
    if (limiterOn)
        safetyLimiter.process (ctx);

    // 7. Measure Output RMS & Update UI Atomics
    double outSumSq = 0.0;
    for (int i = 0; i < numSamples; ++i)
    {
        float mono = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
            mono += mainBus.getSample (ch, i);
        mono /= static_cast<float> (numChannels);
        outSumSq += static_cast<double> (mono) * mono;
    }

    meterOutputRmsDb.store (gainToDb (std::sqrt (static_cast<float> (outSumSq / numSamples))), std::memory_order_relaxed);
    meterRiderGainDb.store (currentGainDb, std::memory_order_relaxed);
    meterDeEssGrDb.store (deEssReductionDb, std::memory_order_relaxed);
    meterVocalActive.store (isVocalActive, std::memory_order_relaxed);
}

juce::AudioProcessorEditor* VoxRideAudioProcessor::createEditor()
{
    return new VoxRideAudioProcessorEditor (*this);
}

void VoxRideAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void VoxRideAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VoxRideAudioProcessor();
}
