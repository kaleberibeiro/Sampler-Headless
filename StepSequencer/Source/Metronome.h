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

    juce::AudioFormatManager mAudioFormatManager;
    std::unique_ptr<juce::AudioFormatReaderSource> pMetronomeSample{nullptr};

};
