# NES-EQ

An 8-bit themed, real-time safe **3-band EQ** audio plugin built with
[JUCE](https://juce.com/). The UI is styled as a chunky NES / pixel-art
control panel — LED power-meter style sliders for each band and a round
red "A" button for bypass.

## Features

- 3-band EQ DSP pipeline:
    - **Low** – low-shelf filter at 200 Hz (±15 dB)
    - **Mid** – peak/bell filter at 1 kHz, Q ≈ 0.9 (±15 dB)
    - **High** – high-shelf filter at 5 kHz (±15 dB)
- Retro NES-style pixel-art UI (classic 2C02 palette).
- Power-meter style vertical sliders with lit LED segments.
- "A" button bypass toggle.
- Real-time safe audio processing:
    - No heap allocations on the audio thread.
    - Per-band gain is linearly smoothed (20 ms ramp) so dragging a
      slider is zipper-noise free. Coefficients are only rebuilt while
      the smoother is moving and are cached once the target is reached.
    - Soft-bypass: the "A" button crossfades between the dry input and
      the processed output over a short ramp, so toggling never clicks.
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
    ThreeBandEQ.{h,cpp}     Mono 3-band EQ (low shelf + peak + high shelf)
    BypassCrossfader.{h,cpp} Click-free dry/wet ramp used by the bypass toggle
    NESLookAndFeel.{h,cpp}  Palette + typeface + basic label drawing
    NESComponents.{h,cpp}   PowerMeterSlider and NESAButton custom components
tests/
    TestsMain.cpp           Console entry point that runs juce::UnitTestRunner
    ThreeBandEQTests.cpp    Frequency-response tests for the EQ DSP
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

A small `juce::UnitTest` suite exercises both the 3-band EQ DSP and the
bypass crossfader:

- Flat response at 0 dB and per-band boost / cut symmetry.
- Gain target changes are smoothed (not stepped) and `snap()` applies
  gains instantly for state restoration.
- Bypass crossfader passes the wet signal through when fully active,
  the dry signal when fully bypassed, and produces no sample-to-sample
  discontinuities during the ramp.
- `update` is a no-op when gains are unchanged (coefficient cache).

To build and run locally:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target NES_EQ_Tests -j
ctest --test-dir build --output-on-failure
```

CI (`.github/workflows/ci.yml`) runs the full VST3 build + test matrix on
Ubuntu, macOS and Windows; macOS additionally builds the AU target.

## Parameters

| ID          | Name   | Range          | Default | Notes                       |
|-------------|--------|----------------|---------|-----------------------------|
| `low_gain`  | Low    | −15 … +15 dB   | 0 dB    | Low-shelf, 200 Hz           |
| `mid_gain`  | Mid    | −15 … +15 dB   | 0 dB    | Peak, 1 kHz, Q ≈ 0.9        |
| `high_gain` | High   | −15 … +15 dB   | 0 dB    | High-shelf, 5 kHz           |
| `bypass`    | Bypass | boolean        | off     | Fully bypasses the EQ chain |

## License

See [JUCE's license](https://juce.com/juce-7-licence/) for any plugin-SDK
related obligations (VST3 SDK, AU SDK) when redistributing the built binaries.
