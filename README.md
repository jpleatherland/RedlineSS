# Redline SS

**Redline SS** is a JUCE-based guitar amp plugin inspired by the feel and attitude of solid-state combo amps, especially red-stripe-era Peavey-style circuits, with enough British-flavoured grind to get into raw Marshall-ish territory.

The goal is not to create a perfect clone of any one amp. The goal is to build a flexible, responsive, high-gain-capable amp simulation that feels good under the fingers, sits well through a cabinet IR, and gives useful control over the parts of an amp that actually matter: gain structure, EQ shape, power-amp response, resonance, presence, transient behaviour, and output feel.

## Current status

This project is currently in active development.

At this stage, the plugin focuses on the core amp sound:

- Preamp drive and clipping behaviour
- EQ shaping
- High-gain voicing
- Fizz control
- Body / low-mid shaping
- Master output level
- Cabinet IR workflow via external IR loader

Planned or experimental features include:

- Presence control
- Resonance / depth control
- T.Dynamics-inspired headroom control
- Transient voltage drop behaviour
- Bloom / recovery behaviour
- More refined low-gain and edge-of-breakup sounds
- Improved UI and preset management

## Design goals

Redline SS is being built around a few simple ideas:

1. **Feel first**  
   The amp should respond naturally to playing dynamics, pickup output, palm mutes, and boost pedals.

2. **Solid-state does not mean sterile**  
   The inspiration comes from amps that are direct, punchy, immediate, and aggressive without needing valve sag or excessive compression. I'm honestly not really into that whole tube sag thing. I want a tight, focused, high-gain sound that still has character and doesn't feel like a brick wall.

3. **Useful controls over historical accuracy**  
   This is not intended to be a schematic-perfect model. Controls may expose behaviours that would normally be hidden inside an amp circuit.

4. **Cabinet choice matters**  
   The plugin is intended to be used with an external cabinet IR loader or cab simulation. Speaker and mic choice have a huge impact on the final sound.

## Intended signal chain

A typical signal chain might look like:

```text
Guitar
  -> boost / overdrive pedal, optional
  -> Redline SS
  -> cabinet IR loader
  -> reverb / delay / post effects
  -> DAW output
```

For best results, use a proper guitar cabinet IR after the plugin.  
A raw amp signal without a cab simulation will sound harsh, fizzy, and unrealistic.

## Suggested cab setup

The plugin has been tested with Bandit-style 1x12 cabinet IRs, including Blue Marvel-style captures.

Good starting points:

- Dynamic mic near the cone edge
- 1x12 open-back or semi-open combo IR
- Low-pass somewhere around 6–8 kHz if needed
- High-pass around 70–100 Hz depending on tuning and mix

The plugin is designed to produce the amp tone. The IR supplies the speaker, mic, and cabinet colour.

## Controls

Current and planned controls may include:

### Gain

Controls the amount of preamp drive and saturation.

Lower settings are intended for crunchy rhythm and edge-of-breakup tones.  
Higher settings move into punk, hard rock, and metal territory.

### Body

Shapes the low-mid weight of the amp.

Use this to add thickness, punch, or chest to the sound without simply increasing bass.

### Bass / Mid / Treble

Main tone-shaping controls.

These may not behave exactly like a passive analogue tone stack. The priority is musical usefulness rather than strict circuit recreation.

### Presence

Planned.

Controls upper-mid and high-frequency bite after the main distortion stage, similar to the way presence affects the perceived attack and edge of an amp.

### Resonance / Depth

Controls low-frequency speaker/power-amp-style thump and cabinet interaction.

### Fizz

Reduces harsh high-frequency distortion artefacts.

Useful for taming direct, high-gain sounds before they hit the cab IR.

### T.SS / Headroom

Planned.

Inspired by variable power-amp headroom behaviour. This control is intended to change how quickly the virtual amp feels like it is running out of clean power.

Lower settings should feel more compressed, saturated, and reactive.  
Higher settings should feel cleaner, tighter, and more immediate.

### Bloom

Planned.

A dynamic recovery effect intended to mimic the way some amps seem to swell or fill out after the initial pick transient.

### Master

Controls final output level.

Use this to level-match tones when changing gain, EQ, or dynamic controls.

## Building

This project uses JUCE.

Build instructions will depend on whether the project is currently using CMake or Projucer.

### CMake build

```bash
cmake -B build
cmake --build build --config Release
```

### Projucer build

1. Open the `.jucer` file in Projucer.
2. Save/export the project.
3. Open the generated IDE project.
4. Build the desired plugin target.

Generated build folders are intentionally ignored by Git.

## Repository structure

Expected layout:

```text
RedlineSS/
  Source/
    PluginProcessor.cpp
    PluginProcessor.h
    PluginEditor.cpp
    PluginEditor.h

  Resources/
    images/
    presets/

  CMakeLists.txt
  RedlineSS.jucer
  README.md
  .gitignore
```

This may change as the project develops.

## Git ignore policy

The repository should include source files, project definitions, assets, and documentation.

The repository should not include:

- Build folders
- IDE cache files
- Compiled plugin binaries
- Local JUCE checkouts
- Commercial IRs
- Third-party SDKs that cannot be redistributed
- Generated temporary files

## Licence

Whatever JUCE says. You can do whatever you want with this.

## Disclaimer

Redline SS is an independent learning project.

It is not affiliated with, endorsed by, or associated with Peavey, Marshall, JUCE, or any other amplifier or audio equipment manufacturer.

Any references to amplifier styles, circuit behaviours, or tonal inspiration are descriptive only.
