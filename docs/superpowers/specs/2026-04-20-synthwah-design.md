# Thom & Guy — Synth Wah Plugin Design

**Date:** 2026-04-20
**Status:** Design approved; pending implementation plan.

## 1. Overview

Thom & Guy is a JUCE audio plugin inspired by the Digitech Synth Wah pedal, aimed specifically at the guitar sound Thomas Bangalter and Guy-Manuel de Homem-Christo used on Daft Punk's *Human After All*. It is an **audio-input effect** (not a MIDI-triggered synth): a mono guitar signal goes in, and a squared-off, envelope-swept synth-like signal comes out.

The plugin emulates the *family* of effects the Digitech pedal produces without attempting bit-exact circuit modeling. The architecture is closer to the classic analog signal flow those pedals use (input squaring + octave divider + resonant filter) than to an oscillator-substitution synth.

### Priority Goals

1. **Envelope-filter synth mode** — mono voice, dirty waveform, envelope-controlled filter. The default sound, and the one that has to nail "Human After All."
2. **Formant filter mode** — two vowel states with envelope-driven morph. Secondary sound.
3. **Envelope feel** — fast-but-not-instant attack, musical decay, touch-responsive sensitivity curve. Cuts across both modes.

### Explicit Non-Goals

- No polyphony. The plugin is monophonic end-to-end.
- No MIDI input. The envelope follower is the only analysis block.
- No pitch tracking. Because there is no oscillator, no pitch detection is needed.
- No circuit-level analog modeling (no component-level emulation of the Digitech pedal).
- No AAX/Pro Tools support in v1. AU, VST3, Standalone only.
- No Windows testing in v1. macOS universal binary only.

## 2. Signal Flow Architecture

```
[Input (mono guitar)]
        │
        ├──► [Envelope Follower] ─► env(t) ──┐ (0..1 control signal)
        │                                     │
        ▼                                     │
[Input Conditioner]                           │
  • DC-block HP @ ~20 Hz                      │
  • Input gain trim                           │
        │                                     │
        ▼                                     │
[Waveshaper Chain]                            │
  • Drive (pre-gain)                          │
  • Morph: soft-clip  ⇄  hard-square          │
  • Sub-octave (flip-flop /2) blend           │
        │                                     │
        ▼                                     │
[Filter Stage] ◄──────────────────────────────┤
  Mode switch:                                │
    • Envelope Mode: SVF (LP or BP),          │
      cutoff = f(env(t))                      │
    • Formant Mode: 2x bank of parallel       │
      bandpass SVFs (vowel A, vowel B),       │
      crossfade = stretch_curve(env(t))       │
        │                                     │
        ▼                                     │
[Output Stage]
  • Wet/Dry blend
  • Output level
        │
        ▼
[Output]
```

### Key Properties

- **Mono throughout.** Stereo input collapsed to mono at the top. Stereo output is dual-mono.
- **Single envelope follower, two consumers.** Both filter modes read from the same `env(t)` — the "touch responsive" feel is built once and inherited by both modes.
- **Modes are mutually exclusive.** Envelope mode and Formant mode do not mix. One filter stage runs at a time.
- **Filter behind an interface.** `Filter` abstract class; `SvfFilter` ships first. A `LadderFilter` can drop in later without changes upstream of the filter stage.

## 3. DSP Components

### 3.1 Input Conditioner

- First-order highpass at ~20 Hz (DC / rumble removal).
- User-facing **Input Gain** trim, ±12 dB.
- No compression, no noise gate.

### 3.2 Envelope Follower (priority #3)

Feel-defining block. Architecture:

1. **Rectify:** `|x|` per sample.
2. **Parallel fast + slow peak detectors, output = max of the two.**
   - Fast: attack ~3–5 ms, release ~15 ms. Captures transient bite.
   - Slow: attack ~10–15 ms, release set by the **Decay** knob (100–800 ms, default 300 ms). Gives musical sustain.
   - The `max()` topology is the trick behind "fast but not instant": the fast detector lets pluck transients through immediately, the slow detector extends them into a musical tail.
3. **Sensitivity curve** applied before output:
   - **Sensitivity** (0–24 dB): input trim into the follower.
   - **Range** (0–1): shapes the top of the curve between linear and exponentially compressed, giving a "touch responsive" knee.
4. Output clamped to `[0, 1]`, log-smoothed at ~2 ms to eliminate zipper noise.

**Exposed controls:** Sensitivity, Range, Decay.
**Hidden (fixed voicing):** fast/slow attack times, max() topology, smoothing constants.

### 3.3 Waveshaper Chain

Three user-facing controls; fixed topology:

1. **Drive** (0–30 dB pre-gain into the shaper).
2. **Morph** (0–1): linear crossfade between two shaper curves:
   - 0 → `tanh(x)` soft clip (asymmetric tube-ish; retains pick dynamics).
   - 1 → `sign(x)` hard square with a few-sample slew limiter to reduce aliasing.
3. **Sub Blend** (0–1): digital flip-flop divider toggling on each positive zero-crossing of the squared signal, producing a sub-octave square. Lowpassed at ~1 kHz before blending into the main path.

**Anti-aliasing:** hard-square and divider paths need oversampling. Use `juce::dsp::Oversampling` (4× minimum, 8× preferred) wrapped **around the waveshaper block only**, not plugin-wide.

### 3.4 Filter Stage

One concrete `SvfFilter` class; multiple instances depending on mode.

#### Envelope Mode

- Single SVF instance, LP or BP output tap (user-selectable).
- Cutoff mapping:
  ```
  cutoff_hz = baseCutoff * 2 ^ (envAmount * env(t))
  ```
  Exponential mapping so sweeps feel musical across the full range. `envAmount` is measured in octaves (−4 … +4) and is signed: positive sweeps up with the envelope, negative sweeps down.
- Resonance (Q) exposed as a shared parameter (0.5–12).

**User controls:** Base Cutoff, Env Amount, Filter Type (LP/BP), Resonance.

#### Formant Mode

- Two formant "banks" A and B. Each bank = **3 parallel bandpass SVFs** tuned to F1/F2/F3 for one vowel.
- Vowel palette: **AH, EH, IH, OH, OO** (5 vowels). F1/F2/F3 values from a static lookup table in `FormantTables.h`.
- Crossfade between bank A and bank B driven by `stretch_curve(env(t))`.
- `stretch_curve` is a user-selected shape from three options: **Exp, Linear, Log**. Not a continuous knob — discrete dropdown.
- Resonance (Q) shared with envelope mode; moderate values (2–6) keep vowels intelligible.

**User controls:** Vowel A, Vowel B, Stretch Curve, Resonance.

#### Coefficient Update Rate

Per-sample coefficient recomputation in both modes. SVF coefficient updates are cheap enough that control-rate interpolation isn't worth the code complexity.

### 3.5 Output Stage

- **Wet/Dry** mix (0–100%).
- **Output Level** trim (±12 dB).
- Final DC-block to catch asymmetry from the shaper.

## 4. Parameter Set

All parameters registered through `AudioProcessorValueTreeState` for automation, preset save/load, and thread-safe access.

| # | Parameter | Range | Default | Skew | Notes |
|---|---|---|---|---|---|
| **Input** | | | | | |
| 1 | Input Gain | −12 … +12 dB | 0 dB | linear | |
| **Envelope Follower** | | | | | |
| 2 | Sensitivity | 0 … 24 dB | 12 dB | linear | trim into follower |
| 3 | Range | 0 … 1 | 0.5 | linear | lin→exp curve knee |
| 4 | Decay | 100 … 800 ms | 300 ms | log (0.3) | slow-detector release |
| **Waveshaper** | | | | | |
| 5 | Drive | 0 … 30 dB | 6 dB | linear | pre-shaper gain |
| 6 | Morph | 0 … 1 | 0.6 | linear | soft ↔ square |
| 7 | Sub Blend | 0 … 1 | 0.3 | linear | flip-flop sub octave |
| **Filter (global)** | | | | | |
| 8 | Filter Mode | Envelope / Formant | Envelope | — | mode switch |
| 9 | Resonance | 0.5 … 12 | 3.0 | log (0.3) | shared across modes |
| **Envelope Mode** (visible when mode = Envelope) | | | | | |
| 10 | Base Cutoff | 80 … 4000 Hz | 250 Hz | log (0.3) | filter starting point |
| 11 | Env Amount | −4 … +4 octaves | +2.5 | linear | signed sweep depth |
| 12 | Filter Type | LP / BP | LP | — | SVF output tap |
| **Formant Mode** (visible when mode = Formant) | | | | | |
| 13 | Vowel A | AH / EH / IH / OH / OO | OO | — | starting vowel |
| 14 | Vowel B | AH / EH / IH / OH / OO | AH | — | target vowel |
| 15 | Stretch Curve | Exp / Lin / Log | Lin | — | morph shape |
| **Output** | | | | | |
| 16 | Wet/Dry | 0 … 100% | 100% | linear | |
| 17 | Output Level | −12 … +12 dB | 0 dB | linear | |

Total: 17 parameters; ~12 visible at a time (mode-specific params swap in/out).

## 5. UI Design

**Style:** pedal-inspired but not skeuomorphic. Dark background, single accent color (muted amber or pale green), no faux chassis screws, no simulated LEDs.

**Layout sketch (~600 × 360 px, resizable):**

```
┌─ THOM & GUY ─────────────────────────────────┐
│                                               │
│  INPUT       ENVELOPE FEEL      DRIVE SECTION │
│  ( Gain )    (Sens)(Range)(Decay) (Drv)(Morph)(Sub) │
│                                               │
│  ─────────────────────────────────────────── │
│                                               │
│  FILTER MODE:  [ ● Envelope    ○ Formant ]   │
│                                               │
│  (mode-specific knobs here — swap in/out)    │
│                                               │
│  ─────────────────────────────────────────── │
│                                               │
│  OUTPUT      (Wet/Dry)  (Level)              │
│                                               │
└───────────────────────────────────────────────┘
```

**Key elements:**

- **Envelope meter** in the header bar: real-time `env(t)` as a horizontal level bar. Makes the "touch responsive" feel visible, which is essential for dialing Sensitivity and Range.
- **Mode switch** visually prominent; its state determines which knob cluster appears below.
- Knobs are standard rotary; values shown as text on hover/drag.
- Resizable via `setResizable` + `ResizableCornerComponent`, with component-level scaling.

### Presets

Ship 4–6 factory presets covering:

- **HAA Rhythm** — Envelope mode, heavy drive, high morph, high sub, fast sweep.
- **Clean Funk** — Envelope mode, low drive, morph ≈ 0.3, BP output, classic auto-wah sweep.
- **Vowel Sweep** — Formant mode, OO → AH, Exp curve.
- **Robot Voice** — Formant mode, IH → OO, Log curve, high resonance.
- **Sub Synth** — Envelope mode, max sub-octave, LP, low cutoff.
- **Subtle Auto-Wah** — Envelope mode, low drive, moderate sweep, BP.

Presets stored as XML in a `Presets/` folder alongside the plugin, loaded via a header dropdown with save/load-user-preset support.

## 6. Project Structure

### Plugin Targets

- **Formats:** AU, VST3, Standalone.
- **Platform:** macOS universal binary (arm64 + x86_64). Windows not tested in v1.
- **Build system:** Projucer + Xcode via `build.sh`, matching the conventions of the sibling `PhysicalModelTest/` project.

### Repository Layout

```
thomAndGuy/
├── ThomAndGuy.jucer              # Projucer project file
├── build.sh                      # wrapper around Projucer + xcodebuild
├── .gitignore
├── README.md
├── docs/
│   └── superpowers/
│       └── specs/
│           └── 2026-04-20-synthwah-design.md
├── Source/
│   ├── PluginProcessor.cpp/.h    # AudioProcessor, APVTS, top-level signal flow
│   ├── PluginEditor.cpp/.h       # AudioProcessorEditor, UI
│   │
│   ├── dsp/
│   │   ├── InputConditioner.cpp/.h
│   │   ├── EnvelopeFollower.cpp/.h
│   │   ├── WaveshaperChain.cpp/.h
│   │   ├── SubOctaveDivider.cpp/.h
│   │   ├── Filter.h                # abstract interface
│   │   ├── SvfFilter.cpp/.h        # concrete SVF implementation
│   │   ├── FormantBank.cpp/.h      # 3-bandpass vowel bank
│   │   ├── FormantTables.h         # F1/F2/F3 lookup per vowel
│   │   └── ParamSmoothers.h        # helpers around juce::SmoothedValue
│   │
│   ├── ui/
│   │   ├── MainPanel.cpp/.h        # top-level editor layout
│   │   ├── KnobGroup.cpp/.h        # reusable labeled-knob cluster
│   │   ├── EnvelopeMeter.cpp/.h    # real-time env(t) visualizer
│   │   ├── ModeSwitch.cpp/.h       # envelope ↔ formant
│   │   ├── PresetBar.cpp/.h        # preset dropdown + save/load
│   │   └── LookAndFeel.cpp/.h      # plugin-wide styling
│   │
│   ├── params/
│   │   ├── ParameterIDs.h          # string constants for APVTS
│   │   └── ParameterLayout.cpp/.h  # APVTS parameter creation
│   │
│   └── presets/
│       └── Factory.cpp/.h          # factory preset loading
│
├── Presets/                        # XML preset files shipped with plugin
│
├── JuceLibraryCode/                # auto-generated by Projucer
└── Builds/                         # auto-generated Xcode project + output
```

### Module Boundaries

- **`dsp/`** — DSP blocks only. Each class has one responsibility, its own header/impl pair, can be instantiated and tested without any JUCE `AudioProcessor`. The `Filter` interface lives here.
- **`ui/`** — GUI only. Reads parameter values through `APVTS::Listener` / `SliderAttachment`; never touches DSP directly. `LookAndFeel` centralizes styling.
- **`params/`** — single source of truth for parameter IDs and APVTS layout. Prevents the classic JUCE bug where editor and processor disagree on a parameter ID string.
- **`presets/`** — preset loading/saving. Separate from `params/` because a preset is a parameter *snapshot*, not a parameter *definition*.

### Dependencies

Pure JUCE. Modules used: `juce_audio_basics`, `juce_audio_devices`, `juce_audio_processors`, `juce_audio_utils`, `juce_core`, `juce_data_structures`, `juce_dsp`, `juce_events`, `juce_graphics`, `juce_gui_basics`, `juce_gui_extra`. `juce_dsp` provides `Oversampling`, `StateVariableTPTFilter`, and `SmoothedValue`. No third-party audio libraries.

### Build Pipeline

`build.sh` follows the sibling-project pattern:

1. Resave `.jucer` via Projucer CLI → regenerates `JuceLibraryCode/` and the Xcode project.
2. Run `xcodebuild` with the appropriate scheme/configuration.
3. (Optional, flag-gated) Copy built AU/VST3 into system plugin folders for quick iteration.

## 7. Testing Strategy

### Layer 1: Unit Tests (offline, headless)

JUCE's `juce::UnitTest` runner, invoked from a small `tests/` target in the `.jucer`. DSP blocks tested in isolation.

**Coverage priorities:**

- `EnvelopeFollower`: synthetic burst (exponential-decay sine) — verify attack rise within window, decay fall within window, fast+slow `max()` behavior on a transient click.
- `WaveshaperChain`: sine input at known amplitude — at morph=0 verify soft-clip harmonic distribution, at morph=1 verify symmetric ±1 square.
- `SubOctaveDivider`: sine at 440 Hz — divider output fundamental at 220 Hz (zero-crossing rate check).
- `SvfFilter`: impulse response — LP output −3 dB at cutoff, −12 dB/oct rolloff; smoke-test against analytical response.
- `FormantBank`: per-vowel F1/F2/F3 centers match `FormantTables.h` within ±5%.

**Explicitly out of scope:**

- Subjective "touch responsiveness" tuning — that's a listening loop, not a test target.
- UI pixel layout.

### Layer 2: Golden-Render Regression Tests

`tests/golden/` contains 2–3 short input audio files (single-note pluck, chord, strum pattern) and expected rendered outputs for a known preset snapshot. Harness:

1. Instantiate processor offline.
2. Set parameters to the snapshot.
3. Feed input file, capture output.
4. Compare against golden with tolerance (−80 dB difference noise floor).

Golden files regenerated manually when a DSP change is intentional; the test fails loudly when it's not.

### Layer 3: Manual Listening Checklist

Tracked as a checklist in `docs/listening-checklist.md`; walked before tagging a release:

- Plug in a guitar DI, preset = "HAA Rhythm" — does it sound like *Human After All*?
- Soft → hard picks — does the envelope feel touch-responsive?
- Formant mode OO→AH — does the morph sound like a vowel sweep?
- Sweep Drive / Morph / Sub Blend — any discontinuities, pops, or broken zones?
- Automate Env Amount −4 → +4 — any zipper noise?
- Load in Logic and Ableton — plugin loads, saves, restores state correctly?

### CI

Deferred. Personal project; no collaboration or release cadence yet. Can be added later without changing the test structure.

### Verification Gates

For any implementation work that touches audio, completion requires:

- All unit tests passing.
- `build.sh` produces clean AU and VST3 with no warnings.
- Plugin loads in Logic and Ableton without crashing or glitching on parameter changes.
- Manual listening checklist completed for the affected feature.

## 8. Open Items

- **Primary test DAW** not yet decided (Logic vs. Ableton vs. Reaper). Default assumption for the checklist: both Logic and Ableton. Revisit once the user confirms.
- **Accent color** for the UI (muted amber vs. pale green) is a visual preference to be finalized during implementation of the `LookAndFeel`.
- **Plugin display name / manufacturer string / plugin code** for the `.jucer` file need to be chosen. Suggested: display name "Thom & Guy," manufacturer "FGSake" (or user's preferred), 4-char plugin code TBD.
