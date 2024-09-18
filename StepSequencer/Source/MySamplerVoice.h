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
  void playSampleProcess(juce::AudioBuffer<float> &buffer, int startSample, int numSamples);
  void playSample(int sampleIndex, int sampleCommand)
  {
    if (sampleCommand == 127)
    {
      samplesPressed[sampleIndex] = true;
    }
    else
    {
      samplesPressed[sampleIndex] = false;
      samplesPosition[sampleIndex] = 0;
    }
  }

  void changeSelectedSample(int sample) { *selectedSample = sample; }
  void changeSampleStart(int knobValue) { sampleStart[*selectedSample] = static_cast<float>(knobValue) / 127.0f; }
  void changeSampleLength(int knobValue) { sampleLength[*selectedSample] = static_cast<float>(knobValue) / 127.0f; }
  void changeAdsrValues(int knobValue, int adsrParam);
  void changeLowPassFilter(double sampleRate, double knobValue);
  void changeHighPassFilter(double sampleRate, double knobValue);
  void changeBandPassFilter(double sampleRate, double knobValue);
  void changeReverb(double knobValue);
  void changeChorus(double knobValue);
  void changePitchShift(int sampleIndex, int knobValue)
  {
    float pitchShift = 0.5f + (knobValue / 127.0f) * (2.0f - 0.5f);
    pitchShiftFactors[sampleIndex] = pitchShift;
    interpolators[sampleIndex].reset();
  }

  void playSequence()
  {
    sequencePlaying = true;
    startTimer(1000 / ((mBpm / 60.f) * 4));
  };

  void stopSequence()
  {
    stopTimer();
    sequencePlaying = false;
    currentSequenceIndex = 0;

    for (int i = 0; i < size; i++)
    {
      samplesPosition[i] = 0;
      sampleMakeNoise[i] = false;
      adsrList[i].noteOff();
    }
  };

  void changeSampleVelocity(int sampleIndex, int knobValue)
  {
    smoothGainRamp[sampleIndex].setTargetValue(static_cast<float>(knobValue) / 127.0f);
    previousGain[sampleIndex] = static_cast<float>(knobValue) / 127.0f;
  };

  void updateSampleIndex(int indexPosition, int padValue)
  {
    if (padValue == 0)
    {
      sequences[*selectedSample][indexPosition] = 0;
    }
    else
    {
      sequences[*selectedSample][indexPosition] = 1;
    }
  };

  std::unique_ptr<int> selectedSample = std::make_unique<int>(0);

private:
  juce::CriticalSection objectLock;
  juce::Synthesiser *mySynth;
  std::array<float, 4> previousGain;
  std::array<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>, 4> smoothGainRamp;
  std::array<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>, 4> smoothLowRamps;
  std::array<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>, 4> smoothHighRamps;
  std::array<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>, 4> smoothBandRamps;
  std::array<juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>, 4> duplicatorsLowPass;
  std::array<juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>, 4> duplicatorsHighPass;
  std::array<juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>, 4> duplicatorsBandPass;
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
  bool sequencePlaying{false};
  std::vector<int> sequences[4];
  std::array<int, 4> samplesPosition = {0, 0, 0, 0};
  std::array<bool, 4> sampleOn = {false, false, false, false};
  std::array<bool, 4> sampleMakeNoise = {false, false, false, false};
  std::array<bool, 4> samplesPressed = {false};
  std::array<bool, 4> isInReleasePhase = {false};
  std::vector<float> sampleStart;
  std::vector<float> sampleLength;
  std::array<juce::ADSR, 4> adsrList;
  std::array<juce::dsp::Reverb, 4> reverbs;
  std::array<juce::dsp::Chorus<float>, 4> chorus;
  std::array<juce::LinearInterpolator, 4> interpolators;
  std::array<float, 4> pitchShiftFactors = {1.0, 1.0, 1.0, 1.0};
  void updateSamplesActiveState();
};
