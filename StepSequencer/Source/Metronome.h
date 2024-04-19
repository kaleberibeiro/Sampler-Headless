#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include <atomic>

class Metronome
{
public:
    Metronome(juce::Synthesiser *mySynth) : mySynth(mySynth)
    {
    }
    // ~Metronome();

    void countSamples(juce::AudioBuffer<float> &buffer, int startSample, int numSamples);
    void prepareToPlay(int samplesPerBlockExpected, double sampleRate);
    void getNextAudioBlock(juce::AudioSourceChannelInfo &bufferToFill);
    void triggerSamples(juce::AudioSourceChannelInfo &bufferToFill);
    void playOneTime(juce::AudioBuffer<float> &outputBuffer);
    void startPlayer();
    void stopPlayer();
    void activateSample(int sampleIndex);

private:
    int mTotalSamples{0};
    double mSampleRate{0};
    int mUpdateInterval{0};
    double mBpm{140.0};
    int mSamplesRemaining{0};
    int currentSample1index{0};
    int currentSample2index{0};
    float previousGain;
    float gain;
    bool isPlaying = false;
    int currentSequenceIndex{0};
    int sequenceSize{16};
    int size{3};
    int numVoices{3};

    juce::Synthesiser *mySynth;
    juce::MidiBuffer midiBuffer;
    juce::MixerAudioSource mixerAudioSource;
    juce::IIRFilterAudioSource *iirFilterAudioSource1;
    juce::IIRFilterAudioSource *iirFilterAudioSource2;
    juce::ReverbAudioSource *reverbAudioSource1;
    juce::ReverbAudioSource *reverbAudioSource2;
    juce::AudioFormatManager mAudioFormatManager;
    juce::ADSR *adsr;
    std::unique_ptr<juce::AudioFormatReaderSource> pSample1{nullptr};
    std::unique_ptr<juce::AudioFormatReaderSource> pSample2{nullptr};
    std::unique_ptr<juce::AudioFormatReaderSource> playSource;
    std::unique_ptr<juce::AudioFormatReaderSource> playSource2;
    juce::AudioTransportSource transport;
    juce::AudioTransportSource transport2;
    std::vector<int> sample1Sequence;
    std::vector<int> sample2Sequence;
    std::unique_ptr<juce::AudioFormatReaderSource> playSources[3];
    std::vector<int> sequences[3];
    bool playingSamples[3] = {true, true, false};
    juce::AudioTransportSource samples[3];
    juce::dsp::Limiter<float> limiter;
    juce::dsp::ProcessSpec processorSpec;
};
