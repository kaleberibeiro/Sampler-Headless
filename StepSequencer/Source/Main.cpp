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

      if (message.getControllerNumber() == 50)
      {
        changePage();
      } else if (isShiftPressed)
      {
        activateSample(message);
      }
      else if (isSampleSelectedPressed)
      {
        selectSample(message);
      }
      else if (message.getControllerNumber() >= 51 && message.getControllerNumber() <= 66)
      {
        mySamplerVoice.updateSampleIndex(getAdjustedIndex(message.getControllerNumber()), message.getControllerValue());
      }
      else if (message.getControllerNumber() >= 67 && message.getControllerNumber() <= 74)
      {
        mySamplerVoice.playSample(message.getControllerNumber() - 67, message.getControllerValue());
      }
      else
      {
        handleOtherControllers(message);
      }

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
  int currentPage = 1;
  const int totalPages = 4;

  void changePage()
  {
    currentPage++;
    if (currentPage > totalPages)
    {
      currentPage = 1; // Wrap back to page 1
    }
    std::cout << "Current Page: " << currentPage << std::endl; // Print the current page for debugging
  }

  // Function to adjust the index based on the current page
  int getAdjustedIndex(int controllerNumber)
  {
    int index = controllerNumber - 51;     // Get the index based on the controller number (0-15)
    return (currentPage - 1) * 16 + index; // Adjust index based on current page
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
      isSampleSelectedPressed = (message.getControllerValue() == 127);
    }
  }

  void activateSample(const juce::MidiMessage &message)
  {
    switch (message.getControllerNumber())
    {
    case 51:
      mySamplerVoice.activateSample(message.getControllerNumber() - 51);
      break;
    case 52:
      mySamplerVoice.activateSample(message.getControllerNumber() - 51);
      break;
    case 53:
      mySamplerVoice.activateSample(message.getControllerNumber() - 51);
      break;
    case 54:
      mySamplerVoice.activateSample(message.getControllerNumber() - 51);
      break;
    case 55:
      mySamplerVoice.activateSample(message.getControllerNumber() - 51);
      break;
    case 56:
      mySamplerVoice.activateSample(message.getControllerNumber() - 51);
      break;
    case 57:
      mySamplerVoice.activateSample(message.getControllerNumber() - 51);
      break;
    case 58:
      mySamplerVoice.activateSample(message.getControllerNumber() - 51);
      break;
    }
  }

  void selectSample(const juce::MidiMessage &message)
  {
    switch (message.getControllerNumber())
    {
    case 51:
      mySamplerVoice.changeSelectedSample(message.getControllerNumber() - 51);
      break;
    case 52:
      mySamplerVoice.changeSelectedSample(message.getControllerNumber() - 51);
      break;
    case 53:
      mySamplerVoice.changeSelectedSample(message.getControllerNumber() - 51);
      break;
    case 54:
      mySamplerVoice.changeSelectedSample(message.getControllerNumber() - 51);
      break;
    case 55:
      mySamplerVoice.changeSelectedSample(message.getControllerNumber() - 51);
      break;
    case 56:
      mySamplerVoice.changeSelectedSample(message.getControllerNumber() - 51);
      break;
    case 57:
      mySamplerVoice.changeSelectedSample(message.getControllerNumber() - 51);
      break;
    case 58:
      mySamplerVoice.changeSelectedSample(message.getControllerNumber() - 51);
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
  std::vector<int> lengthInSamples;
  juce::BigInteger range;
  range.setRange(0, 128, true);

  mAudioFormatManager.registerBasicFormats();

  // Caminho para a pasta "Samples"
  juce::File samplesFolder{juce::File::getSpecialLocation(juce::File::userDesktopDirectory).getChildFile("Samples")};

  // Verifica se a pasta existe
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

  return 0;
}
