# NES-EQ

An 8-bit themed, real-time safe **8-band EQ** audio plugin built with
[JUCE](https://juce.com/). The UI is styled as a chunky NES / pixel-art
control panel — LED power-meter style sliders for each band and a round
red "A" button for bypass.

## Features

- 8-band EQ DSP pipeline:
    - **SUB** – low-shelf filter at 60 Hz (±15 dB)
    - **BASS** – peak/bell filter at 150 Hz, Q ≈ 0.9 (±15 dB)
    - **LO** – peak/bell filter at 400 Hz, Q ≈ 0.9 (±15 dB)
    - **MID** – peak/bell filter at 800 Hz, Q ≈ 0.9 (±15 dB)
    - **HI-M** – peak/bell filter at 1.6 kHz, Q ≈ 0.9 (±15 dB)
    - **PRES** – peak/bell filter at 3.2 kHz, Q ≈ 0.9 (±15 dB)
    - **BRIL** – peak/bell filter at 6.4 kHz, Q ≈ 0.9 (±15 dB)
    - **AIR** – high-shelf filter at 12 kHz (±15 dB)
- Retro NES-style pixel-art UI (classic 2C02 palette).
- Power-meter style vertical sliders with lit LED segments.
- "A" button bypass toggle.
- Real-time safe audio processing:
    - No heap allocations on the audio thread.
    - Coefficients are only rebuilt when a band's target gain actually changes.
    - Uses `juce::ScopedNoDenormals` in `processBlock`.
- Full plugin state save/restore via `AudioProcessorValueTreeState`.
- Stereo and mono bus layouts supported.
- Multi-format build targets:
    - **VST3** on every platform
    - **AU** on macOS (requires Xcode / the Audio Unit SDK)

## Project layout

```
CMakeLists.txt              Top level build – fetches JUCE and configures the plugin
source/
    PluginProcessor.{h,cpp} AudioProcessor – parameters, prepare/process/state
    PluginEditor.{h,cpp}    AudioProcessorEditor – NES themed UI layout
    EightBandEQ.{h,cpp}     Mono 8-band EQ (low shelf + 6 peaks + high shelf)
    NESLookAndFeel.{h,cpp}  Palette + typeface + basic label drawing
    NESComponents.{h,cpp}   PowerMeterSlider and NESAButton custom components
tests/
    TestsMain.cpp           Console entry point that runs juce::UnitTestRunner
    EightBandEQTests.cpp    Frequency-response tests for the 8-band EQ DSP
    PluginProcessorTests.cpp  Parameter, bus layout, bypass, state, and processing tests
.github/workflows/ci.yml    Linux/macOS/Windows build + test matrix
```

## Building

### Linux / macOS / Windows (VST3)

JUCE is fetched automatically via CMake's `FetchContent`. On Linux you will
need the usual JUCE Linux audio / X11 dependencies. On Debian / Ubuntu:

```bash
sudo apt-get install -y \
    libasound2-dev libjack-jackd2-dev libcurl4-openssl-dev \
    libfreetype6-dev libx11-dev libxcomposite-dev libxcursor-dev \
    libxext-dev libxinerama-dev libxrandr-dev libxrender-dev \
    libglu1-mesa-dev mesa-common-dev libfontconfig1-dev
```

Then:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target NES_EQ_VST3 -j
```

The resulting VST3 bundle is written under
`build/NES_EQ_artefacts/Release/VST3/NES-EQ.vst3`.

### macOS (AU + VST3)

On macOS the CMake config automatically adds the `AU` format. Open the
generated Xcode project or run:

```bash
cmake -B build -G Xcode
cmake --build build --target NES_EQ_AU NES_EQ_VST3 --config Release
```

## Tests

A `juce::UnitTest` suite exercises the 8-band EQ DSP and the full
`PluginProcessor` — checking flat response at 0 dB, that each band boosts
the correct frequency region, shelf and peak filter shapes, symmetry of
boost vs. cut, the coefficient cache, bus layouts, bypass, state
serialization round-trip, and mono processing. To build and run locally:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target NES_EQ_Tests -j
ctest --test-dir build --output-on-failure
```

CI (`.github/workflows/ci.yml`) runs the full VST3 build + test matrix on
Ubuntu, macOS and Windows; macOS additionally builds the AU target.

## Parameters

| ID           | Name  | Range          | Default | Notes                        |
|--------------|-------|----------------|---------|------------------------------|
| `band1_gain` | SUB   | −15 … +15 dB   | 0 dB    | Low-shelf, 60 Hz             |
| `band2_gain` | BASS  | −15 … +15 dB   | 0 dB    | Peak, 150 Hz, Q ≈ 0.9       |
| `band3_gain` | LO    | −15 … +15 dB   | 0 dB    | Peak, 400 Hz, Q ≈ 0.9       |
| `band4_gain` | MID   | −15 … +15 dB   | 0 dB    | Peak, 800 Hz, Q ≈ 0.9       |
| `band5_gain` | HI-M  | −15 … +15 dB   | 0 dB    | Peak, 1.6 kHz, Q ≈ 0.9      |
| `band6_gain` | PRES  | −15 … +15 dB   | 0 dB    | Peak, 3.2 kHz, Q ≈ 0.9      |
| `band7_gain` | BRIL  | −15 … +15 dB   | 0 dB    | Peak, 6.4 kHz, Q ≈ 0.9      |
| `band8_gain` | AIR   | −15 … +15 dB   | 0 dB    | High-shelf, 12 kHz           |
| `bypass`     | Bypass| boolean        | off     | Fully bypasses the EQ chain  |

## License

See [JUCE's license](https://juce.com/juce-7-licence/) for any plugin-SDK
related obligations (VST3 SDK, AU SDK) when redistributing the built binaries.
