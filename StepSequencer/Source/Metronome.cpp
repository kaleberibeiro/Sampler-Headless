#include "Metronome.h"
#include <string>
#include <iostream>

using namespace std;

Metronome::Metronome()
{
  sample1Sequence = {1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0};
  sample2Sequence = {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  mAudioFormatManager.registerBasicFormats();

  juce::File myFile{juce::File::getSpecialLocation(juce::File::userDesktopDirectory)};
  auto mySamples = myFile.findChildFiles(juce::File::TypesOfFileToFind::findFiles, true, "mdp-kick-trance.wav");
  auto mySamples2 = myFile.findChildFiles(juce::File::TypesOfFileToFind::findFiles, true, "bass.wav");

  jassert(mySamples[0].exists());
  jassert(mySamples2[0].exists());

  auto formatReader = mAudioFormatManager.createReaderFor(mySamples[0]);
  auto formatReader2 = mAudioFormatManager.createReaderFor(mySamples2[0]);

  std::unique_ptr<juce::AudioFormatReaderSource> tempSource(new juce::AudioFormatReaderSource(formatReader, true));

  transport.setSource(tempSource.get());
  playSource.reset(tempSource.release());

  std::unique_ptr<juce::AudioFormatReaderSource> tempSource2(new juce::AudioFormatReaderSource(formatReader2, true));
  transport2.setSource(tempSource2.get());
  playSource2.reset(tempSource2.release());
  
  pSample1.reset(new juce::AudioFormatReaderSource(formatReader, true));
  pSample2.reset(new juce::AudioFormatReaderSource(formatReader2, true));

  previousGain = 0.4f;
  gain = 0.4f;
}

void Metronome::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
  mSampleRate = sampleRate;
  mUpdateInterval = (60.0 / mBpm * mSampleRate) / 4;
  adsr = new juce::ADSR;
  juce::ADSR::Parameters adsrParams;
  adsrParams.attack = 0.6f;
  adsrParams.decay = 0.6f;
  adsrParams.release = 0.6f;
  // adsrParams.sustain = 1.0f;
  adsr->setSampleRate(sampleRate);
  adsr->setParameters(adsrParams);

  if (pSample1 != nullptr)
  {
    juce::Reverb::Parameters reverbParameters;
    reverbParameters.roomSize = 0.9;
    reverbParameters.dryLevel = 0.9;
    reverbParameters.damping = 0.9;
    reverbParameters.wetLevel = 0.9;

    pSample1->prepareToPlay(samplesPerBlockExpected, sampleRate);
    juce::AudioSource *rawPointer = pSample1.get();
    iirFilterAudioSource1 = new juce::IIRFilterAudioSource(pSample1.get(), false);
    // iirFilterAudioSource1->setCoefficients(juce::IIRCoefficients::makeLowPass(mSampleRate, 100.0));
    reverbAudioSource1 = new juce::ReverbAudioSource(iirFilterAudioSource1, false);
    // reverbAudioSource1->setParameters(reverbParameters);
  }
  if (pSample2 != nullptr)
  {
    juce::Reverb::Parameters reverbParameters;
    reverbParameters.roomSize = 0.9;
    reverbParameters.dryLevel = 0.9;
    reverbParameters.damping = 0.9;
    reverbParameters.wetLevel = 0.9;

    pSample2->prepareToPlay(samplesPerBlockExpected, sampleRate);
    juce::AudioSource *rawPointer = pSample2.get();
    iirFilterAudioSource2 = new juce::IIRFilterAudioSource(rawPointer, false);
    // iirFilterAudioSource2->setCoefficients(juce::IIRCoefficients::makeHighPass(mSampleRate, 2000.0).makeBandPass(mSampleRate, 500.0));
    reverbAudioSource2 = new juce::ReverbAudioSource(iirFilterAudioSource2, false);
    // reverbAudioSource2->setParameters(reverbParameters);
  }
  transport.prepareToPlay(samplesPerBlockExpected, sampleRate);
  transport2.prepareToPlay(samplesPerBlockExpected, sampleRate);
  transport.start();
  transport2.start();
  mixerAudioSource.prepareToPlay(samplesPerBlockExpected, sampleRate);
}

void Metronome::getNextAudioBlock(juce::AudioSourceChannelInfo &bufferToFill)
{
  bufferToFill.clearActiveBufferRegion();
  auto bufferSize = bufferToFill.numSamples;
  mTotalSamples += bufferSize;
  // adsr->noteOn();

  mSamplesRemaining = mTotalSamples % mUpdateInterval;

  if ((mSamplesRemaining + bufferSize) >= mUpdateInterval)
  {
    const auto timeToStartPlaying = mUpdateInterval - mSamplesRemaining;

    for (auto sample = 0; sample < bufferSize; ++sample)
    {
      if (sample == timeToStartPlaying)
      {
        if (currentSample1index < sample1Sequence.size())
        {
          if ((sample1Sequence[currentSample1index] == 1))
          {
            pSample1->setNextReadPosition(0);
            mixerAudioSource.addInputSource(pSample1.get(), true);
            if (sample2Sequence[currentSample2index] == 1)
            {
              pSample2->setNextReadPosition(0);
              mixerAudioSource.addInputSource(pSample2.get(), true);
              /////////// SAMPLE 1 E SAMPLE 2 TRUE //////////
              mixerAudioSource.getNextAudioBlock(bufferToFill);
            }
            else
            {
              if (pSample2->getNextReadPosition() == 0)
              {
                mixerAudioSource.removeInputSource(pSample2.get());
              }
              /////////// SAMPLE 1 TRUE E SAMPLE 2 FALSE//////////
              mixerAudioSource.getNextAudioBlock(bufferToFill);
            }
          }
          else
          {
            if (pSample1->getNextReadPosition() == 0)
            {
              mixerAudioSource.removeInputSource(pSample1.get());
            }

            if (sample2Sequence[currentSample2index] == 1)
            {
              pSample2->setNextReadPosition(0);
              mixerAudioSource.addInputSource(pSample2.get(), true);
              mixerAudioSource.getNextAudioBlock(bufferToFill);
            }
            else
            {
              if (pSample1->getNextReadPosition() == 0)
              {
                mixerAudioSource.removeInputSource(pSample1.get());
              }

              if (pSample2->getNextReadPosition() == 0)
              {
                mixerAudioSource.removeInputSource(pSample2.get());
              }
            }
          }
          currentSample1index++;
          currentSample2index++;
        }
        else
        {
          currentSample1index = 0;
          currentSample2index = 0;

          if ((sample1Sequence[currentSample1index] == 1))
          {
            pSample1->setNextReadPosition(0);
            mixerAudioSource.addInputSource(pSample1.get(), true);
            if (sample2Sequence[currentSample2index] == 1)
            {
              pSample2->setNextReadPosition(0);
              mixerAudioSource.addInputSource(pSample2.get(), true);
              /////////// SAMPLE 1 E SAMPLE 2 TRUE //////////
              mixerAudioSource.getNextAudioBlock(bufferToFill);
            }
            else
            {
              if (pSample2->getNextReadPosition() == 0)
              {
                mixerAudioSource.removeInputSource(pSample2.get());
              }
              /////////// SAMPLE 1 TRUE E SAMPLE 2 FALSE//////////
              mixerAudioSource.getNextAudioBlock(bufferToFill);
            }
          }
          else
          {
            if (pSample1->getNextReadPosition() == 0)
            {
              mixerAudioSource.removeInputSource(pSample1.get());
            }

            if (sample2Sequence[currentSample2index] == 1)
            {
              pSample2->setNextReadPosition(0);
              mixerAudioSource.addInputSource(pSample2.get(), true);
              mixerAudioSource.getNextAudioBlock(bufferToFill);
            }
            else
            {
              if (pSample1->getNextReadPosition() == 0)
              {
                mixerAudioSource.removeInputSource(pSample1.get());
              }

              if (pSample2->getNextReadPosition() == 0)
              {
                mixerAudioSource.removeInputSource(pSample2.get());
              }
            }
          }
          currentSample1index++;
          currentSample2index++;
        }
      }
    }
  }

  if ((pSample1->getNextReadPosition() != 0) || (pSample2->getNextReadPosition() != 0))
  {
    mixerAudioSource.getNextAudioBlock(bufferToFill);
  }

  // // adsr->applyEnvelopeToBuffer(*bufferToFill.buffer, bufferToFill.startSample, bufferToFill.numSamples);

  // // Apply gain ramp
  // // if (juce::approximatelyEqual(gain, previousGain))
  // // {
  // //   bufferToFill.buffer->applyGain(gain);
  // // }
  // // else
  // // {
  // //   bufferToFill.buffer->applyGainRamp(0, bufferToFill.buffer->getNumSamples(), previousGain, gain);
  // //   previousGain = gain;
  // // }

  // bufferToFill.buffer->applyGain(0.4f);

  // adsr->noteOff();
}

void Metronome::playOneTimeMixer(juce::AudioSourceChannelInfo &bufferToFill)
{
  std::cout << "Entrou mixer" << std::endl;
  bufferToFill.clearActiveBufferRegion();

  auto bufferSize = bufferToFill.numSamples;
  mTotalSamples += bufferSize;

  mixerAudioSource.addInputSource(pSample1.get(), true);

  mSamplesRemaining = mTotalSamples % mUpdateInterval;

  if ((mSamplesRemaining + bufferSize) >= mUpdateInterval)
  {
    const auto timeToStartPlaying = mUpdateInterval - mSamplesRemaining;
    pSample1->setNextReadPosition(0);

    for (auto sample = 0; sample < bufferSize; ++sample)
    {
      if (sample == timeToStartPlaying)
      {
        // pSample1->getNextAudioBlock(bufferToFill);
        mixerAudioSource.getNextAudioBlock(bufferToFill);
      }
    }
  }

  if ((pSample1->getNextReadPosition() != 0))
  {
    mixerAudioSource.getNextAudioBlock(bufferToFill);
    // pSample1->getNextAudioBlock(bufferToFill);
  }
}

void Metronome::playOneTime(juce::AudioSourceChannelInfo &bufferToFill)
{
  std::cout << "Entrou sample" << std::endl;
  bufferToFill.clearActiveBufferRegion();

  auto bufferSize = bufferToFill.numSamples;
  mTotalSamples += bufferSize;

  // transport.start();

  mSamplesRemaining = mTotalSamples % mUpdateInterval;

  if ((mSamplesRemaining + bufferSize) >= mUpdateInterval)
  {
    const auto timeToStartPlaying = mUpdateInterval - mSamplesRemaining;
    pSample1->setNextReadPosition(0);
    // transport.setPosition(0.0);
    transport.setPosition(0.0);

    for (auto sample = 0; sample < bufferSize; ++sample)
    {
      if (sample == timeToStartPlaying)
      {
        pSample1->getNextAudioBlock(bufferToFill);
        // transport.getNextAudioBlock(bufferToFill);
      }
    }
  }

  if ((pSample1->getNextReadPosition() != 0))
  {
    // transport.getNextAudioBlock(bufferToFill);
    pSample1->getNextAudioBlock(bufferToFill);
  }
}
