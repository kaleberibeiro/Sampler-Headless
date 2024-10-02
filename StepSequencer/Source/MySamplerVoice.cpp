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
    previousGain[i] = 0.5;

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

    // FLANGER SETUP //
    flanger[i].prepare(spec);
    flanger[i].setCentreDelay(2.0);
    flanger[i].setFeedback(0.8);
    flanger[i].setDepth(0.1);
    flanger[i].setMix(0.0);
  }
}

void MySamplerVoice::countSamples(juce::AudioBuffer<float> &buffer, int startSample, int numSamples)
{
  buffer.clear();

  if (sequencePlaying)
  {
    triggerSamples(buffer, startSample, numSamples);
  }
  else
  {
    playSampleProcess(buffer, startSample, numSamples);
  }
}

void MySamplerVoice::updateSamplesActiveState()
{
  for (int i = 0; i < size; ++i)
  {
    if (sequences[i][currentSequenceIndex] == 1 && sampleOn[i])
    {
      samplesPosition[i] = sampleStart[i];
      sampleMakeNoise[i] = true;
      adsrList[i].reset();
      adsrList[i].noteOn();
    }
    else
    {
      if (sampleOn[i] == false)
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

    juce::SynthesiserSound::Ptr soundPtr = mySynth->getSound(voiceIndex);
    juce::SamplerSound *samplerSound = dynamic_cast<juce::SamplerSound *>(soundPtr.get());
    if (samplerSound != nullptr)
    {
      if (samplesPosition[voiceIndex] == sampleStart[voiceIndex])
      {
        // Reset ADSR and filters only on the first playback
        adsrList[voiceIndex].reset();
        adsrList[voiceIndex].noteOn();
      }

      if (!sampleMakeNoise[voiceIndex])
      {
        smoothGainRamp[voiceIndex].setTargetValue(0.0);
      }
      else
      {
        smoothGainRamp[voiceIndex].setTargetValue(previousGain[voiceIndex]);
      }

      for (int sample = 0; sample < numSamples; ++sample)
      {
        float adsrValue = adsrList[voiceIndex].getNextSample();
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
          const int soundChannelIndex = channel % samplerSound->getAudioData()->getNumChannels();
          const float *audioData = samplerSound->getAudioData()->getReadPointer(soundChannelIndex);
          float finalSamples = (audioData[samplesPosition[voiceIndex]] * adsrValue * smoothGainRamp[voiceIndex].getNextValue());
          sampleBuffer.addSample(channel, startSample + sample, finalSamples);
        }

        if (sampleMakeNoise[voiceIndex])
        {
          if (samplesPosition[voiceIndex] >= lengthInSamples[voiceIndex])
          {
            adsrList[voiceIndex].noteOff();
          }
          else
          {
            ++samplesPosition[voiceIndex];
          }
        }
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
      ///// FLANGER ////////
      flanger[voiceIndex].process(context);

      // Add sample buffer to batch buffer
      for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
      {
        batchBuffer.addFrom(channel, 0, sampleBuffer, channel, 0, numSamples);
      }
    }
  }

  for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
  {
    buffer.addFrom(channel, startSample, batchBuffer, channel, 0, numSamples);
  }
}

void MySamplerVoice::playSampleProcess(juce::AudioBuffer<float> &buffer, int startSample, int numSamples)
{
  const juce::ScopedLock sl(objectLock);

  // Prepare a batch buffer for processing all voices together
  juce::AudioBuffer<float> batchBuffer(buffer.getNumChannels(), numSamples);
  batchBuffer.clear();

  for (int voiceIndex = 0; voiceIndex < mySynth->getNumVoices(); ++voiceIndex)
  {
    juce::AudioBuffer<float> sampleBuffer(buffer.getNumChannels(), numSamples);
    sampleBuffer.clear();

    juce::SynthesiserSound::Ptr soundPtr = mySynth->getSound(voiceIndex);
    juce::SamplerSound *samplerSound = dynamic_cast<juce::SamplerSound *>(soundPtr.get());
    if (samplerSound != nullptr)
    {
      if (samplesPosition[voiceIndex] == sampleStart[voiceIndex])
      {
        // Reset ADSR and filters only on the first playback
        adsrList[voiceIndex].reset();
        adsrList[voiceIndex].noteOn();
      }

      // Handle gain ramping
      if (samplesPressed[voiceIndex])
      {
        smoothGainRamp[voiceIndex].setTargetValue(previousGain[voiceIndex]);
      }
      else
      {
        // Do not reset sample position here; just set target gain to 0
        smoothGainRamp[voiceIndex].setTargetValue(0.0);
      }

      for (int sample = 0; sample < numSamples; ++sample)
      {
        float adsrValue = adsrList[voiceIndex].getNextSample();
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
          const int soundChannelIndex = channel % samplerSound->getAudioData()->getNumChannels();
          const float *audioData = samplerSound->getAudioData()->getReadPointer(soundChannelIndex);
          float finalSamples = (audioData[samplesPosition[voiceIndex]] * adsrValue * smoothGainRamp[voiceIndex].getNextValue());
          sampleBuffer.addSample(channel, startSample + sample, finalSamples);
        }

        // Check if the sample is pressed to update the sample position
        if (samplesPressed[voiceIndex])
        {
          if (samplesPosition[voiceIndex] < lengthInSamples[voiceIndex])
          {
            ++samplesPosition[voiceIndex];
          }
          else
          {
            // Call noteOff when the sample ends
            adsrList[voiceIndex].noteOff();
          }
        }
      }

      // Continue with filter, reverb, chorus processing, etc.
      juce::dsp::AudioBlock<float> audioBlock(sampleBuffer);
      juce::dsp::ProcessContextReplacing<float> context(audioBlock);

      // IIR Filter processing
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

      // Reverb and Chorus processing
      reverbs[voiceIndex].process(context);
      chorus[voiceIndex].process(context);
      flanger[voiceIndex].process(context);

      // Add sample buffer to batch buffer
      for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
      {
        batchBuffer.addFrom(channel, 0, sampleBuffer, channel, 0, numSamples);
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

  // When knobValue is 0, set to default value (bypass)
  double cutoffFrequency = (knobValue == 0) ? maxFreq : maxFreq - (knobValue / 127.0) * (maxFreq - minFreq);

  // Set target value for smooth transition
  smoothLowRamps[*selectedSample].setTargetValue(cutoffFrequency);
}

void MySamplerVoice::changeHighPassFilter(double sampleRate, double knobValue)
{
  double minFreq = 20.0;
  double maxFreq = 5000.0;

  // When knobValue is 0, set to default value (bypass)
  double cutoffFrequency = (knobValue == 0) ? minFreq : minFreq + (knobValue / 127.0) * (maxFreq - minFreq);

  // Set target value for smooth transition
  smoothHighRamps[*selectedSample].setTargetValue(cutoffFrequency);
}

void MySamplerVoice::changeBandPassFilter(double sampleRate, double knobValue)
{
  double minFreq = 500.0;
  double maxFreq = 4000.0;

  // When knobValue is 0, set to default value (bypass)
  double cutoffFrequency = (knobValue == 0) ? (minFreq + maxFreq) / 2 : maxFreq - (knobValue / 127.0) * (maxFreq - minFreq);

  // Set target value for smooth transition
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

    float normalizedValue = static_cast<float>(knobValue) / 127.0f;

    // Adjust parameters with scaling for balance
    revParams.roomSize = juce::jlimit(0.0f, 0.5f, normalizedValue * 0.5f);
    revParams.damping = juce::jlimit(0.0f, 0.4f, normalizedValue * 0.4f);
    revParams.width = juce::jlimit(0.0f, 0.6f, normalizedValue * 0.6f);
    revParams.wetLevel = juce::jlimit(0.0f, 0.2f, normalizedValue * 0.2f);          // Reduced wet level
    revParams.dryLevel = juce::jlimit(0.0f, 0.5f, 1.0f - (normalizedValue * 0.5f)); // Adjusted dry level
    revParams.freezeMode = 0.0f;                                                    // Disabled

    reverbs[*selectedSample].setParameters(revParams);
  }
}

void MySamplerVoice::changeChorus(double knobValue)
{
  float normalizedValue = static_cast<float>(knobValue) / 127.0f;

  // Melhorando a profundidade do chorus para mais "largura" no som
  chorus[*selectedSample].setDepth(juce::jlimit(0.0f, 1.0f, 0.3f + (normalizedValue * 0.4f)));        // Profundidade moderada
  chorus[*selectedSample].setFeedback(juce::jlimit(0.0f, 0.95f, 0.1f + (normalizedValue * 0.2f)));    // Feedback sutil
  chorus[*selectedSample].setRate(juce::jlimit(0.0f, 5.0f, 0.5f + (normalizedValue * 1.5f)));         // Taxa de modulação mais lenta
  chorus[*selectedSample].setCentreDelay(juce::jlimit(5.0f, 15.0f, 7.0f + (normalizedValue * 3.0f))); // Atraso central típico
  chorus[*selectedSample].setMix(juce::jlimit(0.0f, 1.0f, 0.4f + (normalizedValue * 0.3f)));          // Mix equilibrado
                                                                                                      // Controle de mix
}

void MySamplerVoice::changeFlanger(double knobValue)
{
  float normalizedValue = static_cast<float>(knobValue) / 127.0f;

  // Ajustando o flanger para um som clássico
  flanger[*selectedSample].setDepth(juce::jlimit(0.0f, 1.0f, 0.3f + (normalizedValue * 0.3f)));       // Profundidade moderada para um efeito mais suave
  flanger[*selectedSample].setFeedback(juce::jlimit(0.0f, 0.95f, 0.4f + (normalizedValue * 0.3f)));   // Feedback controlado, entre 0.4 e 0.7
  flanger[*selectedSample].setRate(juce::jlimit(0.0f, 1.0f, 0.1f + (normalizedValue * 0.9f)));        // Taxa mais lenta para um flanger clássico
  flanger[*selectedSample].setCentreDelay(juce::jlimit(0.2f, 2.0f, 0.5f + (normalizedValue * 1.5f))); // Atraso central entre 0.5 ms e 2 ms
  flanger[*selectedSample].setMix(juce::jlimit(0.0f, 1.0f, 0.5f + (normalizedValue * 0.4f)));         // Mix entre 0.5 e 0.9 para balancear o efeito
}
