/*
  ==============================================================================

    SamplerVoice.cpp
    Created: 17 Apr 2024 4:16:17pm
    Author:  kalebe

  ==============================================================================
*/

#include "MySamplerVoice.h"

void MySamplerVoice::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
  mSampleRate = sampleRate;
  mUpdateInterval = (60.0 / mBpm * mSampleRate) / 4;

  mySynth->setCurrentPlaybackSampleRate(sampleRate);
}

void MySamplerVoice::countSamples(juce::AudioBuffer<float> &buffer, int startSample, int numSamples)
{
  buffer.clear();
  auto bufferSize = buffer.getNumSamples();
  mTotalSamples += bufferSize;
  mSamplesRemaining = mTotalSamples % mUpdateInterval;

  if ((mSamplesRemaining + bufferSize) >= mUpdateInterval)
  {
    const auto timeToStartPlaying = mUpdateInterval - mSamplesRemaining;
    for (auto sample = 0; sample < bufferSize; ++sample)
    {
      if (sample == timeToStartPlaying)
      {
        mSamplePosition = 0;
        checkSequence(buffer, startSample, numSamples);
      }
    }
  }

  bool willPlay = false;
  for (int i = 0; i < size; ++i)
  {
    if (samplesPosition[i] != 0)
    {
      willPlay = true;
      break;
    }
  }

  if (willPlay)
  {
    triggerSamples(buffer, startSample, numSamples);
  }
}

void MySamplerVoice::checkSequence(juce::AudioBuffer<float> &buffer, int startSample, int numSamples)
{
  bool anySamplePlaying = false; // Flag to track if any sample is playing

  // Reset currentSequenceIndex and positions of samples at the beginning of each sequence
  if (currentSequenceIndex >= sequenceSize)
  {
    currentSequenceIndex = 0;
    for (int i = 0; i < size; ++i)
    {
      samplesPosition[i] = 0;
    }
  }

  for (int i = 0; i < size; ++i)
  {
    if (playingSamples[i])
    {
      if (sequences[i][currentSequenceIndex] == 1)
      {
        // Reset position only if the sample is about to play
        samplesPosition[i] = 0;
        anySamplePlaying = true;
      }
      else if (samplesPosition[i] == 0)
      {
        // Remover sample
      }
    }
  }

  currentSequenceIndex++;

  if (anySamplePlaying)
  {
    triggerSamples(buffer, startSample, numSamples);
  }
}

void MySamplerVoice::triggerSamples(juce::AudioBuffer<float> &buffer, int startSample, int numSamples)
{
  // std::cout << "Sample lenght: " << *lengthInSamples1 << std::endl;
  juce::SynthesiserSound::Ptr soundPtr = mySynth->getSound(0);
  juce::SamplerSound *samplerSound = dynamic_cast<juce::SamplerSound *>(soundPtr.get());

  for (int sample = 0; sample < numSamples; ++sample)
  {
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
      const float *audioData = samplerSound->getAudioData()->getReadPointer(channel);
      if (samplesPosition[0] < *lengthInSamples1)
      {
        buffer.setSample(channel, startSample + sample, audioData[samplesPosition[0]]);
        ++samplesPosition[0];
      }
    }
  }
}