#include "Metronome.h"
#include <string>
#include <iostream>

using namespace std;

// Metronome::Metronome()
// {

//   for (int i = 0; i < numVoices; i++)
//   {
//     mSampler.addVoice(new juce::SamplerVoice());
//   }

//   sample1Sequence = {1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0};
//   sample2Sequence = {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

//   sequences[0] = {1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0};
//   sequences[1] = {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
//   sequences[2] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

//   std::string fileNames[3] = {"mdp-kick-trance.wav", "bass.wav", "closed-hi-hat.wav"};

//   mAudioFormatManager.registerBasicFormats();
//   juce::File myFile{juce::File::getSpecialLocation(juce::File::userDesktopDirectory)};

//   for (int i = 0; i < sizeof(fileNames) / sizeof(std::string); ++i)
//   {
//     std::cout << "Entrou: " << fileNames[i] << sizeof(fileNames) / sizeof(std::string) << std::endl;
//     auto mySamples = myFile.findChildFiles(juce::File::TypesOfFileToFind::findFiles, true, fileNames[i]);
//     jassert(mySamples[0].exists());
//     auto formatReader = mAudioFormatManager.createReaderFor(mySamples[0]);
//     juce::BigInteger range;
//     range.setRange(0, 128, true);

//     mSampler.addSound(new juce::SamplerSound(fileNames[i], *formatReader, range, 60, 0.1, 0.1, 10.0));

//     std::unique_ptr<juce::AudioFormatReaderSource> tempSource(new juce::AudioFormatReaderSource(formatReader, true));

//     samples[i].setSource(tempSource.get());
//     playSources[i].reset(tempSource.release());
//   }

//   // auto mySamples = myFile.findChildFiles(juce::File::TypesOfFileToFind::findFiles, true, "mdp-kick-trance.wav");
//   // auto mySamples2 = myFile.findChildFiles(juce::File::TypesOfFileToFind::findFiles, true, "bass.wav");

//   // jassert(mySamples[0].exists());
//   // jassert(mySamples2[0].exists());

//   // auto formatReader = mAudioFormatManager.createReaderFor(mySamples[0]);
//   // auto formatReader2 = mAudioFormatManager.createReaderFor(mySamples2[0]);

//   // std::unique_ptr<juce::AudioFormatReaderSource> tempSource(new juce::AudioFormatReaderSource(formatReader, true));

//   // transport.setSource(tempSource.get());
//   // playSource.reset(tempSource.release());

//   // std::unique_ptr<juce::AudioFormatReaderSource> tempSource2(new juce::AudioFormatReaderSource(formatReader2, true));
//   // transport2.setSource(tempSource2.get());
//   // playSource2.reset(tempSource2.release());

//   // pSample1.reset(new juce::AudioFormatReaderSource(formatReader, true));
//   // pSample2.reset(new juce::AudioFormatReaderSource(formatReader2, true));

//   // previousGain = 0.4f;
//   // gain = 0.4f;
// }

void Metronome::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
  mSampleRate = sampleRate;
  mUpdateInterval = (60.0 / mBpm * mSampleRate) / 4;

  mySynth->setCurrentPlaybackSampleRate(sampleRate);
}

void Metronome::countSamples(juce::AudioBuffer<float> &buffer, int startSample, int numSamples)
{
  
}

// void Metronome::triggerSamples(juce::AudioSourceChannelInfo &bufferToFill)
// {
//   bool anySamplePlaying = false; // Flag to track if any sample is playing

//   // Reset currentSequenceIndex and positions of samples at the beginning of each sequence
//   if (currentSequenceIndex >= sequenceSize)
//   {
//     currentSequenceIndex = 0;
//     for (int i = 0; i < size; ++i)
//     {
//       samples[i].setPosition(0.0);
//     }
//   }

//   for (int i = 0; i < size; ++i)
//   {
//     if (playingSamples[i])
//     {
//       if (sequences[i][currentSequenceIndex] == 1)
//       {
//         // Reset position only if the sample is about to play
//         samples[i].setPosition(0.0);
//         mixerAudioSource.addInputSource(&samples[i], false);
//         anySamplePlaying = true;
//       }
//       else if (samples[i].getNextReadPosition() == 0)
//       {
//         mixerAudioSource.removeInputSource(&samples[i]);
//       }
//     }
//   }

//   currentSequenceIndex++;

//   if (anySamplePlaying)
//   {
//     // mixerAudioSource.getNextAudioBlock(bufferToFill);
//     mSampler.renderNextBlock(*bufferToFill.buffer, midiBuffer, 0, bufferToFill.numSamples);
//   }
// }

// void Metronome::triggerSamples(juce::AudioSourceChannelInfo &bufferToFill)
// {
//   if (currentSample1index < sample1Sequence.size())
//   {
//     std::cout << "Entrou com index: " << currentSample1index << std::endl;
//     if ((sample1Sequence[currentSample1index] == 1))
//     {
//       samples[0].setPosition(0.0);
//       mixerAudioSource.addInputSource(&samples[0], false);
//       if (sample2Sequence[currentSample2index] == 1)
//       {
//         samples[1].setPosition(0.0);
//         mixerAudioSource.addInputSource(&samples[1], false);
//         mixerAudioSource.getNextAudioBlock(bufferToFill);
//       }
//       else
//       {
//         if (samples[1].getNextReadPosition() == 0)
//         {
//           mixerAudioSource.removeInputSource(&samples[1]);
//         }
//         mixerAudioSource.getNextAudioBlock(bufferToFill);
//       }
//     }
//     else
//     {
//       if (samples[0].getNextReadPosition() == 0)
//       {
//         mixerAudioSource.removeInputSource(&samples[0]);
//       }

//       if (sample2Sequence[currentSample2index] == 1)
//       {
//         samples[1].setPosition(0.0);
//         mixerAudioSource.addInputSource(&samples[1], false);
//         mixerAudioSource.getNextAudioBlock(bufferToFill);
//       }
//       else
//       {
//         if (samples[0].getNextReadPosition() == 0)
//         {
//           mixerAudioSource.removeInputSource(&samples[0]);
//         }

//         if (samples[1].getNextReadPosition() == 0)
//         {
//           mixerAudioSource.removeInputSource(&samples[1]);
//         }
//       }
//     }
//     currentSample1index++;
//     currentSample2index++;
//   }
//   else
//   {
//     currentSample1index = 0;
//     currentSample2index = 0;

//     if ((sample1Sequence[currentSample1index] == 1))
//     {
//       samples[0].setPosition(0.0);
//       mixerAudioSource.addInputSource(&samples[0], false);
//       if (sample2Sequence[currentSample2index] == 1)
//       {
//         samples[1].setPosition(0.0);
//         mixerAudioSource.addInputSource(&samples[1], false);
//         mixerAudioSource.getNextAudioBlock(bufferToFill);
//       }
//       else
//       {
//         if (samples[1].getNextReadPosition() == 0)
//         {
//           mixerAudioSource.removeInputSource(&samples[1]);
//         }
//         mixerAudioSource.getNextAudioBlock(bufferToFill);
//       }
//     }
//     else
//     {
//       if (samples[0].getNextReadPosition() == 0)
//       {
//         mixerAudioSource.removeInputSource(&samples[0]);
//       }

//       if (sample2Sequence[currentSample2index] == 1)
//       {
//         samples[1].setPosition(0.0);
//         mixerAudioSource.addInputSource(&samples[1], false);
//         mixerAudioSource.getNextAudioBlock(bufferToFill);
//       }
//       else
//       {
//         if (samples[0].getNextReadPosition() == 0)
//         {
//           mixerAudioSource.removeInputSource(&samples[0]);
//         }

//         if (samples[1].getNextReadPosition() == 0)
//         {
//           mixerAudioSource.removeInputSource(&samples[1]);
//         }
//       }
//     }
//     currentSample1index++;
//     currentSample2index++;
//   }
// }

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
        if (isPlaying)
        {
          // triggerSamples(bufferToFill);
        }
        else
        {
          currentSequenceIndex = 0;
          for (int i = 0; i < sizeof(samples) / sizeof(juce::AudioTransportSource); ++i)
          {
            samples[i].setPosition(0.0);
          }
        }
      }
    }
  }

  bool willPlay = false;

  for (int i = 0; i < sizeof(samples) / sizeof(juce::AudioTransportSource); ++i)
  {
    if (samples[i].getNextReadPosition() != 0)
    {
      willPlay = true;
      break; // No need to continue checking if at least one sample will play
    }
  }

  if (willPlay)
  {
    // mixerAudioSource.getNextAudioBlock(bufferToFill);
    // mSampler.renderNextBlock(*bufferToFill.buffer, midiBuffer, 0, bufferToFill.numSamples);
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

void Metronome::startPlayer()
{
  isPlaying = true;
}

void Metronome::stopPlayer()
{
  isPlaying = false;
}

void Metronome::activateSample(int index)
{
  playingSamples[index] = true;
}

// void Metronome::playOneTime(juce::AudioBuffer<float> &outputBuffer)
// {
//   // std::cout << "Entrou sample" << std::endl;
//   // // bufferToFill.clearActiveBufferRegion();
//   // std::cout << "Data: " << mSampler.getNumSounds() << " " << mSampler.getNumVoices() << " " << mSampler.getSampleRate() << std::endl;
//   // std::cout << "Can play: " << mSampler.getVoice(0)->canPlaySound(mSampler.getSound(0).get()) << std::endl;

//   mSampler.getVoice(0)->startNote(60, 200, mSampler.getSound(0).get(), 20);

//   mSampler.getVoice(0)->renderNextBlock(outputBuffer, 0, outputBuffer.getNumSamples());

//   mSampler.renderNextBlock(outputBuffer, midiBuffer, 0, outputBuffer.getNumSamples());
//   // mSampler.getVoice(0)
//   //     ->renderNextBlock(*bufferToFill.buffer, 0, bufferToFill.numSamples);
// }
