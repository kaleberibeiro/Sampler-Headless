#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include <atomic>

class Metronome
{
public:
    Metronome();
    // ~Metronome();

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate);
    void getNextAudioBlock(juce::AudioSourceChannelInfo& bufferToFill);

private:
    int mTotalSamples{0};
    double mSampleRate{0};
    int mUpdateInterval{0};
    double mBpm{140.0};
    int mSamplesRemaining{0};
    int currentSample1index{0};
    int currentSample2index{0};

    juce::MixerAudioSource mixerAudioSource;
    juce::IIRFilterAudioSource* iirFilterAudioSource1;
    juce::IIRFilterAudioSource* iirFilterAudioSource2;
    juce::ReverbAudioSource* reverbAudioSource1;
    juce::ReverbAudioSource* reverbAudioSource2;
    juce::AudioFormatManager mAudioFormatManager;
    std::unique_ptr<juce::AudioFormatReaderSource> pSample1{nullptr};
    std::unique_ptr<juce::AudioFormatReaderSource> pSample2{nullptr};
    std::vector<int> sample1Sequence;
    std::vector<int> sample2Sequence;   
};
