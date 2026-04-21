# Thom & Guy — Manual Listening Checklist

Walk this list before tagging a release or claiming significant DSP work "done."

## Signal quality

- [ ] Plug in a guitar DI recording. Select preset **HAA Rhythm**.
      Does it sound like *Human After All*? (Squared-off, sub-present, envelope-swept.)
- [ ] Switch to **Clean Funk**. Classic BP auto-wah sweep, not overdriven.
- [ ] Switch to **Sub Synth**. Very strong sub-octave, slow sweep.

## Envelope feel

- [ ] Select preset **HAA Rhythm**. Play soft → medium → hard picks.
      Envelope response is perceptibly touch-sensitive: soft picks produce a small sweep,
      hard picks produce a large sweep.
- [ ] Rotate **Sensitivity** from 0 → 24 dB while strumming evenly.
      Dynamic response should smoothly scale.
- [ ] Rotate **Range** from 0 → 1.
      At 0, envelope feels linear. At 1, the top of the envelope compresses into a "knee."
- [ ] Rotate **Attack** from 1 ms → 30 ms.
      Short attack: instant open. Long attack: sweep lags noticeably behind the pick.
- [ ] Rotate **Decay** from 100 ms → 800 ms.
      Short decay: sweep closes fast after pick. Long decay: sustains the swept tone.

## Formant mode

- [ ] Switch to Formant mode. Select Vowel A = OO, Vowel B = AH, Depth = 1.
      Pick softly → hard. Output should morph from "oo" → "ah" with the envelope.
- [ ] Set Stretch Curve to **Exp**. Morph should feel slow at low levels, fast at top.
- [ ] Set Stretch Curve to **Log**. Opposite feel: front-loaded morph.
- [ ] Try all five vowel pairs (AH/EH/IH/OH/OO). Each vowel should be distinct.

## Waveshaper behavior

- [ ] Rotate **Drive** from 0 → 30 dB. No clicks, pops, or aliasing artifacts.
- [ ] Rotate **Morph** slowly from 0 → 1 with a sustained note. Smooth crossfade from
      tanh to hard square with no clicks.
- [ ] Rotate **Sub Blend** from 0 → 1. Sub-octave should appear cleanly without the
      main signal dropping out.

## Parameter smoothness

- [ ] Automate **Env Amount** from −4 → +4 octaves. No zipper noise, no discontinuities.
- [ ] Automate **Base Cutoff** from 80 → 4000 Hz. Smooth sweep.
- [ ] Automate **Resonance** from 0.5 → 12. No ringing spikes; filter stays stable.

## Host integration

- [ ] Load **Thom & Guy (VST3)** in Reason. Plugin loads without errors.
- [ ] Save the Reason project. Close the project. Reopen.
      All parameter values restore correctly.
- [ ] Bypass the plugin. Dry signal passes through unchanged.
- [ ] Enable the plugin. Wet/Dry at 100% — output is fully processed.

## Visual QA

- [ ] Envelope meter (yellow LED) tracks input level in real-time.
- [ ] Mode switch visibly highlights the selected mode in magenta.
- [ ] All knob values display correctly with ABC Rom Mono font.
- [ ] Resize window — layout scales cleanly with no overlap or clipping.
