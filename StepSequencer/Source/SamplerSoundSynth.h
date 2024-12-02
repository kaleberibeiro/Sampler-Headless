/*
  ==============================================================================

    SamplerSoundSynth.h
    Created: 14 Mar 2024 11:36:21pm
    Author:  kalebe

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include <atomic>

class SamplerSoundSynth
{
public:
  SamplerSoundSynth();
  ~SamplerSoundSynth();
  // ~SamplerSoundSynth();
  void noteOn(int midiChannel, int midiNoteNumber, float velocity);
  void noteOff(int midiChannel, int midiNoteNumber, float velocity, bool allowTailOff);
  // void prepareToPlay(int samplesPerBlockExpected, double sampleRate);
  // void getNextAudioBlock(juce::AudioSourceChannelInfo &bufferToFill);

private:
  int mTotalSamples{0};
  double mSampleRate{0};
  int mUpdateInterval{0};
  double mBpm{140.0};
  int mSamplesRemaining{0};
  int currentSample1index{0};
  int currentSample2index{0};
  const int mNumVoices{3};

  juce::AudioFormatManager mAudioFormatManager;
  juce::SamplerSound *mSamplerSound1;
  juce::SamplerSound *mSamplerSound2;
  juce::Synthesiser mSampler;
  std::unique_ptr<juce::AudioFormatReaderSource> pSample1{nullptr};
  // juce::MixerAudioSource mixerAudioSource;
  // juce::IIRFilterAudioSource* iirFilterAudioSource1;
  // juce::IIRFilterAudioSource* iirFilterAudioSource2;
  // juce::ReverbAudioSource* reverbAudioSource1;
  // juce::ReverbAudioSource* reverbAudioSource2;
  // std::unique_ptr<juce::AudioFormatReaderSource> pSample2{nullptr};
  // std::vector<int> sample1Sequence;
  // std::vector<int> sample2Sequence;
};
