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
- **Output gain** control (±12 dB) for level matching after EQ.
- **Parameter smoothing** (20 ms linear ramp) eliminates zipper noise
  during slider automation.
- **Click-free bypass** crossfade (50 ms) — no pops when toggling.
- **Factory presets**: Flat, Vocal Boost, Bass Heavy, Bright, Warm, Scoop —
  selectable from the in-plugin dropdown or via the DAW's preset browser.
- Retro NES-style pixel-art UI (classic 2C02 palette).
- Power-meter style vertical sliders with lit LED segments.
- In-plugin preset selector ComboBox.
- "A" button bypass toggle.
- Real-time safe audio processing:
    - Coefficients are only rebuilt when a band's smoothed gain changes
      (small JUCE-internal allocation on rebuild; no allocation at steady state).
    - Sub-block processing (32 samples) for smooth parameter ramps.
    - Output gain is smoothed (20 ms linear ramp) — no clicks on automation.
    - Uses `juce::ScopedNoDenormals` in `processBlock`.
- Full plugin state save/restore (including current preset) via
  `AudioProcessorValueTreeState`.
- Stereo and mono bus layouts supported.
- Multi-format build targets:
    - **VST3** on every platform
    - **AU** on macOS (requires Xcode / the Audio Unit SDK)

## Project layout

```
CMakeLists.txt              Top level build – fetches JUCE and configures the plugin
LICENSE                     MIT license
source/
    PluginProcessor.{h,cpp} AudioProcessor – parameters, prepare/process/state
    PluginEditor.{h,cpp}    AudioProcessorEditor – NES themed UI layout
    ThreeBandEQ.{h,cpp}     Mono 3-band EQ with smoothed gain ramps
    BypassCrossfader.{h,cpp} Click-free wet/dry crossfade helper
    Presets.h               Factory preset definitions
    NESLookAndFeel.{h,cpp}  Palette + typeface + basic label drawing
    NESComponents.{h,cpp}   PowerMeterSlider, NESAButton, NESPresetSelector
tests/
    TestsMain.cpp           Console entry point that runs juce::UnitTestRunner
    ThreeBandEQTests.cpp    Frequency-response and smoothing tests
    BypassCrossfaderTests.cpp Crossfade behaviour tests
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

### Installation

Copy the built VST3 bundle to your system plugin folder:

| Platform | Path |
|----------|------|
| Linux    | `~/.vst3/` |
| macOS    | `~/Library/Audio/Plug-Ins/VST3/` |
| Windows  | `C:\Program Files\Common Files\VST3\` |

For AU on macOS, copy to `~/Library/Audio/Plug-Ins/Components/`.

## Tests

A `juce::UnitTest` suite exercises the 3-band EQ DSP and the bypass
crossfader — checking flat response at 0 dB, band isolation, symmetry
of boost vs. cut, parameter smoothing behaviour, and crossfade
characteristics. To build and run locally:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target NES_EQ_Tests -j
ctest --test-dir build --output-on-failure
```

CI (`.github/workflows/ci.yml`) runs the full VST3 build + test matrix on
Ubuntu, macOS and Windows; macOS additionally builds the AU target.

## Parameters

| ID            | Name   | Range          | Default | Notes                       |
|---------------|--------|----------------|---------|-----------------------------|
| `low_gain`    | Low    | −15 … +15 dB   | 0 dB    | Low-shelf, 200 Hz           |
| `mid_gain`    | Mid    | −15 … +15 dB   | 0 dB    | Peak, 1 kHz, Q ≈ 0.9        |
| `high_gain`   | High   | −15 … +15 dB   | 0 dB    | High-shelf, 5 kHz           |
| `output_gain` | Output | −12 … +12 dB   | 0 dB    | Post-EQ output trim         |
| `bypass`      | Bypass | boolean        | off     | Crossfaded bypass           |

## Factory Presets

| # | Name        | Low   | Mid   | High  |
|---|-------------|-------|-------|-------|
| 0 | Flat        | 0 dB  | 0 dB  | 0 dB  |
| 1 | Vocal Boost | −2 dB | +4.5 dB | +2 dB |
| 2 | Bass Heavy  | +8 dB | −1 dB | −2 dB |
| 3 | Bright      | −1 dB | 0 dB  | +6 dB |
| 4 | Warm        | +3 dB | +1 dB | −4 dB |
| 5 | Scoop       | +3 dB | −5 dB | +3 dB |

## License

MIT — see [LICENSE](LICENSE).

Note: This project uses the JUCE framework. See
[JUCE's license](https://juce.com/juce-7-licence/) for any plugin-SDK
related obligations (VST3 SDK, AU SDK) when redistributing the built binaries.
