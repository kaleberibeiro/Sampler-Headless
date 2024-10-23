#pragma once
#include "../JuceLibraryCode/JuceHeader.h"

// Forward declaration of MySamplerVoice

class MySamplerVoice : public juce::SamplerVoice, juce::HighResolutionTimer
{
public:
  MySamplerVoice(juce::Synthesiser *mySynth, int *lengthInSamples)
      : mySynth(mySynth),
        lengthInSamples(lengthInSamples),
        sampleStart(8, 0),
        adsrList{juce::ADSR(), juce::ADSR(), juce::ADSR(), juce::ADSR()},
        sequences(8, std::vector<std::vector<std::array<int, 2>>>(8, std::vector<std::array<int, 2>>(64, {0, 1}))),
        subStepTimer(*this) // Initialize SubstepTimer with this
  {
    for (int i = 0; i < 8; ++i)
    {
      sampleLength[i] = lengthInSamples[i];
      smoothSampleLength[i].setCurrentAndTargetValue(lengthInSamples[i]);
    }
  }

  bool canPlaySound(juce::SynthesiserSound *sound) override
  {
    return true; // Allow all sounds to be played
  }

  void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound *sound, int pitchWheel) override
  {
    // Implementation for starting a note
  }

  void stopNote(float velocity, bool allowTailOff) override
  {
    // Implementation for stopping a note
  }

  void pitchWheelMoved(int newValue) override
  {
    // Implementation for pitch wheel movement
  }

  void controllerMoved(int controllerNumber, int newValue) override
  {
    // Implementation for controller movement
  }

  void prepareToPlay(int samplesPerBlockExpected, double sampleRate);
  void countSamples(juce::AudioBuffer<float> &buffer, int startSample, int numSamples);
  void checkSequence(juce::AudioBuffer<float> &buffer, int startSample, int numSamples);
  void triggerSamples(juce::AudioBuffer<float> &buffer, int startSample, int numSamples);
  void activateSample(int sample, int sampleValue);
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
    int newLength = static_cast<int>((static_cast<float>(knobValue) / 127.0f) * maxLength);
    sampleLength[*selectedSample] = newLength;
    smoothSampleLength[*selectedSample].setTargetValue(newLength);
  }

  void changeSampleStart(int knobValue)
  {
    int maxLength = lengthInSamples[*selectedSample];
    int startPosition = static_cast<int>((static_cast<float>(knobValue) / 127.0f) * maxLength);
    sampleStart[*selectedSample] = static_cast<float>(startPosition);
  }

  void changeSelectedSequence(int pattern)
  {
    if (currentPatternIndex[*selectedSample] == 0)
    {
      selectedPattern[*selectedSample] = pattern;
    }
    else
    {
      cachedPattern[*selectedSample] = pattern;
    }
  }

  void changePatternLength(int value)
  {
    int newLength = value + 1;

    std::vector<std::array<int, 2>> &currentPattern = sequences[*selectedSample][selectedPattern[*selectedSample]];

    if (newLength > currentPattern.size())
    {
      currentPattern.resize(newLength, {0, 1});
    }
    else if (newLength < currentPattern.size())
    {
      currentPattern.resize(newLength);
    }
  }

  void changeAdsrValues(int knobValue, int adsrParam);
  void changeLowPassFilter(double sampleRate, int knobValue);
  void changeHighPassFilter(double sampleRate, int knobValue);
  void changeBandPassFilter(double sampleRate, int knobValue);
  void changeReverb(int knobValue);
  void changeChorus(int knobValue);
  void changeFlanger(int knobValue);
  void changePhaser(int knobValue);
  void changePanner(int knobValue);
  void changeDelay(int knobValue);

  void changePitchShift(int sampleIndex, int knobValue)
  {
    float pitchShift = 0.5f + (knobValue / 127.0f) * (2.0f - 0.5f);
    pitchShiftFactors[sampleIndex] = pitchShift;
    interpolators[sampleIndex].reset();
  }

  void changeBPM(int knobValue)
  {
    knobValue = juce::jlimit(0, 127, knobValue);
    mBpm = 80 + ((static_cast<float>(knobValue) / 127.0f) * (207 - 80));
    startTimer(1000 / ((mBpm / 60.f) * 4));
  };

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

    for (int i = 0; i < size; i++)
    {
      smoothGainRamp[i].setTargetValue(0.0f);
      adsrList[i].noteOff();
      currentPatternIndex[i] = 0;
    }
  };

  void changeSampleVelocity(int sampleIndex, int knobValue)
  {
    float finalValue = (static_cast<float>(knobValue) / 127.0f) * 0.7;
    smoothGainRamp[sampleIndex].setTargetValue(finalValue);
    previousGain[sampleIndex] = finalValue;
  };

  void updateSampleIndex(int indexPosition, int padValue)
  {
    auto &currentArray = sequences[*selectedSample][selectedPattern[*selectedSample]][indexPosition];

    currentArray[0] = currentArray[0] == 0 ? 1 : 0;
  };

  void chainPattern(int padValue)
  {
    std::vector<int> &currentSequence = sequenceChain[*selectedSample];

    auto it = std::find(currentSequence.begin(), currentSequence.end(), padValue);

    if (it != currentSequence.end())
    {
      currentSequence.erase(it);
    }
    else
    {
      auto insertPosition = std::lower_bound(currentSequence.begin(), currentSequence.end(), padValue);
      currentSequence.insert(insertPosition, padValue);
    }

    isSequenceChained[*selectedSample] = currentSequence.size() > 0;
  }

  void changeSubStep(int stepIndex, int subStepsNumber)
  {
    auto &currentArray = sequences[*selectedSample][selectedPattern[*selectedSample]][stepIndex];

    currentArray[1] = subStepsNumber;
  }

  std::unique_ptr<int> selectedSample = std::make_unique<int>(0);

  void saveData();
  void loadData();
  void clearPattern();
  void clearSampleModulation();

  class SubstepTimer : public juce::HighResolutionTimer
  {
  public:
    SubstepTimer(MySamplerVoice &mySamplerVoice)
        : mySamplerVoice(mySamplerVoice), sampleIndex(-1), subStepCount(1)
    {
    }

    void startForSample(int index, int bpm, int subSteps)
    {
      sampleIndex = index;
      subStepCount = subSteps;

      int interval = 1000 / ((bpm / 60.f) * 4 * subStepCount);
      startTimer(interval);
    }

    void hiResTimerCallback() override
    {
      if (sampleIndex != -1)
      {
        mySamplerVoice.samplesPosition[sampleIndex] = mySamplerVoice.sampleStart[sampleIndex];
        mySamplerVoice.sampleMakeNoise[sampleIndex] = true;
      }
      subStepStart++;
      if (subStepStart >= subStepCount)
      {
        subStepStart = 0;
        stopTimer();
      }
    }

  private:
    MySamplerVoice &mySamplerVoice;
    int subStepStart = 0;
    int sampleIndex;
    int subStepCount;
  };

private:
  juce::CriticalSection objectLock;
  juce::Synthesiser *mySynth;
  SubstepTimer subStepTimer;
  std::array<float, 8> previousGain;
  std::array<juce::SmoothedValue<int, juce::ValueSmoothingTypes::Linear>, 8> smoothSampleLength;
  std::array<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>, 8> smoothGainRamp;
  std::array<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>, 8> smoothLowRamps;
  std::array<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>, 8> smoothHighRamps;
  std::array<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>, 8> smoothBandRamps;
  std::array<juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>, 8> duplicatorsLowPass;
  std::array<juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>, 8> duplicatorsHighPass;
  std::array<juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>, 8> duplicatorsBandPass;
  int *lengthInSamples;
  double mSampleRate{0};
  int mBpm{140};
  int size{8};
  bool sequencePlaying{false};
  std::vector<std::vector<std::vector<std::array<int, 2>>>> sequences;
  std::array<int, 8> selectedPattern = {};
  std::array<int, 8> cachedPattern = {-1, -1, -1, -1, -1, -1, -1, -1};
  std::array<int, 8> currentPatternIndex = {0};
  std::array<int, 8> samplesPosition = {0};
  std::array<bool, 8> sampleMakeNoise = {false};
  std::array<bool, 8> previousSampleMakeNoise = {false};
  std::array<bool, 8> samplesPressed = {false};
  std::array<bool, 8> isSampleMuted = {true, true, true, true, true, true, true, true};
  std::array<bool, 8> isSequenceChained = {false, false, false, false, false, false, false, false};
  std::array<std::vector<int>, 8> sequenceChain;
  std::vector<int> sampleStart;
  std::array<int, 8> sampleLength;
  std::array<juce::ADSR, 8> adsrList;
  std::array<juce::dsp::ProcessorChain<juce::dsp::Reverb, 
    juce::dsp::Chorus<float>, 
    juce::dsp::Chorus<float>, 
    juce::dsp::Panner<float>, 
    juce::dsp::Phaser<float>
  >, 8> effectsChain;
  std::array<juce::dsp::Reverb, 8> reverbs;
  std::array<juce::dsp::Chorus<float>, 8> chorus;
  std::array<juce::dsp::Chorus<float>, 8> flanger;
  std::array<juce::dsp::Panner<float>, 8> panner;
  std::array<juce::dsp::Phaser<float>, 8> phaser;
  std::array<juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear>, 8> delay;
  std::array<juce::LinearInterpolator, 8> interpolators;
  std::array<float, 8> pitchShiftFactors = {0};
  std::array<int, 8> lastReverbKnob = {0};
  std::array<int, 8> lastChorusKnob = {0};
  std::array<int, 8> lastFlangerKnob = {0};
  std::array<int, 8> lastPannerKnob = {0};
  std::array<int, 8> lastPhaserKnob = {0};
  std::array<int, 8> lastLowPassKnob = {0};
  std::array<int, 8> lastHighPassKnob = {0};
  std::array<int, 8> lastBandPassKnob = {0};

  void updateSamplesActiveState();
};
