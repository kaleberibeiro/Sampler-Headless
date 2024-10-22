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
    previousGain[i] = 0.5;

    smoothGainRamp[i].reset(mSampleRate, 0.03);

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

  loadData();
}

void MySamplerVoice::countSamples(juce::AudioBuffer<float> &buffer, int startSample, int numSamples)
{
  buffer.clear();

  triggerSamples(buffer, startSample, numSamples);
}

void MySamplerVoice::updateSamplesActiveState()
{
  const juce::ScopedLock sl(objectLock);

  for (int i = 0; i < size; ++i)
  {
    if (sequences[i][selectedPattern[i]][currentPatternIndex[i]] == 1 && !isSampleMuted[i])
    {
      samplesPosition[i] = sampleStart[i];
      sampleMakeNoise[i] = true;
    }
  }
}

void MySamplerVoice::hiResTimerCallback()
{
  if (sequencePlaying)
  {
    for (int i = 0; i < size; i++)
    {
      int sequenceLength = sequences[i][selectedPattern[i]].size();

      if (currentPatternIndex[i] == 0)
      {
        if (isSequenceChained[i] && !sequenceChain[i].empty())
        {
          int currentChainedPattern = sequenceChain[i].front();
          sequenceChain[i].erase(sequenceChain[i].begin());
          sequenceChain[i].push_back(currentChainedPattern);

          selectedPattern[i] = currentChainedPattern;
        }
        else if (cachedPattern[i] != -1)
        {
          selectedPattern[i] = cachedPattern[i];
          cachedPattern[i] = -1;
        }
      }
    }

    updateSamplesActiveState();

    for (int i = 0; i < size; i++)
    {
      int sequenceLength = sequences[i][selectedPattern[i]].size();
      currentPatternIndex[i] = (currentPatternIndex[i] + 1) % sequenceLength;
    }
  }
}

void MySamplerVoice::activateSample(int sample, int sampleValue)
{
  if (sampleValue == 0)
  {
    isSampleMuted[sample] = true;
    smoothGainRamp[sample].reset(mSampleRate, 0.03);
    smoothGainRamp[sample].setTargetValue(0.0f);
  }
  else
  {
    isSampleMuted[sample] = false;
  }
}

void MySamplerVoice::triggerSamples(juce::AudioBuffer<float> &buffer, int startSample, int numSamples)
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

      if (sampleMakeNoise[voiceIndex] != previousSampleMakeNoise[voiceIndex]) // Only act if state has changed
      {
        if (sampleMakeNoise[voiceIndex])
        {
          smoothGainRamp[voiceIndex].setCurrentAndTargetValue(previousGain[voiceIndex]);
        }
        else
        {
          smoothGainRamp[voiceIndex].setTargetValue(0.0f);
        }

        previousSampleMakeNoise[voiceIndex] = sampleMakeNoise[voiceIndex];
      }

      if ((smoothGainRamp[voiceIndex].getCurrentValue() == 0 || (adsrList[voiceIndex].getNextSample() == 0) && samplesPosition[voiceIndex] >= lengthInSamples[voiceIndex]) && sampleMakeNoise[voiceIndex])
      {
        sampleMakeNoise[voiceIndex] = false;
      }

      if ((smoothGainRamp[voiceIndex].getCurrentValue() != 0 || smoothGainRamp[voiceIndex].getTargetValue() != 0) && sampleMakeNoise[voiceIndex])
      {
        if (samplesPosition[voiceIndex] == sampleStart[voiceIndex])
        {
          adsrList[voiceIndex].reset();
          adsrList[voiceIndex].noteOn();

          duplicatorsLowPass[voiceIndex].reset();
          duplicatorsHighPass[voiceIndex].reset();
          duplicatorsBandPass[voiceIndex].reset();
          reverbs[voiceIndex].reset();
          chorus[voiceIndex].reset();
          flanger[voiceIndex].reset();
          panner[voiceIndex].reset();
          phaser[voiceIndex].reset();
        }

        const int soundChannels = samplerSound->getAudioData()->getNumChannels();

        for (int sample = 0; sample < numSamples; ++sample)
        {
          float gainValue = smoothGainRamp[voiceIndex].getNextValue();

          for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
          {
            const int soundChannelIndex = channel % soundChannels;
            const float *audioData = samplerSound->getAudioData()->getReadPointer(soundChannelIndex);
            float finalSamples = audioData[samplesPosition[voiceIndex]] * gainValue;
            sampleBuffer.addSample(channel, startSample + sample, finalSamples);
          }

          if (samplesPosition[voiceIndex] < lengthInSamples[voiceIndex])
          {
            ++samplesPosition[voiceIndex];
          }
          else if (adsrList[voiceIndex].getNextSample() >= 1)
          {
            adsrList[voiceIndex].noteOff();
          }
        }

        juce::dsp::AudioBlock<float> audioBlock(sampleBuffer);
        juce::dsp::ProcessContextReplacing<float> context(audioBlock);

        if (lastLowPassKnob[voiceIndex] != 0)
        {
          *duplicatorsLowPass[voiceIndex].state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(mSampleRate, smoothLowRamps[voiceIndex].getNextValue());
          duplicatorsLowPass[voiceIndex].process(context);
        }
        if (lastHighPassKnob[voiceIndex] != 0)
        {
          *duplicatorsHighPass[voiceIndex].state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(mSampleRate, smoothHighRamps[voiceIndex].getNextValue());
          duplicatorsHighPass[voiceIndex].process(context);
        }
        if (lastBandPassKnob[voiceIndex] != 0)
        {
          *duplicatorsBandPass[voiceIndex].state = *juce::dsp::IIR::Coefficients<float>::makeBandPass(mSampleRate, smoothBandRamps[voiceIndex].getNextValue());
          duplicatorsBandPass[voiceIndex].process(context);
        }

        reverbs[voiceIndex].process(context);
        chorus[voiceIndex].process(context);
        flanger[voiceIndex].process(context);
        panner[voiceIndex].process(context);
        phaser[voiceIndex].process(context);

        adsrList[voiceIndex].applyEnvelopeToBuffer(sampleBuffer, 0, numSamples);
      }

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

void MySamplerVoice::changeLowPassFilter(double sampleRate, int knobValue)
{
  double minFreq = 100.0;
  double maxFreq = 10000.0;

  double cutoffFrequency = (knobValue == 0) ? maxFreq : maxFreq - (knobValue / 127.0) * (maxFreq - minFreq);

  smoothLowRamps[*selectedSample].setTargetValue(cutoffFrequency);
  lastLowPassKnob[*selectedSample] = knobValue;
}

void MySamplerVoice::changeHighPassFilter(double sampleRate, int knobValue)
{
  double minFreq = 20.0;
  double maxFreq = 5000.0;

  double cutoffFrequency = (knobValue == 0) ? minFreq : minFreq + (knobValue / 127.0) * (maxFreq - minFreq);

  smoothHighRamps[*selectedSample].setTargetValue(cutoffFrequency);
  lastHighPassKnob[*selectedSample] = knobValue;
}

void MySamplerVoice::changeBandPassFilter(double sampleRate, int knobValue)
{
  double minFreq = 500.0;
  double maxFreq = 4000.0;

  double cutoffFrequency = (knobValue == 0) ? (minFreq + maxFreq) / 2 : maxFreq - (knobValue / 127.0) * (maxFreq - minFreq);

  smoothBandRamps[*selectedSample].setTargetValue(cutoffFrequency);
  lastBandPassKnob[*selectedSample] = knobValue;
}

void MySamplerVoice::changeReverb(int knobValue)
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
    revParams.damping = juce::jlimit(0.0f, 0.2f, normalizedValue * 0.4f);
    revParams.width = juce::jlimit(0.0f, 0.4f, normalizedValue * 0.4f);
    revParams.wetLevel = juce::jlimit(0.1f, 0.2f, normalizedValue * 0.2f);
    revParams.dryLevel = juce::jlimit(0.5f, 0.7f, 0.7f - revParams.wetLevel);
    revParams.freezeMode = 0.0f;

    reverbs[*selectedSample].setParameters(revParams);

    lastReverbKnob[*selectedSample] = knobValue;
  }
}

void MySamplerVoice::changeChorus(int knobValue)
{
  float normalizedValue = static_cast<float>(knobValue) / 127.0f;

  // Melhorando a profundidade do chorus para mais "largura" no som
  chorus[*selectedSample].setDepth(juce::jlimit(0.0f, 0.3f, 0.1f + (normalizedValue * 0.2f)));        // Moderate depth (BOSS choruses tend to have rich, lush depth)
  chorus[*selectedSample].setFeedback(juce::jlimit(0.0f, 0.95f, 0.0f + (normalizedValue * 0.15f)));   // Minimal feedback (BOSS pedals usually have no to low feedback)
  chorus[*selectedSample].setRate(juce::jlimit(0.1f, 2.0f, 0.33f + (normalizedValue * 1.0f)));        // Slower modulation rate (BOSS CE-2 ranges from ~0.33Hz to 2Hz)
  chorus[*selectedSample].setCentreDelay(juce::jlimit(5.0f, 10.0f, 5.0f + (normalizedValue * 2.0f))); // Shorter delay for classic chorus (CE-2 is around 5-10 ms delay)
  chorus[*selectedSample].setMix(juce::jlimit(0.0f, 0.6f, normalizedValue));

  lastChorusKnob[*selectedSample] = knobValue;
}

void MySamplerVoice::changeFlanger(int knobValue)
{
  float normalizedValue = static_cast<float>(knobValue) / 127.0f;

  flanger[*selectedSample].setDepth(juce::jlimit(0.0f, 0.5f, 0.2f + (normalizedValue * 0.3f)));       // Deeper modulation (Boss flangers often have strong modulation)
  flanger[*selectedSample].setFeedback(juce::jlimit(0.0f, 0.95f, 0.6f + (normalizedValue * 0.3f)));   // More pronounced feedback, typical of Boss flangers
  flanger[*selectedSample].setRate(juce::jlimit(0.0f, 0.8f, 0.2f + (normalizedValue * 0.6f)));        // Slightly faster rate, but not too extreme
  flanger[*selectedSample].setCentreDelay(juce::jlimit(0.5f, 3.0f, 0.7f + (normalizedValue * 2.3f))); // Wider delay range for the sweeping flanger effect (more noticeable)
  flanger[*selectedSample].setMix(juce::jlimit(0.0f, 0.5f, (normalizedValue)));

  lastFlangerKnob[*selectedSample] = knobValue;
}

void MySamplerVoice::changePanner(int knobValue)
{
  float panningValue = (static_cast<float>(knobValue) / 127.0f) * 2.0f - 1.0f;
  panner[*selectedSample].setPan(panningValue);

  lastPannerKnob[*selectedSample] = knobValue;
}

void MySamplerVoice::changePhaser(int knobValue)
{
  float normalizedValue = static_cast<float>(knobValue) / 127.0f;

  phaser[*selectedSample].setRate(juce::jlimit(0.1f, 10.0f, 0.2f + (normalizedValue * 9.8f)));                     // Phaser rate (0.1Hz to 10Hz)
  phaser[*selectedSample].setDepth(juce::jlimit(0.0f, 1.0f, 0.4f + (normalizedValue * 0.6f)));                     // Phaser depth (0.4 to 1 for stronger modulation)
  phaser[*selectedSample].setCentreFrequency(juce::jlimit(200.0f, 2000.0f, 400.0f + (normalizedValue * 1600.0f))); // Centre frequency (200Hz to 2kHz)
  phaser[*selectedSample].setFeedback(juce::jlimit(-0.95f, 0.95f, (normalizedValue * 1.9f - 0.95f)));              // Feedback (-0.95 to 0.95 for phase intensity)
  phaser[*selectedSample].setMix(juce::jlimit(0.0f, 1.0f, normalizedValue));

  lastPhaserKnob[*selectedSample] = knobValue;
}

void MySamplerVoice::changeDelay(int knobValue)
{
  knobValue = juce::jlimit(0, 127, knobValue);
  float maxDelayInSeconds = 1.0f; // 1 second max delay
  float delayValue = (static_cast<float>(knobValue) / 127.0f) * maxDelayInSeconds * mSampleRate;
  delay[*selectedSample].setDelay(delayValue);
}

void MySamplerVoice::clearPattern()
{
  auto &sequence = sequences[*selectedSample][selectedPattern[*selectedSample]];

  for (int i = 0; i < sequence.size(); i++)
  {
    sequence[i] = 0;
  }
}

void MySamplerVoice::clearSampleModulation()
{
  juce::ADSR::Parameters defaultADSRParams;
  defaultADSRParams.attack = 0.0f;
  defaultADSRParams.decay = 0.1f;
  defaultADSRParams.sustain = 1.0f;
  defaultADSRParams.release = 0.1f;
  adsrList[*selectedSample].setParameters(defaultADSRParams);

  lastLowPassKnob[*selectedSample] = 0;
  lastHighPassKnob[*selectedSample] = 0;
  lastBandPassKnob[*selectedSample] = 0;

  lastReverbKnob[*selectedSample] = 0;
  lastChorusKnob[*selectedSample] = 0;
  lastFlangerKnob[*selectedSample] = 0;
  lastPannerKnob[*selectedSample] = 64;
  lastPhaserKnob[*selectedSample] = 0;

  changeReverb(0);
  changeChorus(0);
  changeFlanger(0);
  changePanner(64);
  changePhaser(0);

  sampleStart[*selectedSample] = 0;
  sampleLength[*selectedSample] = lengthInSamples[*selectedSample];
}

void MySamplerVoice::saveData()
{
  juce::File file = juce::File::getSpecialLocation(juce::File::userDesktopDirectory).getChildFile("data.txt");

  juce::FileOutputStream outputStream(file);

  if (!outputStream.openedOk())
  {
    DBG("Error opening file for writing.");
    return;
  }

  outputStream.setPosition(0);
  outputStream.writeText("SEQUENCES:\n", false, false, "\n");

  for (int sampleIndex = 0; sampleIndex < sequences.size(); sampleIndex++)
  {
    outputStream.writeText("Sample " + juce::String(sampleIndex) + ":\n", false, false, "\n");

    for (int patternIndex = 0; patternIndex < sequences[sampleIndex].size(); ++patternIndex)
    {
      outputStream.writeText("  Pattern " + juce::String(patternIndex) + ": ", false, false, "\n");

      for (int step = 0; step < sequences[sampleIndex][patternIndex].size(); ++step)
      {
        outputStream.writeText(juce::String(sequences[sampleIndex][patternIndex][step]) + " ", false, false, "\n");
      }
      outputStream.writeText("\n", false, false, "\n");
    }
  }

  outputStream.writeText("\nSample stretch:\n", false, false, "\n");

  for (int sampleIndex = 0; sampleIndex < size; sampleIndex++)
  {
    outputStream.writeText("Sample " + juce::String(sampleIndex) + ":\n", false, false, "\n");
    outputStream.writeText("Start=" + juce::String(sampleStart[sampleIndex]) + "\n", false, false, "\n");
    outputStream.writeText("Length=" + juce::String(sampleLength[sampleIndex]) + "\n", false, false, "\n");
    outputStream.writeText("\n", false, false, "\n");
  }

  outputStream.writeText("\nADSR:\n", false, false, "\n");

  for (int sampleIndex = 0; sampleIndex < size; sampleIndex++)
  {
    outputStream.writeText("Sample " + juce::String(sampleIndex) + ":\n", false, false, "\n");
    outputStream.writeText("Attack=" + juce::String(adsrList[sampleIndex].getParameters().attack) + "\n", false, false, "\n");
    outputStream.writeText("Decay=" + juce::String(adsrList[sampleIndex].getParameters().decay) + "\n", false, false, "\n");
    outputStream.writeText("Sustain=" + juce::String(adsrList[sampleIndex].getParameters().sustain) + "\n", false, false, "\n");
    outputStream.writeText("Release=" + juce::String(adsrList[sampleIndex].getParameters().release) + "\n", false, false, "\n");
    outputStream.writeText("\n", false, false, "\n");
  }

  outputStream.writeText("\nFilters:\n", false, false, "\n");

  for (int sampleIndex = 0; sampleIndex < size; sampleIndex++)
  {
    outputStream.writeText("Sample " + juce::String(sampleIndex) + ":\n", false, false, "\n");
    outputStream.writeText("LowPass=" + juce::String(smoothLowRamps[sampleIndex].getCurrentValue()) + "\n", false, false, "\n");
    outputStream.writeText("HighPass=" + juce::String(smoothHighRamps[sampleIndex].getCurrentValue()) + "\n", false, false, "\n");
    outputStream.writeText("BandPass=" + juce::String(smoothBandRamps[sampleIndex].getCurrentValue()) + "\n", false, false, "\n");
    outputStream.writeText("\n", false, false, "\n");
  }

  outputStream.writeText("\nEffects:\n", false, false, "\n");

  for (int sampleIndex = 0; sampleIndex < size; sampleIndex++)
  {
    outputStream.writeText("Sample " + juce::String(sampleIndex) + ":\n", false, false, "\n");
    outputStream.writeText("Reverb=" + juce::String(lastReverbKnob[sampleIndex]) + "\n", false, false, "\n");
    outputStream.writeText("Chorus=" + juce::String(lastChorusKnob[sampleIndex]) + "\n", false, false, "\n");
    outputStream.writeText("Flanger=" + juce::String(lastFlangerKnob[sampleIndex]) + "\n", false, false, "\n");
    outputStream.writeText("Panner=" + juce::String(lastPannerKnob[sampleIndex]) + "\n", false, false, "\n");
    outputStream.writeText("Phaser=" + juce::String(lastPhaserKnob[sampleIndex]) + "\n", false, false, "\n");
    outputStream.writeText("\n", false, false, "\n");
  }

  outputStream.flush();

  DBG("Data successfully saved to " + file.getFullPathName());
}

void MySamplerVoice::loadData()
{
  juce::File file = juce::File::getSpecialLocation(juce::File::userDesktopDirectory).getChildFile("data.txt");

  if (!file.existsAsFile())
  {
    DBG("Error: data file not found.");
    return;
  }

  juce::FileInputStream inputStream(file);

  if (!inputStream.openedOk())
  {
    DBG("Error opening file for reading.");
    return;
  }

  juce::String line;
  int sampleIndex = 0;
  int patternIndex = 0;

  while (!inputStream.isExhausted())
  {
    line = inputStream.readNextLine().trim();

    // Load Sequences
    if (line.startsWith("SEQUENCES:"))
    {
      while ((line = inputStream.readNextLine().trim()).startsWith("Sample"))
      {
        sampleIndex = line.fromFirstOccurrenceOf("Sample", false, false).getIntValue();

        for (patternIndex = 0; patternIndex < sequences[sampleIndex].size(); ++patternIndex)
        {
          line = inputStream.readNextLine().trim();

          juce::String trimmedLine = line.fromFirstOccurrenceOf(":", false, false).trim();

          juce::StringArray stepValues;
          stepValues.addTokens(trimmedLine, false);

          for (int step = 0; step < stepValues.size(); ++step)
          {
            sequences[sampleIndex][patternIndex][step] = stepValues[step].getIntValue();
          }
        }
      }
    }

    // Load Sample Stretch
    if (line.startsWith("Sample stretch:"))
    {
      while ((line = inputStream.readNextLine().trim()).startsWith("Sample"))
      {
        sampleIndex = line.fromFirstOccurrenceOf("Sample", false, false).getIntValue();

        juce::String startLine = inputStream.readNextLine().trim();
        sampleStart[sampleIndex] = startLine.fromFirstOccurrenceOf("Start=", false, false).getIntValue();

        juce::String lengthLine = inputStream.readNextLine().trim();
        sampleLength[sampleIndex] = lengthLine.fromFirstOccurrenceOf("Length=", false, false).getIntValue();
      }
    }

    // Load ADSR
    if (line.startsWith("ADSR:"))
    {
      while ((line = inputStream.readNextLine().trim()).startsWith("Sample"))
      {
        sampleIndex = line.fromFirstOccurrenceOf("Sample", false, false).getIntValue();

        juce::ADSR::Parameters adsrParams;
        juce::String attackLine = inputStream.readNextLine().trim();
        adsrParams.attack = attackLine.fromFirstOccurrenceOf("Attack=", false, false).getFloatValue();

        juce::String decayLine = inputStream.readNextLine().trim();
        adsrParams.decay = decayLine.fromFirstOccurrenceOf("Decay=", false, false).getFloatValue();

        juce::String sustainLine = inputStream.readNextLine().trim();
        adsrParams.sustain = sustainLine.fromFirstOccurrenceOf("Sustain=", false, false).getFloatValue();

        juce::String releaseLine = inputStream.readNextLine().trim();
        adsrParams.release = releaseLine.fromFirstOccurrenceOf("Release=", false, false).getFloatValue();

        adsrList[sampleIndex].setParameters(adsrParams);
      }
    }

    // Load Filters
    if (line.startsWith("Filters:"))
    {
      while ((line = inputStream.readNextLine().trim()).startsWith("Sample"))
      {
        sampleIndex = line.fromFirstOccurrenceOf("Sample", false, false).getIntValue();

        juce::String lowPassLine = inputStream.readNextLine().trim();
        smoothLowRamps[sampleIndex].setCurrentAndTargetValue(lowPassLine.fromFirstOccurrenceOf("LowPass=", false, false).getFloatValue());

        juce::String highPassLine = inputStream.readNextLine().trim();
        smoothHighRamps[sampleIndex].setCurrentAndTargetValue(highPassLine.fromFirstOccurrenceOf("HighPass=", false, false).getFloatValue());

        juce::String bandPassLine = inputStream.readNextLine().trim();
        smoothBandRamps[sampleIndex].setCurrentAndTargetValue(bandPassLine.fromFirstOccurrenceOf("BandPass=", false, false).getFloatValue());
      }
    }

    // Load Effects
    if (line.startsWith("Effects:"))
    {
      while ((line = inputStream.readNextLine().trim()).startsWith("Sample"))
      {
        sampleIndex = line.fromFirstOccurrenceOf("Sample", false, false).getIntValue();

        juce::String reverbLine = inputStream.readNextLine().trim();
        int reverbValue = reverbLine.fromFirstOccurrenceOf("Reverb=", false, false).getIntValue();
        lastReverbKnob[sampleIndex] = reverbValue;
        if (reverbValue != 0)
        {
          changeReverb(reverbValue);
        }

        juce::String chorusLine = inputStream.readNextLine().trim();
        int chorusValue = chorusLine.fromFirstOccurrenceOf("Chorus=", false, false).getIntValue();
        lastChorusKnob[sampleIndex] = chorusValue;
        if (chorusValue != 0)
        {
          changeChorus(chorusValue);
        }

        juce::String flangerLine = inputStream.readNextLine().trim();
        int flangerValue = flangerLine.fromFirstOccurrenceOf("Flanger=", false, false).getIntValue();
        lastFlangerKnob[sampleIndex] = flangerValue;
        if (flangerValue != 0)
        {
          changeFlanger(flangerValue);
        }

        juce::String pannerLine = inputStream.readNextLine().trim();
        int pannerValue = pannerLine.fromFirstOccurrenceOf("Panner=", false, false).getIntValue();
        lastPannerKnob[sampleIndex] = pannerValue;
        if (pannerValue != 0)
        {
          changePanner(pannerValue);
        }

        juce::String phaserLine = inputStream.readNextLine().trim();
        int phaserValue = phaserLine.fromFirstOccurrenceOf("Phaser=", false, false).getIntValue();
        lastPhaserKnob[sampleIndex] = phaserValue;
        if (phaserValue != 0)
        {
          changePhaser(phaserValue);
        }
      }
    }
  }
}