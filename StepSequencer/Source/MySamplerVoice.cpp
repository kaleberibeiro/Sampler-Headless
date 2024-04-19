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
        // std::cout << "Toca aqui: " << sample << " bufferSize: " << bufferSize << "sample lenght: " << *lengthInSamples1 << std::endl;
        triggerSamples(buffer, startSample, numSamples);
      }
    }
  }

  if (mSamplePosition != 0)
  {
    triggerSamples(buffer, startSample, numSamples);
  }
}

void MySamplerVoice::triggerSamples(juce::AudioBuffer<float> &buffer, int startSample, int numSamples)
{
  juce::SynthesiserSound::Ptr soundPtr = mySynth->getSound(0);
  juce::SamplerSound *samplerSound = dynamic_cast<juce::SamplerSound *>(soundPtr.get());

  for (int sample = 0; sample < numSamples; ++sample)
  {
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
      const float *audioData = samplerSound->getAudioData()->getReadPointer(channel);
      if (mSamplePosition < *lengthInSamples1)
      {
        buffer.setSample(channel, startSample + sample, audioData[mSamplePosition]);
        ++mSamplePosition;
      }
      else
      {
        buffer.clear(channel, sample, numSamples - sample);
        return;
      }
    }
  }
}