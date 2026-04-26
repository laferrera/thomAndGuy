# DigiTech Synth Wah Hardware Capture — Design

**Date:** 2026-04-25
**Status:** Design approved; pending implementation plan.
**Related:** `2026-04-20-synthwah-design.md` (the plugin this work calibrates).

## 1. Overview

Thom & Guy is a parametric recreation of the DigiTech X-Series Synth Wah. The plugin's DSP topology — input conditioner → envelope follower → waveshaper chain → SVF or formant bank → output — is already implemented. **What's missing is calibration**: the envelope follower's attack/decay constants, the waveshaper's morph curve, the filter's base cutoff and Env-Amount-in-octaves, the formant bank's F1/F2/F3 vowel centers, and the Q values were all hand-picked. This project captures the real pedal under a controlled stimulus battery so those parameters can be fit from measurement instead of taste.

The capture must be **one-and-done**: the pedal will not be available long-term. We record once, archive the WAV pairs, and do all parameter fitting downstream from disk.

### Goals

1. **Calibration data** for thomAndGuy's existing parametric chain — not a blackbox neural model.
2. **One sitting** of focused recording, ~2 hours, producing a complete archive that can be re-fit later if the topology changes.
3. **Sample-accurate alignment** between stimulus and captured response so envelope time constants and filter impulse responses are recoverable.
4. **Reusability**: the capture toolchain should work for any future analog pedal, not just the Synth Wah.

### Non-goals

1. No bit-exact circuit modeling. We're fitting the pedal to thomAndGuy's parametric topology, not its schematic.
2. No coverage of pedal modes 3 (Synth 1), 4 (Synth 2), 7 (AutoWah). These are deferred from the plugin spec; capturing them adds time without a downstream consumer.
3. No real-time analysis or feedback. Capture is dumb — fit-and-validate happens offline.
4. No DAW automation. The capture script drives Core Audio directly.

### Scope of pedal modes

The pedal has 7 TYPE positions. The plugin covers 4 of them, collapsed into 2 plugin modes:

| Pedal TYPE | Plugin equivalent |
|---|---|
| 1 — Env Up | Envelope mode + positive Env Amount |
| 2 — Env Down | Envelope mode + negative Env Amount |
| 5 — Filter 1 (Synth Talk) | Formant mode, vowel A → vowel B |
| 6 — Filter 2 (Synth Talk inverse) | Formant mode, vowel B → vowel A (swap) |

Modes 2 and 6 are **derivable** from modes 1 and 5 (sign flip, vowel swap). Therefore we **physically capture only modes 1 and 5**. The fits transfer to 2 and 6 without additional measurement.

## 2. Hardware Signal Flow

```
                                            OUT1 (DRY)
   ┌──────────────────┐                      ╲
   │ Focusrite        │ DAC ch1 → [Reamp] → [Pedal IN] → [Pedal] ─→  ADC ch1
   │ (USB to Mac)     │                                              ╲
   │                  │                                       OUT2 (EFFECT)
   │                  │                                                ╲
   │                  │ ←─────────────── ADC ch2 ←───────────────────────
   └──────────────────┘
            │
            │  Core Audio (sounddevice.playrec)
            ▼
       capture.py
```

Three signals are captured per pass:

- **`stimulus.wav`** — the file we *intended* to play (golden, copied from disk)
- **`dryref.wav`** ← ADC ch1, pedal OUT1 — used for sample-accurate alignment via cross-correlation against stimulus, and to verify the reamp+pedal-bypass path is clean
- **`wet.wav`** ← ADC ch2, pedal OUT2 — the modeling target

Recording both pedal outputs separates **analog round-trip latency** (stimulus ↔ dryref) from **internal pedal latency** (dryref ↔ wet). Without this, any wet-vs-stimulus offset is ambiguous between the two.

The pedal is **engaged** (footswitch on) for all 26 captures. Bypass behavior is verified separately during pre-flight calibration (Section 5).

### Open empirical questions

1. **Is the X-Series Synth Wah true-bypass?** Determined by `pedal_bypass` calibration step.
2. **Does OUT1 carry true dry, or buffered/ASIC-output dry?** Determined by null-testing OUT1 against stimulus during the `pedal_bypass` step.

Neither blocks the design; both are read out from the pre-flight nulls and recorded in `calibration.json`.

## 3. Stimulus Design — the Training WAV

One file, 96 kHz / 24-bit / mono, ~4:35 long, played per knob configuration. Segment order: easy-to-fit first, quiet before loud, so partial captures still yield useful data and a misconfigured gain stage clips on the loudest segment last (giving the operator time to abort).

```
[0:00 – 0:05]  SYNC HEADER
                • 1-sample digital impulse @ -6 dBFS
                • 0.5 s log chirp 100 Hz → 10 kHz @ -12 dBFS
                • 4 s silence
                → cross-correlation locks sample-accurate alignment + RT latency

[0:05 – 0:35]  SILENCE (30 s)
                → noise floor; reveals pedal hum/hiss & filter rest state

[0:35 – 1:35]  ENVELOPE-FOLLOWER PROBES (60 s)
                • 12 level-step bursts: 200 ms tone @ 220 Hz, decaying gates
                  cycling -40, -30, -20, -12, -6, -3 dBFS and back
                  → fits attack & decay τ of parallel fast+slow detectors
                • 6 amplitude ramps: 220 Hz tone, 0 → peak → 0 over 4 s,
                  varying peaks → fits the SENS curve shape

[1:35 – 2:35]  WAVESHAPER PROBES (60 s)
                • Stepped 220 Hz sine: -40, -30, -20, -12, -6, -3, 0 dBFS,
                  3 s each, 0.5 s silence between
                  → harmonic amplitudes per level
                • Two-tone (220 Hz + 330 Hz) at -20, -12, -6 dBFS, 3 s each
                  → IMD products
                Note: usable for waveshaper fit only at the lowest input
                levels (-40, -30 dBFS) where the envelope sits below
                threshold across most knob settings; at higher levels the
                segment captures shaper × time-varying filter.

[2:35 – 3:35]  FILTER PROBES (60 s) — held envelope, then sweep
                For each of 5 input levels (-30, -20, -12, -6, -3 dBFS):
                • 1.5 s steady tone @ 220 Hz → lets envelope settle
                • 2 s log sine sweep 50 Hz → 10 kHz at the same level
                • 0.5 s silence
                → fits filter freq response at envelope = constant;
                  → recovers base cutoff, Env-Amount-octaves, Q

[3:35 – 4:05]  NOISE PROBES (30 s)
                • 15 s pink noise @ -20 dBFS
                • 15 s white noise @ -20 dBFS
                → broadband filter shape, vowel formants in mode 5

[4:05 – 4:35]  IMPULSE TRAIN (30 s)
                • 30 single-sample clicks @ -6 dBFS, 1 s apart
                → sharp transient; reveals envelope attack edge clearly
```

Total stimulus length: 4 minutes 35 seconds.

### Mapping segments to the thomAndGuy DSP blocks

| Segment | Fits which block | Required? |
|---|---|---|
| Sync header | (alignment infrastructure) | yes |
| Silence | noise floor, hum baseline | yes |
| Level steps + ramps | `EnvelopeFollower` τ_fast, τ_slow, sensitivity curve | yes |
| Stepped sines + two-tone | `WaveshaperChain` morph curve, sub-octave threshold | yes (low-amplitude regions only) |
| Held tone + sweep | `SvfFilter` cutoff(env), Q | yes |
| Pink/white noise | `FormantBank` F1/F2/F3 (mode 5) | yes for mode 5 |
| Impulse train | sanity check on attack edge | yes (cheap insurance) |

### Decisions explicitly excluded

- **No guitar DI take.** Listening A/B can be done after the fact by recording bass/guitar separately through both pedal and plugin, outside this capture battery.
- **No protractor / printed knob template.** L/M/H positions dialed by eye are sufficient; the fits are robust to ±5° wobble.
- **No repeat takes for averaging.** Core Audio sample-accurate playrec is quiet enough that one take per config suffices.

## 4. Knob Grid — 26 Captures Total

Per mode: **13 configurations** sweeping `(SENS, CONTROL, RANGE)`. Knob positions are discrete labels — **L** ≈ 9 o'clock, **M** ≈ noon, **H** ≈ 3 o'clock. TYPE is fixed per mode session (TYPE=1 for Env Up; TYPE=5 for Filter 1).

```
  #   SENS  CTRL  RNG    purpose
  ──  ────  ────  ───    ───────────────────────────────────
   1   M    M    M       baseline
   2   L    M    M       SENS sweep low
   3   H    M    M       SENS sweep high
   4   M    L    M       CONTROL sweep low
   5   M    H    M       CONTROL sweep high
   6   M    M    L       RANGE sweep low
   7   M    M    H       RANGE sweep high
   8   L    L    L       all-low corner
   9   H    H    H       all-high corner
  10   L    H    H       interaction corner
  11   H    L    H       interaction corner
  12   H    H    L       interaction corner
  13   L    L    H       interaction corner
```

Configs 1–7 are the **load-bearing** fit data (one knob varied at a time over a held baseline isolates each knob's effect). Configs 8–13 detect **cross-knob coupling**; if the per-knob fits assume independence and configs 8–13 disagree, the model needs a cross-term.

Same grid for both modes → **26 captures total**.

### Time budget

26 × ~5:30 wall-clock per pass (4:35 stimulus + ~30 s dial-time + ~30 s settle/sanity) ≈ **2 hours, 20 minutes** of focused recording. Splittable into two ~70-minute sessions.

## 5. Pre-flight Calibration

A one-time ~5-minute pass before the 26 captures begin. Each step writes a WAV plus a row in `calibration.json`. The fit pipeline subtracts these from later captures to remove instrument-chain coloration.

```
calibration/
├── 01_loopback.wav            DAC out1 → ADC in1+in2 directly (no pedal, no reamp)
├── 02_reamp.wav               DAC out1 → reamp → ADC in1 (no pedal)
├── 03_pedal_bypass.wav        DAC out1 → reamp → pedal (BYPASSED) → ADC in1+in2
├── 04_pedal_engaged_silent.wav same path, footswitch ON, silence in
└── calibration.json
```

What each step measures:

| Step | Reveals |
|---|---|
| 01 loopback | Round-trip latency floor (DAC + ADC + USB), interface freq response baseline |
| 02 reamp | Reamp's added latency and any non-flat response |
| 03 pedal bypass | True-bypass vs buffered; bypassed-path coloration if buffered |
| 04 pedal silent (engaged) | Pedal noise floor, hum (60 Hz, 120 Hz), self-oscillation |

`calibration.json` shape:

```json
{
  "samplerate": 96000,
  "rt_latency_samples_loopback": 87,
  "rt_latency_samples_with_pedal": 91,
  "internal_pedal_latency_samples": 4,
  "dac_to_adc_gain_db": -0.12,
  "noise_floor_dbfs": -94.3,
  "hum_60hz_dbfs": -68.1,
  "pedal_is_true_bypass": false,
  "captured_at": "2026-04-25T17:55:00Z"
}
```

If, mid-session, a per-config sync correlation produces a latency more than ±32 samples off `rt_latency_samples_with_pedal`, `capture.py` aborts with a recalibration prompt rather than silently miscalign that pass.

## 6. `capture.py` Architecture

### Location

`thomAndGuy/scripts/capture/` (new directory). Mirrors warmAndFuzzy's `scripts/` layout convention. Self-contained — no cross-repo imports.

### Module layout

```
thomAndGuy/scripts/capture/
├── README.md
├── capture.py                  CLI entry point
├── stimulus.py                 builds the training WAV (Section 3)
├── alignment.py                sync chirp xcorr → sample offset
├── calibration.py              pre-flight pass, writes calibration.json
├── interface.py                thin sounddevice wrapper (playrec, device pick)
├── manifest.py                 reads/appends manifest.csv
├── config.py                   the 13-config grid as a Python list
├── tests/
│   ├── test_alignment.py
│   ├── test_stimulus.py
│   └── test_manifest.py
└── requirements.txt            sounddevice, numpy, scipy, soundfile, pytest
```

### CLI

```bash
# One-time setup
python capture.py build-stimulus --out captures/_stimulus.wav
python capture.py calibrate

# Per session
python capture.py record --mode 1                # walks the 13-config grid
python capture.py record --mode 5

# Recovery
python capture.py record --mode 1 --config 7     # redo a single config
```

### Per-config flow inside `record`

1. Print knob instructions for config N (mode, SENS/CTRL/RNG positions, TYPE).
2. Wait for `[Enter]`.
3. `sounddevice.playrec` — out=ch1, in=ch1+ch2, blocking, sample-accurate via PortAudio.
4. Cross-correlate sync chirp in `dryref` against the stimulus' chirp → sample offset.
5. Trim leading silence so all three WAVs (`_input`, `_dryref`, `_wet`) align at sample 0.
6. Sanity checks (each is a hard-fail if violated):
   - Peak ≤ -1 dBFS on both ADC channels (no clipping).
   - Dryref-vs-stimulus correlation > 0.95 inside the flat passband.
   - Wet RMS > `noise_floor_dbfs + 12 dB` (pedal is engaged and audibly active).
   - Per-pass latency within ±32 samples of `calibration.json`.
7. Write the three WAVs and append a row to `manifest.csv`.
8. Print `OK ✓` with brief stats; loop to next config.

### Failure messages — operator-actionable, not stack traces

```
m1_sm_cm_rm: ADC ch2 peak -0.3 dBFS (clip risk).
  → Reduce reamp output by ~6 dB and rerun this config.

m1_sl_cm_rm: sync correlation 0.32 (threshold 0.95).
  → Sync chirp not detected — check ch1 cabling.

m1_sl_cl_rl: wet RMS -89 dBFS, only 5 dB above noise floor.
  → Pedal may be bypassed or SENS is off the grid.

m5_sm_cm_rm: round-trip latency drifted to 168 samples (was 91).
  → Recalibrate before continuing.
```

### Output layout per pass

```
captures/
  m1_sm_cm_rm_input.wav        stimulus echo (1 ch, identical content per pass)
  m1_sm_cm_rm_dryref.wav       pedal OUT1 (1 ch)
  m1_sm_cm_rm_wet.wav          pedal OUT2 (1 ch)
  ...
  manifest.csv                 one row per capture
  calibration/                 pre-flight artifacts (Section 5)
```

`manifest.csv` schema:

```csv
mode,sens,control,range,prefix,timestamp,rt_latency_samples
1,M,M,M,m1_sm_cm_rm,2026-04-25T18:00:00Z,123
1,L,M,M,m1_sl_cm_rm,2026-04-25T18:06:00Z,123
...
5,H,H,L,m5_sh_ch_rl,2026-04-25T19:32:00Z,123
```

### Resolved engineering choices

| Choice | Value | Reason |
|---|---|---|
| Sample rate | 96 kHz | matches BrutProbe, headroom for filter analysis |
| Bit depth | 24-bit | adequate dynamic range without disk-bloat of 32f |
| Channels | mono | pedal is mono; stereo WAVs would waste disk |
| API | `sounddevice.playrec` (blocking) | simple, sample-accurate, in-memory |
| Alignment | `scipy.signal.correlate` (FFT) | sharp peak from chirp |
| GUI | none | terminal-only is right for a single-operator rig |

## 7. Testing Strategy

Pure-Python tests, no JUCE involvement, run via `pytest scripts/capture/tests/`. Three layers:

1. **Synthetic alignment test** — generate a stimulus, prepend a known offset of silence, assert `alignment.locate(stimulus, recorded)` recovers the offset to ±1 sample. Includes a noisy variant (additive white noise at SNR=20 dB) to assert cross-correlation is robust.
2. **Stimulus shape test** — `stimulus.build()` returns a numpy array; assert per-segment FFT energy lands in the expected freq/time bins, total length matches, sync chirp is at sample 0.
3. **Manifest round-trip** — write 26 rows, read them back, assert byte-equal.

End-to-end correctness is verified manually during the calibration step itself: if `01_loopback.wav` doesn't null against the stimulus to better than -60 dB, the rig is broken and the data is untrustworthy. This is a precondition gate, not a CI test.

## 8. Out of Scope (Explicit)

Documented to keep the spec focused; captured here so future work has a clean starting point.

- **Fitting pipeline.** This spec covers capture only. Per-block fits (envelope τ, waveshaper polynomial, filter freq response, vowel formants) are downstream work, fed by the WAV pairs and `manifest.csv` produced here.
- **Pedal modes 3, 4, 7** (Synth 1, Synth 2, AutoWah). Deferred from the plugin spec.
- **Modes 2 (Env Down) and 6 (Filter 2).** Derived from modes 1 and 5 by sign-flip / vowel-swap; no separate captures.
- **Real-instrument recordings.** Optional listening-test material; can be recorded any time outside this battery.
- **CI.** Personal project; no collaboration cadence yet.
- **Windows / Linux capture.** macOS Core Audio only.

## 9. Resolved Decisions

- **Approach:** Python `capture.py` driving Core Audio via `sounddevice` (not Reason External FX, not a neural profiler).
- **Modes captured:** 1 (Env Up) and 5 (Filter 1) only.
- **Knob grid:** 13 configs per mode, calibration-grade (`L/M/H` positions, dialed by eye).
- **Stimulus:** synthetic only (no guitar DI takes).
- **Sample rate / bit depth:** 96 kHz / 24-bit mono.
- **Capture location:** `thomAndGuy/scripts/capture/` (self-contained, no cross-repo dependencies).
