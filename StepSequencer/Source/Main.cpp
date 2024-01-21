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
    // Create an AudioBuffer<float> for outputChannelData
    juce::AudioBuffer<float> outputBuffer(outputChannelData, numOutputChannels, numSamples);

    // Use AudioSourceChannelInfo with the AudioBuffer
    juce::AudioSourceChannelInfo bufferToFill(&outputBuffer, 0, numSamples);

    bufferToFill.clearActiveBufferRegion();
    metronome.getNextAudioBlock(bufferToFill);
    // metronome.playAudioOneTime(channelInfo);
  }

private:
  Metronome &metronome;
  juce::AudioIODevice *device;
  juce::AudioSourceChannelInfo channelInfo;
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
