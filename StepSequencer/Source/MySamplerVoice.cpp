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

  // INITIALIZE ADSR SAMPLE RATE
  for (int i = 0; i < size; i++)
  {
    adsrList[i].setSampleRate(sampleRate);
  }
  // INITIALIZE ADSR PARAMS
  juce::ADSR::Parameters adsrParams;
  adsrParams.attack = 0.0f;
  adsrParams.sustain = 1.0f;
  adsrParams.decay = 0.1f;
  adsrParams.release = 0.1f;

  for (int i = 0; i < size; i++)
  {
    adsrList[i].setParameters(adsrParams);
  }

  // INITIALIZE PROCESSSPEC
  juce::dsp::ProcessSpec spec;
  spec.sampleRate = sampleRate;
  spec.maximumBlockSize = samplesPerBlockExpected;
  spec.numChannels = 2;

  // PREPARE IIR FILTER
  for (int i = 0; i < size; i++)
  {
    lowPassFilters[i].prepare(spec);
    highPassFilters[i].prepare(spec);
    bandPassFilters[i].prepare(spec);

    // REVERB SETUP //
    reverbs[i].prepare(spec);
    reverbs[i].setEnabled(false);

    // CHORUS SETUP //
    chorus[i].prepare(spec);
    chorus[i].setCentreDelay(7.0);
    chorus[i].setFeedback(0.3);
    chorus[i].setDepth(0.1);
    chorus[i].setMix(0.0);
  }

  startTimer(1000 / ((mBpm / 60.f) * 4));
}

void MySamplerVoice::countSamples(juce::AudioBuffer<float> &buffer, int startSample, int numSamples)
{
  buffer.clear();

  bool willPlay = false;

  // Check if any active sample is still playing or needs to be triggered
  for (int i = 0; i < size; ++i)
  {
    if (sampleMakeNoise[i] && sampleOn[i])
    {
      // std::cout << "Entrou aquiiiiii, sample ativo: " << i << " is actice? " << sampleMakeNoise[i] << std::endl;
      willPlay = true;
      break;
    }
  }

  // Determine if any sample needs to be triggered based on its position
  // for (int i = 0; i < size; ++i)
  // {
  //   if (samplesPosition[i] != (0 + lengthInSamples[i] * sampleStart[i]) &&
  //       samplesPosition[i] != lengthInSamples[i] * sampleLength[i] && sampleOn[i])
  //   {
  //     // std::cout << "Entrou aqui baixoooooo, sample ativo: " << i << std::endl;
  //     willPlay = true;
  //     break; // Exit loop early if at least one sample needs to be triggered
  //   }
  // }

  if (willPlay)
  {
    triggerSamples(buffer, startSample, numSamples);
  }
}

void MySamplerVoice::updateSamplesActiveState()
{
  for (int i = 0; i < size; ++i)
  {
    if (sequences[i][currentSequenceIndex] == 1 && sampleOn[i])
    {
      samplesPosition[i] = (0 + lengthInSamples[i] * sampleStart[i]);
      sampleMakeNoise[i] = true;
    }
    else
    {
      if (samplesPosition[i] == (0 + lengthInSamples[i] * sampleStart[i]) || !sampleOn[i])
      {
        sampleMakeNoise[i] = false;
      }
    }
  }
}

void MySamplerVoice::hiResTimerCallback()
{
  if (currentSequenceIndex >= sequenceSize)
  {
    currentSequenceIndex = 0;
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
    juce::AudioBuffer<float> sampleBuffer(buffer.getNumChannels(), numSamples);
    sampleBuffer.clear();

    if (sampleMakeNoise[voiceIndex])
    {
      juce::SynthesiserSound::Ptr soundPtr = mySynth->getSound(voiceIndex);
      juce::SamplerSound *samplerSound = dynamic_cast<juce::SamplerSound *>(soundPtr.get());
      if (samplerSound != nullptr)
      {
        const int totalSamples = lengthInSamples[voiceIndex] * sampleLength[voiceIndex];
        int remainingSamples = totalSamples - samplesPosition[voiceIndex];
        int samplesToCopy = std::min(remainingSamples, numSamples);

        if (samplesPosition[voiceIndex] == 0 && sampleMakeNoise[voiceIndex])
        {
          // Reset ADSR and filters only on the first playback
          adsrList[voiceIndex].reset();
          adsrList[voiceIndex].noteOn();
          lowPassFilters[voiceIndex].snapToZero();
          highPassFilters[voiceIndex].snapToZero();
          bandPassFilters[voiceIndex].snapToZero();
        }

        for (int sample = 0; sample < samplesToCopy; ++sample)
        {
          float adsrValue = adsrList[voiceIndex].getNextSample();
          for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
          {
            const int soundChannelIndex = channel % samplerSound->getAudioData()->getNumChannels();
            const float *audioData = samplerSound->getAudioData()->getReadPointer(soundChannelIndex);
            float filteredSample = lowPassFilters[voiceIndex].processSample(audioData[samplesPosition[voiceIndex]] * adsrValue * sampleVelocity[voiceIndex]);
            filteredSample = highPassFilters[voiceIndex].processSample(filteredSample);
            filteredSample = bandPassFilters[voiceIndex].processSample(filteredSample);
            sampleBuffer.addSample(channel, startSample + sample, filteredSample);
          }

          ++samplesPosition[voiceIndex];

          if (samplesPosition[voiceIndex] >= lengthInSamples[voiceIndex])
          {
            // Signal the ADSR to start the release phase when reaching the end of the sample
            adsrList[voiceIndex].noteOff();
            sampleMakeNoise[voiceIndex] = false; // The sample finished playing
          }
        }

        ///// REVERB ////////
        juce::dsp::AudioBlock<float> audioBlock(sampleBuffer);
        juce::dsp::ProcessContextReplacing<float> context(audioBlock);

        reverbs[voiceIndex].process(context);
        chorus[voiceIndex].process(context);

        // Add sample buffer to batch buffer
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
          batchBuffer.addFrom(channel, 0, sampleBuffer, channel, 0, numSamples);
        }
      }
    }
  }

  for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
  {
    buffer.addFrom(channel, startSample, batchBuffer, channel, 0, numSamples);
  }
}

void MySamplerVoice::changeAdsrValues(int value, int adsrParam)
{

  juce::ADSR::Parameters currentParams = adsrList[*selectedSample].getParameters();

  switch (adsrParam)
  {
  case 25: // Attack
    currentParams.attack = static_cast<float>(value) / 127.0f;
    break;
  case 26: // Decay
    currentParams.decay = static_cast<float>(value) / 127.0f;
    break;
  case 27: // Sustain
    currentParams.sustain = static_cast<float>(value) / 127.0f;
    break;
  case 28: // Release
    currentParams.release = static_cast<float>(value) / 127.0f;
    break;
  default:
    std::cout << "NÃO EXISTE ESTE VALOR" << std::endl;
    return;
  }

  adsrList[*selectedSample].setParameters(currentParams);
}

void MySamplerVoice::activateSample(int sample)
{
  // If the sample is playing, gracefully stop it using ADSR
  if (sampleOn[sample])
  {
    // Call noteOff to allow a smooth release phase
    adsrList[sample].noteOff();
    // Keep playing the sample until the release phase completes
    sampleOn[sample] = false;
  }
  else
  {
    // Activate the sample
    sampleOn[sample] = true;
    samplesPosition[sample] = 0;
    // Trigger the ADSR envelope
    adsrList[sample].reset();
    adsrList[sample].noteOn();
  }
}

void MySamplerVoice::changeLowPassFilter(double sampleRate, double knobValue)
{
  double minFreq = 100.0;
  double maxFreq = 10000.0;
  double cutoffFrequency = maxFreq - (knobValue / 127.0) * (maxFreq - minFreq);
  auto newCoefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, cutoffFrequency);
  *lowPassFilters[*selectedSample].coefficients = *newCoefficients;
}

void MySamplerVoice::changeHighPassFilter(double sampleRate, double knobValue)
{
  double minFreq = 20.0;
  double maxFreq = 5000.0;
  double cutoffFrequency = minFreq + (knobValue / 127.0) * (maxFreq - minFreq);
  auto newCoefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, cutoffFrequency);
  *highPassFilters[*selectedSample].coefficients = *newCoefficients;
}

void MySamplerVoice::changeBandPassFilter(double sampleRate, double knobValue)
{
  double minFreq = 500.0;
  double maxFreq = 4000.0;
  double cutoffFrequency = minFreq + (knobValue / 127.0) * (maxFreq - minFreq);
  auto newCoefficients = juce::dsp::IIR::Coefficients<float>::makeBandPass(sampleRate, cutoffFrequency);
  *bandPassFilters[*selectedSample].coefficients = *newCoefficients;
}

void MySamplerVoice::changeReverb(double knobValue)
{
  if (knobValue == 0)
  {
    reverbs[*selectedSample].setEnabled(false);
  }
  else
  {
    reverbs[*selectedSample].setEnabled(true);
    juce::dsp::Reverb::Parameters revPrams;
    revPrams.roomSize = static_cast<float>(knobValue) / 127.0f;
    reverbs[*selectedSample].setParameters(revPrams);
  }
}

void MySamplerVoice::changeChorus(double knobValue)
{
  chorus[*selectedSample].setMix(static_cast<float>(knobValue) / 127.0f);
}
