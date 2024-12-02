/*
  ==============================================================================

    SamplerSoundSynth.cpp
    Created: 14 Mar 2024 11:36:21pm
    Author:  kalebe

  ==============================================================================
*/

#include "SamplerSoundSynth.h"
#include <string>
#include <iostream>

using namespace std;

SamplerSoundSynth::SamplerSoundSynth()
{

  mAudioFormatManager.registerBasicFormats();

  juce::File myFile{juce::File::getSpecialLocation(juce::File::userDesktopDirectory)};
  auto mySamples = myFile.findChildFiles(juce::File::TypesOfFileToFind::findFiles, true, "hard-kick.wav");
  auto mySamples2 = myFile.findChildFiles(juce::File::TypesOfFileToFind::findFiles, true, "bass.wav");

  jassert(mySamples[0].exists());
  jassert(mySamples2[0].exists());

  auto formatReader = mAudioFormatManager.createReaderFor(mySamples[0]);
  auto formatReader2 = mAudioFormatManager.createReaderFor(mySamples2[0]);

  mSamplerSound1 = new juce::SamplerSound("SamplerSound1", *formatReader, juce::BigInteger(), 0, 0.0, 0.0, 0.0);
  mSamplerSound2 = new juce::SamplerSound("SamplerSound2", *formatReader2, juce::BigInteger(), 0, 0.0, 0.0, 0.0);

  // mSynthesiser1.addSound(mSamplerSound1);

  for (int i = 0; i < mNumVoices; i++)
  {
    mSampler.addVoice(new juce::SamplerVoice());
  }

  juce::BigInteger range;
  range.setRange(0, 128, true);

  mSampler.addSound(new juce::SamplerSound("SamplerSound1", *formatReader, range, 60, 0.1, 0.1, 20.0));
}
