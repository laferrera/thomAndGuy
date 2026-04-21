# Thom & Guy

A JUCE audio-effect plugin inspired by the Digitech Synth Wah pedal, aimed at the Daft Punk *Human After All* guitar sound.

See `docs/superpowers/specs/2026-04-20-synthwah-design.md` for the design spec.

## Build

Requires JUCE at `~/JUCE/` (Projucer.app and modules).

```bash
./build.sh           # debug build, opens Standalone
./build.sh Release   # release build
./build.sh test      # build + run unit tests
```

## Formats

Builds AU, VST3, and Standalone. VST3 is the primary target (Reason compatibility).
