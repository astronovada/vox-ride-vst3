# VoxRide — VST3 / AU DAW Plugin (JUCE 8 C++20)

Real-time automatic vocal gain rider and clarity studio plugin with stereo sidechain music detection, movable Min/Max range stops, dynamic De-Esser, High-Pass, De-Mud, Presence/Air EQ, and True-Peak Limiter.

## Baked-In Default Session Settings
- **Active Preset**: Lead Vocal — Balanced Ride
- **Target Level**: -18.0 dBFS
- **Range**: -6.0 dB to +6.0 dB
- **Response Speed**: FAST
- **Vocal / Music Sensitivity**: 1.5 / 2.0
- **Vocal Clarity**: ON (HPF 85 Hz, De-Mud 30%, Presence 38%, De-Ess 32%)

## Build Requirements
- CMake 3.22+
- C++20 Compiler:
  - **macOS**: Xcode Command Line Tools (`xcode-select --install`)
  - **Windows**: Visual Studio 2022 (Desktop development with C++)
  - **Linux**: GCC/Clang + ALSA/FreeType/X11 dev headers
- Internet access on first build (CMake automatically fetches JUCE 8.0.4).

## Quick Build (One Command)
- **macOS**: `./scripts/build-macos.sh`
- **Windows**: Double-click `scripts\build-windows.bat`
- **Linux**: `./scripts/build-linux.sh`

## Using Sidechain Music Sensitivity in Your DAW
1. Insert **VoxRide** on your Lead Vocal track.
2. Route your Instrumental / Music Bus to VoxRide's **Sidechain** input in Ableton Live, Logic Pro, FL Studio, Cubase, or Reaper.
3. Raise **Music Sensitivity** so VoxRide automatically lifts your vocal target when the backing chorus swells.
