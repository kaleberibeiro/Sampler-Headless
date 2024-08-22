#pragma once
#include "../JuceLibraryCode/JuceHeader.h"

class MySamplerVoice : public juce::SamplerVoice, juce::HighResolutionTimer
{
public:
  MySamplerVoice(juce::Synthesiser *mySynth, int *lengthInSamples)
      : mySynth(mySynth),
        lengthInSamples(lengthInSamples),
        sampleStart(4, 0.0f),
        sampleLength(4, 1.0f),
        adsrList{juce::ADSR(), juce::ADSR(), juce::ADSR(), juce::ADSR()}
  {
    sequences[0] = {1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0};
    sequences[1] = {0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0};
    sequences[2] = {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    sequences[3] = {0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0};
  }

  bool canPlaySound(juce::SynthesiserSound *sound) override
  {
    return true;
  }

  void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound *sound, int pitchWheel) override {}
  void stopNote(float velocity, bool allowTailOff) override {}
  void pitchWheelMoved(int newValue) override {}
  void controllerMoved(int controllerNumber, int newValue) override {}

  void prepareToPlay(int samplesPerBlockExpected, double sampleRate);
  void countSamples(juce::AudioBuffer<float> &buffer, int startSample, int numSamples);
  void checkSequence(juce::AudioBuffer<float> &buffer, int startSample, int numSamples);
  void triggerSamples(juce::AudioBuffer<float> &buffer, int startSample, int numSamples);
  void activateSample(int sample);
  void hiResTimerCallback() override;

  void changeSelectedSample(int sample) { *selectedSample = sample; }
  void changeSampleStart(int knobValue) { sampleStart[*selectedSample] = static_cast<float>(knobValue) / 127.0f; }
  void changeSampleLength(int knobValue) { sampleLength[*selectedSample] = static_cast<float>(knobValue) / 127.0f; }
  void changeAdsrValues(int knobValue, int adsrParam);
  void changeLowPassFilter(double sampleRate, double knobValue);
  void changeHighPassFilter(double sampleRate, double knobValue);
  void changeBandPassFilter(double sampleRate, double knobValue);
  void changeReverb(double knobValue);
  void changeChorus(double knobValue);

  std::unique_ptr<int> selectedSample = std::make_unique<int>(0);

private:
  juce::CriticalSection objectLock;
  juce::Synthesiser *mySynth;
  int *lengthInSamples;
  int mSamplePosition = 0;
  int mTotalSamples{0};
  double mSampleRate{0};
  int mUpdateInterval{0};
  double mBpm{140.0};
  int mSamplesRemaining{0};
  int currentSequenceIndex{0};
  int sequenceSize{16};
  int size{4};
  std::vector<int> sequences[4];
  std::array<int, 4> samplesPosition = {0, 0, 0, 0};
  std::array<bool, 4> playingSamples = {false, false, false, false};
  std::array<bool, 4> activeLowPass = {false, false, false, false};
  std::array<bool, 4> activeSample = {false, false, false, false};
  std::array<float, 4> sampleVelocity = {0.4, 0.6, 0.5, 0.8};
  std::vector<float> sampleStart;
  std::vector<float> sampleLength;
  std::array<juce::ADSR, 4> adsrList;
  std::array<juce::dsp::IIR::Filter<float>, 4> lowPassFilters;
  std::array<juce::dsp::IIR::Filter<float>, 4> highPassFilters;
  std::array<juce::dsp::IIR::Filter<float>, 4> bandPassFilters;
  std::array<juce::dsp::Reverb, 4> reverbs;
  std::array<juce::dsp::Chorus<float>, 4> chorus;
  void updateSamplesActiveState();
};
