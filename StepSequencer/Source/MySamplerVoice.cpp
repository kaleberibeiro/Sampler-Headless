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

    // PHASER SETUP //
    phaser[i].prepare(spec);
    phaser[i].setMix(0.0);

    // PANNER //
    panner[i].prepare(spec);
    panner[i].setRule(juce::dsp::PannerRule::linear);

    // DELAY //
    delay[i].prepare(spec);
    delay[i].setMaximumDelayInSamples(static_cast<int>(sampleRate));
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
        float gainValue = smoothGainRamp[voiceIndex].getNextValue();
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
          const int soundChannelIndex = channel % samplerSound->getAudioData()->getNumChannels();
          const float *audioData = samplerSound->getAudioData()->getReadPointer(soundChannelIndex);
          float finalSamples = audioData[samplesPosition[voiceIndex]] * gainValue;
          sampleBuffer.addSample(channel, startSample + sample, finalSamples);
        }

        if (sampleMakeNoise[voiceIndex])
        {
          if (samplesPosition[voiceIndex] >= lengthInSamples[voiceIndex])
          {
            if (adsrList[voiceIndex].getNextSample() >= 1)
            {
              adsrList[voiceIndex].noteOff();
            }
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
      ///// PANNER ////////
      panner[voiceIndex].process(context);
      ///// PHASER ////////
      phaser[voiceIndex].process(context);
      ///// DELAY ////////
      // delay[voiceIndex].process(context);

      adsrList[voiceIndex].applyEnvelopeToBuffer(sampleBuffer, 0, numSamples);

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
        adsrList[voiceIndex].noteOn();
      }

      if (samplesPressed[voiceIndex])
      {
        smoothGainRamp[voiceIndex].setTargetValue(previousGain[voiceIndex]);
      }
      else
      {
        smoothGainRamp[voiceIndex].setTargetValue(0.0);
      }

      for (int sample = 0; sample < numSamples; ++sample)
      {
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
          const int soundChannelIndex = channel % samplerSound->getAudioData()->getNumChannels();
          const float *audioData = samplerSound->getAudioData()->getReadPointer(soundChannelIndex);
          float finalSamples = (audioData[samplesPosition[voiceIndex]] * smoothGainRamp[voiceIndex].getNextValue());
          sampleBuffer.addSample(channel, startSample + sample, finalSamples);
        }

        if (samplesPressed[voiceIndex])
        {
          if (samplesPosition[voiceIndex] < lengthInSamples[voiceIndex])
          {
            ++samplesPosition[voiceIndex];
          }
          else
          {
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
      panner[voiceIndex].process(context);
      phaser[voiceIndex].process(context);

      adsrList[voiceIndex].applyEnvelopeToBuffer(sampleBuffer, 0, numSamples);

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
  if (sampleOn[sample])
  {
    // adsrList[sample].noteOff();
    sampleOn[sample] = false;
  }
  else
  {
    sampleOn[sample] = true;
    samplesPosition[sample] = 0;
    adsrList[sample].reset();
    adsrList[sample].noteOn();
  }
}

void MySamplerVoice::changeLowPassFilter(double sampleRate, double knobValue)
{
  double minFreq = 100.0;
  double maxFreq = 10000.0;

  double cutoffFrequency = (knobValue == 0) ? maxFreq : maxFreq - (knobValue / 127.0) * (maxFreq - minFreq);

  smoothLowRamps[*selectedSample].setTargetValue(cutoffFrequency);
}

void MySamplerVoice::changeHighPassFilter(double sampleRate, double knobValue)
{
  double minFreq = 20.0;
  double maxFreq = 5000.0;

  double cutoffFrequency = (knobValue == 0) ? minFreq : minFreq + (knobValue / 127.0) * (maxFreq - minFreq);

  smoothHighRamps[*selectedSample].setTargetValue(cutoffFrequency);
}

void MySamplerVoice::changeBandPassFilter(double sampleRate, double knobValue)
{
  double minFreq = 500.0;
  double maxFreq = 4000.0;

  double cutoffFrequency = (knobValue == 0) ? (minFreq + maxFreq) / 2 : maxFreq - (knobValue / 127.0) * (maxFreq - minFreq);

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
  chorus[*selectedSample].setDepth(juce::jlimit(0.0f, 0.3f, 0.1f + (normalizedValue * 0.2f)));        // Moderate depth (BOSS choruses tend to have rich, lush depth)
  chorus[*selectedSample].setFeedback(juce::jlimit(0.0f, 0.95f, 0.0f + (normalizedValue * 0.15f)));   // Minimal feedback (BOSS pedals usually have no to low feedback)
  chorus[*selectedSample].setRate(juce::jlimit(0.1f, 2.0f, 0.33f + (normalizedValue * 1.0f)));        // Slower modulation rate (BOSS CE-2 ranges from ~0.33Hz to 2Hz)
  chorus[*selectedSample].setCentreDelay(juce::jlimit(5.0f, 10.0f, 5.0f + (normalizedValue * 2.0f))); // Shorter delay for classic chorus (CE-2 is around 5-10 ms delay)
  chorus[*selectedSample].setMix(juce::jlimit(0.0f, 0.6f, normalizedValue));
}

void MySamplerVoice::changeFlanger(double knobValue)
{
  float normalizedValue = static_cast<float>(knobValue) / 127.0f;

  flanger[*selectedSample].setDepth(juce::jlimit(0.0f, 0.5f, 0.2f + (normalizedValue * 0.3f)));       // Deeper modulation (Boss flangers often have strong modulation)
  flanger[*selectedSample].setFeedback(juce::jlimit(0.0f, 0.95f, 0.6f + (normalizedValue * 0.3f)));   // More pronounced feedback, typical of Boss flangers
  flanger[*selectedSample].setRate(juce::jlimit(0.0f, 0.8f, 0.2f + (normalizedValue * 0.6f)));        // Slightly faster rate, but not too extreme
  flanger[*selectedSample].setCentreDelay(juce::jlimit(0.5f, 3.0f, 0.7f + (normalizedValue * 2.3f))); // Wider delay range for the sweeping flanger effect (more noticeable)
  flanger[*selectedSample].setMix(juce::jlimit(0.0f, 0.5f, (normalizedValue)));                       // Full mix from 0 (dry) to 1 (wet)
}

void MySamplerVoice::changePanner(int knobValue)
{
  knobValue = juce::jlimit(0, 127, knobValue);
  float panningValue = (static_cast<float>(knobValue) / 127.0f) * 2.0f - 1.0f;
  panner[*selectedSample].setPan(panningValue);
}

void MySamplerVoice::changePhaser(double knobValue)
{
  float normalizedValue = static_cast<float>(knobValue) / 127.0f;

  phaser[*selectedSample].setRate(juce::jlimit(0.1f, 10.0f, 0.2f + (normalizedValue * 9.8f)));                     // Phaser rate (0.1Hz to 10Hz)
  phaser[*selectedSample].setDepth(juce::jlimit(0.0f, 1.0f, 0.4f + (normalizedValue * 0.6f)));                     // Phaser depth (0.4 to 1 for stronger modulation)
  phaser[*selectedSample].setCentreFrequency(juce::jlimit(200.0f, 2000.0f, 400.0f + (normalizedValue * 1600.0f))); // Centre frequency (200Hz to 2kHz)
  phaser[*selectedSample].setFeedback(juce::jlimit(-0.95f, 0.95f, (normalizedValue * 1.9f - 0.95f)));              // Feedback (-0.95 to 0.95 for phase intensity)
  phaser[*selectedSample].setMix(juce::jlimit(0.0f, 1.0f, normalizedValue));                                       // Mix between dry (0) and wet (1)
}

void MySamplerVoice::changeDelay(int knobValue)
{
  knobValue = juce::jlimit(0, 127, knobValue);
  float maxDelayInSeconds = 1.0f; // 1 second max delay
  float delayValue = (static_cast<float>(knobValue) / 127.0f) * maxDelayInSeconds * mSampleRate;
  delay[*selectedSample].setDelay(delayValue);
}
