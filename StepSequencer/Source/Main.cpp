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

    actionsToAssign = {
        "Play Sequence",
        "Shift",
        "Second page effects",
        "Pattern chaining",
        "Finger Drum mode",
        "Sub Step mode",
        "Selecte Pattern",
        "Pattern Length",
        "Save",
        "Reset Sequence",
        "Reset Sample",
    };
  }

  void handleIncomingMidiMessage(juce::MidiInput *source, const juce::MidiMessage &message) override
  {
    if (message.isController())
    {
      if (isLearningMode)
      {
        if (message.getControllerValue() == 127)
        {
          assignControllerToAction(message);
        }
        return;
      }

      handleMomentaryBtn(message);
      handleMuteBtn(message);
      handleOtherControllers(message);

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
        //////// FUNCIONA /////////
        if ((message.getControllerNumber() >= 51 && message.getControllerNumber() <= 58) && message.getControllerValue() == 127)
        {
          if (message.getControllerValue() == 127)
          {
            mySamplerVoice.chainPattern(message.getControllerNumber() - 51);
          }
        }
      }
      else if (isSubStepMode)
      {
        //////// FUNCIONA /////////
        if (message.getControllerNumber() >= 51 && message.getControllerNumber() <= 114)
        {
          if (message.getControllerValue() == 127)
          {
            if (selectedStep == -1)
            {
              selectedStep = message.getControllerNumber() - 51;
            }
            else
            {
              int controllerNumber = message.getControllerNumber();

              if ((controllerNumber >= 51 && controllerNumber <= 54) ||
                  (controllerNumber >= 67 && controllerNumber <= 70) ||
                  (controllerNumber >= 83 && controllerNumber <= 86) ||
                  (controllerNumber >= 99 && controllerNumber <= 102))
              {
                int subStepValue = (controllerNumber - 51) % 4;
                mySamplerVoice.changeSubStep(selectedStep, subStepValue + 1);
                selectedStep = -1;
              }
            }
          }
        }
      }
      else if (fingerDrumMode)
      {
        if (message.getControllerNumber() >= 51 && message.getControllerNumber() <= 58)
        {
          mySamplerVoice.playSample(message.getControllerNumber() - 51, message.getControllerValue());
        }
      }
      else if (isPatterLength)
      {
        ///////// FUNCIONA //////////
        if (message.getControllerNumber() >= 51 && message.getControllerNumber() <= 114)
        {
          if (message.getControllerValue() == 127)
          {
            mySamplerVoice.changePatternLength(message.getControllerNumber() - 51);
          }
        }
      }
      else if (message.getControllerNumber() >= 51 && message.getControllerNumber() <= 114)
      {
        ////////// FUNCIONA ///////////
        if (message.getControllerValue() == 127)
        {
          mySamplerVoice.updateSampleIndex(message.getControllerNumber() - 51, message.getControllerValue());
        }
      }
    }

    // std::cout << "CC: " << message.getControllerNumber() << std::endl;
  }

  void initiateLearningMode()
  {
    isLearningMode = true;
    currentActionIndex = 0;
    promptForAction();
  }

private:
  bool isLearningMode = false;
  std::vector<std::string> actionsToAssign;
  int currentActionIndex;
  std::map<std::string, int> actionMap;
  juce::Synthesiser &mySynth;
  MySamplerVoice &mySamplerVoice;
  juce::AudioIODevice *device;
  int count = 0;
  bool isShiftPressed = false;
  bool isSelectSequencePressed = false;
  bool isPatternChainning = false;
  bool fingerDrumMode = false;
  bool isPatterLength = false;
  bool momentaryBtnStates[8] = {false};
  const int muteSamplesBtnIndex[8] = {18, 21, 24, 27, 30, 33, 36, 39};
  const int momentaryBtnIndices[8] = {16, 19, 22, 25, 28, 31, 34, 37};
  int knobPage = 1;
  bool isSubStepMode = false;
  int selectedStep = -1;

  void promptForAction()
  {
    if (currentActionIndex < actionsToAssign.size())
    {
      std::string currentAction = actionsToAssign[currentActionIndex];
      std::cout << "Press " << currentAction << " button...\n"; // User feedback
    }
    else
    {
      isLearningMode = false;
      std::cout << "All actions have been assigned!\n";
    }
  }

  void assignControllerToAction(const juce::MidiMessage &message)
  {
    int ccNumber = message.getControllerNumber();
    std::string currentAction = actionsToAssign[currentActionIndex];
    actionMap[currentAction] = ccNumber;

    std::cout << "Assigned CC " << ccNumber << " to action: " << currentAction << ".\n";

    // Move to the next action
    currentActionIndex++;
    promptForAction();
  }

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
      fingerDrumMode = (message.getControllerValue() == 127);
      mySamplerVoice.fingerDrumMode(message.getControllerValue() == 127);
    }
    else if (message.getControllerNumber() == 41)
    {
      isPatternChainning = (message.getControllerValue() == 127);
    }
    else if (message.getControllerNumber() == 42)
    {
      isSubStepMode = (message.getControllerValue() == 127);
    }
  }

  void selectPattern(const juce::MidiMessage &message)
  {
    int controllerNumber = message.getControllerNumber();

    if (((controllerNumber >= 51 && controllerNumber <= 58) ||
         (controllerNumber >= 67 && controllerNumber <= 74) ||
         (controllerNumber >= 83 && controllerNumber <= 90) ||
         (controllerNumber >= 99 && controllerNumber <= 106)) &&
        message.getControllerValue() == 127)
    {
      ///////// FUNCIONA /////////
      int patternIndex = (controllerNumber - 51) % 8;
      mySamplerVoice.changeSelectedSequence(patternIndex);
    }
  }

  void selectSample(const juce::MidiMessage &message)
  {
    int controllerNumber = message.getControllerNumber();

    if (((controllerNumber >= 51 && controllerNumber <= 58) ||
         (controllerNumber >= 67 && controllerNumber <= 74) ||
         (controllerNumber >= 83 && controllerNumber <= 90) ||
         (controllerNumber >= 99 && controllerNumber <= 106)) &&
        message.getControllerValue() == 127)
    {
      ///////// FUNCIONA ////////////
      int sampleIndex = (controllerNumber - 51) % 8;
      mySamplerVoice.changeSelectedSample(sampleIndex);
    }
  }

  void handleOtherControllers(const juce::MidiMessage &message)
  {
    if (actionMap.find("Play Sequence") != actionMap.end())
    {
      int assignedCC = actionMap["Play Sequence"];
      if (message.getControllerNumber() == assignedCC)
      {
        if (message.getControllerValue() == 0)
        {
          mySamplerVoice.stopSequence();
        }
        else
        {
          std::cout << "Entrou no play sequence" << std::endl;
          mySamplerVoice.playSequence();
        }
      }
    }
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
      mySamplerVoice.changeSampleVelocity(message.getControllerNumber(), message.getControllerValue());
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
        mySamplerVoice.changeTremolo(message.getControllerValue());
      }
      else if (knobPage == 2)
      {
        // mySamplerVoice.changeDelay(message.getControllerValue());
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

int main(int argc, char *argv[])
{
  bool midiLearningMode = false;
  int numMidiControllers = 0;

  // Verificar argumentos para o modo MIDI Learning e o número de controladores desejado
  for (int i = 1; i < argc; ++i)
  {
    if (std::string(argv[i]) == "--midi-learning")
      midiLearningMode = true;
    else
      numMidiControllers = std::stoi(argv[i]);
  }

  juce::AudioDeviceManager devmgr;
  devmgr.initialiseWithDefaultDevices(0, 2);
  juce::AudioIODevice *device = devmgr.getCurrentAudioDevice();
  juce::Synthesiser mySynth;
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
    MyAudioIODeviceCallback audioCallback(mySynth, myVoice, device);
    MyMidiInputCallback midiInputCallback(mySynth, myVoice, device);
    devmgr.addAudioCallback(&audioCallback);

    if (midiLearningMode)
    {
      auto midiInputs = juce::MidiInput::getAvailableDevices();
      // MyMidiInputCallback midiInputCallback;

      // Listar dispositivos MIDI com índices
      std::cout << "Dispositivos MIDI disponíveis:" << std::endl;
      for (size_t i = 0; i < midiInputs.size(); ++i)
      {
        std::cout << i << ": " << midiInputs[i].name << std::endl;
      }

      // Coletar seleção de dispositivos do usuário
      std::cout << "Insira o(s) índice(s) do(s) dispositivo(s) MIDI que deseja ativar, separados por espaços (ex: 0 2 3): ";
      std::string input;
      std::getline(std::cin, input);

      std::istringstream iss(input);
      int deviceIndex;
      std::vector<int> selectedDevices;

      while (iss >> deviceIndex)
      {
        if (deviceIndex >= 0 && deviceIndex < midiInputs.size())
        {
          selectedDevices.push_back(deviceIndex);
        }
        else
        {
          std::cerr << "Índice inválido: " << deviceIndex << std::endl;
        }
      }

      // Ativar dispositivos selecionados
      for (int index : selectedDevices)
      {
        auto selectedDevice = midiInputs[index];
        std::cout << "Ativando dispositivo: " << selectedDevice.name << std::endl;

        if (!devmgr.isMidiInputDeviceEnabled(selectedDevice.identifier))
          devmgr.setMidiInputDeviceEnabled(selectedDevice.identifier, true);

        devmgr.addMidiInputDeviceCallback(selectedDevice.identifier, &midiInputCallback);
      }

      midiInputCallback.initiateLearningMode();
    }
    else
    {
      std::cerr << "Modo MIDI Learning não ativado ou número inválido de controladores especificado." << std::endl;

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
    }

    bool playAudio = true;

    while (playAudio)
    {
      if (devmgr.getCpuUsage() > 0.01)
      {
        std::cout << "cpu: " << devmgr.getCpuUsage() << std::endl;
      }
      if (keyPressed())
      {
        playAudio = false;
      }
    }
  }

  return 0;
}
