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
      handleMomentaryBtn(message);
      handleMuteBtn(message);

      if (isShiftPressed)
      {
        selectSample(message);
      }
      else if (isSelectSequencePressed)
      {
        selectPattern(message);
      }
      else if (isPatternChainning)
      {
        if (message.getControllerNumber() >= 51 && message.getControllerNumber() <= 58)
        {
          mySamplerVoice.chainPattern(message.getControllerNumber() - 51);
        }
      }
      else if (isSamplePlay)
      {
        if (message.getControllerNumber() >= 51 && message.getControllerNumber() <= 58)
        {
          mySamplerVoice.playSample(message.getControllerNumber() - 51, message.getControllerValue());
        }
      }
      else if (isPatterLength)
      {
        if (message.getControllerNumber() >= 51 && message.getControllerNumber() <= 114)
        {
          mySamplerVoice.changePatternLength(message.getControllerNumber() - 51);
        }
      }
      else if (message.getControllerNumber() >= 51 && message.getControllerNumber() <= 114)
      {
        mySamplerVoice.updateSampleIndex(message.getControllerNumber() - 51, message.getControllerValue());
      }
      else
      {
        handleOtherControllers(message);
      }
    }

    std::cout << "CC: " << message.getControllerNumber() << std::endl;
  }

private:
  juce::Synthesiser &mySynth;
  MySamplerVoice &mySamplerVoice;
  juce::AudioIODevice *device;
  int count = 0;
  bool isShiftPressed = false;
  bool isSelectSequencePressed = false;
  bool isPatternChainning = false;
  bool isSamplePlay = false;
  bool isPatterLength = false;
  bool momentaryBtnStates[8] = {false};
  const int muteSamplesBtnIndex[8] = {18, 21, 24, 27, 30, 33, 36, 39};
  const int momentaryBtnIndices[8] = {16, 19, 22, 25, 28, 31, 34, 37};
  int knobPage = 1;

  void handleMuteBtn(const juce::MidiMessage &message)
  {
    for (int i = 0; i < 8; ++i)
    {
      if (message.getControllerNumber() == muteSamplesBtnIndex[i])
      {
        mySamplerVoice.activateSample(i, message.getControllerValue());
      }
    }
  }

  void handleMomentaryBtn(const juce::MidiMessage &message)
  {
    for (int i = 0; i < 8; ++i)
    {
      if (message.getControllerNumber() == momentaryBtnIndices[i])
      {
        momentaryBtnStates[i] = (message.getControllerValue() == 127);
      }
    }

    if (message.getControllerNumber() == 45)
    {
      isShiftPressed = (message.getControllerValue() == 127);
    }
    else if (message.getControllerNumber() == 46)
    {
      isSelectSequencePressed = (message.getControllerValue() == 127);
    }
    else if (message.getControllerNumber() == 47)
    {
      isPatterLength = (message.getControllerValue() == 127);
    }
    else if (message.getControllerNumber() == 44)
    {
      isSamplePlay = (message.getControllerValue() == 127);
    }
    else if (message.getControllerNumber() == 41)
    {
      isPatternChainning = (message.getControllerValue() == 127);
    }
  }

  void selectPattern(const juce::MidiMessage &message)
  {
    int controllerNumber = message.getControllerNumber();

    if ((controllerNumber >= 51 && controllerNumber <= 58) ||
        (controllerNumber >= 67 && controllerNumber <= 74) ||
        (controllerNumber >= 83 && controllerNumber <= 90) ||
        (controllerNumber >= 99 && controllerNumber <= 106))
    {
      int patternIndex = (controllerNumber - 51) % 8;
      mySamplerVoice.changeSelectedSequence(patternIndex);
    }
  }

  void selectSample(const juce::MidiMessage &message)
  {
    int controllerNumber = message.getControllerNumber();

    if ((controllerNumber >= 51 && controllerNumber <= 58) ||
        (controllerNumber >= 67 && controllerNumber <= 74) ||
        (controllerNumber >= 83 && controllerNumber <= 90) ||
        (controllerNumber >= 99 && controllerNumber <= 106))
    {
      int sampleIndex = (controllerNumber - 51) % 8;
      mySamplerVoice.changeSelectedSample(sampleIndex);
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
    case 4:
    case 5:
    case 6:
    case 7:
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
      if (knobPage == 1)
      {
        mySamplerVoice.changeAdsrValues(message.getControllerValue(), 25);
      }
      else if (knobPage == 2)
      {
        mySamplerVoice.changeSampleStart(message.getControllerValue());
      }
      break;
    case 9:
      if (knobPage == 1)
      {
        mySamplerVoice.changeAdsrValues(message.getControllerValue(), 26);
      }
      else if (knobPage == 2)
      {
        mySamplerVoice.changeSampleLength(message.getControllerValue());
      }
      break;
    case 10:
      if (knobPage == 1)
      {
        mySamplerVoice.changeAdsrValues(message.getControllerValue(), 27);
      }
      else if (knobPage == 2)
      {
        mySamplerVoice.changeChorus(message.getControllerValue());
      }
      break;
    case 11:
      if (knobPage == 1)
      {
        mySamplerVoice.changeAdsrValues(message.getControllerValue(), 28);
      }
      else if (knobPage == 2)
      {
        mySamplerVoice.changeFlanger(message.getControllerValue());
      }
      break;
    case 12:
      if (knobPage == 1)
      {
        mySamplerVoice.changeLowPassFilter(device->getCurrentSampleRate(), message.getControllerValue());
      }
      else if (knobPage == 2)
      {
        mySamplerVoice.changePanner(message.getControllerValue());
      }
      break;
    case 13:
      if (knobPage == 1)
      {
        mySamplerVoice.changeHighPassFilter(device->getCurrentSampleRate(), message.getControllerValue());
      }
      else if (knobPage == 2)
      {
        mySamplerVoice.changePhaser(message.getControllerValue());
      }
      break;
    case 14:
      if (knobPage == 1)
      {
        mySamplerVoice.changeBandPassFilter(device->getCurrentSampleRate(), message.getControllerValue());
      }
      else if (knobPage == 2)
      {
        mySamplerVoice.changeDelay(message.getControllerValue());
      }
      break;
    case 15:
      if (knobPage == 1)
      {
        mySamplerVoice.changeReverb(message.getControllerValue());
      }
      else if (knobPage == 2)
      {
        mySamplerVoice.changeBPM(message.getControllerValue());
      }
      break;
    case 16:
      break;
    case 40:
      if (message.getControllerValue() == 0)
      {
        knobPage = 1;
      }
      else
      {
        knobPage = 2;
      }
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
    case 48:
      mySamplerVoice.saveData();
      break;
    case 49:
      mySamplerVoice.clearPattern();
      break;
    case 50:
      mySamplerVoice.clearSampleModulation();
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
    juce::AudioBuffer<float> outputBuffer(outputChannelData, numOutputChannels, numSamples);
    mySamplerVoice.countSamples(outputBuffer, 0, outputBuffer.getNumSamples());
  }

private:
  juce::Synthesiser &mySynth;
  MySamplerVoice &mySamplerVoice;
  juce::AudioIODevice *device;
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
  // Metronome metronome(&mySynth);
  juce::AudioFormatManager mAudioFormatManager;
  std::vector<int> lengthInSamples;
  juce::BigInteger range;
  range.setRange(0, 128, true);

  mAudioFormatManager.registerBasicFormats();

  juce::File samplesFolder{juce::File::getSpecialLocation(juce::File::userDesktopDirectory).getChildFile("Samples")};

  if (!samplesFolder.exists() || !samplesFolder.isDirectory())
  {
    std::cerr << "A pasta 'Samples' não foi encontrada no ambiente de trabalho!" << std::endl;
    return -1;
  }

  juce::Array<juce::File> sampleFiles = samplesFolder.findChildFiles(juce::File::TypesOfFileToFind::findFiles, true, "*.*");

  if (sampleFiles.isEmpty())
  {
    std::cerr << "Nenhum ficheiro de áudio encontrado na pasta 'Samples'!" << std::endl;
    return -1;
  }

  mySynth.clearVoices();
  mySynth.clearSounds();

  int maxSamples = 8; // Limitar a 4 samples
  for (int i = 0; i < sampleFiles.size() && i < maxSamples; ++i)
  {
    std::cout << "Carregando ficheiro: " << sampleFiles[i].getFileName().toStdString() << std::endl;

    std::unique_ptr<juce::AudioFormatReader> formatReader(mAudioFormatManager.createReaderFor(sampleFiles[i]));
    if (formatReader != nullptr)
    {
      lengthInSamples.push_back(formatReader->lengthInSamples);
      mySynth.addSound(new juce::SamplerSound(sampleFiles[i].getFileNameWithoutExtension(), *formatReader, range, 60, 0.0, 0.0, 30.0));
    }
    else
    {
      std::cerr << "Não foi possível ler o ficheiro: " << sampleFiles[i].getFileName().toStdString() << std::endl;
    }
  }

  MySamplerVoice myVoice(&mySynth, lengthInSamples.data());

  for (int i = 0; i < maxSamples; ++i)
  {
    mySynth.addVoice(&myVoice);
  }

  if (device && device->isOpen())
  {
    int lastInputIndex = 0;
    auto list = juce::MidiInput::getAvailableDevices();
    auto midiInputs = juce::MidiInput::getAvailableDevices();
    auto midiOutputs = juce::MidiOutput::getAvailableDevices();

    for (int i = 0; i < midiOutputs.size(); i++)
    {
      std::cout << "MIDI IDENTIFIER: " << midiOutputs[i].identifier << std::endl;
      std::cout << "MIDI NAME: " << midiOutputs[i].name << std::endl;
    }

    for (int i = 0; i < midiInputs.size(); i++)
    {
      std::cout << "MIDI IDENTIFIER: " << midiInputs[i].identifier << std::endl;
      std::cout << "MIDI NAME: " << midiInputs[i].name << std::endl;
    }

    auto nanoPad = midiInputs[1];
    auto nanoKontrol = midiInputs[2];

    devmgr.setDefaultMidiOutputDevice(nanoKontrol.identifier);

    if (!devmgr.isMidiInputDeviceEnabled(nanoPad.identifier))
      devmgr.setMidiInputDeviceEnabled(nanoPad.identifier, true);
    if (!devmgr.isMidiInputDeviceEnabled(nanoKontrol.identifier))
      devmgr.setMidiInputDeviceEnabled(nanoKontrol.identifier, true);

    MyAudioIODeviceCallback audioCallback(mySynth, myVoice, device);
    MyMidiInputCallback midiInputCallback(mySynth, myVoice, device);
    devmgr.addAudioCallback(&audioCallback);

    devmgr.addMidiInputDeviceCallback(nanoPad.identifier, &midiInputCallback);
    devmgr.addMidiInputDeviceCallback(nanoKontrol.identifier, &midiInputCallback);

    juce::MidiOutput *midiOutttt = devmgr.getDefaultMidiOutput();

    std::cout << "Midi out: " << devmgr.getDefaultMidiOutputIdentifier() << std::endl;

    juce::MidiMessage offMessage = juce::MidiMessage::controllerEvent(1, 18, 0); // 1 is the channel, 18 is the controller number, and 0 is the value to turn it off.

    midiOutttt->sendMessageNow(offMessage);

    bool playAudio = true;

    while (playAudio)
    {
      if (devmgr.getCpuUsage() > 0.5)
      {
        std::cout << "cpu: " << devmgr.getCpuUsage() << std::endl;
      }
      // Check if a key is pressed to stop audio playback
      if (keyPressed())
      {
        // Stop the audio device before exiting
        // device->stop();
        playAudio = false;
      }
    }
  }

  return 0;
}
