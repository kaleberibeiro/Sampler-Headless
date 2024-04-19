#pragma once
#include "../JuceLibraryCode/JuceHeader.h"

class MySamplerVoice : public juce::SamplerVoice
{
public:
  MySamplerVoice(juce::Synthesiser *mySynth, int *lengthInSamples1) : mySynth(mySynth), lengthInSamples1(lengthInSamples1) {}

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
  void triggerSamples(juce::AudioBuffer<float> &buffer, int startSample, int numSamples);

  void renderNextBlock(juce::AudioBuffer<float> &buffer, int startSample, int numSamples) override
  {
  }

private:
  juce::Synthesiser *mySynth;
  int *lengthInSamples1;
  int mSamplePosition = 0;
  int mTotalSamples{0};
  double mSampleRate{0};
  int mUpdateInterval{0};
  double mBpm{140.0};
  int mSamplesRemaining{0};
  int currentIndexSequence{0};
};
