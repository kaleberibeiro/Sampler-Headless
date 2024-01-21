#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include <atomic>

class Metronome : public juce::AudioSourcePlayer
{
public:
    Metronome(juce::AudioDeviceManager& deviceManager);
    ~Metronome();

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate);
    void getNextAudioBlock(juce::AudioSourceChannelInfo& bufferToFill);
    void playAudio();
    void playAudioOneTime(juce::AudioSourceChannelInfo& bufferToFill);
    void reset();
    void audioStoped();

private:
    int mTotalSamples{0};
    double mSampleRate{0};
    int mUpdateInterval{0};
    double mBpm{140.0};
    int mSamplesRemaining{0};

    juce::AudioDeviceManager& devmgr;
    juce::AudioFormatManager mAudioFormatManager;
    std::unique_ptr<juce::AudioFormatReaderSource> pMetronomeSample{nullptr};
    std::unique_ptr<juce::AudioSourcePlayer> player;

};
