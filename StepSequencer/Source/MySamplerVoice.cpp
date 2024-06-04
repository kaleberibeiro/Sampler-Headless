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
  mUpdateInterval = (60.0 / (mBpm)*mSampleRate);

  mySynth->setCurrentPlaybackSampleRate(sampleRate);
}

void MySamplerVoice::countSamples(juce::AudioBuffer<float> &buffer, int startSample, int numSamples)
{
  buffer.clear();
  auto bufferSize = buffer.getNumSamples();
  mTotalSamples += bufferSize;
  mSamplesRemaining = mTotalSamples % mUpdateInterval;
  bool willPlay = false;

  if ((mSamplesRemaining + bufferSize) >= mUpdateInterval)
  {
    const auto timeToStartPlaying = mUpdateInterval - mSamplesRemaining;
    for (auto sample = 0; sample < bufferSize; ++sample)
    {
      if (sample == timeToStartPlaying)
      {
        checkSequence(buffer, startSample, numSamples);
      }
    }
  }
  else
  {
    for (int i = 0; i < size; ++i)
    {
      if (samplesPosition[i] != (0 + lengthInSamples[i] * sampleStart[i]) && samplesPosition[i] != lengthInSamples[i] * sampleLength[i])
      {
        willPlay = true;
      }
      else
      {
        activeSample[i] = false;
      }
    }

    if (willPlay)
    {
      triggerSamples(buffer, startSample, numSamples);
    }
  }
}

void MySamplerVoice::checkSequence(juce::AudioBuffer<float> &buffer, int startSample, int numSamples)
{
  bool anySamplePlaying = false; // Flag to track if any sample is playing

  if (currentSequenceIndex >= sequenceSize)
  {
    currentSequenceIndex = 0;
    for (int i = 0; i < size; ++i)
    {
      samplesPosition[i] = (0 + lengthInSamples[i] * sampleStart[i]);
    }
  }

  for (int i = 0; i < size; ++i)
  {
    if (playingSamples[i])
    {
      if (sequences[i][currentSequenceIndex] == 1)
      {
        samplesPosition[i] = (0 + lengthInSamples[i] * sampleStart[i]);
        activeSample[i] = true;
        anySamplePlaying = true;
      }
      else if (samplesPosition[i] == (0 + lengthInSamples[i] * sampleStart[i]))
      {
        activeSample[i] = false;
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
  const juce::ScopedLock sl(objectLock);

  for (int voiceIndex = 0; voiceIndex < mySynth->getNumVoices(); ++voiceIndex)
  {
    if (activeSample[voiceIndex])
    {
      juce::SynthesiserSound::Ptr soundPtr = mySynth->getSound(voiceIndex);
      juce::SamplerSound *samplerSound = dynamic_cast<juce::SamplerSound *>(soundPtr.get());
      if (samplerSound != nullptr)
      {
        const int totalSamples = lengthInSamples[voiceIndex] * sampleLength[voiceIndex]; // Total length of the sample

        int remainingSamples = totalSamples - samplesPosition[voiceIndex];
        int samplesToCopy = std::min(remainingSamples, numSamples);

        for (int sample = 0; sample < samplesToCopy; ++sample)
        {
          for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
          {
            const float *audioData = samplerSound->getAudioData()->getReadPointer(channel);
            buffer.setSample(channel, startSample + sample, audioData[samplesPosition[voiceIndex]] * sampleVelocity[voiceIndex]);
          }
          ++samplesPosition[voiceIndex];
        }

        if (samplesPosition[voiceIndex] >= totalSamples)
        {
          clearCurrentNote();
        }

        // for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        // {
        //   if (activeLowPass[voiceIndex])
        //   {
        //     lowPassFilter.processSamples(buffer.getWritePointer(channel), numSamples);
        //   }
        // }
      }
    }
  }
}

// void MySamplerVoice::triggerSamples(juce::AudioBuffer<float> &buffer, int startSample, int numSamples)
// {
//   juce::SynthesiserSound::Ptr soundPtr = mySynth->getSound(0);
//   juce::SamplerSound *samplerSound = dynamic_cast<juce::SamplerSound *>(soundPtr.get());
//   int totalSamples = lengthInSamples[0]; // Total length of the sample
//   if (samplesPosition[0] >= totalSamples)
//   {
//     buffer.clear(startSample, numSamples);
//     return;
//   }

//   int remainingSamples = totalSamples - samplesPosition[0];

//   int samplesToCopy = std::min(remainingSamples, numSamples);

//   for (int sample = 0; sample < samplesToCopy; ++sample)
//   {
//     for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
//     {
//       const float *audioData = samplerSound->getAudioData()->getReadPointer(channel);
//       buffer.setSample(channel, startSample + sample, audioData[samplesPosition[0]]);
//     }
//     ++samplesPosition[0];
//   }

//   if (samplesPosition[0] >= totalSamples)
//   {
//     this->clearCurrentNote();
//   }
// }
