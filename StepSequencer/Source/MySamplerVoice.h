#pragma once
#include "../JuceLibraryCode/JuceHeader.h"

class MySamplerVoice : public juce::SamplerVoice, juce::HighResolutionTimer
{
public:
  MySamplerVoice(juce::Synthesiser *mySynth, int *lengthInSamples)
      : mySynth(mySynth),
        lengthInSamples(lengthInSamples),
        sampleStart(8, 0),
        sampleLength(8, *lengthInSamples),
        adsrList{juce::ADSR(), juce::ADSR(), juce::ADSR(), juce::ADSR()},
        sequences(8, std::vector<int>(64, 0))
  {
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
      samplesPosition[sampleIndex] = 0;
      samplesPressed[sampleIndex] = true;
    }
    else
    {
      samplesPressed[sampleIndex] = false;
    }
  }

  void changeSelectedSample(int sample) { *selectedSample = sample; }
  void changeSampleLength(int knobValue)
  {
    int maxLength = lengthInSamples[*selectedSample];
    std::cout << "O Tamanho do sample é: " << maxLength << std::endl;

    int newLength = static_cast<int>((static_cast<float>(knobValue) / 127.0f) * maxLength);
    std::cout << "O valor do knob é: " << knobValue << " e isso dá um tamanho final de: " << newLength << std::endl;
    sampleLength[*selectedSample] = static_cast<float>(newLength);
  }
  void changeSampleStart(int knobValue)
  {
    int maxLength = lengthInSamples[*selectedSample];
    int startPosition = static_cast<int>((static_cast<float>(knobValue) / 127.0f) * maxLength);
    sampleStart[*selectedSample] = static_cast<float>(startPosition);
  }

  void changeAdsrValues(int knobValue, int adsrParam);
  void changeLowPassFilter(double sampleRate, double knobValue);
  void changeHighPassFilter(double sampleRate, double knobValue);
  void changeBandPassFilter(double sampleRate, double knobValue);
  void changeReverb(double knobValue);
  void changeChorus(double knobValue);
  void changeFlanger(double knobValue);
  void changePitchShift(int sampleIndex, int knobValue)
  {
    float pitchShift = 0.5f + (knobValue / 127.0f) * (2.0f - 0.5f);
    pitchShiftFactors[sampleIndex] = pitchShift;
    interpolators[sampleIndex].reset();
  }

  void playSequence()
  {
    for (int i = 0; i < size; i++)
    {
      samplesPosition[i] = 0;
    }
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
      // samplesPosition[i] = 0;
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
    sequences[*selectedSample][indexPosition] = !sequences[*selectedSample][indexPosition];
  };

  std::unique_ptr<int> selectedSample = std::make_unique<int>(0);

private:
  juce::CriticalSection objectLock;
  juce::Synthesiser *mySynth;
  std::array<float, 8> previousGain;
  std::array<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>, 8> smoothGainRamp;
  std::array<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>, 8> smoothLowRamps;
  std::array<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>, 8> smoothHighRamps;
  std::array<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>, 8> smoothBandRamps;
  std::array<juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>, 8> duplicatorsLowPass;
  std::array<juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>, 8> duplicatorsHighPass;
  std::array<juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>, 8> duplicatorsBandPass;
  int *lengthInSamples;
  int mSamplePosition = 0;
  int mTotalSamples{0};
  double mSampleRate{0};
  int mUpdateInterval{0};
  double mBpm{140.0};
  int mSamplesRemaining{0};
  int currentSequenceIndex{0};
  int sequenceSize{64};
  int size{8};
  bool sequencePlaying{false};
  std::vector<std::vector<int>> sequences;
  std::array<int, 8> samplesPosition = {0};
  std::array<bool, 8> sampleOn = {false};
  std::array<bool, 8> sampleMakeNoise = {false};
  std::array<bool, 8> samplesPressed = {false};
  std::vector<int> sampleStart;
  std::vector<int> sampleLength;
  std::array<juce::ADSR, 8> adsrList;
  std::array<juce::dsp::Reverb, 8> reverbs;
  std::array<juce::dsp::Chorus<float>, 8> chorus;
  std::array<juce::dsp::Chorus<float>, 8> flanger;
  std::array<juce::LinearInterpolator, 8> interpolators;
  std::array<float, 8> pitchShiftFactors = {1.0};
  void updateSamplesActiveState();
};
