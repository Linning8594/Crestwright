# Contributing to Crestwright

Thanks for your interest in Crestwright. The project is licensed under the
GNU Affero General Public License, version 3 or later (AGPL-3.0-or-later).
See [LICENSE](LICENSE).

## Licence of contributions

By submitting a pull request, patch, or any other contribution, you agree that
your contribution is licensed under AGPL-3.0-or-later, the same terms as the
rest of the project. You confirm that you have the right to submit the work and
that it does not contain third-party code, presets, or measurement data that
cannot be distributed under those terms.

## Reporting bugs

Open a GitHub issue and include:

- operating system and DAW/host,
- Crestwright version,
- steps to reproduce,
- expected and actual behaviour,
- a short audio example if the problem is audible (only audio you have the
  right to share).

Do not attach proprietary presets, third-party plugin measurement data, or
audio you do not own. The reference preset used to derive the phase module is
not part of this repository and should not be added to it.

Security problems should **not** be reported in a public issue. See
[SECURITY.md](SECURITY.md).

## Building

Requirements:

- CMake 3.22 or newer
- Ninja
- Visual Studio 2022 MSVC on Windows, or Xcode / Clang on macOS
- a JUCE 9 source checkout (the CMake option `JUCE_DIR` points to it; the
  default is `D:/JUCE`)

On Windows:

```bat
build.bat
```

The build products are written to
`build/Crestwright_artefacts/Release/`:

- `VST3/Crestwright.vst3`
- `Standalone/Crestwright.exe`

macOS instructions are in [BUILD-MACOS.md](BUILD-MACOS.md).

## Testing the phase module

`verification/test_phase.cpp` compiles the same `Source/PhaseNetwork.h` that
ships in the plugin and measures the actual DSP code:

```bat
cd verification
cl /O2 /std:c++17 /EHsc /Fe:test_phase.exe test_phase.cpp
test_phase.exe
```

`verification/measure_gd.py` measures group delay from WAV files:

```bash
python measure_gd.py --make-impulse impulse.wav --sr 44100
python measure_gd.py dry.wav wet_1x.wav --sr 44100
```

Please run the phase test after changing anything in `Source/PhaseNetwork.h`
or the phase path in `Source/PluginProcessor.cpp`. The expected results are
listed in [docs/PHASE-DESIGN.md](docs/PHASE-DESIGN.md).

## Code style

- C++17, JUCE conventions: four spaces, braces on their own line, `noexcept`
  on audio-thread helpers.
- No allocation, locking, logging, or file access inside `processBlock()`.
- Keep the phase module zero-latency and pure allpass. Do not add a dry/wet
  mix, EQ, or drive stage to it.
- Keep filter state and coefficients in double precision; keep the pole
  radius at or below 0.9995; keep the NaN/Inf reset.
- Depth changes must stay click-free through the 30 ms output crossfade.

## Pull requests

- Keep each change focused and explain the problem it solves.
- Update `CHANGELOG.md` for user-visible changes.
- State which tests you ran and on which sample rates.
- By opening a pull request you accept the contribution licence above.
