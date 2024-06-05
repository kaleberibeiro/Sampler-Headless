#include "../JuceLibraryCode/JuceHeader.h"
#include "Metronome.h"
#include "MySamplerVoice.h"
#include <iostream>
#include <vector>
#include <unistd.h>
#include <termios.h>

class MyMidiInputCallback : public juce::MidiInputCallback
{
public:
  MyMidiInputCallback(juce::Synthesiser &mySynth, MySamplerVoice &mySamplerVoice, juce::AudioIODevice *device)
      : mySynth(mySynth), mySamplerVoice(mySamplerVoice), device(device)
  {
  }

  void handleIncomingMidiMessage(juce::MidiInput *source, const juce::MidiMessage &message) override
  {
    if (message.isController())
    {
      switch (message.getControllerNumber())
      {
      case 1:
        mySamplerVoice.changeSelectedSample(0);
        break;
      case 2:
        mySamplerVoice.changeSelectedSample(1);
        break;
      case 3:
        mySamplerVoice.changeSelectedSample(2);
        break;
      case 9:
        mySamplerVoice.changeSampleStart(message.getControllerValue());
        break;
      case 10:
        mySamplerVoice.changeSampleLength(message.getControllerValue());
        break;
      case 20:
        mySamplerVoice.changeLowPassFilter(device->getCurrentSampleRate(), message.getControllerValue());
        break;

      default:
        break;
      }
    }
    else
    {
      if (message.isNoteOnOrOff())
      {
        std::cout << "Is Key: " << std::endl;
      }
    }
  }

private:
  juce::Synthesiser &mySynth;
  MySamplerVoice &mySamplerVoice;
  juce::AudioIODevice *device;
  juce::AudioSourceChannelInfo channelInfo;
  int count = 0;
};

class MyAudioIODeviceCallback : public juce::AudioIODeviceCallback
{
public:
  MyAudioIODeviceCallback(juce::Synthesiser &mySynth, MySamplerVoice &mySamplerVoice, juce::AudioIODevice *device)
      : mySynth(mySynth), mySamplerVoice(mySamplerVoice), device(device)
  {
  }

  void audioDeviceAboutToStart(juce::AudioIODevice *device) override
  {
    mySamplerVoice.prepareToPlay(device->getDefaultBufferSize(), device->getCurrentSampleRate());
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
    juce::MidiBuffer midiBuffer;
    // mySynth.renderNextBlock(outputBuffer, midiBuffer, 0, outputBuffer.getNumSamples());
    // juce::SynthesiserSound *sound1 = mySynth.getSound(0).get();
    // mySynth.getVoice(0)->startNote(40, 20, sound1, 20);
    // mySynth.getVoice(0)->renderNextBlock(outputBuffer, 0, outputBuffer.getNumSamples());
    mySamplerVoice.countSamples(outputBuffer, 0, outputBuffer.getNumSamples());
    // mySynth.noteOn(0, 60, 0.6);
    // mySynth.renderNextBlock(outputBuffer, midiBuffer, 0, outputBuffer.getNumSamples());
  }

private:
  juce::Synthesiser &mySynth;
  MySamplerVoice &mySamplerVoice;
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
  juce::Synthesiser mySynth;
  Metronome metronome(&mySynth);
  juce::AudioFormatManager mAudioFormatManager;
  int lengthInSamples[3] = {0, 0, 0};
  std::string fileNames[3] = {"mdp-kick-trance.wav", "closed-hi-hat.wav", "bass.wav"};
  juce::BigInteger range;
  range.setRange(0, 128, true);

  mAudioFormatManager.registerBasicFormats();
  juce::File myFile{juce::File::getSpecialLocation(juce::File::userDesktopDirectory)};

  mySynth.clearVoices();
  mySynth.clearSounds();

  for (int i = 0; i < 3; i++)
  {
    auto mySamples = myFile.findChildFiles(juce::File::TypesOfFileToFind::findFiles, true, fileNames[i]);
    jassert(mySamples[0].exists());
    auto formatReader = mAudioFormatManager.createReaderFor(mySamples[0]);
    lengthInSamples[i] = formatReader->lengthInSamples;

    mySynth.addSound(new juce::SamplerSound(fileNames[i], *formatReader, range, 60, 0.0, 0.0, 3.0));
  }

  MySamplerVoice myVoice(&mySynth, lengthInSamples);

  for (int i = 0; i < 3; i++)
  {
    mySynth.addVoice(&myVoice);
  }

  if (device && device->isOpen())
  {
    MyAudioIODeviceCallback audioCallback(mySynth, myVoice, device);
    MyMidiInputCallback midiInputCallback(mySynth, myVoice, device);
    devmgr.addAudioCallback(&audioCallback);

    int lastInputIndex = 0;
    auto list = juce::MidiInput::getAvailableDevices();
    auto midiInputs = juce::MidiInput::getAvailableDevices();

    auto newInput = midiInputs[0];

    if (!devmgr.isMidiInputDeviceEnabled(newInput.identifier))
      devmgr.setMidiInputDeviceEnabled(newInput.identifier, true);

    devmgr.addMidiInputDeviceCallback(newInput.identifier, &midiInputCallback);
    bool playAudio = true;

    while (playAudio)
    {
      // std::cout << "cpu: " << devmgr.getCpuUsage() << std::endl;
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
