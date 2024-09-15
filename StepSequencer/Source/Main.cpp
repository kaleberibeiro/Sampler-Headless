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
      case 51:
        mySamplerVoice.changeSelectedSample(0);
        if (mySamplerVoice.selectedSample && *(mySamplerVoice.selectedSample) == 0)
        {
          mySamplerVoice.activateSample(0);
        }
        break;
      case 53:
        mySamplerVoice.changeSelectedSample(1);
        if (mySamplerVoice.selectedSample && *(mySamplerVoice.selectedSample) == 1)
        {
          mySamplerVoice.activateSample(1);
        }
        break;
      case 55:
        mySamplerVoice.changeSelectedSample(2);
        if (mySamplerVoice.selectedSample && *(mySamplerVoice.selectedSample) == 2)
        {
          mySamplerVoice.activateSample(2);
        }
        break;
      case 57:
        mySamplerVoice.changeSelectedSample(3);
        if (mySamplerVoice.selectedSample && *(mySamplerVoice.selectedSample) == 3)
        {
          mySamplerVoice.activateSample(3);
        }
        break;
      case 0:
        mySamplerVoice.changeSampleVelocity(message.getControllerNumber(), message.getControllerValue());
        break;
      case 1:
        mySamplerVoice.changeSampleVelocity(message.getControllerNumber(), message.getControllerValue());
        break;
      case 2:
        mySamplerVoice.changeSampleVelocity(message.getControllerNumber(), message.getControllerValue());
        break;
      case 3:
        mySamplerVoice.changeSampleVelocity(message.getControllerNumber(), message.getControllerValue());
        break;
      case 90:
        mySamplerVoice.changeSampleStart(message.getControllerValue());
        break;
      case 100:
        mySamplerVoice.changeSampleLength(message.getControllerValue());
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
        mySamplerVoice.PlaySequence();
        break;
      default:
        break;
      }
      std::cout << "Not Key: " << message.getControllerNumber() << std::endl;
      std::cout << "Not Key value: " << message.getControllerValue() << std::endl;
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
  int lengthInSamples[4] = {0, 0, 0, 0};
  std::string fileNames[4] = {"hard-kick.wav", "snare.wav", "house-lead.wav", "dry-808-ride.wav"};
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
