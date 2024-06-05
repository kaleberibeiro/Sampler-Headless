#pragma once
#include "../JuceLibraryCode/JuceHeader.h"

class MySamplerVoice : public juce::SamplerVoice, juce::HighResolutionTimer
{
public:
  MySamplerVoice(juce::Synthesiser *mySynth, int *lengthInSamples)
      : mySynth(mySynth), lengthInSamples(lengthInSamples), sampleStart(3, 0.0f), sampleLength(3, 1.0f)
  {
    sequences[0] = {1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0};
    sequences[1] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
    sequences[2] = {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    for (int i = 0; i < 3; ++i)
    {
      std::cout << "Sample length: " << i << " | " << lengthInSamples[i] << std::endl;
    }
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
  void hiResTimerCallback() override;

  void changeSelectedSample(int sample) { *selectedSample = sample; }
  void changeSampleStart(int knobValue) { sampleStart[*selectedSample] = static_cast<float>(knobValue) / 127.0f; }
  void changeSampleLength(int knobValue) { sampleLength[*selectedSample] = static_cast<float>(knobValue) / 127.0f; }
  void changeLowPassFilter(double sampleRate, double knobValue){};

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
  int currentIndexSequence{0};
  int currentSequenceIndex{0};
  int sequenceSize{16};
  int size{3};
  std::vector<int> sequences[3];
  std::array<int, 3> samplesPosition = {0, 0, 0};
  std::array<bool, 3> playingSamples = {true, true, true};
  std::array<bool, 3> activeLowPass = {false, false, false};
  std::array<bool, 3> activeSample = {false, false, false};
  std::array<float, 3> sampleVelocity = {0.6, 0.6, 0.5};
  std::vector<float> sampleStart;
  std::vector<float> sampleLength;
  std::unique_ptr<int> selectedSample = std::make_unique<int>(0);
  juce::IIRFilter lowPassFilter;
  juce::IIRFilter highPassFilter;
  juce::IIRFilter bandPassFilter;

  void resetSamplesPosition();
  void updateSamplesActiveState();
};