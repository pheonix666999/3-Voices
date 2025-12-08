# 3 Voice Unison Mod

A 3-voice unison modulation audio plugin with per-voice controls and distortion.

## Features

- **3 Independent Voices (I, II, III)** - Each can be toggled on/off
- **Per-Voice Modulation Controls:**
  - Speed - LFO rate for modulation (0.1 - 10 Hz)
  - Delay Time - Base delay time (1 - 50 ms)
  - Depth - Modulation depth (0 - 100%)
- **Per-Voice Distortion:**
  - Distortion amount fader
  - Tube saturation (toggle)
  - Bit crusher (toggle)
  - Both can be active simultaneously
- **Global Controls:**
  - Input Gain (-24 to +24 dB)
  - Output Gain (-24 to +24 dB)
  - Width - Stereo width (pans Voice II left, Voice III right)
  - Mix - Dry/Wet mix (0 - 100%)

## Building

### Option 1: Using Projucer (Recommended)

1. Download JUCE from https://juce.com/get-juce/
2. Open `ThreeVoicesUnisonMod.jucer` in Projucer
3. Set your JUCE modules path in Projucer
4. Select your target platform (Windows/macOS)
5. Click "Save Project and Open in IDE"
6. Build in your IDE (Visual Studio/Xcode)

### Option 2: Using CMake

1. Clone JUCE into this directory:
   ```
   git clone https://github.com/juce-framework/JUCE.git
   ```

2. Create build directory:
   ```
   mkdir build && cd build
   ```

3. Configure and build:
   ```
   cmake ..
   cmake --build . --config Release
   ```

## Plugin Formats

- VST3
- AU (macOS only)
- Standalone

## Signal Flow

1. Input Gain
2. For each active voice:
   - Modulated delay (chorus/vibrato effect)
   - Optional Tube saturation
   - Optional Bit crusher
3. Width processing (panning voices II and III)
4. Dry/Wet Mix
5. Output Gain

## Notes

- If all voices are off, the signal passes through dry
- Distortion is only applied if Tube and/or Bit is enabled
- Width at 0% keeps all voices centered, 100% pans fully L/R
