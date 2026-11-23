# Building Crestwright on macOS

Crestwright 1.2.2 · JUCE 9.0.5 · VST3 + Standalone

This guide builds the plugin from source on macOS. The result is a VST3 bundle
and a standalone app. No signing certificate is required for personal use.

## Requirements

- macOS 11 or newer
- Xcode Command Line Tools (`xcode-select --install`)
- CMake 3.22 or newer
- Ninja (optional — the Xcode generator also works)
- Git (installed with the Command Line Tools)

With Homebrew:

```bash
brew install cmake ninja
```

## 1. Get JUCE 9.0.5

Crestwright needs a JUCE source checkout (the same version used on Windows):

```bash
git clone --branch 9.0.5 --depth 1 https://github.com/juce-framework/JUCE.git ~/JUCE
```

If the `9.0.5` tag is unavailable, clone `master` and check out the commit whose
`CMakeLists.txt` contains `project(JUCE VERSION 9.0.5 ...)`.

## 2. Get the Crestwright source

Unzip the source archive, or clone the repository if you have access:

```bash
git clone https://github.com/Linning8594/Crestwright.git Crestwright
cd Crestwright
```

## 3. Configure and build

Universal binary (Apple Silicon + Intel):

```bash
cmake -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DJUCE_DIR="$HOME/JUCE" \
  -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0

cmake --build build --config Release
```

To build only for the current machine's architecture, omit the
`CMAKE_OSX_ARCHITECTURES` line. If Ninja is not installed, use `-G Xcode`
instead and build with `cmake --build build --config Release`.

## 4. Output

- VST3: `build/Crestwright_artefacts/Release/VST3/Crestwright.vst3`
- Standalone: `build/Crestwright_artefacts/Release/Standalone/Crestwright.app`

## 5. Install (per user, no sudo)

```bash
mkdir -p ~/Library/Audio/Plug-Ins/VST3
cp -R build/Crestwright_artefacts/Release/VST3/Crestwright.vst3 ~/Library/Audio/Plug-Ins/VST3/
cp -R build/Crestwright_artefacts/Release/Standalone/Crestwright.app /Applications/
```

For a system-wide install, copy the VST3 to `/Library/Audio/Plug-Ins/VST3/` with
`sudo`.

## 6. First launch on macOS

Bundles that were downloaded may be quarantined. If the DAW does not list the
plugin, or the standalone is blocked, run:

```bash
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/Crestwright.vst3
xattr -dr com.apple.quarantine /Applications/Crestwright.app
```

If the standalone is still refused, ad-hoc sign the bundles:

```bash
codesign --force --deep --sign - /Applications/Crestwright.app
codesign --force --deep --sign - ~/Library/Audio/Plug-Ins/VST3/Crestwright.vst3
```

## Notes

- Keep `BUNDLE_ID`, `PLUGIN_CODE` and `PLUGIN_MANUFACTURER_CODE` unchanged;
  changing them makes existing projects fail to find the plugin.
- Cubase, Nuendo and REAPER use the VST3 build.
- An Audio Unit is not built by default. If Logic Pro support is needed, change
  `FORMATS VST3 Standalone` to `FORMATS VST3 Standalone AU` in `CMakeLists.txt`
  and rebuild; the AU appears under `Crestwright_artefacts/Release/AU/`.
- The Windows installer script (`installer/Crestwright.iss`) is not used on macOS.
- There are no external runtime dependencies; the Inter fonts are embedded in
  the binary.
- Source version: 1.2.2.
