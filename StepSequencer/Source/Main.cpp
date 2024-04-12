#include "../JuceLibraryCode/JuceHeader.h"
#include "Metronome.h"
#include <iostream>
#include <vector>
#include <unistd.h>
#include <termios.h>

class MyAudioIODeviceCallback : public juce::AudioIODeviceCallback
{
public:
  MyAudioIODeviceCallback(Metronome &metronome, juce::AudioIODevice *device)
      : metronome(metronome), device(device)
  {
  }

  void audioDeviceAboutToStart(juce::AudioIODevice *device) override
  {
    metronome.prepareToPlay(device->getDefaultBufferSize(), device->getCurrentSampleRate());
  }

  void audioDeviceStopped() override
  {
    // Implement cleanup logic if needed
  }

  void audioDeviceIOCallbackWithContext(const float *const *inputChannelData,
                                        int numInputChannels,
                                        float *const *outputChannelData,
                                        int numOutputChannels,
                                        int numSamples,
                                        const juce::AudioIODeviceCallbackContext &context) override
  {
    // Create AudioBuffer objects for input and output
    juce::AudioBuffer<float> outputBuffer(outputChannelData, numOutputChannels, numSamples);
    // juce::dsp::Limiter<float> limiter;
    // juce::dsp::ProcessSpec processorSpec;
    // processorSpec.numChannels = numOutputChannels;
    // processorSpec.sampleRate = device->getCurrentSampleRate();
    // processorSpec.maximumBlockSize = device->getDefaultBufferSize();

    // limiter.setThreshold(-2.0f);
    // // limiter.setRelease(2.0f);
    // limiter.prepare(processorSpec);

    // Process the metronome for output
    juce::AudioSourceChannelInfo bufferToFill(&outputBuffer, 0, numSamples);
    bufferToFill.clearActiveBufferRegion();
    metronome.getNextAudioBlock(bufferToFill);
    // metronome.playOneTime(bufferToFill);
    // count++;
    // if (count < 1000)
    // {
    //   metronome.playOneTime(bufferToFill);
    // }
    // else
    // {
    //   metronome.playOneTimeMixer(bufferToFill);
    // }

    // std::cout << "Count: " << count << std::endl;
    // std::cout << "Entrou" << std::endl;

    // juce::dsp::AudioBlock<float> audioBlock(bufferToFill.buffer->getArrayOfWritePointers(), numOutputChannels, numSamples);
    // limiter.process(juce::dsp::ProcessContextReplacing<float>(audioBlock));
  }

private:
  Metronome &metronome;
  juce::AudioIODevice *device;
  juce::AudioSourceChannelInfo channelInfo;
  int count = 0;
};

// Function to check if a key has been pressed
bool keyPressed()
{
  fd_set fds;
  struct timeval tv = {0, 0};
  FD_ZERO(&fds);
  FD_SET(STDIN_FILENO, &fds);
  return select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv) == 1;
}

int main()
{

  juce::AudioDeviceManager devmgr;
  devmgr.initialiseWithDefaultDevices(0, 2);
  juce::AudioIODevice *device = devmgr.getCurrentAudioDevice();
  Metronome metronome;

  // auto activeInputChannels = device->getActiveInputChannels();
  // juce::Array<int> activeBits = activeInputChannels.getBitRangeAsInt(0, activeInputChannels.getHighestBit() + 1);

  // juce::String activeChannelsString;

  // for (int bit : activeBits)
  // {
  //   activeChannelsString += juce::String(bit) + ", ";
  // }

  // if (activeChannelsString.isNotEmpty())
  // {
  //   // Remove the trailing ", "
  //   activeChannelsString = activeChannelsString.dropLastCharacters(2);
  // }

  // std::cout << "Active input channels: " << activeChannelsString << std::endl;

  if (device && device->isOpen())
  {
    MyAudioIODeviceCallback audioCallback(metronome, device);
    devmgr.addAudioCallback(&audioCallback);
    // Start the audio device with your callback
    // device->start(&audioCallback);

    // metronome.playAudioOneTime();

    bool playAudio = true;

    while (playAudio)
    {
      // Check if a key is pressed to stop audio playback
      if (keyPressed())
      {
        // Stop the audio device before exiting
        // device->stop();
        playAudio = false;
      }
    }
  }

  // Clean up resources before exiting
  // ...

  return 0;
}
