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

### Pedal Mode Coverage (reference)

The Digitech Synth Wah has seven modes selected via its Type knob. This plugin covers four of them, subsumed into two modes:

| Pedal mode | Plugin equivalent |
|---|---|
| 1. Env Up | Envelope Mode + positive Env Amount |
| 2. Env Down | Envelope Mode + negative Env Amount |
| 5. Filter 1 (Synth Talk) | Formant Mode, vowel A → vowel B |
| 6. Filter 2 (Synth Talk, inverse) | Formant Mode, vowel B → vowel A (swap the two vowels) |

The two Envelope directions collapse into one mode with a signed Env Amount knob, and the two Filter directions collapse into one mode with user-selected vowel endpoints. The plugin is more flexible than the pedal in both cases.

### Explicit Non-Goals

- No polyphony. The plugin is monophonic end-to-end.
- No MIDI input. The envelope follower is the only analysis block.
- No pitch tracking. Because there is no oscillator, no pitch detection is needed.
- No circuit-level analog modeling (no component-level emulation of the Digitech pedal).
- No AAX/Pro Tools support in v1. AU, VST3, Standalone only.
- No Windows testing in v1. macOS universal binary only.

### Deferred Pedal Modes (not in v1)

Documented so future work has a clean starting point:

- **Synth 1 / Synth 2** (pedal modes 3, 4) — monophonic synth tone generator with opening/closing filter envelope. These modes require an internal oscillator and a pitch tracker, both of which we deliberately omitted to keep v1 scoped. Can be added as a new `Filter Mode` option later without disturbing the envelope-mode or formant-mode paths.
- **Auto Wah** (pedal mode 7) — LFO-driven wah sweep, decoupled from input envelope. Out of scope because the plugin's identity is "touch response." If added later, it would live as a third `Filter Mode` with an LFO replacing the envelope follower as the cutoff driver.

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
   - Fast: attack set by the **Attack** knob (1–30 ms, default 4 ms); release ~15 ms. Captures transient bite.
   - Slow: attack ~10–15 ms; release set by the **Decay** knob (100–800 ms, default 300 ms). Gives musical sustain.
   - The `max()` topology is the trick behind "fast but not instant": the fast detector lets pluck transients through immediately, the slow detector extends them into a musical tail.
   - The Attack knob matches the semantics of the pedal's "Synth Attack" / "Filter Attack" control — how quickly the filter opens under a pick.
3. **Sensitivity curve** applied before output:
   - **Sensitivity** (0–24 dB): input trim into the follower. Analogous to the pedal's "Trigger Sensitivity."
   - **Range** (0–1): shapes the top of the curve between linear and exponentially compressed, giving a "touch responsive" knee.
4. Output clamped to `[0, 1]`, log-smoothed at ~2 ms to eliminate zipper noise.

**Exposed controls:** Sensitivity, Attack, Decay, Range.
**Hidden (fixed voicing):** fast detector's release, slow detector's attack, max() topology, smoothing constants.

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
- Crossfade between bank A and bank B is:
  ```
  morph = formantDepth * stretch_curve(env(t))
  ```
  where `formantDepth` (0–1) is the user-facing **Depth** knob — analogous to the pedal's "Freq Envelope" control. Depth = 1 sweeps fully from vowel A to vowel B; Depth = 0.3 only partially morphs toward B.
- `stretch_curve` is a user-selected shape from three options: **Exp, Linear, Log**. Not a continuous knob — discrete dropdown.
- Resonance (Q) shared with envelope mode; moderate values (2–6) keep vowels intelligible.

**User controls:** Vowel A, Vowel B, Stretch Curve, Depth, Resonance.

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
| 2 | Sensitivity | 0 … 24 dB | 12 dB | linear | trim into follower ("Trigger Sensitivity") |
| 3 | Attack | 1 … 30 ms | 4 ms | log (0.3) | fast-detector attack |
| 4 | Range | 0 … 1 | 0.5 | linear | lin→exp curve knee |
| 5 | Decay | 100 … 800 ms | 300 ms | log (0.3) | slow-detector release |
| **Waveshaper** | | | | | |
| 6 | Drive | 0 … 30 dB | 6 dB | linear | pre-shaper gain |
| 7 | Morph | 0 … 1 | 0.6 | linear | soft ↔ square |
| 8 | Sub Blend | 0 … 1 | 0.3 | linear | flip-flop sub octave |
| **Filter (global)** | | | | | |
| 9 | Filter Mode | Envelope / Formant | Envelope | — | mode switch |
| 10 | Resonance | 0.5 … 12 | 3.0 | log (0.3) | shared across modes |
| **Envelope Mode** (visible when mode = Envelope) | | | | | |
| 11 | Base Cutoff | 80 … 4000 Hz | 250 Hz | log (0.3) | filter starting point |
| 12 | Env Amount | −4 … +4 octaves | +2.5 | linear | signed sweep depth |
| 13 | Filter Type | LP / BP | LP | — | SVF output tap |
| **Formant Mode** (visible when mode = Formant) | | | | | |
| 14 | Vowel A | AH / EH / IH / OH / OO | OO | — | starting vowel |
| 15 | Vowel B | AH / EH / IH / OH / OO | AH | — | target vowel |
| 16 | Stretch Curve | Exp / Lin / Log | Lin | — | morph shape |
| 17 | Formant Depth | 0 … 1 | 1.0 | linear | morph amount (pedal's "Freq Envelope") |
| **Output** | | | | | |
| 18 | Wet/Dry | 0 … 100% | 100% | linear | |
| 19 | Output Level | −12 … +12 dB | 0 dB | linear | |

Total: 19 parameters; ~14 visible at a time (mode-specific params swap in/out).

## 5. UI Design

The plugin UI inherits the **AmpersandAmpersand (`&&`) design system** defined in `../PhysicalModelTest/DESIGN.md`. That system is the authoritative visual reference; this section documents only the plugin-specific applications of it.

### Design System Inheritance

- **Port `AmpersandLookAndFeel.cpp/.h`** from `../PhysicalModelTest/Source/` as the starting point for this plugin's JUCE `LookAndFeel`. Keep it in `Source/ui/LookAndFeel.cpp/.h`. Extend only if the plugin needs components the sibling didn't.
- **Reuse the ABC Rom and ABC Rom Mono font families** from `../PhysicalModelTest/ABC ROM/` and `.../ABC ROM Mono/`. Bundle the required weights (Regular, Medium, Bold, Mono Regular, Mono Bold) as binary resources in the `.jucer`.
- Follow the system's color tokens, spacing scale (4px base grid), 0px border-radius rule, and typography pairing (ABC Rom for labels, ABC Rom Mono for numerical values).

### Semantic Mapping (plugin-specific)

| UI Element | Design System Role | Color / Treatment |
|---|---|---|
| Page background | Ground | `#000000` |
| Header bar | Panel | `#1A1A1A` |
| Knob group panels | Card | `#333333`, 1px border `#4D4D4D` |
| Inactive knob / control | Node resting | `#666666` |
| Knob hover / focus | Interactive | `#0093D3` (cyan) |
| Mode switch selected state | Selected | `#CC016B` (magenta) |
| **Envelope meter** | **Live signal** | `#FFF10D` (yellow) + `0 0 6px #FFF10D` LED glow + 1200 ms pulse when input is present |
| Parameter value readouts | Mono label | ABC Rom Mono 12px weight 400, `#FFFEFE` |
| Knob labels | UI label | ABC Rom 12px weight 400 uppercase, `#666666`, 0.20px tracking |
| Plugin wordmark | Display | ABC Rom Mono 20px weight 700, `#FFFEFE` |
| Section dividers | Border subtle | 1px solid `#1A1A1A` |

The envelope meter's yellow LED treatment is the centerpiece of the UI — it's the literal `env(t)` signal, so using yellow is semantically correct per the design system's "yellow = live signal, yellow glow = functional shadow" rule. This is the one place where the design system's rarely-used LED shadow is warranted.

### Layout Sketch (~600 × 360 px, resizable)

```
┌─────────────────────────────────────────────────────┐
│  && THOM & GUY    [envelope meter ▓▓▓▓▓▓░░░░]       │  header: #1A1A1A
│                                                     │
│  ┌──INPUT──┐ ┌──────ENVELOPE FEEL──────┐ ┌──DRIVE──┐ │  cards: #333
│  │ (Gain)  │ │ (Sens) (Atk) (Range)(Dec)│ │ (Drv)   │ │
│  └─────────┘ └──────────────────────────┘ │ (Morph) │ │
│                                            │ (Sub)   │ │
│                                            └─────────┘ │
│  ─────────────────────────────────────────────────  │
│                                                     │
│  FILTER MODE:  [ ● ENVELOPE ]  [ ○ FORMANT ]       │  mode switch
│                                                     │
│  ┌──────── mode-specific knob cluster ────────┐   │
│  │ (Base Cutoff) (Env Amount) (Reso) [LP/BP] │   │
│  └──────────────────────────────────────────────┘   │
│                                                     │
│  ─────────────────────────────────────────────────  │
│                                                     │
│  OUTPUT:  (Wet/Dry)  (Level)                        │
│                                                     │
└─────────────────────────────────────────────────────┘
```

### Key Elements

- **Envelope meter** in the header bar: a horizontal bar showing real-time `env(t)`, rendered in `#FFF10D` with the LED glow. This makes the "touch responsive" feel visible and is essential for dialing Sensitivity and Range by eye.
- **Mode switch** uses the design system's selected-state magenta — the selected mode's label is white on `#CC016B`; the unselected mode is `#666` on transparent. Its state reshapes the knob cluster below.
- **Knobs** use the system's specialized knob treatment: 2px track `#333333`, 16×16px square thumb (`#0093D3` default, `#CC016B` when dragged), value readout in ABC Rom Mono 12px right-aligned. Value turns `#FFF10D` when the parameter is actively being modulated by the envelope (e.g., the Base Cutoff readout pulses yellow during an envelope sweep).
- **Resizable** via `setResizable` + `ResizableCornerComponent`. Use JUCE's component-scale mechanism so the 4px grid holds at any size.
- **No shadows anywhere** except the yellow LED glow on the envelope meter. No rounded corners anywhere.

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

- **Formats:** AU, VST3, Standalone. **VST3 is the primary target** because Reason (the primary test DAW) accepts VST3 only.
- **Platform:** macOS universal binary (arm64 + x86_64). Windows not tested in v1.
- **Build system:** Projucer + Xcode via `build.sh`, matching the conventions of the sibling `PhysicalModelTest/` project.

### `.jucer` Configuration (key flags)

This plugin is an **audio effect**, not a synth instrument. Configure the `.jucer` accordingly — the sibling `CelloModel.jucer` uses synth-oriented flags that **must not** be copied verbatim:

| Field | Value | Note |
|---|---|---|
| `name` / project name | `ThomAndGuy` | file: `ThomAndGuy.jucer` |
| `pluginName` | `Thom & Guy` | user-facing name in host plugin lists |
| `companyName` | `AmpersandAmpersand` | manufacturer string |
| `pluginManufacturerCode` | `Ampr` | 4-char manufacturer ID for AU |
| `pluginCode` | `ThGy` | 4-char plugin ID, unique per plugin |
| `projectType` | `audioplug` | — |
| `pluginIsSynth` | `0` | effect, not instrument |
| `pluginWantsMidiIn` | `0` | no MIDI |
| `pluginProducesMidiOut` | `0` | — |
| `pluginAUMainType` | `'aufx'` | effect unit |
| `pluginVST3Category` | `Fx\|Filter` | filter-family effect |
| `pluginCharacteristicsValue` | *(unset)* | no `pluginIsSynth` characteristic |

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
│   │   ├── EnvelopeMeter.cpp/.h    # real-time env(t) visualizer (yellow LED)
│   │   ├── ModeSwitch.cpp/.h       # envelope ↔ formant
│   │   ├── PresetBar.cpp/.h        # preset dropdown + save/load
│   │   └── LookAndFeel.cpp/.h      # ported from ../PhysicalModelTest/Source/AmpersandLookAndFeel
│   │
│   ├── fonts/                      # ABC Rom + ABC Rom Mono (from sibling project)
│   │   ├── ABCRom-Regular.otf
│   │   ├── ABCRom-Medium.otf
│   │   ├── ABCRom-Bold.otf
│   │   ├── ABCRomMono-Regular.otf
│   │   └── ABCRomMono-Bold.otf
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
- Load in Reason (VST3) — plugin loads, saves, restores state correctly?

### CI

Deferred. Personal project; no collaboration or release cadence yet. Can be added later without changing the test structure.

### Verification Gates

For any implementation work that touches audio, completion requires:

- All unit tests passing.
- `build.sh` produces clean AU and VST3 with no warnings.
- Plugin loads in Reason (VST3) without crashing or glitching on parameter changes.
- Manual listening checklist completed for the affected feature.

## 8. Resolved Decisions

- **Primary test DAW:** Reason (VST3 only). No secondary DAW compatibility target for v1.
- **UI style:** Inherit the AmpersandAmpersand (`&&`) design system from `../PhysicalModelTest/DESIGN.md`.
- **Plugin identity:**
  - Plugin display name: **Thom & Guy**
  - Manufacturer / company name: **AmpersandAmpersand**
  - Manufacturer 4-char code: **Ampr**
  - Plugin 4-char code: **ThGy**
