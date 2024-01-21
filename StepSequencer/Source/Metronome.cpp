#include "Metronome.h"
#include <string>
#include <iostream>

using namespace std;

Metronome::Metronome()
{
    mAudioFormatManager.registerBasicFormats();

    juce::File myFile{juce::File::getSpecialLocation(juce::File::userDesktopDirectory)};
    auto mySamples = myFile.findChildFiles(juce::File::TypesOfFileToFind::findFiles, true, "hard-kick.wav");

    jassert(mySamples[0].exists());

    auto formatReader = mAudioFormatManager.createReaderFor(mySamples[0]);
    double inputFileSampleRate = formatReader->sampleRate;
    std::cout << "Sample Rate of sample: " << inputFileSampleRate << std::endl;

    pMetronomeSample.reset(new juce::AudioFormatReaderSource(formatReader, true));
}


void Metronome::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    mSampleRate = sampleRate;
    mUpdateInterval = 60.0 / mBpm * mSampleRate;

    if (pMetronomeSample != nullptr)
    {
        pMetronomeSample->prepareToPlay(samplesPerBlockExpected, sampleRate);
    }
}

void Metronome::getNextAudioBlock(juce::AudioSourceChannelInfo &bufferToFill)
{
    auto bufferSize = bufferToFill.numSamples;
    std::cout << "Metronome getNextAudioBlock: Samples to fill = " << bufferToFill.numSamples << std::endl;
    mTotalSamples += bufferSize;

    mSamplesRemaining = mTotalSamples % mUpdateInterval;

    std::cout << mSamplesRemaining << std::endl;

    // Check if it's time to start playing the metronome sound
    if ((mSamplesRemaining + bufferSize) >= mUpdateInterval)
    {
        const auto timeToStartPlaying = mUpdateInterval - mSamplesRemaining;

        pMetronomeSample->setNextReadPosition(0);

        for (auto sample = 0; sample < bufferSize; ++sample)
        {
            if (sample == timeToStartPlaying)
            {
                // Play the metronome sound
                std::cout << "Play" << std::endl;
                pMetronomeSample->getNextAudioBlock(bufferToFill);
            }
        }
    }

    // Reset the metronome sample position if needed
    if (pMetronomeSample->getNextReadPosition() != 0)
    {
        std::cout << "Entrou aqui" << pMetronomeSample->getNextReadPosition() << std::endl;
        // Recreate the AudioFormatReaderSource with a new AudioFormatReader if necessary
        // auto formatReader = mAudioFormatManager.createReaderFor(mySamples[0]);
        // pMetronomeSample.reset(new AudioFormatReaderSource(formatReader, true));

        // Now proceed to read the next audio block
        pMetronomeSample->getNextAudioBlock(bufferToFill);
    }
}

