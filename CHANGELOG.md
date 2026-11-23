# Changelog

All notable changes to Crestwright are documented here.
The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and the project uses [Semantic Versioning](https://semver.org/).

## [1.1.0] - 2026-11-23

### Changed

- Phase module updated to the revised six-band allpass preset (v2). The
  lowest section moved to 21.272 Hz; the other five sections are unchanged.
- Group delay at 1x is now 22.06 ms at 20 Hz, 16.40 ms at 30 Hz and 10.97 ms
  at 50 Hz (44.1 kHz).
- Depth scaling: 22.06 / 44.13 / 66.19 / 88.25 ms at 20 Hz for 1x–4x.
- Version bumped to 1.1.0.

### Measurements

- Phase magnitude deviation below 1e-6 dB from 10 Hz to 20 kHz
  (4.9e-7 dB measured at 44.1 kHz).
- No change to latency, compressor or clipper behaviour.

## [1.0.0] - 2026-11-22

### Added

- Initial public release of Crestwright.
- Six-section second-order allpass phase module with 1x / 2x / 3x / 4x depth.
- 30 ms click-free output crossfade between depth settings.
- FET-style stereo-linked compressor with a detector-side high-pass.
- Parallel compressor mix, soft clipper, auto gain and polarity inversion.
- Zero-latency signal path; double-precision phase-module state.

### Measurements

- Phase magnitude deviation below 1e-6 dB from 10 Hz to 20 kHz.
- Phase group delay matches the specification within tolerance from 20 Hz to
  500 Hz at 1x; the small residual above 1 kHz is inherent to the approved
  six-band preset.
- Phase module CPU cost: about 0.18% of one core at 4x, 48 kHz stereo.

### Licence

- Released under the GNU Affero General Public License, version 3 or later.
- Third-party components: JUCE 9 (AGPLv3 / commercial), Steinberg VST3 SDK
  (MIT), Inter font (SIL OFL 1.1). See `THIRD-PARTY-NOTICES.md`.
