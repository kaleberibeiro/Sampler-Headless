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
  auto mySamples = myFile.findChildFiles(juce::File::TypesOfFileToFind::findFiles, true, "hard-kick.wav");
  auto mySamples2 = myFile.findChildFiles(juce::File::TypesOfFileToFind::findFiles, true, "bass.wav");

  jassert(mySamples[0].exists());
  jassert(mySamples2[0].exists());

  auto formatReader = mAudioFormatManager.createReaderFor(mySamples[0]);
  auto formatReader2 = mAudioFormatManager.createReaderFor(mySamples2[0]);

  pSample1.reset(new juce::AudioFormatReaderSource(formatReader, true));
  pSample2.reset(new juce::AudioFormatReaderSource(formatReader2, true));
}

void Metronome::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
  mSampleRate = sampleRate;
  mUpdateInterval = (60.0 / mBpm * mSampleRate) / 4;

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

  mixerAudioSource.prepareToPlay(samplesPerBlockExpected, sampleRate);
}

void Metronome::getNextAudioBlock(juce::AudioSourceChannelInfo &bufferToFill)
{
  bufferToFill.clearActiveBufferRegion();
  auto bufferSize = bufferToFill.numSamples;
  mTotalSamples += bufferSize;

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
          // if (sample1Sequence[currentSample1index] == 1)
          // {
          //   pSample1->setNextReadPosition(0);
          //   std::cout << "Tocou kick: " << currentSample1index << std::endl;
          //   mixerAudioSource.addInputSource(pSample1.get(), false);
          // }
          // else
          // {
          //   mixerAudioSource.removeInputSource(pSample1.get());
          // }

          if (sample2Sequence[currentSample2index] == 1)
          {
            pSample2->setNextReadPosition(0);
            mixerAudioSource.removeInputSource(pSample2.get());
            std::cout << "Tocou hi-hat: " << currentSample2index << std::endl;
            mixerAudioSource.addInputSource(pSample2.get(), false);
          }
          else
          {
            if (!pSample2->getNextReadPosition() != 0)
            {
              mixerAudioSource.removeInputSource(pSample2.get());
            }
          }

          mixerAudioSource.getNextAudioBlock(bufferToFill);

          currentSample1index++;
          currentSample2index++;
        }
        else
        {
          currentSample1index = 0;
          currentSample2index = 0;

          // if (sample1Sequence[currentSample1index] == 1)
          // {
          //   pSample1->setNextReadPosition(0);
          //   std::cout << "Tocou kick: " << currentSample1index << std::endl;
          //   mixerAudioSource.addInputSource(pSample1.get(), false);
          // }
          // else
          // {
          //   mixerAudioSource.removeInputSource(pSample1.get());
          // }

          if (sample2Sequence[currentSample2index] == 1)
          {
            pSample2->setNextReadPosition(0);
            mixerAudioSource.removeInputSource(pSample2.get());
            std::cout << "Tocou hi-hat: " << currentSample2index << std::endl;
            mixerAudioSource.addInputSource(pSample2.get(), false);
          }
          else
          {
            if (!pSample2->getNextReadPosition() != 0)
            {
              mixerAudioSource.removeInputSource(pSample2.get());
            }
          }

          mixerAudioSource.getNextAudioBlock(bufferToFill);

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
}
