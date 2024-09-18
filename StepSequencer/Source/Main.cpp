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
      // Handle the shift and sample select keys
      handleMomentaryBtn(message);

      if (isShiftPressed)
      {
        // Activate samples when Shift (key 45) is pressed
        activateSample(message);
      }
      else if (isSampleSelectedPressed)
      {
        // Select samples when Sample Select (key 46) is pressed
        selectSample(message);
      }
      else if (message.getControllerNumber() >= 51 && message.getControllerNumber() <= 66)
      {
        // Call updateSampleIndex for controllers 51 to 66
        mySamplerVoice.updateSampleIndex(message.getControllerNumber() - 51, message.getControllerValue());
      }
      else if (message.getControllerNumber() >= 67 && message.getControllerNumber() <= 74)
      {
        mySamplerVoice.playSample(message.getControllerNumber() - 67, message.getControllerValue());
      }
      else
      {
        // Handle other controller changes
        handleOtherControllers(message);
      }
      // Logging for debugging
      // std::cout << "Controller Number: " << message.getControllerNumber() << std::endl;
      // std::cout << "Controller Value: " << message.getControllerValue() << std::endl;
    }
  }

private:
  juce::Synthesiser &mySynth;
  MySamplerVoice &mySamplerVoice;
  juce::AudioIODevice *device;
  juce::AudioSourceChannelInfo channelInfo;
  int count = 0;
  bool isShiftPressed = false;
  bool isSampleSelectedPressed = false;
  bool momentaryBtnStates[8] = {false};
  const int momentaryBtnIndices[8] = {16, 19, 22, 25, 28, 31, 34, 37};

  void handleMomentaryBtn(const juce::MidiMessage &message)
  {
    for (int i = 0; i < 8; ++i)
    {
      if (message.getControllerNumber() == momentaryBtnIndices[i])
      {
        momentaryBtnStates[i] = (message.getControllerValue() == 127); // Button is pressed when value is 127
      }
    }

    if (message.getControllerNumber() == 45)
    {
      isShiftPressed = (message.getControllerValue() == 127);
    }
    else if (message.getControllerNumber() == 46)
    {
      isSampleSelectedPressed = (message.getControllerValue() == 127);
    }
  }

  void activateSample(const juce::MidiMessage &message)
  {
    switch (message.getControllerNumber())
    {
    case 51:
      mySamplerVoice.activateSample(0);
      break;
    case 52:
      mySamplerVoice.activateSample(1);
      break;
    case 53:
      mySamplerVoice.activateSample(2);
      break;
    case 54:
      mySamplerVoice.activateSample(3);
      break;
    }
  }

  void selectSample(const juce::MidiMessage &message)
  {
    switch (message.getControllerNumber())
    {
    case 51:
      mySamplerVoice.changeSelectedSample(0);
      break;
    case 52:
      mySamplerVoice.changeSelectedSample(1);
      break;
    case 53:
      mySamplerVoice.changeSelectedSample(2);
      break;
    case 54:
      mySamplerVoice.changeSelectedSample(3);
      break;
    }
  }

  void handleOtherControllers(const juce::MidiMessage &message)
  {
    switch (message.getControllerNumber())
    {
    case 0:
    case 1:
    case 2:
    case 3:
      if (momentaryBtnStates[message.getControllerNumber()])
      {
        mySamplerVoice.changePitchShift(message.getControllerNumber(), message.getControllerValue());
      }
      else
      {
        mySamplerVoice.changeSampleVelocity(message.getControllerNumber(), message.getControllerValue());
      }
      break;
    case 8:
      mySamplerVoice.changeAdsrValues(message.getControllerValue(), 25);
      break;
    case 9:
      mySamplerVoice.changeAdsrValues(message.getControllerValue(), 26);
      break;
    case 10:
      mySamplerVoice.changeAdsrValues(message.getControllerValue(), 27);
      break;
    case 11:
      mySamplerVoice.changeAdsrValues(message.getControllerValue(), 28);
      break;
    case 12:
      mySamplerVoice.changeLowPassFilter(device->getCurrentSampleRate(), message.getControllerValue());
      break;
    case 13:
      mySamplerVoice.changeHighPassFilter(device->getCurrentSampleRate(), message.getControllerValue());
      break;
    case 14:
      mySamplerVoice.changeBandPassFilter(device->getCurrentSampleRate(), message.getControllerValue());
      break;
    case 15:
      mySamplerVoice.changeReverb(message.getControllerValue());
      break;
    case 16:
      mySamplerVoice.changeChorus(message.getControllerValue());
      break;
    case 43:
      if (message.getControllerValue() == 0)
      {
        mySamplerVoice.stopSequence();
      }
      else
      {
        mySamplerVoice.playSequence();
      }
      break;
    case 90:
      mySamplerVoice.changeSampleStart(message.getControllerValue());
      break;
    case 100:
      mySamplerVoice.changeSampleLength(message.getControllerValue());
      break;
    }
  }
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

    mySamplerVoice.countSamples(outputBuffer, 0, outputBuffer.getNumSamples());
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
  int lengthInSamples[4] = {0, 0, 0, 0};
  std::string fileNames[4] = {"mdp-kick-trance.wav", "snare.wav", "techno-bass.wav", "dry-808-ride.wav"};
  juce::BigInteger range;
  range.setRange(0, 128, true);

  mAudioFormatManager.registerBasicFormats();
  juce::File myFile{juce::File::getSpecialLocation(juce::File::userDesktopDirectory)};

  mySynth.clearVoices();
  mySynth.clearSounds();

  for (int i = 0; i < 4; i++)
  {
    auto mySamples = myFile.findChildFiles(juce::File::TypesOfFileToFind::findFiles, true, fileNames[i]);
    jassert(mySamples[0].exists());
    auto formatReader = mAudioFormatManager.createReaderFor(mySamples[0]);
    lengthInSamples[i] = formatReader->lengthInSamples;

    mySynth.addSound(new juce::SamplerSound(fileNames[i], *formatReader, range, 60, 0.0, 0.0, 3.0));
  }

  MySamplerVoice myVoice(&mySynth, lengthInSamples);

  for (int i = 0; i < 4; i++)
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

    for (int i = 0; i < midiInputs.size(); i++)
    {
      std::cout << "MIDI IDENTIFIER: " << midiInputs[i].identifier << std::endl;
      std::cout << "MIDI NAME: " << midiInputs[i].name << std::endl;
    }

    auto nanoPad = midiInputs[1];
    auto nanoKontrol = midiInputs[2];

    if (!devmgr.isMidiInputDeviceEnabled(nanoPad.identifier))
      devmgr.setMidiInputDeviceEnabled(nanoPad.identifier, true);
    if (!devmgr.isMidiInputDeviceEnabled(nanoKontrol.identifier))
      devmgr.setMidiInputDeviceEnabled(nanoKontrol.identifier, true);

    devmgr.addMidiInputDeviceCallback(nanoPad.identifier, &midiInputCallback);
    devmgr.addMidiInputDeviceCallback(nanoKontrol.identifier, &midiInputCallback);
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
