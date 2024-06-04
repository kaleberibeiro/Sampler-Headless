#pragma once
#include "../JuceLibraryCode/JuceHeader.h"

class MySamplerVoice : public juce::SamplerVoice
{
public:
  MySamplerVoice(juce::Synthesiser *mySynth, int *lengthInSamples) : mySynth(mySynth), lengthInSamples(lengthInSamples)
  {
    sequences[0] = {1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0};
    sequences[1] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
    sequences[2] = {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    sampleStart = new float[3]{0.0, 0.0, 0.0};
    sampleLength = new float[3]{1, 1, 1};

    for (int i = 0; i < 3; i++)
    {
      std::cout << "Sample lenght: " << i << " | " << lengthInSamples[i] << std::endl;
    }
  }

  bool canPlaySound(juce::SynthesiserSound *sound) override
  {
    // Retorna verdadeiro se esta voz puder reproduzir o som dado
    return true;
  }

  void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound *sound, int pitchWheel) override
  {
    // Implementação vazia para iniciar uma nova nota
  }

  void stopNote(float velocity, bool allowTailOff) override
  {
    // Implementação vazia para parar uma nota
  }

  void pitchWheelMoved(int newValue) override
  {
    // Implementação vazia para movimento da alavanca de pitch
  }

  void controllerMoved(int controllerNumber, int newValue) override
  {
    // Implementação vazia para movimento de um controlador MIDI
  }

  void countSamples(juce::AudioBuffer<float> &buffer, int startSample, int numSamples);
  void prepareToPlay(int samplesPerBlockExpected, double sampleRate);
  void checkSequence(juce::AudioBuffer<float> &buffer, int startSample, int numSamples);
  void triggerSamples(juce::AudioBuffer<float> &buffer, int startSample, int numSamples);
  void changeSelectedSample(int sample)
  {
    *selectedSample = sample;
  }

  void changeSampleStart(int knobValue)
  {
    sampleStart[*selectedSample] = static_cast<float>(knobValue) / 127.0f;
  }

  void changeSampleLength(int knobValue)
  {
    sampleLength[*selectedSample] = static_cast<float>(knobValue) / 127.0f;
  }

  void changeLowPassFilter(double sampleRate, double knobValue)
  {
    juce::IIRCoefficients filterValue;
    filterValue.makeHighPass(sampleRate, 7000);
    lowPassFilter.setCoefficients(filterValue);
    if (knobValue > 0)
    {
      activeLowPass[*selectedSample] = true;
    }
    else
    {
      activeLowPass[*selectedSample] = false;
    }
  }

  void renderNextBlock(juce::AudioBuffer<float> &buffer, int startSample, int numSamples) override
  {
  }

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
  int samplesPosition[3] = {0, 0, 0};
  bool playingSamples[3] = {true, true, false};
  bool activeLowPass[3] = {false, false, false};
  bool activeSample[3] = {false, false, false};
  float sampleVelocity[3] = {0.6, 1.0, 0.5};
  float *sampleStart;
  float *sampleLength;
  int *selectedSample = new int(0);
  juce::IIRFilter lowPassFilter;
  juce::IIRFilter highPassFilter;
  juce::IIRFilter bandPassFilter;
};
