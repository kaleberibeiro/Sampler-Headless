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
  MyMidiInputCallback(Metronome &metronome, juce::AudioIODevice *device)
      : metronome(metronome), device(device)
  {
  }

  void handleIncomingMidiMessage(juce::MidiInput *source, const juce::MidiMessage &message) override
  {
    std::cout << "Entrou aqui: " << &source << "  " << &message << std::endl;
    if (message.isController())
    {
      if (message.getControllerNumber() == 0)
      {
        metronome.startPlayer();
      }
      else
      {
        if (message.getControllerNumber() == 1)
        {
          metronome.stopPlayer();
        }
        else
        {
          if (message.getControllerNumber() == 4)
          {
            metronome.activateSample(2);
          }
        }
      }
      std::cout << "Is Controller: "
                << "Number -> " << message.getControllerNumber() << " Value: " << message.getControllerValue() << std::endl;
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
  Metronome &metronome;
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
    // juce::AudioSourceChannelInfo bufferToFill(&outputBuffer, 0, numSamples);
    // bufferToFill.clearActiveBufferRegion();
    // metronome.getNextAudioBlock(bufferToFill);
    // metronome.playOneTime(outputBuffer);
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
  devmgr.initialiseWithDefaultDevices(0, 1);
  juce::AudioIODevice *device = devmgr.getCurrentAudioDevice();
  juce::Synthesiser mySynth;
  Metronome metronome(&mySynth);
  juce::AudioFormatManager mAudioFormatManager;
  int lengthInSamples[3] = {0, 0, 0};
  std::string fileNames[3] = {"bass.wav", "closed-hi-hat.wav", "bass.wav"};
  juce::BigInteger range;
  range.setRange(0, 128, true);

  mAudioFormatManager.registerBasicFormats();
  juce::File myFile{juce::File::getSpecialLocation(juce::File::userDesktopDirectory)};

  mySynth.clearVoices();
  mySynth.clearSounds();

  for (int i = 0; i < 1; i++)
  {
    auto mySamples = myFile.findChildFiles(juce::File::TypesOfFileToFind::findFiles, true, fileNames[i]);
    jassert(mySamples[0].exists());
    auto formatReader = mAudioFormatManager.createReaderFor(mySamples[0]);
    lengthInSamples[i] = formatReader->lengthInSamples;

    mySynth.addSound(new juce::SamplerSound(fileNames[i], *formatReader, range, 60, 0.0, 0.0, 20.0));
  }

  MySamplerVoice myVoice(&mySynth, lengthInSamples);

  for (int i = 0; i < 1; i++)
  {
    mySynth.addVoice(&myVoice);
  }

  if (device && device->isOpen())
  {
    MyAudioIODeviceCallback audioCallback(mySynth, myVoice, device);
    MyMidiInputCallback midiInputCallback(metronome, device);
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
