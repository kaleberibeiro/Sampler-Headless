# Sampler Headless
This project is an audio sampler designed to run in headless environments, making it ideal for use on systems such as the Raspberry Pi. It allows for sample manipulation, sequencing, effect application, and MIDI control, making it highly configurable for live performance or music production. This guide details the process of setting up and installing the Sampler on a Raspberry Pi. Please follow the steps below carefully to ensure proper functionality.
# Installation on Raspberry Pi
This guide outlines the process for configuring and installing the Sampler on a Raspberry Pi. Follow the steps below carefully to ensure everything functions correctly.
## 1. Preparing the Environment on the Raspberry Pi
Before beginning the installation, ensure that the Raspberry Pi is updated and has the necessary packages installed.
### Updating the System
Run the following commands to update the system packages:
```
sudo apt update
sudo apt upgrade -y
 ```
### Installing Required Dependencies
Ensure that the libraries and tools needed to compile and run the project are installed:

```
sudo apt install -y build-essential git cmake libasound2-dev libcurl4-openssl-dev pkg-config
```

## 2. Cloning the Repository
Clone the project repository onto the Raspberry Pi:
```
git clone https://github.com/kaleberibeiro/Sampler-Headless.git
```
Navigate to the project directory:
```
cd Sampler-Headless
```
## 3. Configuring the Project
Ensure that the path to the JUCE library is correctly configured.

### Installing the JUCE Library
If JUCE is not already installed on the Raspberry Pi, clone it as follows:
```
git clone https://github.com/juce-framework/JUCE.git ~/JUCE
```

## 4. Compiling the Project
Use the following command to compile the project:
```
make
```
If the compilation completes without errors, an executable file will be generated in the project folder.

## 5. Running the Project
To run the StepSequencer, use the command:
```
./StepSequencer
```
## 6. Troubleshooting
Error libasound.so.2 => not found: Install the ALSA library with the following command:
```
sudo apt install libasound2
```
Error pkg-config not found: Install pkg-config with:
```
sudo apt install pkg-config
```
Other Compilation Errors: Ensure that all dependencies are installed and that the path to JUCE is correctly set in the Makefile.

# MIDI Learning
This feature allows for quick and easy configuration of MIDI controllers. To enter the MIDI controller setup mode, run the program as follows:
```
./StepSequencer --midi-learning
```
After executing the above command, a list of available MIDI devices will be displayed, and you will need to input the index of the devices to configure. For example:
```
Dispositivos MIDI disponíveis:
0: Midi Through Port-0
Insira o(s) índice(s) do(s) dispositivo(s) MIDI que deseja ativar, separados por espaços (ex: 0 2 3):
```
Once the devices are selected, the configuration process will begin. A message such as:

```
Press Play Sequence button...
```
will appear in the console, and the user needs to press the desired button to assign the function. Feedback will be shown as follows:

```
Assigned CC 43 to action: Play Sequence.
```
Repeat the process for all functions, and once complete, the configuration will be finished.

# Features
## Sequencing
### Samples
8 Samples slots available.

### Sequences
Each sample has a total of 8 sequences that can be chained together.

### Steps
Each pattern can have a maximum of 64 steps per sequence, and this number is adjustable.

### Rhythmic Subdivisions
This mode allows for the division of a single step into smaller sound events, such as 2, 3, or 4 subdivisions. To use this feature, simply enter the mode, select a step from the sequence, and set the desired number of subdivisions.

## BPM
The BPM range for this sampler varies from 80 to 207 BPM.

## Manipulation and Effects
- Sample start
- Sample end
- ADSR envelope
 - Attack
 - Decay
 - Sustain
 - Release
- Low-pass filter
- High-pass filter
- Tremolo
- Reverb
- Chorus
- Flanger
- Phaser
- Panner

## Finger Drumming
To activate the Finger Drumming mode, the sequence must be stopped for the Finger Drumming button to function.

## Extras
### Parameter Storage
It is possible to save all parameters and sequences with a single button press.

### Panic Buttons
This sampler is equipped with two panic buttons: one clears the selected sequence, and the other resets the sample's manipulations.

## Live Mode
A Live Mode option is available for this sampler, designed for live performances. It allows for the insertion and removal of samples from the audio output without interrupting the sequence.

# Video Demonstration
[![Watch the video](https://img.youtube.com/vi/rLZ05pbEgV0/0.jpg)](https://www.youtube.com/watch?v=rLZ05pbEgV0)

