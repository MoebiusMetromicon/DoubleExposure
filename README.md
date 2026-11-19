# DoubleExposure

DoubleExposure is digital tape-style echo and loop-layering audio effect. Currently it exists only as a native plug-in for your DAW, but will eventually become its own standalone FX pedal.

## Concept

Capturing short loops on a tape delay with carefully controlled feedback is one of my favourite techniques to create interesting drones. But as soon as you find a nice loop, you can't use your delay pedal as an actual delay anymore. This sparked the inital idea for DoubleExposure: Loop the current echo signal in seperate buffers, so that the live input is still echoed normally. The user-facing controls act as one-knob meta settings, controlling multiple different parameters under the hood.

## Features
- Long delay times with smooth tape-style delay time behavior
- Feedback up to self-oscillation
- _Variable Character:_ DJ-style low pass / high pass with drive or modulated noise
- _Modulation:_ Sine wave modulator with variable speed or wow-and-flutter-like random fluctuation
- _Integrated Reverb:_ Smooth, spacey ambient reverb or short, metallic refelctions
- _Multiple Tape Cores:_ 1 core for delay, 3 cores for loops
- _Space Swirl Effect:_ Loops can be added / removed instantly or with a swirly pitch up / down effect
- _Smooth Loop Fades:_ Loops can crossfade into themselves with a fade  time up to half the loop length
- _Loop Removing Order:_ Users can select if the oldest or the newest loops are removed first
- _Loop Glitches:_ The time offset of loops can change at random to create interesting glitch effects
- _Stereo Effects:_ Optional stereo cross feedback, inverted modulation polarity for left and right channels & independent stereo loop glitching

## Project Structure
- _external:_ Third party code, currently Mutable Instrument's stmlib, DaisyExamples and juce library modules.
- _pedal:_ Code and ressources exclusive to the eventual hardware fx pedal based on the Daisy Seed 3 dev platform. This code is build using make and the tools provided by the DaisyExamples.
- _plugin:_ Code and ressources exclusive to the juce plugin. This part of the project is managed by the projucer / the _.jucer_ project file.
- _shared:_ Main audio processing and helper code used by both the plugin and the hardware pedal. Everything here is implemented header-only.

## Building the Project

_Coming soon..._