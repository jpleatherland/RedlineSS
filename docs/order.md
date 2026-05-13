first pass:
inputHighPass();
gainStage1();
asymmetricClipper1();
lowPassFizzFilter();
output();

full:
processBlock()
{
    dcBlock();

    inputBuffer();

    inputHighPass();

    preGainVoicingFilter();

    cleanOrLeadChannel();

    gainStage1();

    interstageHighPass1();

    asymmetricClipper1();

    gainStage2();

    interstageEQ();

    asymmetricClipper2();

    saturationCompressor();

    lowPassFizzFilter();

    toneStackBass();

    toneStackMid();

    toneStackTreble();

    presenceFilter();

    masterVolume();

    transTubePowerAmp();

    speakerResonance();

    cabinetSimulation();

    outputLimiter();

    outputDCBlock();
}
Yep. Don’t think “components”, think “behavioural stages”.

A very sane Bandit-inspired architecture in JUCE would be something like this:

```cpp id="8k5h7m"
processBlock()
{
    dcBlock();

    inputBuffer();

    inputHighPass();

    preGainVoicingFilter();

    cleanOrLeadChannel();

    gainStage1();

    interstageHighPass1();

    asymmetricClipper1();

    gainStage2();

    interstageEQ();

    asymmetricClipper2();

    saturationCompressor();

    lowPassFizzFilter();

    toneStackBass();

    toneStackMid();

    toneStackTreble();

    presenceFilter();

    masterVolume();

    transTubePowerAmp();

    speakerResonance();

    cabinetSimulation();

    outputLimiter();

    outputDCBlock();
}
```

Now what each one *really* means:

---

## `dcBlock()`

Removes DC offset.

Basically always present in audio systems.

Usually a tiny HPF:

```text id="h0u1jq"
~5–20 Hz
```

---

## `inputBuffer()`

High impedance guitar input feel.

This affects:

* pickup interaction
* brightness
* attack

Can initially just be:

```cpp id="0mfqqn"
x = input;
```

Later:

* input impedance simulation
* pickup loading

---

## `inputHighPass()`

Critical for tightness.

Stops low-end mud before distortion.

Typical:

```text id="9s7ns5"
70–150 Hz
```

This is a HUGE part of “tight amp”.

---

## `preGainVoicingFilter()`

Shapes the amp character before clipping.

Examples:

* upper-mid hump
* low-mid scoop
* bright cap feel

This is where “Marshall/Fender/Peavey” starts emerging.

---

## `cleanOrLeadChannel()`

Branching logic.

Bandit has multiple voicings/channels.

Could literally be:

```cpp id="0z3xbn"
if (leadChannel)
```

---

## `gainStage1()`

First amplification stage.

Usually not ultra distorted.

Adds:

* edge
* texture
* harmonics

---

## `interstageHighPass1()`

VERY important.

Real amps repeatedly remove bass between gain stages.

This is why modern amps stay tight.

Without this:

* flub
* woof
* farting palm mutes

---

## `asymmetricClipper1()`

This is the “TransTube-ish” area.

Asymmetry sounds more amp-like.

Example:

```cpp id="i1q6r7"
if (x > 0)
    x = tanh(x * 1.2f);
else
    x = tanh(x * 0.8f);
```

Creates:

* even harmonics
* less sterile sound

---

## `gainStage2()`

More saturation.

Often where “lead channel” really happens.

---

## `interstageEQ()`

Additional shaping between clipping stages.

Can dramatically alter:

* aggression
* tightness
* chewiness

This stage matters more than people think.

---

## `asymmetricClipper2()`

Second nonlinear stage.

Often:

* more compressed
* smoother
* less dynamic

---

## `saturationCompressor()`

Optional.

Can emulate:

* transistor sag
* diode compression
* pseudo power supply sag

Even subtle compression changes “feel”.

---

## `lowPassFizzFilter()`

One of the most important stages.

Most distorted guitar tones are FAR darker than isolated guitar makes people think.

Typical:

```text id="6c8glk"
4–8 kHz LPF
```

This kills:

* fizz
* mosquito highs
* plugin harshness

---

## `toneStackBass()`

## `toneStackMid()`

## `toneStackTreble()`

Classic amp EQ.

Can be:

* Fender stack
* Marshall stack
* active EQ
* simplified filters

Bandit’s EQ topology is more active/transistor-ish than vintage tube amps.

---

## `presenceFilter()`

Upper treble/power amp shaping.

Often affects:

* attack
* air
* pick response

Usually operates differently than treble.

---

## `masterVolume()`

Simple level trim.

But placement matters:

* pre power amp
* post power amp

changes feel.

---

## `transTubePowerAmp()`

This is the magic goblin later.

Simulates:

* dynamic headroom
* speaker interaction
* damping
* clipping behaviour

Initially:
could literally just be:

```cpp id="vplp18"
softClip();
```

Later:

* dynamic saturation
* frequency-dependent clipping
* speaker feedback emulation

---

## `speakerResonance()`

Extremely important for “amp in room”.

Usually:

* low resonance bump
* upper rolloff

Typical guitar speaker behaviour:

```text id="yb8gsi"
big resonance ~70–120 Hz
strong treble rolloff
mid weirdness
```

---

## `cabinetSimulation()`

IR or simplified cab filter.

Honestly:
this alone makes things sound “real”.

Without cab simulation:
all amp sims sound like bees.

---

## `outputLimiter()`

Stops accidental explosions.

Also smooths nastiness.

---

## `outputDCBlock()`

Safety cleanup.

---

Now the REALLY important thing:

## Do not implement all this immediately.

Your first version should honestly be:

```cpp id="ahk6q8"
inputHighPass();
gainStage1();
asymmetricClipper1();
lowPassFizzFilter();
output();
```

That alone will already start teaching you:

* gain staging
* filter ordering
* feel
* tightness
* fizz management

Then you expand iteratively.

