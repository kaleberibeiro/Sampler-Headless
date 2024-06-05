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
  std::cout << "Is Timer Running Before? " << isTimerRunning() << std::endl;
  mSampleRate = sampleRate;
  mUpdateInterval = (60.0 / (mBpm)*mSampleRate);

  mySynth->setCurrentPlaybackSampleRate(sampleRate);

  std::cout << "Starting Timer..." << std::endl;
  startTimer(1000 / ((mBpm / 60.f) * 4));
  std::cout << "Is Timer Running After? " << isTimerRunning() << std::endl;
}

void MySamplerVoice::countSamples(juce::AudioBuffer<float> &buffer, int startSample, int numSamples)
{
  buffer.clear();

  bool willPlay = false;

  // Check if any active sample is still playing or needs to be triggered
  for (int i = 0; i < size; ++i)
  {
    if (activeSample[i])
    {
      willPlay = true;
      break; // Exit loop early if at least one active sample is found
    }
  }

  // Determine if any sample needs to be triggered based on its position
  for (int i = 0; i < size; ++i)
  {
    if (samplesPosition[i] != (0 + lengthInSamples[i] * sampleStart[i]) &&
        samplesPosition[i] != lengthInSamples[i] * sampleLength[i])
    {
      willPlay = true;
      break; // Exit loop early if at least one sample needs to be triggered
    }
  }

  if (willPlay)
  {
    triggerSamples(buffer, startSample, numSamples);
  }
}

void MySamplerVoice::updateSamplesActiveState()
{
  for (int i = 0; i < size; ++i)
  {
    if (playingSamples[i])
    {
      if (sequences[i][currentSequenceIndex] == 1)
      {
        samplesPosition[i] = (0 + lengthInSamples[i] * sampleStart[i]);
        activeSample[i] = true;
      }
      else if (samplesPosition[i] == (0 + lengthInSamples[i] * sampleStart[i]))
      {
        activeSample[i] = false;
      }
    }
  }
}

void MySamplerVoice::hiResTimerCallback()
{
  if (currentSequenceIndex >= sequenceSize)
  {
    currentSequenceIndex = 0;
    resetSamplesPosition();
  }

  updateSamplesActiveState();

  currentSequenceIndex++;
}

void MySamplerVoice::triggerSamples(juce::AudioBuffer<float> &buffer, int startSample, int numSamples)
{
  const juce::ScopedLock sl(objectLock);

  // Prepare a batch buffer for processing all voices together
  juce::AudioBuffer<float> batchBuffer(buffer.getNumChannels(), numSamples);
  batchBuffer.clear();

  for (int voiceIndex = 0; voiceIndex < mySynth->getNumVoices(); ++voiceIndex)
  {
    if (activeSample[voiceIndex])
    {
      juce::SynthesiserSound::Ptr soundPtr = mySynth->getSound(voiceIndex);
      juce::SamplerSound *samplerSound = dynamic_cast<juce::SamplerSound *>(soundPtr.get());
      if (samplerSound != nullptr)
      {
        const int totalSamples = lengthInSamples[voiceIndex] * sampleLength[voiceIndex];
        int remainingSamples = totalSamples - samplesPosition[voiceIndex];
        int samplesToCopy = std::min(remainingSamples, numSamples);

        for (int sample = 0; sample < samplesToCopy; ++sample)
        {
          for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
          {
            const int soundChannelIndex = channel % samplerSound->getAudioData()->getNumChannels();
            const float *audioData = samplerSound->getAudioData()->getReadPointer(soundChannelIndex);
            batchBuffer.addSample(channel, startSample + sample, audioData[samplesPosition[voiceIndex]] * sampleVelocity[voiceIndex]);
          }
          ++samplesPosition[voiceIndex];
        }

        if (samplesPosition[voiceIndex] >= totalSamples)
        {
          // Reset sample position
          samplesPosition[voiceIndex] = 0;
          // Deactivate the sample
          activeSample[voiceIndex] = false;
        }
      }
    }
  }

  // Mix the batch buffer into the output buffer for both channels
  for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
  {
    buffer.addFrom(channel, startSample, batchBuffer, channel, 0, numSamples);
  }
}

void MySamplerVoice::resetSamplesPosition()
{
  for (int i = 0; i < size; ++i)
  {
    samplesPosition[i] = (0 + lengthInSamples[i] * sampleStart[i]);
  }
}
