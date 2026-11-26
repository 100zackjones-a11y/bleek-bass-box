# BLEEK BASS BOX

A generative jungle / drum & bass bassline instrument by **BLEEK STATE AUDIO**, available as a VST3 / AU plugin (macOS and Windows) and as a [web version](https://100zackjones-a11y.github.io/bleek-bass-box/).

It writes basslines in the style of a chosen era (1994–2004) and archetype, shapes them around a drum pattern, plays them through a built-in mono sampler in sync with your project, and lets you drag the MIDI straight into Ableton.

## Features

- **Eras:** '94–95 Foundation / Ragga Jungle, '96–97 Dark Jungle / Early Techstep, '98–99 Techstep / Early Neurofunk, '00–01 Minimal Dark Rollers, '02–04 Liquid Funk & Jump-Up
- **12 archetypes:** Roots Jungle Sub, 808-Style Sub, Dub, Reese, Techstep, Neurofunk, Rolling, Minimal Dark Roller, Liquid Funk, Jump-Up, Stabbing, Pedal-Note Pulse
- **Drum awareness:** shapes the line around a Two-Step, Amen, Think or Rolling 16th break
- **Controls:** density, syncopation, phrase variation, note length, chromaticism, octave movement, swing, root note (C1–B2), phrase length (1–8 bars)
- **Locks:** regenerate while keeping the rhythm, pitch, phrase or groove fixed
- **Sound:** mono sampler with ADSR. Load your own sample or use the default sine sub.
- **MIDI:** live MIDI out, and drag-and-drop `.mid` export into your DAW

## Download

Get the latest build from the [**Releases**](https://github.com/100zackjones-a11y/bleek-bass-box/releases) page:

| File | What it is |
|---|---|
| `BLEEK-BASS-BOX-macOS-VST3.zip` | VST3 for macOS (Apple Silicon + Intel) |
| `BLEEK-BASS-BOX-macOS-AU.zip` | Audio Unit for macOS |
| `BLEEK-BASS-BOX-Windows-VST3.zip` | VST3 for Windows x64 |
| `BLEEK-BASS-BOX-Windows-Standalone.zip` | Standalone app for Windows |

## Install (Ableton Live 12)

**macOS**
1. Unzip, then move `BLEEK BASS BOX.vst3` to `~/Library/Audio/Plug-Ins/VST3/` and `BLEEK BASS BOX.component` to `~/Library/Audio/Plug-Ins/Components/`.
2. The builds aren't notarized yet, so macOS will block them at first. Clear the quarantine flag once:
   ```bash
   xattr -cr ~/Library/Audio/Plug-Ins/VST3/BLEEK\ BASS\ BOX.vst3 ~/Library/Audio/Plug-Ins/Components/BLEEK\ BASS\ BOX.component
   ```
3. In Live: **Settings → Plug-Ins → Rescan**. Look under **BLEEK STATE AUDIO**.

**Windows**
1. Unzip and copy `BLEEK BASS BOX.vst3` to `C:\Program Files\Common Files\VST3\`.
2. In Live: **Settings → Plug-Ins → Rescan**.

## Building from source

See [BUILD_INSTRUCTIONS.md](BUILD_INSTRUCTIONS.md). In short, using CMake 3.22+ and a C++20 compiler:

```bash
cmake -B build
cmake --build build --config Release
```

JUCE 8.0.4 is fetched automatically.

## CI and releases

Every push to `main` builds macOS and Windows. Each build is checked with [pluginval](https://github.com/Tracktion/pluginval) (strictness 5), and the AU is also checked with Apple's `auval`.

To publish a release, push a version tag:

```bash
git tag v1.0.1
git push origin v1.0.1
```

CI then builds with that version number and attaches the zips to a new GitHub Release.
