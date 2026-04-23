/*
==============================================================================

    SamplerVoice.cpp
    Created: 17 Apr 2024 4:16:17pm
    Author:  kalebe

  ==============================================================================
*/

#include "MySamplerVoice.h"

void MySamplerVoice::updateStepTiming()
{
  const double samplesPerStep = (mSampleRate * 60.0) / (juce::jmax(1, mBpm) * 4.0);
  stepIntervalSamples = juce::jmax(1, static_cast<int>(std::round(samplesPerStep)));
  if (samplesUntilNextStep <= 0 || samplesUntilNextStep > stepIntervalSamples)
  {
    samplesUntilNextStep = stepIntervalSamples;
  }
}

void MySamplerVoice::processSequencerStep()
{
  for (int i = 0; i < size; ++i)
  {
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

  for (int i = 0; i < size; ++i)
  {
    const int sequenceLength = static_cast<int>(sequences[i][selectedPattern[i]].size());
    currentPatternIndex[i] = (currentPatternIndex[i] + 1) % juce::jmax(1, sequenceLength);
  }
}

void MySamplerVoice::advanceSequencer(int numSamples)
{
  if (!sequencePlaying)
  {
    return;
  }

  if (mBpm != lastBpm)
  {
    updateStepTiming();
    lastBpm = mBpm;
  }

  for (int sample = 0; sample < numSamples; ++sample)
  {
    if (--samplesUntilNextStep <= 0)
    {
      processSequencerStep();
      samplesUntilNextStep += stepIntervalSamples;
    }

    for (int voiceIndex = 0; voiceIndex < size; ++voiceIndex)
    {
      if (subStepRemaining[voiceIndex] <= 0)
      {
        continue;
      }

      if (--subStepSamplesUntilTrigger[voiceIndex] <= 0)
      {
        samplesPosition[voiceIndex] = sampleStart[voiceIndex];
        sampleMakeNoise[voiceIndex] = true;
        --subStepRemaining[voiceIndex];
        subStepSamplesUntilTrigger[voiceIndex] = subStepIntervalSamples[voiceIndex];
      }
    }
  }
}

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
  spec.numChannels = 1;

  for (int i = 0; i < size; i++)
  {
    lfos[i].initialise([](float x)
                       { return std::sin(x); });
    lfos[i].prepare(spec);
    lfos[i].setFrequency(0.0f);
  }

  spec.numChannels = 2;

  // PREPARE IIR FILTER
  for (int i = 0; i < size; i++)
  {
    previousGain[i] = 0.5;
    smoothGainRamp[i].reset(mSampleRate, 0.03);
    smoothGainRampFinger[i].reset(mSampleRate, 0.03);
    smoothSampleLength[i].reset(mSampleRate, 0.1);
    smoothLowRamps[i].reset(mSampleRate, 0.02);
    smoothHighRamps[i].reset(mSampleRate, 0.02);
    lowPasses[i].prepare(spec);
    lowPasses[i].reset();
    highPasses[i].prepare(spec);
    spec.maximumBlockSize = lengthInSamples[i];
    effectsChain[i].prepare(spec);
  }

  loadData();

  for (int i = 0; i < size; i++)
  {
    processSample(i);
  }

  mixScratchBuffer.setSize(2, juce::jmax(1, samplesPerBlockExpected));
  voiceScratchBuffer.setSize(2, juce::jmax(1, samplesPerBlockExpected));
  updateStepTiming();
}

void MySamplerVoice::countSamples(juce::AudioBuffer<float> &buffer, int startSample, int numSamples)
{
  const double startMs = juce::Time::getMillisecondCounterHiRes();

  if (numSamples > mixScratchBuffer.getNumSamples())
  {
    mixScratchBuffer.setSize(2, numSamples, false, false, true);
    voiceScratchBuffer.setSize(2, numSamples, false, false, true);
  }

  advanceSequencer(numSamples);

  buffer.clear();

  triggerSamples(buffer, startSample, numSamples);

  if (!sequencePlaying && fingerMode)
  {
    playSampleProcess(buffer, startSample, numSamples);
  }

  const double elapsedMs = juce::Time::getMillisecondCounterHiRes() - startMs;
  lastBlockProcessMs.store(elapsedMs, std::memory_order_relaxed);

  double currentMax = maxBlockProcessMs.load(std::memory_order_relaxed);
  while (elapsedMs > currentMax &&
         !maxBlockProcessMs.compare_exchange_weak(currentMax, elapsedMs, std::memory_order_relaxed))
  {
  }

  const double budgetMs = (static_cast<double>(numSamples) / juce::jmax(1.0, mSampleRate)) * 1000.0;
  if (elapsedMs > budgetMs)
  {
    overBudgetBlocks.fetch_add(1, std::memory_order_relaxed);
  }
}

void MySamplerVoice::updateSamplesActiveState()
{
  for (int i = 0; i < size; ++i)
  {
    auto &currentArray = sequences[i][selectedPattern[i]][currentPatternIndex[i]];
    if (currentArray[0] == 1 && !isSampleMuted[i])
    {
      samplesPosition[i] = sampleStart[i];
      sampleMakeNoise[i] = true;

      if (currentArray[1] > 1)
      {
        subStepRemaining[i] = currentArray[1] - 1;
        subStepIntervalSamples[i] = juce::jmax(1, stepIntervalSamples / currentArray[1]);
        subStepSamplesUntilTrigger[i] = subStepIntervalSamples[i];
      }
      else
      {
        subStepRemaining[i] = 0;
        subStepSamplesUntilTrigger[i] = 0;
      }
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

void MySamplerVoice::processSample(int voiceIndex)
{
  juce::ScopedNoDenormals noDenormals;

  juce::SynthesiserSound::Ptr soundPtr = mySynth->getSound(voiceIndex);
  juce::SamplerSound *samplerSound = dynamic_cast<juce::SamplerSound *>(soundPtr.get());
  if (samplerSound == nullptr)
    return;

  const int sourceNumSamples = samplerSound->getAudioData()->getNumSamples();
  const int safeLength = juce::jlimit(1, sourceNumSamples, sampleLength[voiceIndex]);

  sampleLength[voiceIndex] = safeLength;
  smoothSampleLength[voiceIndex].setCurrentAndTargetValue(safeLength);

  juce::AudioBuffer<float> sampleBuffer(2, safeLength);
  sampleBuffer.clear();

  const int soundChannels = samplerSound->getAudioData()->getNumChannels();

  effectsChain[voiceIndex].reset();

  for (int channel = 0; channel < sampleBuffer.getNumChannels(); ++channel)
  {
    const int soundChannelIndex = channel % soundChannels;
    const float *audioData = samplerSound->getAudioData()->getReadPointer(soundChannelIndex);

    for (int sample = 0; sample < safeLength; ++sample)
    {
      sampleBuffer.setSample(channel, sample, audioData[sample]);
    }
  }

  initialProcessedSamples[voiceIndex].setSize(2, safeLength);
  initialProcessedSamples[voiceIndex].clear();

  if (initialProcessedSamples[voiceIndex].getNumSamples() == 0)
  {
    initialProcessedSamples[voiceIndex].setSize(2, safeLength);
  }
  for (int channel = 0; channel < sampleBuffer.getNumChannels(); ++channel)
  {
    initialProcessedSamples[voiceIndex].copyFrom(channel, 0, sampleBuffer, channel, 0, safeLength);
  }

  processEffects(sampleBuffer, voiceIndex);
}

void MySamplerVoice::triggerSamples(juce::AudioBuffer<float> &buffer, int startSample, int numSamples)
{
  juce::ScopedNoDenormals noDenormals;

  auto &batchBuffer = mixScratchBuffer;
  batchBuffer.clear();

  std::array<int, 8> activeVoices = {0, 0, 0, 0, 0, 0, 0, 0};
  int activeVoiceCount = 0;

  for (int voiceIndex = 0; voiceIndex < size; ++voiceIndex)
  {
    const auto cache = std::atomic_load(&finalProcessedSamples[voiceIndex]);
    if (cache == nullptr || cache->getNumSamples() == 0)
    {
      continue;
    }

    const bool hasGainRamp = smoothGainRamp[voiceIndex].getCurrentValue() != 0.0f ||
                             smoothGainRamp[voiceIndex].getTargetValue() != 0.0f;

    if (sampleMakeNoise[voiceIndex] || hasGainRamp)
    {
      activeVoices[activeVoiceCount++] = voiceIndex;
    }
  }

  lastActiveVoiceCount.store(activeVoiceCount, std::memory_order_relaxed);

  for (int activeIndex = 0; activeIndex < activeVoiceCount; ++activeIndex)
  {
    const int voiceIndex = activeVoices[activeIndex];
    auto cache = std::atomic_load(&finalProcessedSamples[voiceIndex]);
    if (cache == nullptr || cache->getNumSamples() == 0)
    {
      continue;
    }

    auto &sampleBuffer = voiceScratchBuffer;
    sampleBuffer.clear();

    if (sampleMakeNoise[voiceIndex] != previousSampleMakeNoise[voiceIndex])
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

    if (sampleMakeNoise[voiceIndex] && samplesPosition[voiceIndex] >= sampleLength[voiceIndex] && !adsrList[voiceIndex].isActive())
    {
      sampleMakeNoise[voiceIndex] = false;
    }

    if ((smoothGainRamp[voiceIndex].getCurrentValue() != 0 || smoothGainRamp[voiceIndex].getTargetValue() != 0) && sampleMakeNoise[voiceIndex])
    {
      if (samplesPosition[voiceIndex] == sampleStart[voiceIndex])
      {
        adsrList[voiceIndex].reset();
        adsrList[voiceIndex].noteOn();
      }

      const int cacheNumSamples = cache->getNumSamples();
      for (int sample = 0; sample < numSamples; ++sample)
      {
        float gainValue = smoothGainRamp[voiceIndex].getNextValue();
        float adsrValue = adsrList[voiceIndex].getNextSample();

        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
          if (samplesPosition[voiceIndex] < cacheNumSamples)
          {
            float finalSample = cache->getSample(channel, samplesPosition[voiceIndex]);
            finalSample *= gainValue * adsrValue;

            float tremoloGain;
            getFiltersAndTremolo(tremoloGain, voiceIndex);
            processFiltersAndTremolo(finalSample, voiceIndex, tremoloGain);
            sampleBuffer.addSample(channel, startSample + sample, finalSample);
          }
          else
          {
            if (samplesPosition[voiceIndex] > 0)
            {
              float lastValidSample = cache->getSample(channel, cacheNumSamples - 1);
              float fadeOutFactor = 1.0f - (static_cast<float>(samplesPosition[voiceIndex] - cacheNumSamples) / numSamples);
              float fadeOutSample = lastValidSample * fadeOutFactor * gainValue * adsrValue;

              sampleBuffer.addSample(channel, startSample + sample, fadeOutSample);
            }
            else
            {
              sampleBuffer.addSample(channel, startSample + sample, 0.0f);
            }
          }
        }

        if (samplesPosition[voiceIndex] < smoothSampleLength[voiceIndex].getNextValue())
        {
          ++samplesPosition[voiceIndex];
        }
        else if (adsrList[voiceIndex].getNextSample() >= 1)
        {
          adsrList[voiceIndex].noteOff();
        }
      }
    }

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
      batchBuffer.addFrom(channel, 0, sampleBuffer, channel, 0, numSamples);
    }
  }

  for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
  {
    buffer.addFrom(channel, startSample, batchBuffer, channel, 0, numSamples);
  }
}

void MySamplerVoice::playSampleProcess(juce::AudioBuffer<float> &buffer, int startSample, int numSamples)
{
  auto &batchBuffer = mixScratchBuffer;
  batchBuffer.clear();

  std::array<int, 8> activeVoices = {0, 0, 0, 0, 0, 0, 0, 0};
  int activeVoiceCount = 0;

  for (int voiceIndex = 0; voiceIndex < size; ++voiceIndex)
  {
    const bool hasGainRamp = smoothGainRampFinger[voiceIndex].getCurrentValue() != 0.0f ||
                             smoothGainRampFinger[voiceIndex].getTargetValue() != 0.0f;
    if (sampleMakeNoiseFinger[voiceIndex] || hasGainRamp)
    {
      activeVoices[activeVoiceCount++] = voiceIndex;
    }
  }

  for (int activeIndex = 0; activeIndex < activeVoiceCount; ++activeIndex)
  {
    const int voiceIndex = activeVoices[activeIndex];
    auto cache = std::atomic_load(&finalProcessedSamples[voiceIndex]);
    if (cache == nullptr || cache->getNumSamples() == 0)
    {
      continue;
    }

    auto &sampleBuffer = voiceScratchBuffer;
    sampleBuffer.clear();

    if (smoothGainRampFinger[voiceIndex].getCurrentValue() == 0.0f &&
        smoothGainRampFinger[voiceIndex].getTargetValue() == 0.0f)
    {
      sampleMakeNoiseFinger[voiceIndex] = false;
    }

    if (sampleMakeNoiseFinger[voiceIndex])
    {
      if (samplesPositionFinger[voiceIndex] == sampleStart[voiceIndex])
      {
        adsrList[voiceIndex].reset();
        adsrList[voiceIndex].noteOn();
      }

      const int cacheNumSamples = cache->getNumSamples();

      for (int sample = 0; sample < numSamples; ++sample)
      {
        float gainValue = smoothGainRampFinger[voiceIndex].getNextValue();
        float adsrValue = adsrList[voiceIndex].getNextSample();
        float tremoloGain;
        getFiltersAndTremolo(tremoloGain, voiceIndex);

        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
          if (samplesPositionFinger[voiceIndex] >= cacheNumSamples)
          {
            sampleMakeNoiseFinger[voiceIndex] = false;
            adsrList[voiceIndex].noteOff();
            break;
          }

          float finalSample = cache->getSample(channel, samplesPositionFinger[voiceIndex]);
          finalSample *= gainValue * adsrValue;
          processFiltersAndTremolo(finalSample, voiceIndex, tremoloGain);
          sampleBuffer.addSample(channel, startSample + sample, finalSample);
        }

        if (samplesPositionFinger[voiceIndex] < smoothSampleLength[voiceIndex].getNextValue())
        {
          ++samplesPositionFinger[voiceIndex];
        }
        else if (adsrList[voiceIndex].getNextSample() >= 1)
        {
          adsrList[voiceIndex].noteOff();
        }
      }
    }

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
      batchBuffer.addFrom(channel, 0, sampleBuffer, channel, 0, numSamples);
    }
  }

  for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
  {
    buffer.addFrom(channel, startSample, batchBuffer, channel, 0, numSamples);
  }
}

void MySamplerVoice::getFiltersAndTremolo(float &tremoloGain, int voiceIndex)
{
  float frequency = lfos[voiceIndex].getFrequency();

  if (frequency < 0.1f)
  {
    tremoloGain = 1.0f;
  }
  else
  {
    tremoloGain = 0.5f * (1.0f + lfos[voiceIndex].processSample(0.0f));
  }

  if (smoothLowRamps[voiceIndex].isSmoothing())
  {
    lowPasses[voiceIndex].coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(mSampleRate, smoothLowRamps[voiceIndex].getNextValue());
  }
  if (smoothHighRamps[voiceIndex].isSmoothing())
  {
    highPasses[voiceIndex].coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(mSampleRate, smoothHighRamps[voiceIndex].getNextValue());
  }
}

void MySamplerVoice::processFiltersAndTremolo(float &filteredSamples, int voiceIndex, float tremoloGain)
{
  juce::ScopedNoDenormals noDenormals;

  if (smoothHighRamps[voiceIndex].getCurrentValue() > 20.0)
  {
    highPasses[voiceIndex].snapToZero();
    filteredSamples = highPasses[voiceIndex].processSample(filteredSamples);
  }
  if (smoothLowRamps[voiceIndex].getCurrentValue() < 20000)
  {
    lowPasses[voiceIndex].snapToZero();
    filteredSamples = lowPasses[voiceIndex].processSample(filteredSamples);
  }

  filteredSamples *= tremoloGain;
}

void MySamplerVoice::processEffects(juce::AudioBuffer<float> &buffer, int sampleIndex)
{
  juce::AudioBuffer<float> tempBuffer(buffer);
  juce::dsp::AudioBlock<float> audioBlock(tempBuffer);
  juce::dsp::ProcessContextReplacing<float> context(audioBlock);
  effectsChain[sampleIndex].reset();

  effectsChain[sampleIndex].process(context);

  auto renderedCache = std::make_shared<juce::AudioBuffer<float>>(tempBuffer);
  std::atomic_store(&finalProcessedSamples[sampleIndex], renderedCache);
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
  double maxFreq = 20000.0;

  double cutoffFrequency = (knobValue == 0) ? maxFreq : maxFreq - (knobValue / 127.0) * (maxFreq - minFreq);

  smoothLowRamps[*selectedSample].setTargetValue(cutoffFrequency);
  lastLowPassKnob[*selectedSample] = knobValue;
}

void MySamplerVoice::changeLowPassFilterInicial(double sampleRate, int sampleIndex, int knobValue)
{
  double minFreq = 100.0;
  double maxFreq = 20000.0;

  double cutoffFrequency = (knobValue == 0) ? maxFreq : maxFreq - (knobValue / 127.0) * (maxFreq - minFreq);

  smoothLowRamps[sampleIndex].setCurrentAndTargetValue(cutoffFrequency);
  lastLowPassKnob[sampleIndex] = knobValue;
  lowPasses[sampleIndex].coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(mSampleRate, cutoffFrequency);
}

void MySamplerVoice::changeHighPassFilter(double sampleRate, int knobValue)
{
  double minFreq = 20.0;
  double maxFreq = 1000.0;

  double cutoffFrequency = (knobValue == 0) ? minFreq : minFreq + (knobValue / 127.0) * (maxFreq - minFreq);

  smoothHighRamps[*selectedSample].setTargetValue(cutoffFrequency);
  lastHighPassKnob[*selectedSample] = knobValue;
}

void MySamplerVoice::changeHighPassFilterInicial(double sampleRate, int sampleIndex, int knobValue)
{
  double minFreq = 20.0;
  double maxFreq = 1000.0;

  double cutoffFrequency = (knobValue == 0) ? minFreq : minFreq + (knobValue / 127.0) * (maxFreq - minFreq);

  smoothHighRamps[sampleIndex].setCurrentAndTargetValue(cutoffFrequency);
  lastHighPassKnob[sampleIndex] = knobValue;
  highPasses[sampleIndex].coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(mSampleRate, cutoffFrequency);
}

void MySamplerVoice::changeTremolo(int knobValue)
{
  float frequency = (knobValue / 127.0f) * 20.0f;
  lfos[*selectedSample].setFrequency(frequency);
}

void MySamplerVoice::changeTremoloInicial(int sampleIndex, int knobValue)
{
  float frequency = (knobValue / 127.0f) * 20.0f;
  lfos[sampleIndex].setFrequency(frequency);
}

void MySamplerVoice::changeReverb(int knobValue)
{
  if ((knobValue > lastReverbKnob[*selectedSample] + 5) || (knobValue < lastReverbKnob[*selectedSample] - 5))
  {
    juce::dsp::Reverb::Parameters revParams;
    float normalizedValue = static_cast<float>(knobValue) / 127.0f;

    revParams.roomSize = juce::jlimit(0.0f, 0.5f, normalizedValue * 0.5f);
    revParams.damping = juce::jlimit(0.0f, 0.2f, normalizedValue * 0.4f);
    revParams.width = juce::jlimit(0.0f, 0.4f, normalizedValue * 0.4f);
    revParams.wetLevel = juce::jlimit(0.0f, 0.3f, normalizedValue * 0.2f);
    revParams.dryLevel = juce::jlimit(0.5f, 0.7f, 0.7f - revParams.wetLevel);
    revParams.freezeMode = 0.0f;

    effectsChain[*selectedSample].get<0>().setParameters(revParams);

    juce::AudioBuffer<float> tempBuffer(initialProcessedSamples[*selectedSample]);

    juce::dsp::AudioBlock<float> audioBlock(tempBuffer);
    juce::dsp::ProcessContextReplacing<float> context(audioBlock);
    effectsChain[*selectedSample].process(context);

    std::atomic_store(&finalProcessedSamples[*selectedSample], std::make_shared<juce::AudioBuffer<float>>(tempBuffer));

    lastReverbKnob[*selectedSample] = knobValue;
  }
}

void MySamplerVoice::changeReverbInicial(int sample, int knobValue)
{

  juce::dsp::Reverb::Parameters revParams;

  float normalizedValue = static_cast<float>(knobValue) / 127.0f;

  // Adjust parameters with scaling for balance
  revParams.roomSize = juce::jlimit(0.0f, 0.5f, normalizedValue * 0.5f);
  revParams.damping = juce::jlimit(0.0f, 0.2f, normalizedValue * 0.4f);
  revParams.width = juce::jlimit(0.0f, 0.4f, normalizedValue * 0.4f);
  revParams.wetLevel = juce::jlimit(0.0f, 0.3f, normalizedValue * 0.2f);
  revParams.dryLevel = juce::jlimit(0.5f, 0.7f, 0.7f - revParams.wetLevel);
  revParams.freezeMode = 0.0f;

  effectsChain[*selectedSample].get<0>().setParameters(revParams);
  lastReverbKnob[sample] = knobValue;
}

void MySamplerVoice::changeChorus(int knobValue)
{
  float normalizedValue = static_cast<float>(knobValue) / 127.0f;

  // effectsChain[*selectedSample].get<1>().setDepth(juce::jlimit(0.0f, 0.3f, 0.05f + (normalizedValue * 0.25f)));
  // effectsChain[*selectedSample].get<1>().setFeedback(juce::jlimit(0.0f, 0.15f, 0.0f + (normalizedValue * 0.15f)));
  // effectsChain[*selectedSample].get<1>().setRate(juce::jlimit(0.1f, 1.5f, 0.1f + (normalizedValue * 1.4f)));
  if ((knobValue > lastChorusKnob[*selectedSample] + 5) || (knobValue < lastChorusKnob[*selectedSample] - 5))
  {
    effectsChain[*selectedSample].get<1>().setMix(juce::jlimit(0.0f, 0.8f, normalizedValue));
    juce::AudioBuffer<float> tempBuffer(initialProcessedSamples[*selectedSample]);
    juce::dsp::AudioBlock<float> audioBlock(tempBuffer);
    juce::dsp::ProcessContextReplacing<float> context(audioBlock);
    effectsChain[*selectedSample].process(context);

    std::atomic_store(&finalProcessedSamples[*selectedSample], std::make_shared<juce::AudioBuffer<float>>(tempBuffer));

    lastChorusKnob[*selectedSample] = knobValue;
  }
}

void MySamplerVoice::changeChorusInicial(int sample, int knobValue)
{
  float normalizedValue = static_cast<float>(knobValue) / 127.0f;

  effectsChain[sample].get<1>().setDepth(0.2);
  effectsChain[sample].get<1>().setFeedback(0.1);
  effectsChain[sample].get<1>().setRate(1.5);
  effectsChain[sample].get<1>().setCentreDelay(7.0f);
  effectsChain[sample].get<1>().setMix(juce::jlimit(0.0f, 0.8f, normalizedValue));

  lastChorusKnob[sample] = knobValue;
}

void MySamplerVoice::changeFlanger(int knobValue)
{
  if ((knobValue > lastFlangerKnob[*selectedSample] + 5) || (knobValue < lastFlangerKnob[*selectedSample] - 5))
  {
    float normalizedValue = static_cast<float>(knobValue) / 127.0f;

    effectsChain[*selectedSample].get<2>().setDepth(juce::jlimit(0.0f, 0.5f, 0.2f + (normalizedValue * 0.3f)));
    effectsChain[*selectedSample].get<2>().setFeedback(juce::jlimit(0.0f, 0.95f, 0.6f + (normalizedValue * 0.3f)));
    effectsChain[*selectedSample].get<2>().setRate(juce::jlimit(0.0f, 0.8f, 0.2f + (normalizedValue * 0.6f)));
    effectsChain[*selectedSample].get<2>().setCentreDelay(juce::jlimit(0.5f, 3.0f, 0.7f + (normalizedValue * 2.3f)));
    effectsChain[*selectedSample].get<2>().setMix(juce::jlimit(0.0f, 0.5f, (normalizedValue)));

    juce::AudioBuffer<float> tempBuffer(initialProcessedSamples[*selectedSample]);

    juce::dsp::AudioBlock<float> audioBlock(tempBuffer);
    juce::dsp::ProcessContextReplacing<float> context(audioBlock);
    effectsChain[*selectedSample].process(context);

    std::atomic_store(&finalProcessedSamples[*selectedSample], std::make_shared<juce::AudioBuffer<float>>(tempBuffer));

    lastFlangerKnob[*selectedSample] = knobValue;
  }
}

void MySamplerVoice::changeFlangerInicial(int sample, int knobValue)
{
  float normalizedValue = static_cast<float>(knobValue) / 127.0f;

  effectsChain[sample].get<2>().setDepth(juce::jlimit(0.0f, 0.5f, 0.2f + (normalizedValue * 0.3f)));
  effectsChain[sample].get<2>().setFeedback(juce::jlimit(0.0f, 0.95f, 0.6f + (normalizedValue * 0.3f)));
  effectsChain[sample].get<2>().setRate(juce::jlimit(0.0f, 0.8f, 0.2f + (normalizedValue * 0.6f)));
  effectsChain[sample].get<2>().setCentreDelay(juce::jlimit(0.5f, 3.0f, 0.7f + (normalizedValue * 2.3f)));
  effectsChain[sample].get<2>().setMix(juce::jlimit(0.0f, 0.5f, (normalizedValue)));

  lastFlangerKnob[sample] = knobValue;
}

void MySamplerVoice::changePanner(int knobValue)
{
  if ((knobValue > lastPannerKnob[*selectedSample] + 5) || (knobValue < lastPannerKnob[*selectedSample] - 5))
  {
    float panningValue = (static_cast<float>(knobValue) / 127.0f) * 2.0f - 1.0f;
    effectsChain[*selectedSample].get<3>().setPan(panningValue);

    juce::AudioBuffer<float> tempBuffer(initialProcessedSamples[*selectedSample]);

    juce::dsp::AudioBlock<float> audioBlock(tempBuffer);
    juce::dsp::ProcessContextReplacing<float> context(audioBlock);
    effectsChain[*selectedSample].process(context);

    std::atomic_store(&finalProcessedSamples[*selectedSample], std::make_shared<juce::AudioBuffer<float>>(tempBuffer));

    lastPannerKnob[*selectedSample] = knobValue;
  }
}

void MySamplerVoice::changePannerInicial(int sample, int knobValue)
{
  float panningValue = (static_cast<float>(knobValue) / 127.0f) * 2.0f - 1.0f;
  effectsChain[sample].get<3>().setPan(panningValue);

  lastPannerKnob[sample] = knobValue;
}

void MySamplerVoice::changePhaser(int knobValue)
{
  if ((knobValue > lastPhaserKnob[*selectedSample] + 5) || (knobValue < lastPhaserKnob[*selectedSample] - 5))
  {
    float normalizedValue = static_cast<float>(knobValue) / 127.0f;

    effectsChain[*selectedSample].get<4>().setRate(juce::jlimit(0.1f, 10.0f, 0.2f + (normalizedValue * 9.8f)));                     // Phaser rate (0.1Hz to 10Hz)
    effectsChain[*selectedSample].get<4>().setDepth(juce::jlimit(0.0f, 1.0f, 0.4f + (normalizedValue * 0.6f)));                     // Phaser depth (0.4 to 1 for stronger modulation)
    effectsChain[*selectedSample].get<4>().setCentreFrequency(juce::jlimit(200.0f, 2000.0f, 400.0f + (normalizedValue * 1600.0f))); // Centre frequency (200Hz to 2kHz)
    effectsChain[*selectedSample].get<4>().setFeedback(juce::jlimit(-0.95f, 0.95f, (normalizedValue * 1.9f - 0.95f)));              // Feedback (-0.95 to 0.95 for phase intensity)
    effectsChain[*selectedSample].get<4>().setMix(juce::jlimit(0.0f, 1.0f, normalizedValue));
    juce::AudioBuffer<float> tempBuffer(initialProcessedSamples[*selectedSample]);
    juce::dsp::AudioBlock<float> audioBlock(tempBuffer);
    juce::dsp::ProcessContextReplacing<float> context(audioBlock);

    effectsChain[*selectedSample].process(context);

    std::atomic_store(&finalProcessedSamples[*selectedSample], std::make_shared<juce::AudioBuffer<float>>(tempBuffer));

    lastPhaserKnob[*selectedSample] = knobValue;
  }
}

void MySamplerVoice::changePhaserInicial(int sample, int knobValue)
{
  float normalizedValue = static_cast<float>(knobValue) / 127.0f;

  effectsChain[sample].get<4>().setRate(juce::jlimit(0.1f, 10.0f, 0.2f + (normalizedValue * 9.8f)));                     // Phaser rate (0.1Hz to 10Hz)
  effectsChain[sample].get<4>().setDepth(juce::jlimit(0.0f, 1.0f, 0.4f + (normalizedValue * 0.6f)));                     // Phaser depth (0.4 to 1 for stronger modulation)
  effectsChain[sample].get<4>().setCentreFrequency(juce::jlimit(200.0f, 2000.0f, 400.0f + (normalizedValue * 1600.0f))); // Centre frequency (200Hz to 2kHz)
  effectsChain[sample].get<4>().setFeedback(juce::jlimit(-0.95f, 0.95f, (normalizedValue * 1.9f - 0.95f)));              // Feedback (-0.95 to 0.95 for phase intensity)
  effectsChain[sample].get<4>().setMix(juce::jlimit(0.0f, 1.0f, normalizedValue));

  lastPhaserKnob[sample] = knobValue;
}

void MySamplerVoice::clearPattern()
{
  auto &sequence = sequences[*selectedSample][selectedPattern[*selectedSample]];

  for (int i = 0; i < sequence.size(); i++)
  {
    sequence[i][0] = 0;
    sequence[i][1] = 1;
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

  lowPasses[*selectedSample].coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(mSampleRate, 20000);
  highPasses[*selectedSample].coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(mSampleRate, 20);
  lastReverbKnob[*selectedSample] = 0;
  lastChorusKnob[*selectedSample] = 0;
  lastFlangerKnob[*selectedSample] = 0;
  lastPannerKnob[*selectedSample] = 64;
  lastPhaserKnob[*selectedSample] = 0;
  lastTremoloKnob[*selectedSample] = 0;

  changeReverb(0);
  changeChorus(0);
  changeFlanger(0);
  changePanner(64);
  changePhaser(0);
  changeTremolo(0);

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
        // Acessar o array de dois elementos e formatar a saída
        const auto &stepArray = sequences[sampleIndex][patternIndex][step];
        outputStream.writeText("[" + juce::String(stepArray[0]) + "," + juce::String(stepArray[1]) + "] ", false, false, "\n");
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
    outputStream.writeText("LowPass=" + juce::String(lastLowPassKnob[sampleIndex]) + "\n", false, false, "\n");
    outputStream.writeText("HighPass=" + juce::String(lastHighPassKnob[sampleIndex]) + "\n", false, false, "\n");
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
    outputStream.writeText("Tremolo=" + juce::String(lastTremoloKnob[sampleIndex]) + "\n", false, false, "\n");
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
      while (!inputStream.isExhausted() && (line = inputStream.readNextLine().trim()).startsWith("Sample"))
      {
        sampleIndex = line.fromFirstOccurrenceOf("Sample", false, false).getIntValue();

        // Read patterns associated with the sample
        for (int patternIndex = 0; patternIndex < sequences[sampleIndex].size(); ++patternIndex)
        {
          if (inputStream.isExhausted())
            break; // Safety check

          line = inputStream.readNextLine().trim();
          juce::String trimmedLine = line.fromFirstOccurrenceOf(":", false, false).trim();

          juce::StringArray stepValues;
          stepValues.addTokens(trimmedLine, false);

          for (int step = 0; step < stepValues.size(); ++step)
          {
            juce::StringArray values;

            // Trim both brackets and then split by comma
            juce::String stepString = stepValues[step].trimCharactersAtEnd("]").trimCharactersAtStart("[");
            values.addTokens(stepString, ",", "\"");

            if (values.size() == 2)
            {
              sequences[sampleIndex][patternIndex].resize(stepValues.size());          // Ensure we have enough space
              sequences[sampleIndex][patternIndex][step][0] = values[0].getIntValue(); // First value
              sequences[sampleIndex][patternIndex][step][1] = values[1].getIntValue(); // Second value
            }
          }
        }
      }
      continue; // Go to the next iteration to avoid reading other sections
    }

    // Load Sample Stretch
    if (line.startsWith("Sample stretch:"))
    {
      while ((line = inputStream.readNextLine().trim()).startsWith("Sample"))
      {
        sampleIndex = line.fromFirstOccurrenceOf("Sample", false, false).getIntValue();

        juce::String startLine = inputStream.readNextLine().trim();
        juce::String lengthLine = inputStream.readNextLine().trim();

        if (sampleIndex < 0 || sampleIndex >= size)
          continue;

        const int loadedStart = startLine.fromFirstOccurrenceOf("Start=", false, false).getIntValue();
        const int loadedLength = lengthLine.fromFirstOccurrenceOf("Length=", false, false).getIntValue();

        sampleLength[sampleIndex] = juce::jlimit(1, lengthInSamples[sampleIndex], loadedLength);
        sampleStart[sampleIndex] = juce::jlimit(0, sampleLength[sampleIndex] - 1, loadedStart);
      }
    }

    // Load ADSR
    if (line.startsWith("ADSR:"))
    {
      while ((line = inputStream.readNextLine().trim()).startsWith("Sample"))
      {
        sampleIndex = line.fromFirstOccurrenceOf("Sample", false, false).getIntValue();

        if (sampleIndex < 0 || sampleIndex >= size)
          continue;

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

        if (sampleIndex < 0 || sampleIndex >= size)
          continue;

        juce::String lowPassLine = inputStream.readNextLine().trim();
        changeLowPassFilterInicial(mSampleRate, sampleIndex, lowPassLine.fromFirstOccurrenceOf("LowPass=", false, false).getFloatValue());

        juce::String highPassLine = inputStream.readNextLine().trim();
        changeHighPassFilterInicial(mSampleRate, sampleIndex, highPassLine.fromFirstOccurrenceOf("HighPass=", false, false).getFloatValue());
      }
    }

    // Load Effects
    if (line.startsWith("Effects:"))
    {
      while ((line = inputStream.readNextLine().trim()).startsWith("Sample"))
      {
        sampleIndex = line.fromFirstOccurrenceOf("Sample", false, false).getIntValue();

        if (sampleIndex < 0 || sampleIndex >= size)
          continue;

        juce::String reverbLine = inputStream.readNextLine().trim();
        int reverbValue = reverbLine.fromFirstOccurrenceOf("Reverb=", false, false).getIntValue();
        changeReverbInicial(sampleIndex, reverbValue);

        juce::String chorusLine = inputStream.readNextLine().trim();
        int chorusValue = chorusLine.fromFirstOccurrenceOf("Chorus=", false, false).getIntValue();
        changeChorusInicial(sampleIndex, chorusValue);

        juce::String flangerLine = inputStream.readNextLine().trim();
        int flangerValue = flangerLine.fromFirstOccurrenceOf("Flanger=", false, false).getIntValue();
        changeFlangerInicial(sampleIndex, flangerValue);

        juce::String pannerLine = inputStream.readNextLine().trim();
        int pannerValue = pannerLine.fromFirstOccurrenceOf("Panner=", false, false).getIntValue();
        changePannerInicial(sampleIndex, pannerValue);

        juce::String phaserLine = inputStream.readNextLine().trim();
        int phaserValue = phaserLine.fromFirstOccurrenceOf("Phaser=", false, false).getIntValue();
        changePhaserInicial(sampleIndex, phaserValue);

        juce::String tremoloLine = inputStream.readNextLine().trim();
        int tremoloValue = tremoloLine.fromFirstOccurrenceOf("Tremolo=", false, false).getIntValue();
        // changePhaserInicial(sampleIndex, phaserValue);
      }
    }
  }
}