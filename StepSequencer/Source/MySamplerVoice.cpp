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
    smoothGainRamp[i].reset(sampleRate, 0.01);
    smoothGainRamp[i].setTargetValue(0.5);

    duplicatorsLowPass[i].prepare(spec);
    duplicatorsHighPass[i].prepare(spec);
    duplicatorsBandPass[i].prepare(spec);
    duplicatorsLowPass[i].reset();
    duplicatorsHighPass[i].reset();
    duplicatorsBandPass[i].reset();

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

  if (sequencePlaying)
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
      if (!sampleOn[i])
      {
        sampleMakeNoise[i] = false;
      }
    }
  }
}

void MySamplerVoice::hiResTimerCallback()
{
  if (sequencePlaying)
  {
    if (currentSequenceIndex >= sequenceSize)
    {
      currentSequenceIndex = 0;
    }

    updateSamplesActiveState();

    currentSequenceIndex++;
  }
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
        }

        for (int sample = 0; sample < samplesToCopy; ++sample)
        {
          // PRIMEIRO PASSO VER SE O SAMPLE ACABOU //
          if (samplesPosition[voiceIndex] >= lengthInSamples[voiceIndex])
          {
            // Call noteOff to allow a smooth release phase
            adsrList[voiceIndex].noteOff();
          }

          float adsrValue = adsrList[voiceIndex].getNextSample();
          for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
          {
            const int soundChannelIndex = channel % samplerSound->getAudioData()->getNumChannels();
            const float *audioData = samplerSound->getAudioData()->getReadPointer(soundChannelIndex);
            float finalSamples = (audioData[samplesPosition[voiceIndex]] * adsrValue * smoothGainRamp[voiceIndex].getNextValue());
            sampleBuffer.addSample(channel, startSample + sample, finalSamples);
          }

          ++samplesPosition[voiceIndex];
        }

        ///// AUDIO BLOCK ////////
        juce::dsp::AudioBlock<float> audioBlock(sampleBuffer);
        juce::dsp::ProcessContextReplacing<float> context(audioBlock);

        ///// IIR FILTERS ///////
        if (smoothLowRamps[voiceIndex].getCurrentValue() > 0)
        {
          *duplicatorsLowPass[voiceIndex].state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(mSampleRate, smoothLowRamps[voiceIndex].getCurrentValue());
          duplicatorsLowPass[voiceIndex].process(context);
        }
        if (smoothHighRamps[voiceIndex].getCurrentValue() > 0)
        {
          *duplicatorsHighPass[voiceIndex].state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(mSampleRate, smoothHighRamps[voiceIndex].getCurrentValue());
          duplicatorsHighPass[voiceIndex].process(context);
        }
        if (smoothBandRamps[voiceIndex].getCurrentValue() > 0)
        {
          *duplicatorsBandPass[voiceIndex].state = *juce::dsp::IIR::Coefficients<float>::makeBandPass(mSampleRate, smoothBandRamps[voiceIndex].getCurrentValue());
          duplicatorsBandPass[voiceIndex].process(context);
        }

        ///// REVERB ////////
        reverbs[voiceIndex].process(context);
        ///// CHORUS ////////
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
    // Trigger the release phase of the ADSR envelope
    adsrList[sample].noteOff();
    sampleOn[sample] = false;
    // Optional: Add logic to manage the release phase duration
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
  smoothLowRamps[*selectedSample].setTargetValue(cutoffFrequency);
}

void MySamplerVoice::changeHighPassFilter(double sampleRate, double knobValue)
{
  double minFreq = 20.0;
  double maxFreq = 5000.0;
  double cutoffFrequency = minFreq + (knobValue / 127.0) * (maxFreq - minFreq);
  smoothHighRamps[*selectedSample].setTargetValue(cutoffFrequency);
}

void MySamplerVoice::changeBandPassFilter(double sampleRate, double knobValue)
{
  double minFreq = 500.0;
  double maxFreq = 4000.0;
  double cutoffFrequency = maxFreq - (knobValue / 127.0) * (maxFreq - minFreq);
  smoothBandRamps[*selectedSample].setTargetValue(cutoffFrequency);
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

    juce::dsp::Reverb::Parameters revParams;

    // Map knobValue from 0-127 to 0.0-1.0
    float normalizedValue = static_cast<float>(knobValue) / 127.0f;

    // Adjusting the parameters for more natural reverb
    revParams.roomSize = juce::jlimit(0.1f, 0.9f, normalizedValue);       // Room Size (0.1 to 0.9 for more natural effect)
    revParams.damping = juce::jlimit(0.1f, 0.7f, normalizedValue * 0.5f); // Damping (0.1 to 0.7)
    revParams.width = juce::jlimit(0.2f, 1.0f, normalizedValue);          // Width (0.2 to 1.0 for wider stereo image)
    revParams.wetLevel = juce::jlimit(0.1f, 0.9f, normalizedValue);
    revParams.dryLevel = 1.0;      
    revParams.freezeMode = 0.0f;                                          // Disabled

    reverbs[*selectedSample].setParameters(revParams);
  }
}

void MySamplerVoice::changeChorus(double knobValue)
{
  chorus[*selectedSample].setMix(static_cast<float>(knobValue) / 127.0f);
}
