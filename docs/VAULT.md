# CHROME VAULT by TrapWin

A separate plugin product (target `ChromeVault`, VST3 / AU / Standalone) next to EVOLVE (`KeysKilla`) and
EVOLVE FX PRO (`EvolveFX`). A preset instrument: open, click a sound, play. Every preset is 1-4 layers ("parts"); since
**v2** a part is a **synth** patch (the EVOLVE engine), a **sample** instrument from the CHROME VAULT sample library, or a
**model** (physical / modelled instrument). 16 categories. UI 100 % English, no real brand / product / company names
anywhere (instrument nouns only: "Concert Grand", never a maker's name).

Identity (never change after release): company `TrapWin`, bundle `com.trapwin.chromevault`, manufacturer code `Trpw`,
plugin code `Cvlt`, product name `CHROME VAULT`, version 1.0.0 (`set(CV_VERSION ...)` in `CMakeLists.txt`, separate
from the EVOLVE project version). Installers, CI artifacts and end-user install notes: `docs/CHROME_VAULT_INSTALL.md`.
Sample credits: `docs/CHROME_VAULT_CREDITS.md`.

**Home note.** FL Studio calls MIDI 60 "C5". Everything is designed, previewed, rendered and labelled with MIDI 60 = C5
(`vault::homeNote = 60`, `vault::octaveForMiddleC = 5`, `vault::noteName (60) == "C5"`): preview-on-click, DRAG WAV and
the waveform thumbnails play MIDI 60 for **every** category (basses too), the on-screen keyboard shows C3..B6 with C5
marked. Sound rule for designers: at C5 every sound must be comfortable - the played fundamental is the loudest partial
(or within 3 dB) except intentionally inharmonic bells / FX, octave-up layers >= 12 dB under the main part, tame 2-5 kHz,
nothing piercing. `VaultTests -libreport` prints these measurements per instrument.

## v2 status (what exists, what is next)

* v3 (FX chain, modulation, sound macros, arp, gate, MAIN knobs, locks; CVB v3), the v3 editor and the v3 factory bank
  (every preset finished by its designer recipe): see "v3" and "The v3 factory bank" below.
* Done (core): part kinds + `CVB` v2 (v1 still read), the multi-zone sampler, the sample library format + loader + LRU
  cache, the library builder `VaultLibBuild`, the model interface +
  registry (+ the `sine` example), the 16 v2 categories, Style / Character tags removed everywhere, C5 + keyboard-focus
  fixes in the editor, tests.
* Done (library curation): the full instrument map (all pitched VSCO / VCSL instruments, `tools/vault/library_map.json`),
  built into `/home/user/cv-library` and measured (`VaultLibBuild -qc`, `docs/VAULT_INSTRUMENTS.md`).
* Done (v2 factory bank): `resources/vault/factory.cvb` is the v2 bank, 3,377 presets assembled from the six designer
  recipes (`tools/vault/recipes/*.cpp`) by `VaultBankGen -recipe all -qa` - see "The v2 factory bank" below and the QC
  report `docs/VAULT_BANK_REPORT.md`. Nobody has listened to it yet: every judgement is measured. The owner's listening
  page (64 presets, 4 per category) is built by `tools/vault/listen_page.py`.
* Not done: the installer packaging of the library is written (below) but has not run on GitHub yet.
* Done (models): 20 physical models for what the sample libraries lack - guitars (steel, nylon, banjo,
  12-string, clean / crunch electric, jazz archtop; pluck / palm mute / harmonic), electric basses (finger, pick, slap,
  fretless), tine / reed electric pianos, struck-string "funk keys", felt toy piano, female / male voice (ah oh oo ee,
  "ha", breathy, whisper), choir (8-24 singers), vocal pad. Details, research, targets and measurements:
  `docs/VAULT_MODELS.md`.
* Library packaging + CI: the library lives on the orphan branch `vault-library` (`Library/library.json` +
  `Library/audio/**`, made with `tools/vault/package_library.py branch`); CI tests with it and packs it into the
  installers + `CHROME-VAULT-Library.zip` (details: `docs/CHROME_VAULT_INSTALL.md`, "For developers").

## v3: FX chain, modulation, sound macros, arpeggiator, gate

Every preset now ships "finished": an FX chain, modulation, four named sound macros and (optionally) arp / gate settings,
all of which the MAIN page tweaks and the ADVANCED tabs edit. Status: engine, data format, processor API, recipe API,
the v3 editor (MAIN / LAYERS / FX / MOD / ARP·GATE, from the approved mockup) and the v3 factory bank (every preset with
its own chain, movement and four named macros, see "The v3 factory bank") are done.

### Data model (`VaultTypes.h`)

```cpp
struct VaultFxSlot  { vault::FxType type; bool on = true; std::array<float, vault::maxFxParams> p;   // vault::fxTypeInfo (type).params order
                      static VaultFxSlot make (FxType); float get (const char* id) const; VaultFxSlot& set (const char* id, float v); };
struct VaultModDest { juce::String target; float amount; };
struct VaultLfo     { uint8_t shape (sine, triangle, square, saw, sampleHold); float rate (Hz); bool sync; uint8_t div (vault::divisions());
                      float delay, fade (s); float phase (0..1); bool retrig; float depth (0..1); std::vector<VaultModDest> dests;   // <= 4
                    };
struct VaultMod     { VaultLfo lfo[2]; std::vector<VaultModDest> modWheel, velocity, aftertouch; };              // <= 4 routings each
struct VaultMacroTarget { juce::String target; float min, max, curve (-1..1); };
struct VaultMacro   { juce::String name; std::vector<VaultMacroTarget> targets; };                            // <= 8 targets
struct VaultArp     { bool on; uint8_t mode (up, down, upDown, asPlayed, random, chord, downUp, upAndDown, converge, diverge,
                      thumb, pinky), rate (division), octaves (1..4); float gate (0.01..1), swing (0..1); bool latch; uint8_t steps (1..32);
                      std::array<float,32> vel; std::array<uint8_t,32> type (rest / play / tie); std::array<int8_t,32> oct (-2..2);
                      uint8_t octMode (octUp, octDown, octUpDown, octAlternate), velMode (velPlayedPattern, velPattern, velPlayed),
                      retrig (retrigOff, retrigChord, retrigNote, retrigBar), latchMode (latchReplace, latchAdd);
                      std::array<float,32> len (0.05..1, x gate); std::array<uint8_t,32> slide (0 / 1); };   // vault::numSteps = 32
struct VaultGate    { bool on; uint8_t rate, steps (1..32); std::array<float,32> level; float depth, smooth, swing, mix; };
struct VaultPreset  { ...v2 fields...; bool v3; std::vector<VaultFxSlot> fx (<= 8, <= 2 per type); float finish;
                      VaultMod mod; std::array<VaultMacro, 4> macros; VaultArp arp; VaultGate gate; };   // macroDefaults = the macro values
```

`v3 = false` marks a v1 / v2 style preset (FX from part 0's EVOLVE FX keys, classic macros). `vault::upgradeToV3 (p)`
converts it; `VaultEngine::compile` and `serializeBank` do that by themselves, `deserializeBank` returns v1 / v2 files
converted (`keepLegacy = true` keeps them as stored). Arp / gate default to "Init" (Up, 1/16, 1 octave, 80 %, all steps on,
lengths 100 %, no slides, octave mode Up, velocity played x steps, retrigger New chord, latch replaces; gate 1/16 on / off)
and stay off unless a preset really needs them.

### FX types (`vault::fxTypeInfo`, parameters in order; `fx.<type>.<param>` targets)

| type id | name | parameters (range, default) |
| --- | --- | --- |
| `eq` | EQ | tilt (+-12 dB, 0: high shelf +t, low shelf -t/2), low (+-18 dB), lowFreq (30..1000 Hz, 160), mid (+-18 dB), midFreq (150..12000, 1000), midQ (0.2..8, 0.7), high (+-18 dB), highFreq (1000..18000, 6000) |
| `comp` | Compressor | threshold (-48..0 dB, -18), ratio (1..20, 3), attack (0.1..200 ms, 10), release (10..2000 ms, 150), makeup (0..24 dB), mix (0..1, 1), knee (0..12 dB, 6) |
| `drive` | Drive | amount (0..1), type (Tube, Tape, Hard, Blown, Fold, Bit), tone (500..20000 Hz low-pass, 20000 = off), mix, cleanLow (keep the lows clean), output (+-12 dB) |
| `lofi` | Lo-Fi | crush, wow, mix |
| `chorus` | Chorus | mix, rate (0.05..5 Hz, 0.33), depth (0..1, 0.5 = +-4 ms), mode (Chorus, Ensemble) |
| `phaser` | Phaser | mix, rate (0.35 Hz), depth (1 = 3.5 octaves), feedback (0.5), center (250 Hz) |
| `flanger` | Flanger | mix, rate (0.22 Hz), depth, feedback (-0.95..0.95, 0.65) |
| `filter` | Filter | cutoff (20..20000 Hz), reso, mode (LP, HP, BP), env (-1..1 = +-4 oct with the input level), lfo (0..1 = +-2 oct), lfoRate, lfoSync, lfoDiv, mix |
| `delay` | Delay | mix, time (division, 1/8D), feedback (0..0.95), mode (Ping-pong, Straight, Tape), sync (on), ms (free time), hiCut, loCut, flutter |
| `reverb` | Reverb | mix, size, type (Hall, Plate, Cloud, Room, Shimmer), decay (x0.25..4), predelay (+0..250 ms), damping (-1..1), lowCut (160 Hz), shimmer (octave feedback), freeze |
| `width` | Width | width (0..1, 0.5 = as is), monoBelow (0..400 Hz, 0 = off) |
| `reverse` | Reverse | mix, length (division, 1/8) |
| `transient` | Transient | attack (0..1) |
| `body` | Body | model (Kalimba, Bell, Glass, Metal Pipe, Wood Box, String), mix |
| `ghost` | Ghost | mix, blur, reverse (the EVOLVE ghost bus; needs synth layers with `ghost`) |
| `stutter` | Stutter | amount, rate (division), seed (EVOLVE circuit bend) |
| `halftime` | Half-Time | mix |

The DSP is the EVOLVE FX (`Source/Fx.h`: FxRack is now built from independent stages - `DriveStage`, `ChorusStage`,
`PhaserStage`, `FlangerStage`, `EqStage`, `DelayStage`, `ReverseStage`, `PunchStage`, `BodyStage`, `LofiStage`,
`GhostStage`, `CircuitStage`, `HalftimeStage` - and `FdnReverb` in `Studio.h` gained room / shimmer / decay / pre-delay /
damping / low cut). EVOLVE renders exactly as before (the stages default to the values FxRack always used).
`Source/vault/VaultFx.h` wraps every type as a slot processor (`vault::FxProcessor`) in `vault::FxChain`: a pool of 2
processors per type (allocation-free structure changes), on / off with a 10 ms crossfade (a processor switched on again
starts clean), the global FX switch fades every slot, then the output stage of the v2 rack (mono lows for bass presets,
DC block, part 0's `gain` as output gain, the "finish" glue / air / widening = the old MASTER, its -0.8 dBFS limiter).

**Default slots.** If a preset has no slot of a type a MAIN page EFFECTS knob (reverb, delay, chorus, drive, width, EQ
tilt) or a classic macro needs, the engine adds a silent one (reverb = Hall size 0.6, delay = 1/8D ping-pong 0.35) at its
canonical place (the v2 rack order: transient, comp, drive, body, lo-fi, stutter, half-time, filter, chorus, phaser,
flanger, EQ, delay, reverb, reverse, ghost, width). It does nothing until the knob is turned up.

### Targets (modulation and macros)

| target | macro (min..max) | modulation unit (LFO x amount, routings x amount) |
| --- | --- | --- |
| `pitch` | offset, semitones | semitones |
| `cutoff` / `reso` | offset: octaves / +0..1 (every layer) | octaves / + |
| `volume` / `pan` | offset (gain x (1 + v)) / +-1 | gain x (1 + v) / + |
| `quick.<cutoff, reso, attack, release, envAmt, glide, vibrato, tremolo, reverb, delay, chorus, drive, width, eqTilt, pan>` | offset on the MAIN knob (0..1) | + |
| `classic.<tone, space, drive, motion>` | the v2 macro value (0.5 = as designed) | + |
| `layer<1-4>.level` / `.pan` | level 0..2 / pan -1..1 (set) | gain x (1 + v) / + |
| `layer<1-4>.cutoff` / `.reso` / `.attack` / `.release` / `.tune` | offset: octaves / + / x2^v / x2^v / semitones | same |
| `fx.<type>.<param>` (first slot of the type, or its default slot) / `fx<1-8>.<param>` (chain position) | the parameter's value (set) | 0..1 of the parameter's range (log for frequencies) |
| `lfo<1-2>.depth` / `lfo<1-2>.rate` | depth 0..1 / rate Hz (set) | + / x2^(4 v) |
| `gate.depth` / `gate.mix` / `gate.smooth` / `arp.gate` / `finish` | value 0..1 (set) | + |

Order per block (128 samples, control rate): base values -> sound macros (absolute targets set to min + (max - min) x
curve (m); relative ones offset; curve > 0 = slow start m^(1+3c), < 0 = fast start) -> mod wheel / velocity (the last
note-on) / aftertouch routings -> LFO 1, LFO 2 (each sees what the routings / LFO 1 did to its depth / rate) -> classic
macros on the first slot of each type -> MAIN knobs -> FX modulation in the normalised range, values clamped. FX
parameters are smoothed by their processors (20 ms), layer gains / volume / pan ramp over the block. A target into the
FX chain is resolved when the sound loads: by slot position (`fx3.mix`) if the slot still has that type, else the first
slot of that type (a locked chain), else the target is dropped.

LFOs: sine / triangle / square / saw (down) / S&H, free (Hz) or tempo-synced (any division incl. dotted / triplet),
start phase, delay + fade from the first note, retrig (phase and delay restart when the first key goes down; synced:
counted from that beat) or free (synced: aligned to the host's bar grid). The value is computed once per block.

### MAIN page quick knobs (processor parameters, any preset)

0..1, 0.5 = as designed (vibrato / tremolo: 0 = off): cutoff (+-4 octaves on every layer: synth filter, sample filter,
model tone low-pass), resonance (+-1), attack / release (x1/16..x16 plus up to 2 / 4 s), filter env amount (+-1, synth
layers), glide (+-1, synth layers; legato only: a note glides only from a note still held, a detached note - the
piano roll - starts on its own pitch; `kk::VoiceParams::glideLegatoOnly`, owner report item 0; every synth note also
starts its drift / unison / ALIVE random generator from the same seed, `kk::VoiceParams::noteSeeded` - a note's random
detune no longer depends on how many notes came before it; KEYS KILLA leaves both off), vibrato (built-in 5.5 Hz, up to +-60 cents, no LFO needed), tremolo (built-in
4.5 Hz), reverb / delay / chorus / drive / width (the first slot of that type: below 0.5 scales the designed amount down to
0, above it moves it up to 0.7 / 0.6 / 1 / 0.8 / 1), EQ tilt (+-9 dB), pan (output balance). Volume stays the `volume`
parameter (dB). The EFFECTS LEDs (`setQuickFxOn`) switch the slot; with no slot, switching on adds one.

### Arpeggiator and gate (`Source/vault/VaultMod.h`)

Research (what Ableton's Arpeggiator, Logic's Arpeggiator, Omnisphere, Nexus, Serum 2, Pigments, Kontakt Play Series and
Steinberg's arps do) and the decisions it led to:

| Topic | Common practice | CHROME VAULT |
| --- | --- | --- |
| Note order | Up / Down / Up-Down / Down-Up / Up & Down / Converge / Diverge / Thumb / Pinky / As Played / Random / Chord | all 12 (`VaultArp::Mode`, append-only numbers) |
| Octaves | 1-4, range up (Logic), per-pass transpose (Ableton) | 1-4 + mode Up / Down (over the whole range), Up-Down (one octave per pass up and back), Alternate (each note then its octaves); no repeated pitch when held notes are an octave apart |
| Pattern | 16-32 steps with per-step velocity, length, tie, octave / transpose, slide (Omnisphere, Serum 2, Nexus) | 1-32 steps: VELOCITY, LENGTH (x GATE), ON / TIE / rest, SLIDE, OCTAVE (-2..+2) |
| Velocity | played vs pattern amount (Omnisphere's Velocity knob, Logic's Note Vel) | Played x Steps (default), Steps only, Played only; a step at 0 is silent |
| Retrigger | Off / Note / Beat (Ableton), Restart on new chord (Steinberg), "a chord = 0 -> 1 keys" | Off (runs on), New chord (default), Every note, Every bar (host time signature) |
| Latch | Reset / Add / Transpose (Logic), Hold + add / remove (Ableton) | Off / On (a new chord replaces) / Add (new keys join); the sustain pedal holds the keys (not the notes) |
| Sync | host tempo + song position; free (ms) when stopped | host playing: steps, swing and lanes on the song grid (ppq 0 = bar 1, so loops repeat exactly), sample-accurate inside the block; stopped: from the first key (key sync) |
| Swing | 50 % (straight) .. 75 % | 50-75 % on every 2nd step (`swing` 0..1), shown as 50-75 % |
| Chord changes | restart or carry on; legato notes do not restart | the engine orders offs -> pedal / CC -> ons on one sample (a grid-line chord change is a clean new chord whatever order the host sends); without a restart the order carries on after the last played note (Up: the next one above it); notes keep their own length, nothing doubles |
| Late chords | quantize to the next step | a chord up to min (45 % of a step, 1/16 beat) after a step line plays at once (that step), later ones wait for the next line; in Chord mode keys that come a moment after the step join it |
| MIDI export | drag the pattern | `arpPatternToMidi` renders with the same `vault::Arp` on the host grid from bar 1 - the file is what the engine plays (test: equal ticks, notes, velocities) |

* Clock (`vault::StepGrid`, shared by the arp and the gate): step k starts at origin + k x step, odd steps later by swing x
  step / 2. Host playing: origin = ppq 0; a jump of the song position (loop, relocation, start) re-aligns and restarts the
  pattern - a step line that passed a moment ago plays at once unless a step played just before the jump (no double). A loop
  that wraps inside a block (hosts that do not split blocks; `VaultTransport::loopStart / loopEnd` from
  `getLoopPoints()`) is split on the sample: the part after the loop end plays from the loop start. Transport stop: the
  pattern carries on in the same rhythm on the free clock. Sounding notes keep their lengths across every jump.
* Rate: every division incl. dotted / triplet from 1/1 to 1/64 (`1/64D`, `1/64T` were appended to `vault::divisions()`);
  a rate change while playing lets the due step come, then uses the new length.
* Notes end: after their LENGTH x GATE share (ties extend over the following tie steps; SLIDE holds the note into the next
  one so mono / legato layers glide); all keys up: the pattern stops on the next step line and nothing rings past it;
  arp off, preset switch, all-notes-off, transport stop and plugin bypass (`processBlockBypassed` ends every note once)
  leave nothing hanging. `VaultEngine::arpHeldNotes` gives the arp's notes (latched ones too, in played order) for the export.
* Gate (per sound slot, on the layer mix before the FX so tails fill the gaps): 1-32 steps (levels 0..1), rate, depth,
  smooth (edge time, share of a step), swing, mix; the same clock (host grid / first note, the loop wrap too; with the arp
  on and the host stopped the keys start it - a new chord - not the arp's notes, so arp and gate share one free clock and
  the LFO retrigger follows the keys too). Its switch fades over 10 ms.

### Locks ("keep while browsing")

`VaultEngine::setLocks (fx, arp, gate)` / `VaultProcessor::setLock (Section, bool)`: on a preset switch the locked
sections of the sound that plays now stay (FX lock: chain + finish, and the FX knobs keep their positions; arp / gate
locks: settings and switch). The switch is still the 40 ms crossfade.

### Engine / processor API

```cpp
// VaultEngine (audio thread unless noted)
void setPreset (const VaultCompiledPreset&, bool instant = false, bool applyLocks = true);
void updateSound (const VaultCompiledPreset&);   // an edit of the current sound, applied in place (else a crossfaded switch)
void setMacro (int, float); void setQuick (int, float); void setFxOn (bool); void setArpOn (bool); void setGateOn (bool);
void setLocks (bool fx, bool arp, bool gate);    // any thread
int arpStep() const; float lfoPhase (int) const; float lfoOutput (int) const;   // UI (approximate)
int arpHeldNotes (std::vector<std::pair<int, float>>&) const;   // any thread: the arp's keys (latched too), played order
void soundingNotes (uint64_t mask[2]) const;     // any thread, one reader (the on-screen keyboard): the notes the voices
                                                 // play now (arp output while it runs, latched patterns too; else the keys)
                                                 // | every note started since the last call (bit k of mask[k >> 6] = note k)
bool isNoteSounding (int note) const;            // the current bits only
int chainSize() const; vault::FxType chainType (int); int chainSlot (int); float chainParam (int, int); float chainOnGain (int);
int readNoteLog (NoteLog*, int) const;           // tests: the notes the voices got (arp output)

// VaultProcessor (message thread)
const VaultPreset& currentSound() const; bool isSoundEdited() const; void revertSound();
void editSound (const std::function<void (VaultPreset&)>&);   // any v3 change; compiled here, handed over lock-free
juce::String macroName (int) const;
float getQuick (int) const; void setQuick (int, float); void resetQuick();
bool getSwitch (const char* id) const; void setSwitch (const char* id, bool);   // idFxOn, idArpOn, idGateOn
void setLock (Section, bool); bool getLock (Section) const;
int fxSlotForQuick (int q) const; bool isQuickFxOn (int q) const; void setQuickFxOn (int q, bool on);
int arpStep() const; float lfoOutput (int) const; const VaultEngine& audioEngine() const;
juce::MidiFile arpPatternMidi (double bars = 2) const; juce::File writeArpMidiFile (double bars = 2) const;
```

Parameters (automatable; IDs never change): `presetIndex`, `macro1`..`macro4` (the 4 sound macros, named per preset in
the UI), `volume` (dB), `qCutoff qReso qAttack qRelease qEnvAmt qGlide qVibrato qTremolo qReverb qDelay qChorus qDrive
qWidth qEqTilt qPan` (0..1), `fxOn`, `arpOn`, `gateOn` (bool). The host shows the MAIN knob parameters in the plugin's
units (`quickParamText`): on the message thread the very text the knob shows ("14% Plate", "2.4 kHz", "off"), on any other
thread the offset in its own units ("−1.2 oct", "×2.00 time", "+30%", "24 ct", "as designed"; `quickParamFromText` reads
them back). `selectPreset` sets the macros to the preset's values, the
arp / gate switches to the preset's (unless locked) and the MAIN knobs back to neutral (the FX ones stay while the FX are
locked). Plugin state version 2 adds the knobs, switches, locks and - when it differs from the bank preset - the whole
current sound (one-preset CVB, base64); version 1 states load with everything as designed.

### CVB v3 (bank format version 3)

After each preset's parts (v2 layout unchanged): `float finish`, `uint8 fxCount`, per slot `uint8 type, uint8 on,
uint8 paramCount, paramCount x float`; 2 x LFO (`uint8 shape, float rate, uint8 sync, uint8 div, float delay, fade, phase,
uint8 retrig, float depth, dests`); modWheel / velocity / aftertouch dests (`uint8 n, n x (z-string target, float amount)`);
4 x macro (`z-string name, uint8 n, n x (target, float min, max, curve)`); arp (`uint8 on, mode, rate, octaves, float gate,
swing, uint8 latch, steps, 16 x float vel, 16 x uint8 type, 16 x int8 oct`); gate (`uint8 on, rate, steps, 16 x float level,
float depth, smooth, swing, mix`). Versions 1 and 2 are still read (converted). Parameter tables only grow at the end.

**CVB v4** (written now; `bankFormatVersion = 4`): the v3 layout with 32-step lanes - arp: `... uint8 steps, uint8 laneCount
(32), laneCount x float vel, x uint8 type, x int8 oct, uint8 octMode, velMode, retrig, latchMode, laneCount x float len,
laneCount x uint8 slide`; gate: `uint8 on, rate, steps, uint8 laneCount, laneCount x float level, ...`. v3 files (the
shipped factory bank) read with 16 lanes and the new fields at their defaults.

### The v2 -> v3 conversion (`Source/vault/VaultConvert.cpp`, `VaultBankGen -convert in.cvb out.cvb`)

The v2 master FX were one `kk::FxRack` driven by part 0's EVOLVE FX keys (including what the EVOLVE macros, time macro
and morph did to them). The conversion builds exactly those FxParams and turns each stage that does something into a slot
in the rack's order; MASTER becomes `finish`; the pure FX keys are removed from the parts; the TONE / SPACE / DRIVE /
MOTION macros become sound macros 1-4 named Tone / Space / Drive / Motion with the single target `classic.*` (the engine
applies the v2 macro formulas to the first slot of each type, and the default slots stand in for the stages a v2 rack
always had). Result: `resources/vault/factory.cvb` is the v2 bank in CVB v3 (3,377 presets, 1.75 slots on average, at
most 7) and every preset renders bit-identically to the v2 engine - with the macros as designed and moved, with pitch
bend / mod wheel / pedal (checked for the whole bank with `VaultTests -convcheck <v2 bank>` against the frozen v2 engine in
`tests/legacy/`, and for a sample of the EVOLVE starter sounds in every `VaultTests` run). Only two EVOLVE starter sounds
with the FUTURE macro far up used more than 8 rack stages; they keep the first 8.

### v3 recipe API (`tools/vault/recipes/Recipe.h`)

```cpp
namespace recipe::fx {
  enum class Reverb { hall, plate, cloud, room, shimmer };  enum class Delay { pingPong, straight, tape };
  enum class Drive { tube, tape, hard, blown, fold, bit };  enum class Filter { lowPass, highPass, bandPass };
  VaultFxSlot reverb (Reverb, float size, float decay, float mix);            // decay = x the type's natural decay
  VaultFxSlot delay (const char* division, float feedback, float mix, Delay = Delay::pingPong);   // "1/8D", "1/4T", "1/16"
  VaultFxSlot delayMs (float ms, float feedback, float mix, Delay = Delay::straight);
  VaultFxSlot chorus (float mix, float rateHz = 0.33f, float depth = 0.5f, bool ensemble = false);  VaultFxSlot ensemble (float mix, ...);
  VaultFxSlot phaser (float mix, float rateHz = 0.35f, float depth = 1, float feedback = 0.5f);
  VaultFxSlot flanger (float mix, float rateHz = 0.22f, float depth = 1, float feedback = 0.65f);
  VaultFxSlot eq (float lowDb, float midDb, float highDb, float tiltDb = 0, float midHz = 1000);  VaultFxSlot tilt (float db);
  VaultFxSlot comp (float thresholdDb, float ratio, float attackMs = 10, float releaseMs = 150, float makeupDb = 0, float mix = 1);
  VaultFxSlot drive (Drive, float amount, float mix = 1, float toneHz = 20000);  VaultFxSlot lofi (float crush, float wow, float mix = 1);
  VaultFxSlot filter (Filter, float cutoffHz, float reso, float envAmt = 0, float lfoAmt = 0, float lfoRateHz = 1);
  VaultFxSlot width (float width, float monoBelowHz = 0);  VaultFxSlot transient (float);  VaultFxSlot reverse (float mix, const char* division = "1/8");
  VaultFxSlot off (VaultFxSlot);                       // in the chain but switched off
  // any parameter: fx::reverb (...).set ("predelay", 20)
}
LfoBuilder recipe::lfo (VaultLfo::Shape = VaultLfo::sine);   // .rate (hz) .sync ("1/4D") .delay (s) .fade (s) .phase (p) .free() .depth (d) .to (target, amount)
MacroBuilder recipe::macro (const char* name);                // .to (target, min, max, curve = 0)
VaultArp recipe::arp (VaultArp::Mode, const char* division, int octaves = 1, float gate = 0.8f, float swing = 0, bool on = false);
VaultGate recipe::gate (const char* division, std::initializer_list<float> steps, float depth = 1, float smooth = 0.25f, float mix = 1, bool on = false);
Design recipe::design (VaultPreset&);   // converts a v2-style preset first, then:
  // .fx ({...}) .addFx (slot) .finish (a) .macro (index, macro, value = 0.5) .clearMacros() .lfo (index, lfo)
  // .modWheel (target, amount) .velocity (target, amount) .aftertouch (target, amount) .arp (a) .gate (g)
```

Example (an electric piano that already sounds finished):

```cpp
using namespace recipe;   // in a recipe file
VaultPreset p;            // id, name, category, parts as before (no FX keys needed)
p.id = "cv.keys.0900"; p.name = "Warm Tine Room"; p.category = vault::catKeysOrgan;
p.parts.push_back (modelPart ("ep.tine.mellow", "Tine Piano"));
design (p)
    .fx ({ fx::comp (-20, 2.5f, 15, 120), fx::drive (fx::Drive::tape, 0.15f, 0.7f, 9000), fx::chorus (0.25f, 0.5f),
           fx::eq (1.0f, 0, -2.0f), fx::delay ("1/8D", 0.25f, 0.10f), fx::reverb (fx::Reverb::room, 0.5f, 1.0f, 0.18f) })
    .finish (0.2f)
    .clearMacros()
    .macro (0, macro ("Bark").to ("fx.drive.amount", 0.0f, 0.6f).to ("fx.eq.mid", 0, 3), 0.25f)
    .macro (1, macro ("Space").to ("fx.reverb.mix", 0.05f, 0.45f).to ("fx.delay.mix", 0.0f, 0.3f), 0.3f)
    .macro (2, macro ("Tremolo").to ("lfo1.depth", 0, 1), 0.0f)
    .macro (3, macro ("Tone").to ("cutoff", -1.5f, 1.0f), 0.6f)
    .lfo (0, lfo (VaultLfo::sine).sync ("1/8T").to ("volume", 0.5f).to ("pan", 0.3f))
    .modWheel ("lfo1.depth", 1.0f)
    .velocity ("fx.drive.amount", 0.2f);
```

(This exact example is compiled, validated and rendered by `VaultTests` - "recipe API: the docs example".)

The bank gate (`RecipeGate.h`) renders presets with their whole v3 sound (arp / gate when `on`), so every C5 comfort
rule (fundamental dominance, 2-5 kHz <= 10 %, > 6 / 8 kHz shares, category loudness, chord headroom) applies to the FX as
designed; keep bright FX (drive, shimmer, exciters) under those limits.

## Files

| File | What |
| --- | --- |
| `Source/vault/VaultTypes.h/.cpp` | `VaultPart` (kind, source, articulation, values), `VaultPreset` (+ v3: FX chain, modulation, sound macros, arp, gate), the 16 categories, sample-parameter / FX-type / quick-knob / division tables, targets, home note, the `CVB` bank format (v3 written, v1 / v2 read) |
| `Source/vault/VaultConvert.cpp` | `vault::upgradeToV3` (v1 / v2 -> v3, lossless) |
| `Source/vault/VaultFx.h/.cpp` | `vault::FxProcessor` (one per FX type, around the EVOLVE stages of `Source/Fx.h`) and `vault::FxChain` (pool, crossfaded bypass, output stage) |
| `Source/vault/VaultMod.h/.cpp` | `vault::Arp`, `vault::Gate`, LFO shapes, `vault::arpPatternToMidi` |
| `tests/VaultV3Tests.cpp`, `tests/legacy/` | the v3 tests (`VaultTests -v3`, part of `-unit`), `-convcheck`; the frozen v2 engine + rack used as the conversion reference |
| `Source/vault/VaultLibrary.h/.cpp` | `vault::SampleLibrary` (library.json index, background loader, LRU cache, location search), `InstrumentHandle`, `SampleInstrumentData` (decoded zones, `pick()`) |
| `Source/vault/VaultSampler.h/.cpp` | `vault::SamplerPart` (64-voice multi-zone sampler) and `vault::SincTables` (16-tap windowed-sinc interpolation) |
| `Source/vault/models/ModelVoice.h` | the model interface (`vault::models::ModelVoice`, `ModelInfo`, `ModelRegistry`) |
| `Source/vault/models/ModelRegistry.cpp` | the registry + `registerBuiltinModels()` (one line per model) + the `sine` example model |
| `Source/vault/models/BuiltinModels.h`, `ModelDsp.h` | the model families' info factories; shared DSP blocks (delay line + Lagrange read, allpass, biquads, formants, anti-aliased clipper, noise) |
| `Source/vault/models/StringModels.cpp` | waveguide strings: guitars, electric basses, struck-string keys (`docs/VAULT_MODELS.md`) |
| `Source/vault/models/KeysModels.cpp` | tine / reed electric pianos (modal tine / reed + non-linear pickups), felt toy piano |
| `Source/vault/models/VoiceModels.cpp` | LF-pulse source-filter voices, choir (8-24 singers, half-rate), vocal pad |
| `tools/vault/ModelLab.cpp`, `models_measure.py` | console target `VaultModelLab` (demos, CPU, measurement renders) + the measurement / comparison script |
| `tests/VaultModelTests.cpp` | the model unit tests (part of `VaultTests`; alone: `VaultTests -models`) |
| `Source/vault/VaultEngine.h/.cpp` | `VaultEngine` - arp -> 4 parts of any kind -> modulation -> gate -> FX chain -> trim -> mono lows -> volume -> limiter; macros, LFOs, quick knobs, locks; offline `render()` |
| `Source/vault/VaultProcessor.h/.cpp` | `VaultProcessor` - the `juce::AudioProcessor` (parameters, bank loading, library pins, state) |
| `Source/vault/VaultEditor.h/.cpp` | the editor window (scaling, display fit, focus hand-back) + `VaultMainView` (top bar, pages, keyboard) |
| `Source/vault/ui/*.h` | `VaultTheme.h` (palette, fonts, LookAndFeel), `VaultKnob.h` (the v3 knob + host-parameter attachment), `VaultControls.h` (panels, pills, dropdowns, segments, power buttons, tabs, LED toggles), `VaultWidgets.h` (category column, preset list, arrows, drag-out pill, keyboard, kind icons, spinner), `VaultBrowserModel.h` (search / filters / favorites / recent, instrument names), `VaultSettings.h`, `VaultPages.h` (the pages + shared context), `VaultMainView.h` |
| `Source/vault/ui/*.cpp` | the v3 pages: `VaultMainPage.cpp`, `VaultLayersPage.cpp`, `VaultFxPage.cpp`, `VaultModPage.cpp`, `VaultArpPage.cpp`, `VaultPagesCommon.cpp` (value texts, "Modulate with..." menu) |
| `tools/vault/LibBuild.cpp` | console target `VaultLibBuild`: builds the sample library from the CC0 sources |
| `tools/vault/library_map.json` | the builder's map: instruments -> source folders, file-name patterns, articulations, options |
| `tools/vault/make_instruments_doc.py` | writes `docs/VAULT_INSTRUMENTS.md` from a built library + its `-qc` run |
| `docs/VAULT_INSTRUMENTS.md` | the library's instruments, articulations, ranges, sizes and QC numbers (generated) |
| `tools/vault/package_library.py` | library packaging: `check` (files exist, each < 50 MB), `size`, `zip` (release zip / parts), `branch` (builds the orphan `vault-library` branch) |
| `tools/vault/recipes/Recipe.h`, `recipes/*.cpp` | the v2 preset recipes: one file per sound designer (`keys`, `strings`, `orch`, `winds`, `perc-voc`, `synth`), each registered with `VAULT_RECIPE` |
| `tools/vault/recipes/RecipeGate.h` | the v2 bank gate + assembly (`VaultBankGen -recipe <name>\|all`): C5 renders, comfort rules, loudness + peak control, chord headroom, distinctness, report, listening renders |
| `tools/vault/bank_report_doc.py`, `listen_page.py`, `listen_picks.py` | `docs/VAULT_BANK_REPORT.md` from the assembly report; the owner's listening page (4 presets per category) |
| `tools/vault/listen3_page.py` | the v3 listening page from `VaultBankGen -listen3` clips (phrase + macro-1 sweep per preset, Czech headings, votes) |
| `tools/vault/BankGen.cpp` | console target `VaultBankGen`: `-recipe` runs the v2 recipes (RecipeGate.h); the rest is the legacy v1 synth-bank generator (`-v1`, never writes the shipped bank by default) |
| `tools/vault/BankQC.h`, `BankAudit.h` | v1 quality gate / audit (shared with VaultTests) |
| `tests/VaultTests.cpp` | console target `VaultTests` |
| `resources/vault/*.cvb` | the preset bank(s) embedded into the plugin |

Every `Source/vault/*.cpp` and `Source/vault/models/*.cpp` is compiled into `ChromeVault` and `VaultTests` automatically
(CMake `file(GLOB ... CONFIGURE_DEPENDS)`); console tools use `CV_ENGINE_SOURCES` + `CV_MODEL_SOURCES`.
`JUCE_USE_OGGVORBIS=1` is set for ChromeVault, VaultTests and VaultLibBuild.

## Architecture

```
MIDI ─┬─> part 0  synth:  kk::SynthEngine (32 voices)                      ─┐  x level, pan (equal power), part gain
      ├─> part 1  sample: vault::SamplerPart (64 voices) <- InstrumentHandle ├─> sum ─> mod / gate ─> vault::FxChain (v3; v2: FxRack of part 0)
      ├─> part 2  model:  pool of vault::models::ModelVoice (ModelInfo::maxVoices)
      └─> part 3  (any kind; transposed by octave*12+semi)                   ─┘
   ─> side high-pass 160 Hz 8th order (mono below ~120 Hz) + 5 Hz DC blocker on the mid
   ─> peak control (only presets with `cv.peak`: 1 ms look-ahead peak compressor, ratio 6, before the trim)
   (preset gain trim per sound slot; preset switch = two slots crossfading 40 ms equal power, unchanged from v1)
   ─> master volume ─> safety limiter (-1 dBFS, instant attack, 120 ms release) + soft ceiling 0.98 ─> out
```

* Every engine part slot owns a synth engine, a sampler and (lazily) one model-voice pool per registered model, so a
  preset switch never allocates: `setPreset` copies the compiled preset into the free slot.
* **FX** come from part 0's values whatever its kind (EVOLVE FX keys: `revMix revSize revType delayMix delayTime delayFb
  chorus phaser flanger drive driveType crush eqLow eqHigh width master gain bassMode ...`). For a sample / model part 0 the
  FX defaults are neutral: `revMix` 0, `master` 0 (no MASTER glue / air), EVOLVE macros `m2` / `m5` 0 (no hidden reverb,
  delay, chorus); everything else EVOLVE default (drive 0, width 0.5 = as recorded). Set them explicitly in part 0.
* **Gain**: part 0's `gain` (dB) is the FX output gain; parts 1-3 are scaled by `dB(gain_k - gain_0)` on top of `level`.
  `VaultPreset::gain` (linear) is the loudness trim after the FX.
* **Macros** (0..1, 0.5 = as designed): on synth parts exactly as v1. On sample / model parts:
  * TONE: filter tilt - the part cutoff x 2^((2t-1) x 1.5) (a sample part with its filter off gets a low-pass from
    ~7 kHz at TONE 0; above 0.5 it opens), plus the master high / low shelf tilt (+-5 / -+2.5 dB) as in v1.
  * SPACE: master reverb / delay / width (v1).
  * DRIVE: master saturation (v1).
  * MOTION: vibrato - sample parts: `s.vibrato` cents x (designed amount below 0.5) + up to 14 cents at 1.0 (rate
    `s.vibRate`); model parts: up to 14 cents at 5.2 Hz above 0.5 - plus the master chorus (v1).
* **Loading**: a sample part whose instrument is not in RAM yet is silent (notes are ignored) until the library has
  decoded it; `VaultEngine::isLoading()` / `VaultProcessor::isLoading()` report it and the editor shows "Loading samples...".
  A model part is silent until its voice pool exists (made by `VaultEngine::serviceResources()` off the audio thread -
  the processor's 20 Hz timer, offline renders call it before the first sample).
* Offline renders (`render`, `renderMidi`) decode the needed instruments synchronously first, so they are complete.

### The sampler (`vault::SamplerPart`)

* 64 voices per part (+8 spare slots so stolen voices fade out over 4 ms instead of cutting); `s.polyphony` lowers the
  limit (the oldest note - released ones first - fades out). The same key struck again releases its previous note over at
  most 250 ms (one-shots: at most 2 rings per key). No allocation anywhere after `prepare()` (tested with an operator-new
  counter over 400 blocks of notes, pedal, bends, macros and sample / synth / model preset switches).
* Interpolation: 16-tap Kaiser-windowed sinc (beta 6.5), 256 phases + linear phase interpolation, 13 tables: table t is
  used for read increments up to 2^(t/12) and cuts at 0.92 x Nyquist / increment, so pitching up to +12 semitones does
  not alias and pitching down does not image. Samples are stored as planar int16 with 16 zero frames of padding.
* Zones: key range, velocity range (1..127), round robin (per key counter), root + fine tune, gain, start, sustain loop
  (`loopStart`, `loopEnd`, crossfade `xfade` frames). The loop crossfade is done at run time: over the last `xfade`
  frames before `loopEnd` the voice blends linearly into the frames before `loopStart`, then jumps to `loopStart` - the
  audio files are untouched and every loop is seamless by construction (tested on a deliberately non-periodic loop).
* Envelope: natural (0.5 ms declick; 3 ms when `s.start` moves into the sound), or the ADSR override (`s.attack`,
  `s.decay` + `s.sustain`, `s.release`). Release: exponential to -60 dB in the release time (articulation default or
  `s.release`), looped notes keep looping while they fade. One-shot articulations ignore note-off. Release samples (zones
  with `"release": true`) are triggered on a real key release, level = articulation `releaseGainDb` x velocity x
  -`releaseDecayDb` per second held (max -30 dB).
* Velocity: the layer by range; level on top = velocity^(1.6 x `s.velSens`). Filter: 12 dB/oct SVF per voice (LP / HP / BP,
  `s.cutoff`, `s.reso`, `s.velCutoff`), off when low-pass >= 19 kHz. Pitch bend `s.bendRange`; glide is off.
* Pitch includes the file / host sample-rate ratio (tested in tune within +-5 cents at 44.1 and 48 kHz).
* CPU: 48 held voices (pitched, looped, filtered) cost about 10 % of one core (~0.2 % per voice). RAM: decoded int16,
  e.g. the concert grand 251 MB (1.2 s to decode), violins 40 MB, flute 44 MB, marimba 13 MB.

## Data model (`Source/vault/VaultTypes.h`)

```cpp
enum class VaultPartKind : uint8_t { synth = 0, sample = 1, model = 2 };
struct VaultPart {
    VaultPartKind kind = VaultPartKind::synth;
    juce::String source;          // sample: library instrument id ("piano.grand.concert"), model: model id ("sine"), synth: empty
    juce::String articulation;    // sample: articulation name ("sustain"); empty / unknown = the instrument's first
    std::vector<std::pair<juce::String, float>> values;   // synth: EVOLVE IDs; sample: "s.*" + FX keys; model: "m.*" + FX keys
    float level = 1.0f;  float pan = 0.0f;  int octave = 0;  int semi = 0;  juce::String label;
    float get (const juce::String& id, float fallback) const;  void set (const juce::String& id, float v);
};
struct VaultPreset {
    juce::String id, name;  int category = 0;  std::vector<VaultPart> parts;   // 1..4
    float gain = 1.0f;  float macroDefaults[4] { 0.5f, 0.5f, 0.5f, 0.5f };
    bool usesSamples() const;
};
namespace vault {
  enum { numCategories = 16, maxParts = 4, numMacros = 4 };
  enum CategoryIndex { catPiano, catKeysOrgan, catGuitars, catBass, catStrings, catBrass, catFlutesWoodwinds, catOrchestral,
                       catBellsMallets, catVocalsChoir, catPlucks, catLeads, catPads, catSynth, catWorld, catFxAtmos };
  const juce::StringArray& categoryNames();   // "Piano", "Keys & Organ", "Guitars", "Bass", "Strings", "Brass", "Flutes & Woodwinds",
                                              // "Orchestral", "Bells & Mallets", "Vocals & Choir", "Plucks", "Leads", "Pads", "Synth", "World", "FX & Atmos"
  const char* categorySlug (int c);           // piano keys gtr bass str brass wind orch bell vox pluck lead pad synth world fx (ids "cv.<slug>.<nnnn>")
  constexpr int homeNote = 60, octaveForMiddleC = 5;   juce::String noteName (int midi);   // 60 -> "C5"
  struct ParamInfo { const char* id; float lo, hi, def; const char* what; };
  const std::vector<ParamInfo>& sampleParams();  int sampleParamIndex (const juce::String& id);
  enum SampleParam { spAttack, spDecay, spSustain, spRelease, spVelSens, spCutoff, spReso, spFilterType, spVelCutoff, spTune, spStart,
                     spBendRange, spVibrato, spVibRate, spWidth, spReleaseSamples, spPolyphony, spOneShot, numSampleParams };
  juce::MemoryBlock serializeBank (const std::vector<VaultPreset>&, bool compress = true);           // writes v2
  bool deserializeBank (const void* data, size_t size, std::vector<VaultPreset>& out, juce::String* error = nullptr, int* version = nullptr);
  bool saveBankFile (const juce::File&, const std::vector<VaultPreset>&);
  bool loadBankFile (const juce::File&, std::vector<VaultPreset>& out, juce::String* error = nullptr);
  constexpr int bankFormatVersion = 2;
  bool validatePreset (const VaultPreset& p, juce::String* why = nullptr);
  namespace v1 { enum CategoryIndex { cat808Bass, ... }; int toV2Category (int v1Category, const VaultPreset&); juce::String cleanText (const juce::String&); }
}
```

Style and Character tags no longer exist (types, bank format, browser, UI, tests).

### Sample-part parameters (`VaultPart::values`, missing = default)

| id | range | default | meaning |
| --- | --- | --- | --- |
| `s.attack` | 0..10 s | 0 | extra attack (0 = the sample's own onset) |
| `s.decay` | 0..30 s | 0 | decay to `s.sustain` (0 = no decay stage, natural envelope) |
| `s.sustain` | 0..1 | 1 | sustain level of the ADSR override |
| `s.release` | 0..20 s | 0 | release time to -60 dB (0 = the articulation's) |
| `s.velSens` | 0..1 | 0.5 | velocity -> level on top of the layers (gain = vel^(1.6 x amount)) |
| `s.cutoff` | 20..20000 Hz | 20000 | filter cutoff (low-pass at 20000 = off) |
| `s.reso` | 0..1 | 0 | resonance (0 = Butterworth) |
| `s.filterType` | 0..2 | 0 | 0 LP, 1 HP, 2 BP (12 dB/oct SVF) |
| `s.velCutoff` | 0..1 | 0 | soft notes darker: cutoff x 2^(-4 x amount x (1 - vel)) |
| `s.tune` | -100..100 cents | 0 | fine tune |
| `s.start` | 0..1000 ms | 0 | sample start offset |
| `s.bendRange` | 0..24 | 2 | pitch-bend range (semitones) |
| `s.vibrato` | 0..50 cents | 0 | designed vibrato depth (MOTION scales / adds) |
| `s.vibRate` | 0.5..9 Hz | 5.2 | vibrato rate |
| `s.width` | 0..2 | 1 | stereo width of the part (0 mono, 1 as recorded) |
| `s.releaseSamples` | 0/1 | 1 | play the articulation's release samples |
| `s.polyphony` | 1..64 | 64 | voices of the part |
| `s.oneShot` | -1/0/1 | -1 | -1 articulation default (a one-shot articulation with an attack of 0.3 s or more - a swell - releases on note-off), 0 note-off releases, 1 plays to the end |

Preset-level keys (part 0 of any kind, set by the bank gate, designers leave them out): `cv.peak` (-60..0 dB, threshold of
the peak control before the trim, 0 = off) and `cv.peakvel` (0..4, the threshold follows the last note-on velocity:
thr x vel^exp). The peak control is a 1 ms look-ahead peak compressor (hold 15 ms, release 50 ms, ratio 6): it lets very
percussive presets (muted plucks, staccato keys) reach the category loudness without the safety limiter; the preset
then plays 1 ms later (not reported as plugin latency).

`validatePreset`: synth parts take EVOLVE keys only and no `source`; sample parts need a `source` and take EVOLVE keys +
the `s.*` table (range-checked); model parts need a `source` and take EVOLVE keys + any `m.*` key (the model's table is
applied at compile time, unknown `m.*` keys are ignored, values clamped to the model's ranges).

## Bank file format `CVB` (v2 layout; v3 appends the sections described in "CVB v3" above; v1 still read)

```
"CVB1" | uint16 version (2) | uint16 flags (bit0 = gzip/zlib payload) | uint32 presetCount | uint32 rawPayloadSize | payload
payload v2: uint16 keyCount, keyCount x UTF-8 zero-terminated key strings (canonical EVOLVE keys in kk::vk order, then the
            sample keys, then any other key used)
            per preset: id, name (z-strings), uint8 category, float gain, 4 x float macroDefaults, uint8 partCount,
                        per part: uint8 kind, source, articulation, label (z-strings), float level, float pan, int8 octave,
                                  int8 semi, uint16 valueCount, valueCount x (uint16 keyIndex, float32 value)
payload v1: as v2 without kind / source / articulation, with uint32 styles + uint32 chars after the category, uint8 key indices.
            Read as synth parts; tags dropped; category -> vault::v1::toV2Category (808 & Bass -> Bass, Piano & Keys -> Piano when a
            name / label says piano / grand / upright / felt else Keys & Organ, Arps & Sequences -> Synth, Brass & Horns -> Brass,
            Flutes & Winds -> Flutes & Woodwinds, the rest 1:1); "808" in names / labels -> "Sub".
all little-endian (juce::MemoryOutputStream). v2 round trips bit-exactly.
```

## Sample library

### Location and loading (`vault::SampleLibrary`, `Source/vault/VaultLibrary.h`)

Search order (first folder holding a readable `library.json` wins): `$CHROME_VAULT_LIBRARY` (absolute), then the system
folder - Windows `%ProgramData%\TrapWin\CHROME VAULT\Library`, macOS `/Library/Application Support/TrapWin/CHROME VAULT/Library`,
Linux `/usr/share/TrapWin/CHROME VAULT/Library` - then the user folder - Windows `%APPDATA%\TrapWin\CHROME VAULT\Library`,
macOS `~/Library/Application Support/TrapWin/CHROME VAULT/Library`, Linux `~/.local/share/TrapWin/CHROME VAULT/Library`.
RAM budget for decoded audio: 1.5 GB default, `$CHROME_VAULT_RAM_MB` or `setMemoryBudget()`.

```cpp
namespace vault {
class SampleLibrary {
    static std::shared_ptr<SampleLibrary> getShared();        // process-wide, at the standard location; lives while held
    static juce::Array<juce::File> searchLocations();
    explicit SampleLibrary (const juce::File& libraryFolder); // explicit folder (tests / tools)
    bool libraryFound() const;  juce::File libraryPath() const;  juce::String errorMessage() const;
    int numInstruments() const;  InstrumentHandle* instrument (int index) const;
    InstrumentHandle* find (const juce::String& id) const;   // nullptr if unknown
    void setMemoryBudget (int64_t bytes);  int64_t memoryBudget() const;  int64_t memoryUsed() const;
    void pin (InstrumentHandle*);  void unpin (InstrumentHandle*);   // keep in RAM (+ request the load) / allow eviction
    bool loadNow (InstrumentHandle*);                         // non-audio thread: decode now in this thread
    bool waitUntilIdle (int timeoutMs);                       // tests
};
class InstrumentHandle {                                       // one per library instrument, stable address
    const InstrumentDesc desc;                                 // id, name, family, credit, articulations (zones from library.json)
    const SampleInstrumentData* data() const;                  // audio thread: decoded data or nullptr
    void acquire(); void release();                            // the engine holds what a playing preset uses (no eviction meanwhile)
    void request();                                            // audio-thread safe: ask the loader for it
    bool isLoaded() const; bool hasFailed() const; bool isLoading() const;
};
struct SampleInstrumentData {                                  // immutable once published
    const Zone* pick (int articulation, int key, int velocity1to127, unsigned rrCounter, bool release) const;
};
}
```

Threading: one loader thread per library decodes instruments (files in parallel, up to 4 threads) into int16 RAM and
publishes them with an atomic pointer. The audio thread only does atomic loads / increments (`acquire`, `request`,
`data`). Eviction (LRU by last use, only instruments with no engine user, no pin and no pending request) unpublishes first
and re-checks the user count, so an engine that acquired meanwhile keeps its data. The processor pins the current
preset's instruments (`selectPreset` pins at once, so loading starts before the audio thread switches) and unpins old ones
(20 Hz timer). A missing / unreadable file marks the instrument failed (silent, `isMissingSamples()`, no retry loop).

### Format (`Library/library.json` + `Library/audio/**.ogg`)

```json
{ "format": "CHROME VAULT library", "version": 1, "sampleRate": 44100, "credits": "...",
  "instruments": [
    { "id": "piano.grand.concert", "name": "Concert Grand", "family": "Piano", "credit": "VCSL ... - CC0",
      "articulations": [
        { "name": "sustain", "oneShot": false, "release": 0.32, "releaseDecayDb": 4.0, "releaseGainDb": 0,
          "zones": [
            { "file": "piano.grand.concert/sustain_060_v3_r1.ogg", "root": 60, "fine": -3.1, "lo": 60, "hi": 61,
              "vlo": 85, "vhi": 127, "rr": 0, "gain": 0, "frames": 617400, "channels": 2 },
            { "file": "strings.violins.ensemble/sustain_059_v2_r1.ogg", "root": 59, "loopStart": 431235, "loopEnd": 498117, "xfade": 6615, "...": "..." },
            { "file": "piano.grand.concert/sustain_060_v3_r1_rel.ogg", "root": 60, "lo": 60, "hi": 61, "vlo": 85, "vhi": 127, "release": true } ] } ] } ] }
```

* `file` relative to `Library/audio/`; OggVorbis (or WAV / FLAC), 44.1 kHz, mono or stereo as recorded.
* `root` = MIDI note of the recorded pitch (C4 = 60 scientific = "C5" in FL Studio), `fine` = cents to add to play in tune,
  `lo` / `hi` key range, `vlo` / `vhi` velocity range (1..127), `rr` round-robin index inside the key / velocity cell,
  `gain` dB, `start` first frame, `loopStart` / `loopEnd` / `xfade` frames (loop = loopEnd > loopStart), `release: true`
  = release-trigger zone, `frames` / `channels` informative. Articulation: `oneShot`, `release` (s), `releaseGainDb`,
  `releaseDecayDb` (dB per second held). Unknown keys are ignored; zones with a bad range are skipped.
* Velocity cells: the zones whose range holds the velocity; if none, the nearest layer. Several zones in a cell = round robin.

### Builder (`VaultLibBuild`, `tools/vault/LibBuild.cpp`, map `tools/vault/library_map.json`)

```
cmake --build build-vault --target VaultLibBuild
build-vault/VaultLibBuild_artefacts/Release/VaultLibBuild [-map tools/vault/library_map.json] [-out /abs/Library] [-only id1,id2] [-threads N] [-dry]
build-vault/VaultLibBuild_artefacts/Release/VaultLibBuild -qc [-out /abs/Library] [-only ids] [-demo /abs/dir]   # QC of a built library
python3 tools/vault/make_instruments_doc.py /home/user/cv-library tools/vault/library_map.json docs/VAULT_INSTRUMENTS.md
```

Run from the repo root. Output defaults to the map's `output` (`/home/user/cv-library`, outside the repo; never commit
audio). Instruments not rebuilt in a run are kept in `library.json`. `build_report.txt` lists every file (detected pitch,
cents, aperiodicity, loudness, gain, key / velocity range, loop points and correlation, warnings, dropped takes).

Map format: `roots` (name -> folder), `output`, `defaults`, `instruments[]` = `{ id, name, family, credit, articulations[] }`,
articulation = `{ name, sources[], options }`, source = `{ "files": "<root>:<folder>/<glob>", "pattern": "...", "trigger":
"release", "levelDb": -22 }`. Pattern placeholders: `{note}` (C4, F#3, Bb2, A#-1), `{vel}` (any token: 1, vl3, soft), `{rr}`
(1-based number), `{group}` (a token that ties files together, e.g. one timpani tuning "6C"), `*` anything. More source
keys: `vel` (a fixed layer name for every file of the source - several sources can feed one layer), `octave` (the names
are this many octaves off; set by hand, the automatic octave check then leaves the source alone), `exclude` (file-name
wildcards to leave out), `pitch: "detect"` + `range` [lo, hi] MIDI + `partials` (no usable note names: the root is
measured by a harmonic-sum search with those partial ratios, e.g. timpani `[1, 1.5, 1.98]`; files of one `{group}` share
the median). Options (defaults in `defaults`, override per articulation): `loop`, `oneShot`, `release`,
`releaseDecayDb`, `maxSeconds` (16), `tailFadeMs` (250, when capped), `preRollMs` (4), `silenceDb` (-62), `peakDb` (-1, the
instrument's loudest peak), `minLoopMs` (300), `maxLoopMs` (2600), `xfadeMs` (100; 150 for weakly correlated loops),
`extendKeys` (4 in the map: the outermost zones reach this many semitones beyond their root), `maxStretch` (4: the
farthest a zone plays from its root while any velocity layer has a closer sample), `quality` (6 = Ogg q0.6),
`velOrder` (layer names soft -> loud, else numeric order), `layerDb` (imposed layer levels for sources that were
normalised per layer, e.g. `{ "2": -15, "3": -7.5, "4": 0 }`), `tune` (`"correct"` default / `"keep"`: keep the natural
tuning, fix only > 30 cents / `"name"`: inharmonic bells - the names are the roots, no pitch correction),
`pitchMethod` (`"spectrum"`: struck bars and tines - the fine tune comes from the fundamental's spectral peak, +-0.75
semitone, and a key between two semitones goes to the nearest one; YIN is pulled by their inharmonic partials), `pitchFrom`
(s, where the pitch of a one-shot is measured; 0.5 for pianos - the attack glides sharp; set explicitly it also applies to
loops, which otherwise are measured over the whole sustain), `pitchSpan` (s, length of the pitch analysis; 0 = 0.35 s for
one-shots / the sustain for loops; voices use 0.15 + 0.8 s), `pitchMedian` (`"dense"`: YIN frames every half window
(7-64) and the median of the clearly periodic ones - voices; default: the median of 7 frames). Map-level keys: `report` (report file name, default
`build_report.txt`) and `credits` (a line such as "Vocals: ..." merged into `library.json`'s `credits`; a later build of
the same map replaces its line). A second map, `tools/vault/library_map_vocals.json`, adds the vocal instruments (prepared
by `tools/vault/vocal_prep.py`; see `docs/VAULT_VOCAL_SOURCES.md`).

Per file: read; resample to 44.1 kHz (windowed sinc) if needed; identical channels -> mono; mean + 5 Hz DC removal; trim to
the first / last sample above `silenceDb` of the peak with a pre-roll before the onset (-40 dB) and a raised-cosine fade-in
over it; length cap with a tail fade; YIN (threshold 0.15, parabolic interpolation, median of 7 frames - `pitchMedian`
"dense": of the frames every half window, the clearly periodic ones -, search 0.45-2.2 x the named pitch) -> the naming
convention offset per articulation (VSCO / VCSL names are an octave off: corrected and reported; `octave` / `pitch: detect`
sources skip this), a single file whose YIN pitch disagrees with its name gets a second opinion from a harmonic-sum
spectrum (+-2.5 semitones, harmonics 1-5): the root moves only when both agree within 2 semitones, otherwise the name stays
and the spectrum's detune is used (YIN alone mis-hears bright trebles and weak bass fundamentals), the residual detune
written as `fine` (round robins are renumbered 0..n-1 per key / layer cell, so a corrected root never overwrites another
take's output file); the noise floor (RMS of the lead-in before the onset vs the peak, `noise` in the zone, informative);
loudness (BS.1770 K-weighting, loudest 400 ms); loops for `loop` articulations (rising zero crossings of the mid in the
stable sustain, normalized cross-correlation of [-xfade, +12 ms] around both points minus level mismatch, bonus for
length; >= 300 ms). Per articulation: velocity layers matched to a smoothed per-register loudness curve of the top layer
plus the layer offset (natural median spacing kept, at least 2.5 dB apart, whole range <= 18 dB, or `layerDb`); takes more
than 12 dB off their layer (vs the top layer of the same note) are dropped as broken; release samples set `levelDb` under
their note; round robins renumbered per key / layer cell (no file collisions); key ranges: midpoints between a layer's
roots, but a zone plays at most `maxStretch` semitones from its root where another velocity layer has a closer sample
(the sampler falls back to the nearest velocity layer there), only keys no layer reaches within `maxStretch` keep the
longer stretch (reported); one gain normalises the instrument peak to `peakDb`; looped files are cut 300 ms after the
loop end; encoded OggVorbis q 0.6. The builder finally reads its own output with `vault::SampleLibrary`.

`-qc` measures the built library as the plugin loads it (tuning per zone, start clicks, loop wraps, velocity-layer order,
key gaps / stretch, size; `-demo <dir>`: a 2 s C5 note of every instrument as `<id>.wav`) and writes `qc_report.txt` +
`qc.json` next to `library.json`; `make_instruments_doc.py` turns them into `docs/VAULT_INSTRUMENTS.md`.

`-retune [-only ids] [-dry] [-emit Source/vault/LibraryTuningData.cpp]` (tools/vault/Retune.h; also runs after every
build for the instruments built) tunes the zones AGAINST EACH OTHER (owner report item 0: "B4 sounds like C5"). The
builder tunes every file on its own; where two zones meet on the keyboard their own errors add up (a Kawai A#0 the builder
could not measure was 75 cents flat - next to its neighbour it played almost the same pitch; a male-voice take 85 cents
flat; piano bass samples 15 cents sharp next to 15 flat made 70-cent semitones). The pass measures what the pitch test
measures: every pair of zones that meets (zone i at key k next to zone j at key k + 1, or both at key k in another
velocity layer / round robin) is PLAYED at those keys (resampled from the zone start the way the sampler plays it) and
compared with the pitch test's own measurements (`vaultpitch::compareNotes`: YIN, the log-spectrum cross-correlation on
each note's window and on the same window, the strongest partial, the YIN tracks, the fundamental partial); the interval
most of them agree on counts (`Interval::consensus`), measured early (from the attack) and late (from 0.4 s, a sustained
sample is tuned on its average, not on its first moments). Bars / tines (`pitchCheck: spectrum`) use the fundamental
partial: their upper modes are inharmonic and differ from bar to bar (a vibraphone's third mode moves 10.8x -> 10.4x
between two bars and pulls a whole-spectrum comparison 50 cents off). YIN at the zone's root anchors the absolute pitch
for `pitchCheck: yin` articulations (the median YIN deviation is taken out every round, and no zone moves more than 10
cents away from its own YIN pitch at any key it was measured at, unless three or more comparisons agree: an unsteady
neighbour cannot pull a zone out of tune, an attack that misleads YIN cannot keep one there),
the builder's tuning (the mean change is 0) for the others; a robust least-squares fit gives every zone's deviation
(prior: the tuning as built; a zone that meets only one other zone moves half way); measure -> fit -> apply runs three
rounds (the zones are re-measured at their new tuning). The pass always starts from the root / fine in library.json
(never the loader's patched values), so runs do not drift. `fine` (and `root` past +-50 cents) is corrected and the
articulation gets `"retuned": true`.
`retune_report.txt` lists the moves; `RETUNE_DEBUG=<instrument/articulation>` prints every pair of every round.
`-emit` writes the plugin's built-in tuning patch (`Source/vault/LibraryTuning.h`, generated `LibraryTuningData.cpp`):
a library.json from before the pass (no `retuned` flag) is corrected at load time (`SampleLibrary::parseIndex`, only zones
that still hold the old root / fine), so an installed library plays in tune with the new plugin and no new download.
Run it on the ORIGINAL library.json (`library.before-retune.json`): the patch is keyed on the values the owner has
installed.

Hand fixes after the pass (`Source/vault/LibraryZoneFixes.cpp`, applied by the loader to EVERY library.json, each with
its reason): `ZoneRangeFix` takes a zone out or changes its key / velocity range, keyed on the file and its range as built
(a contrabass spiccato C3 take that holds a loud E3 over the C - the keys played a C-E dyad - is out, its soft take covers
the loud velocities; the loud horn staccato covers the low keys that played the soft take 20 cents off); `ZoneTunePin`
sets a zone's root / fine, keyed on the file (a vintage-trombone buzz take labelled D5 that sounds D#5; takes whose pitch
sits 15-60 cents off their neighbours: contrabass tremolo, tenor sax, ocarina, mbira, dan tranh vibrato, ...). They were
found with `VaultTests -pitch --library` and the bank run, and fitted as "the smallest move that lets every neighbour
step of the take measure one semitone" in the dry articulation AND the presets that failed on it.

### The full library (`/home/user/cv-library`)

Every musically useful pitched instrument and articulation of both sources is mapped: **64 instruments, 146
articulations, 5274 zones, 483 MB on disk** (OggVorbis q0.6; 4.3 GB if everything were decoded at once - the plugin
loads only what presets use). The instrument list
with articulations, ranges, sizes, the QC numbers per articulation, the C5 test-note measurements and the curation notes
(what was left out and why) are in **`docs/VAULT_INSTRUMENTS.md`**. The ids of the first build are unchanged
(`piano.grand.concert`, `strings.violins.ensemble`, `winds.flute.concert`, `mallets.marimba`); new articulations were
added to them (`pedal`, `tremolo`, `pizzicato`, `spiccato`, `expressive`, `staccato`).

## Models (`Source/vault/models/ModelVoice.h`)

```cpp
namespace vault::models {
constexpr int maxModelParams = 32;
class ModelVoice {                       // ONE monophonic voice
    virtual void prepare (double sampleRate, int maxBlockSize) = 0;           // non-audio thread, may allocate
    virtual void setParams (const float* values, int numParams) = 0;          // audio thread, ModelInfo::params order, clamped
    virtual void noteOn (int midiNote, float velocity, float freqHz) = 0;     // freq includes the part transpose
    virtual void noteOff (float releaseVelocity) = 0;
    virtual void setPitch (float freqHz) = 0;                                 // bend / vibrato, at most once per 128 samples
    virtual void render (float* left, float* right, int numSamples) = 0;      // ADD into the buffers
    virtual void kill() = 0;
    virtual bool isActive() const = 0;
};
struct ModelInfo { juce::String id, name; std::vector<ParamInfo> params; int maxVoices = 16; std::function<std::unique_ptr<ModelVoice>()> create; };
class ModelRegistry { static ModelRegistry& get(); void add (ModelInfo); int indexOf (const juce::String&) const;
                      const ModelInfo* find (const juce::String&) const; const ModelInfo& at (int) const; int size() const; juce::StringArray ids() const; };
void registerBuiltinModels (ModelRegistry&);   // ModelRegistry.cpp: one r.add (...) line per model
}
```

Rules: nothing but `prepare()` may allocate, lock or do I/O; render ADDS; at C5 velocity 1.0 peak about -12..-6 dBFS,
never above -1 dBFS; deterministic. The engine does voice allocation (free voice, else the oldest is killed), sustain
pedal, pitch bend (EVOLVE key `bendRange` of the part, default 2), MOTION vibrato, TONE low-pass, level / pan / FX.
Example: `sine` (`m.decay`, `m.release`, `m.level`). A preset uses a model with `kind = model`, `source = "<id>"`,
`values = { { "m.<param>", v } ... }`.

Built-in physical models (ids for `VaultPart::source`; parameters, articulations and measurements in
`docs/VAULT_MODELS.md`): `guitar.acoustic` `guitar.nylon` `guitar.banjo` `guitar.twelve` `guitar.electric` `guitar.crunch`
`guitar.jazz` `bass.finger` `bass.pick` `bass.slap` `bass.fretless` `ep.tine.mellow` `ep.tine.bell` `ep.reed` `ep.clav`
`keys.toy` `voice.female` `voice.male` `voice.choir` `voice.pad`. Articulations are parameters (`m.artic`: strings 0 pluck /
1 palm mute / 2 harmonic; voices 0 sung / 1 "ha" / 2 breathy / 3 whisper), vowels `m.vowel` (0 ah, 1 oh, 2 oo, 3 ee).

Voice models and pitch (owner report item 0): a voice model sings a range, like a sampled instrument (`lowestNote` /
`highestNote` in VoiceModels.cpp, MIDI with C5 = 60: female 36..89, male and choir 28..127, vocal pad 33..127; outside it the
voice stays silent - a glottal model at 35 Hz or a whisper at 4 kHz has no clear pitch left). The choir / pad section's
pitch scatter (`m.detune`, SD in cents) puts the singers on the quantiles of a normal distribution, shuffled per note (a
symmetric spread, densest at the note; random Gaussian draws left every key a few cents off in its own direction, often
as two clusters), and the spread and vibrato narrow above ~G4 (400 Hz / f0, at least x0.25: up there the fundamental sits
on the first formant, whose slope leans a wide cluster sharp or flat). The solo voices' slow drift is +-2 cents (was
+-6) and the jitter ~2 cents.

## API for preset generators / tools (`Source/vault/VaultEngine.h`)

```cpp
// Offline, no GUI / plugin needed, thread-safe (each call has its own engine). Note on at 0, note off at 70 % of `seconds`.
// Sample instruments are decoded (synchronously) and model voices made before the first sample.
static juce::AudioBuffer<float> VaultEngine::render (const VaultPreset&, int midiNote, float velocity, double seconds, double sampleRate);
static juce::AudioBuffer<float> VaultEngine::render (const VaultPreset&, int midiNote, float velocity, double seconds, double sampleRate, const float* macros);
static juce::AudioBuffer<float> VaultEngine::renderMidi (const VaultPreset&, const juce::MidiBuffer&, int numSamples, double sampleRate, const float* macros = nullptr);
static juce::AudioBuffer<float> VaultEngine::renderMidi (const VaultPreset&, const juce::MidiBuffer&, int numSamples, double sampleRate,
                                                         const float* macros, std::shared_ptr<vault::SampleLibrary> library);   // explicit library
static float VaultEngine::measureLoudnessDb (const juce::AudioBuffer<float>&, double sampleRate);   // loudest 400 ms RMS, dBFS
static VaultCompiledPreset VaultEngine::compile (const VaultPreset&, std::shared_ptr<vault::SampleLibrary> library = nullptr);  // nullptr = getShared()
```

Use `vault::homeNote` (60 = C5) as the design / QC note for every category. Rendering is deterministic.

Engine (real-time) API, used by the processor:

```cpp
class VaultEngine {
    void prepare (double sampleRate, int maxBlockSize, int numPartsToPrepare = vault::maxParts, bool crossfadeSlot = true);
    void releaseResources();  bool isPrepared() const;
    void setPreset (const VaultCompiledPreset& p, bool instant = false);   // audio thread, no allocation, 40 ms crossfade
    void setMacro (int index, float value01);  float getMacro (int index) const;  void setVolume (float linearGain);
    void process (juce::AudioBuffer<float>&, const juce::MidiBuffer&, const VaultTransport& = VaultTransport());
    void allNotesOff (bool hard);
    void serviceResources();     // NON-audio thread: create the model voice pools current presets asked for
    bool isLoading() const;      // a part of the current preset waits for its instrument / model voices
};
```

## API for the editor (`Source/vault/VaultProcessor.h`)

Unchanged from v1 plus:

```cpp
void previewNote (int midiNote = vault::homeNote, float velocity = 0.8f, double seconds = 0.9);
bool libraryFound() const;  juce::File libraryPath() const;  vault::SampleLibrary* sampleLibrary() const;
bool isLoading() const;          // the current preset waits for samples / model voices
bool isMissingSamples() const;   // the current preset needs an instrument that is not installed / failed
```

The bank keeps the shared library alive (`Bank::library`), presets are compiled against it once per process.
`maxPresets` is still 5000 (the `presetIndex` parameter range 0..4999).

## Editor UI (`VaultEditor`) - v3

The approved v3 mockup in JUCE: pick a category, pick a sound, it already sounds finished; tweak it on the MAIN page;
everything else is in the ADVANCED tabs. Base size 1180 x 760, scaled 80-200 % as before (sizes, settings file, C5
naming, library-missing card and loading state unchanged from v2; details below). The keyboard rules changed in v3 (see
**Computer keyboard and focus** below: the v2 rule "the editor never uses a key" is gone).

* **Top bar**: chrome logo | MAIN | ADVANCED › LAYERS, FX, MOD, ARP · GATE (every tab switches on the mouse-down; ADVANCED
  is a clickable group header that opens LAYERS and is lit, with a thin line under the group, while an ADVANCED page
  shows) | quick toggles FX / ARP / GATE (LED; the host
  switches `fxOn`, `arpOn`, `gateOn`; a small lock when the section is kept while browsing) | size menu (`100%`; it lists
  only the sizes whose window fits the screen the editor is on - `scaleChoices()` -, opens as a desktop menu at the button's
  screen area, so JUCE keeps it on the screen; `setUiScale` never makes the window larger than the screen and the corner
  drag stops at the screen: `limitToDisplay`, re-checked about once a second when the window moves to another screen) |
  settings (preview on click, reset the MAIN knobs, revert the sound, library / bank info).
* **MAIN** (Nexus-style): the centre "screen" - ◀ search ▶, Favorites, Recent; All Sounds + 16 categories with counts;
  the list (name, "3 layers", heart; a `juce::ListBox`). Search: every word must match; the best name matches come first
  (the exact name, names starting with the search, a name word starting with it, names containing it, then matches in
  instruments / category / labels - `VaultBrowserModel::nameScore`), the category counts follow the results (All Sounds =
  every result) and the field shows how many it found ("24 sounds" / "none"). The category is kept in
  `VaultProcessor::editorMemory` (saved with the project as `uiCategory`): a reopened window shows the same list, scrolled
  so the playing sound is in the middle and highlighted (also on the very first open). The current sound: name ("edited"
  after ANY change - the sound, a MAIN knob off "as designed", a macro moved, a switch: `hasEdits()`; a click on the badge
  offers "Revert to the preset" = `revertAll()`; clicking the same sound again keeps everything), a spinner +
  "Loading…" while its samples load), its composition ("Bells & Mallets · Tubular Bells + Vibraphone + Sine Bass"),
  More like this (same category + main instrument), Drag WAV (the current sound with its edits, C5, 2 s), heart. Status
  messages replace the composition line for 4 s. Library missing / instrument missing: the calm card over the bottom of
  the list (title, "Its synth layers still play.", the folder looked in).
  Left: FILTER · AMP (cutoff, reso, attack, release, env amount, glide) + MOTION (vibrato, tremolo; 0 = off). Right:
  EFFECTS (reverb, delay, chorus, drive, width, eq tilt; the LED / a click on the name switches the chain slot on / off,
  adding a default slot when the sound has none; an effect that is off reads "off" and sits at the minimum - turning it
  switches its slot on again; with every FX bypassed the knobs are disabled) + OUTPUT (volume, pan). Under the screen: the 4 SOUND MACROS named per
  preset (`macroName()`; an unused macro shows "—" and is disabled).
  **Every MAIN knob shows the effective value of what it moves** (`vui::val::quickPosition / quickText`,
  `Source/vault/ui/VaultValues.h`), not the host parameter: the reverb knob of a sound whose reverb mix is 14 % sits at
  14 % and reads "14% Plate" (sound macros - most factory presets set the reverb / delay mix with one - and the MAIN knob
  included), CUTOFF / RESO / ATTACK / RELEASE / ENV AMT / GLIDE show the effective value of the first layer they act on
  (synth / sampled first; "2.4 kHz", "0.3 s", "natural" for a sample's own envelope; ENV AMT / GLIDE are "—" without a
  synth layer), vibrato / tremolo / pan their amount. A small ice tick on the scale marks "as designed"; double-click
  goes back there (the parameter to neutral). Turning a knob solves the host parameter (still the offset of
  `vault::quickInfo`, 0.5 = as designed) for the position. The knobs follow preset loads, host automation and macro moves.
  Clicking the sound that is already loaded (list row, Return in the search) keeps every edit - it only scrolls to it and
  auditions it; another sound starts from its bank preset as before.
* **LAYERS**: one card per layer (3 columns, 4 for 4 layers): power (level really 0 - a sound macro that sets the level
  gets its range to 0 too - / back as it was), "A · Instrument", kind
  pill; LEVEL / PAN / OCTAVE / FINE; AMP ENVELOPE (graph + A D S R; sampled layers: 0 = the sample's own); FILTER
  (LP 12 / LP 24 / HP / BP for synth layers, LP / HP / BP for sampled ones) + CUTOFF / RESO / ENV (sampled: VEL) / KEY. A
  modelled layer shows its model's first 8 parameters instead. VOICE row: POLY / MONO / LEGATO, GLIDE, UNISON, DETUNE (synth
  layers), VEL SENS, BEND (synth + sampled), Reset to preset (`revertAll()`). Fewer than 3 layers: one "+ Add layer"
  column next to them, sharing the page (one layer = half the page; a copy of layer A - as it is, an octave up or down;
  `LayersPage::addLayer`), right-click on the last
  card removes it (`removeLastLayer`, its macro / modulation targets go with it). LEVEL, PAN, CUTOFF, RESO,
  ENV (synth), A, R and GLIDE show the effective value (`vui::val::layerValue`: the synth patch's EVOLVE macros, the sound
  macros - `layer<n>.level` is set by a macro in a quarter of the factory bank -, the MAIN knobs); turning one makes that
  the value (`setLayerValue`: an absolute macro target gets its min / max moved so the macro keeps working around the new
  value; otherwise the stored value is solved, the MAIN knobs stay). Double-click = the bank preset's stored value and
  macro ranges.
* **FX**: the 8 slots in a 4 x 2 grid. Card: drag grip (drop on another card = move there), power, type, ▾ menu
  (variant - Hall / Plate / ..., Tape / Tube ..., Ping-pong / Straight / Tape ... -, change type, move, reset, remove), its
  3 main knobs (`FxPage::mainParams`, 44 px) and under them up to 3 more of its continuous parameters (36 px; the cards fill
  the page in two rows), every value always shown ("off" for a Mono Below of 0 Hz). Every empty slot: + Add effect (it goes
  after the last effect; <= 2 of a type). FX preset menu (as designed, none,
  Warm Room, Wide Hall, Tape Echo, Lo-Fi Room, Shimmer Space, Punchy, Dreamy Chorus - all moderate), Bypass all (`fxOn`),
  Keep while browsing (`setLock (Section::fx)`). The slot knobs show the value the engine uses (`vui::val::fxValue`: sound
  macros - `fx.reverb.mix` / `.decay` are macro targets in 90 % of the factory bank -, classic macros, the MAIN knob of the
  first slot of a type); turning one sets it (`setFxValue`: a macro target's range is moved, the MAIN knob of that slot
  is folded in - set to neutral - so MAIN and FX show the same number). Double-click = the bank preset's value.
* **MOD**: 2 LFO cards - power (depth really 0 - also the sound macros that set `lfo<n>.depth` - / back), shape segments, the wave (with the live phase while notes play), RATE
  (divisions when synced, 0.05-20 Hz free), DELAY, FADE, PHASE, SYNC / HZ, RETRIG / FREE, DESTINATIONS chips ("● Cutoff ·
  +1.00 oct": drag up / down for the amount, double-click removes, right-click invert / remove) + Add (target menu). MOD
  WHEEL · VELOCITY · AFTERTOUCH: one bipolar knob per routing, grouped under MOD WHEEL / VELOCITY / AFTERTOUCH captions,
  the amount always shown ("+1.20 oct"), + Wheel / + Velocity / + Aftertouch. Targets are named by what they move: a slot
  target `fx<n>.<param>` reads "Reverb Mix" (the effect in that slot; "Reverb 2 Mix" for a second one - `mod::longName`).
  **Right-click any knob** (MAIN, LAYERS, FX, MOD rate, ARP gate, gate depth / smooth / mix) -> "Modulate with…" LFO 1,
  LFO 2, Mod Wheel, Velocity, Aftertouch (add with a default amount or remove), Remove all modulation. A modulated knob
  draws its value arc green and a thin green ring for the summed amount (`vui::mod::show`); the menu's choice runs
  `vui::mod::apply`, which edits the sound and refreshes the open page at once.
* **ARP · GATE**: ARPEGGIATOR - power (`arpOn`), Pattern menu (Init, the sound's own pattern, Start from: Accented 16ths,
  Offbeat 8ths, Gated 8ths, Octave Bounce, 3-3-2 Pulse, Slide Steps - rhythm only, no genre names; all on, accent on the
  beats, velocity ramps, random velocities / octaves, lengths back to full, no slides, shift, reverse, double the length),
  Latch (Off / On / Add), Keep while browsing, Drag MIDI (whole bars, >= 2, one full pattern; `writeArpMidiFile`). RATE
  (every rate, labelled like the gate's: "1/16T", "1/8D"; `ArpGatePage::rateKnobDivisions` - ARP RATE and GATE RATE step
  through the same list, so one rate sits at the same angle on both) + Straight / Triplet / Dotted (from 1/2 on), OCTAVES, GATE, SWING (50-75 %), STEPS (1-32); Order, Octaves (mode), Velocity
  (source), Retrigger dropdowns. 32-step lanes in groups of 4: VELOCITY (draw), LENGTH (draw), ON / TIE (click on / off,
  right-click tie), SLIDE (click), OCTAVE (click up, right-click down); steps past STEPS are dimmed, the running step is
  outlined. GATE - power (`gateOn`), Pattern menu (Init, the sound's own, all open, off-beats, quarter pulses, invert,
  random, shift, double the length), Keep while browsing, RATE (every division), STEPS (1-32), DEPTH, SMOOTH, SWING, MIX,
  32 steps (click toggles, drag draws levels).
* **Look - style A "Machined"** (owner approved, `docs/qa/design/README.md`, `knob-options.html` function `A`):
  **Knob** (`vui::Knob`, `drawKnobBody`): the reference geometry at 58 px scaled to the knob (u = size / 58) - 11 scale
  ticks (#3A4558, every 5th longer), a dark track (#1B2330) with a 2.5 px ice value ring (#9FC3FF; green when modulated,
  grey when its effect is off), a soft drop shadow, a bevelled edge (#7C889C -> #0D1118) round the machined cap (radial
  #5A6578 -> #2B3341 -> #151A22 from the upper left), a white pointer (#F2F5FA); the ice "as designed" tick (`markValue`)
  and the green modulation arc sit on the scale. LABEL in letter-spaced small caps (#A9B6CC) under it, the value under that
  (dimmer; never clipped - narrowed to the knob's width) always on MAIN / FX / MOD routings (`valueAlways`), elsewhere on
  hover / drag. Sizes: 50 MAIN, 46 macros, 38 / 34 LAYERS, 44 / 36 FX, 36 LFO, 44 routings, 40 ARP.
  **Surfaces** (`VaultControls.h`): `drawPanel` (an anodised plate: dark gradient, lit top edge, outline, soft shadow),
  `drawButtonFace` (raised dark-metal keys for pills, dropdowns, toggles, tabs, ◀ ▶, gear; an ice-lit edge when on),
  `drawWell` (recessed tracks: segment switches, the search field), `drawLed` (green LED with glow; power buttons are round
  push buttons with the LED in the middle, the top-bar toggles carry one); the chosen tab is a raised key with an ice LED
  strip; the top bar is a brushed faceplate with a bevelled edge.
  **On-screen keyboard** (`vui::VaultKeyboard`): realistic white keys (#E8ECF2 -> #D3D9E2 -> #AEB7C4, a darker 3 px bottom
  lip #98A2B0, 1 px gaps), black keys with depth, a front bevel and a shadow on the white keys, a felt-strip shadow at
  the top. The keys that SOUND are ice blue (#BFD6FF -> #8DB2F0) - `VaultEngine::soundingNotes` polled 30x per second:
  the arp's notes while it runs, latched ones too, else the played keys; a held key the arp is not playing gets a thin ice
  edge; nothing is lit at rest (no permanent C5). Drag up / right (shift = fine), mouse wheel, double-click = the bank
  preset's value (`onReset` where that is more than a position), right-click = menu; never takes the keyboard.
  Labels that do not fit use a short form (OCT, ATK ...). Host parameters through `vui::KnobAttachment`
  (`juce::ParameterAttachment`: gestures, host automation moves the knob); sound edits through `VaultProcessor::editSound`.
  The pages read `currentSound()`; ~10x per second the view compares the sound / switches / locks with what it showed
  and refreshes only when something changed elsewhere (host, state restore).
* **Computer keyboard and focus** (`vui::ComputerKeys`, `VaultEditor::keyPressed / keyStateChanged`): the editor takes the
  keyboard when the user clicks into the plugin (never on open - until then the DAW's own typing keyboard works), and then
  the computer keyboard plays the sound in FL Studio's layout: lower row Z S X D C V G B H N J M , L . ; / = C4..E5
  (Z = MIDI 48), upper row Q 2 W 3 E R 5 T 6 Y 7 U I 9 O 0 P [ = ] = C5..G6 (Q = C5 = MIDI 60), velocity 100, note-off when
  the key goes up (polled with `KeyPress::isKeyCurrentlyDown` on every key event, so the physical key counts, whatever the
  keyboard layout types), Left / Right arrow = octave down / up (-3..+3, held notes end), Up / Down = previous / next sound,
  Tab is swallowed. The notes go into `getKeyboardState()`: they play and light the on-screen keyboard, which marks the
  computer-key range with a thin ice line and "C4 Z" / "C5 Q" on its C keys (tooltip and a status line on the first
  focus / an octave change explain the keys). Every note stops when the editor loses the keyboard. No child component
  keeps the keyboard (`setWantsKeyboardFocus (false)` + `setMouseClickGrabsKeyboardFocus (false)` everywhere, the resize
  corner included; after any click the editor's mouse listener gives it back to the editor). The search box
  (`vui::SearchField`) does not even want the keyboard until it is clicked - so window activation, focus traversal and Tab
  can never land there -, then letters type into it (and play nothing); Return, Escape or a click anywhere else hand the
  keyboard back to the editor and disarm the box.
* Owner report 1 re-verification (`xvfb-run -a VaultTests -report1`, tests/VaultReport1Verify.inc): the editor on a real
  desktop window, driven through its peer (mouse clicks and key presses as the OS delivers them, the window focused),
  items 1-16 of docs/qa/FL_TEST_REPORT_1.md and the MUST NOT BREAK list (FX on / off / drag / MAIN sync, MOD LFO, the
  arp's 12 orders at 140 BPM host tempo, GATE, Favorites / Recent / arrows / drag WAV, settings after reopening the
  window and after a saved project). Under xvfb the WM atoms are interned on a kept-open display first (Xvfb resets
  its atoms when the last client disconnects, and JUCE's peer then failed with BadAtom).
* Checks (`xvfb-run -a VaultTests -uitest`; the owner-report part alone: `-uifix [keys|pages|values|render|more]`; `more`:
  search ranking + counts over 80 searches, the list opening on the playing sound and the kept category (state too),
  Return / Escape handing the keyboard back, 3 MAIN knobs edited -> "edited" kept by a second click and cleared by Revert,
  the green ring at once on MAIN / LAYERS / FX / MOD, empty FX slots, "off" / "Reverb Mix" texts, the keyboard lighting
  only what sounds (latched arp included), + Add layer, host parameter text on and off the message thread, the size menu
  on a 1366 x 728 screen; every knob reading "off" / "0%" sits at its minimum): the
  computer keys (layout, note-on / off, octave, focus loss, a key playing the processor), focus (window activation picks
  the editor, a click into the search box arms it, a click on the FX toggle takes it back and the next key plays a note
  instead of typing), every tab / ADVANCED switching on a click, the same preset clicked again keeping its edits, 50 random
  presets with random MAIN knobs / macros: every MAIN / FX knob == the running engine's FX chain, LAYERS level / pan == the
  effective values, knob position == text, knobs moving with the preset; the FX reverb MIX of a macro-driven sound turned
  to 42 % plays 42 % and the MAIN knob shows it; renders (two identical processors, one knob moved): every LAYERS / VOICE /
  FX mix control of a synth, a sampled and a modelled sound changes the sound. Everything from v2, plus every page at 80 / 100 %: smallest text, no two
  controls overlapping, nothing outside its parent, every knob label fitting its knob - with a 3-layer and a 4-layer sound.
  Snapshots: `VaultTests -shot out.png "main,rich,demo,page:fx,scale:80"` (`page:main|layers|fx|mod|arp`, `rich` = the
  3-layer sound with the longest FX chain, `four` = a 4-layer sound with modelled layers, `demo` = LFO / wheel / velocity
  routings, arp / gate lanes and a tremolo so the green rings and lanes show).

## Editor UI (`VaultEditor`) - v2 changes (where v3 differs, v3 wins: no info panel, waveform thumbnail, LAYERS chips, RANDOM or status bar any more)

* Categories: the 16 v2 categories with the real counts. Filters: LAYERS (All 1 2 3 4) + Clear filters (STYLE / CHARACTER
  rows and tag pills removed everywhere).
* **Instrument names** (`VaultBrowserModel::instrumentName (part, library)`): a sample layer is named by its library
  instrument ("Concert Grand", `InstrumentDesc::name`); without the library from its id (`nameFromId`:
  "strings.violins.ensemble" -> "Ensemble Violins"); a model layer by its `ModelInfo::name`; a synth layer by its label.
  `mainInstruments (preset)`: layer A first, then the others by level, no duplicates.
* List info column: the main instrument(s) of the preset ("Concert Grand · Violin Section"), as many as fit the column -
  further ones are left out whole, never cut with "..." (`PresetList::infoText`). Search matches name, category,
  instrument names, layer labels, "sampled" / "modelled", instrument ids and articulations.
* Layer rows (info panel): letter, a small kind icon (sample = waveform bars, model = sine, synth = saw), the instrument /
  model / synth-layer name, the layer's label in the dim colour when it says more ("Felt"), transpose, level. A sample
  layer of the shown sound that is still loading gets a turning spinner instead of the icon; one that is not installed
  says "not installed".
* Info panel pills: Sampled / Modelled / Synth, "Loading…" (with a spinner) while the current sound's samples load,
  "Plays at C5". Loading never blocks the UI: picking a sound only pins its instruments (the library decodes them on its
  thread), the sound's sampled layers start once they are in (`VaultInfoPanel::notice()`, `partState()`). The waveform
  thumbnail of a sampled sound is drawn once its instruments are in RAM (the thumbnail thread never decodes samples), DRAG
  WAV waits until loading is done ("Samples are still loading…").
* **Library missing**: when the shown sound needs samples and no library is installed, a calm card replaces the pills and
  the waveform: "Sound library not found — run the CHROME VAULT installer", "Its synth layers still play." / "Synth sounds
  keep playing.", and the folder looked in (`libraryPath()`, the first search location). Synth-only sounds show nothing
  and play normally. An installed library that lacks an instrument: "This sound needs a newer sound library — run the
  latest CHROME VAULT installer".
* Preview on click, DRAG WAV (file "<name> C5.wav") and thumbnails: MIDI 60 for every category.
* Keyboard: `setOctaveForMiddleC (5)`, labels C3 C4 **C5** (bold, blue, dot) C6.
* **Keyboard focus** (v2 only - v3 replaced it, see "Computer keyboard and focus" above): the editor never uses a key. Nothing takes the keyboard on click except the search box
  (`setWantsKeyboardFocus (false)` + `setMouseClickGrabsKeyboardFocus (false)` on every component, list rows included as
  the list creates them; the editor itself too); Return, Escape (also clears the text) and a click anywhere else hand the
  keyboard back (`giveFocusBack()`: `giveAwayKeyboardFocus` + `unfocusAllComponents`), so the DAW's computer-keyboard piano
  and shortcuts always work. The PC-key note playing (A-L, Z / X) and arrow / Page / Home / End key browsing are removed;
  ◀ ▶ and Up / Down while typing in the search box still step. `EDITOR_WANTS_KEYBOARD_FOCUS` stays TRUE (macOS needs it
  for typing into the search box).
* Status bar: "Click a sound to hear it at C5 · ◀ ▶ next sound · Instant load", or for the current sound "Loading
  samples… the sound plays as soon as they are in" / "Sound library not found — synth sounds play, sampled layers are
  silent" / "This sound needs a newer sound library — its synth layers play".

### Layout, sizes, settings (unchanged from v1)

Laid out at the base size **1180 x 760** and scaled as one piece with an affine transform (vector drawing, crisp at any
scale): 80 / 90 / 100 / 115 / 130 / 150 / 175 / 200 % from the scale pill in the status bar (sizes that would not fit
the screen are greyed out), or by dragging the corner (aspect kept). Minimum 80 % = 944 x 608.
First open: the largest preset up to 100 % (never more, even on 4K) that fits 85 % of the screen's usable area (the
display showing the editor, else the main display): 1366 x 768 laptop -> 80 %, 1920 x 1080 and up -> 100 %. Afterwards the
user's choice is remembered in the settings file and restored, shrunk to the largest preset that fits the screen it opens
on (200 % chosen on 4K opens at 90 % on a 1366 x 768 laptop; the 200 % choice stays saved).
Readability: no text is smaller than 14 px in the base layout (11.2 px at 80 %), text colours are never darker than
#8593AB, primary text #E3E8F0; `VaultTests -uitest` paints the editor at 80 / 100 / 150 / 200 % and checks the smallest
font, and that every bank name and info line fits its list column without "...".

* Top bar: chrome logo, search (every word must match; Up / Down step while typing, Esc clears + hands the keyboard back,
  Return loads the first hit if needed + hands the keyboard back), current sound with ◀ ▶ (wrap), RANDOM, RECENT (last 30),
  FAVORITES.
* Left: All Sounds + the 16 categories with the real counts of the loaded bank.
* Centre: LAYERS chips + Clear filters; the list is a `juce::ListBox` (only visible rows exist). Row: selected dot, name,
  info line, layer badge, heart. Single click = load + a short audition note at C5 when "Preview on click" is on.
* Right: name, category, pills, waveform thumbnail (C5, background thread, cached, follows the macros), LAYERS stack,
  DRAG WAV (current sound with the current macros, C5, 2 s, 44.1 kHz 24-bit stereo, temp folder `CHROME VAULT/`), the
  macro knobs TONE SPACE DRIVE MOTION and VOLUME (APVTS attachments).
* Bottom: status bar, Preview on click, scale; 4-octave keyboard C3-B6 (FL naming) feeding `getKeyboardState()`.
* Settings (`VaultSettings`, one shared object per process, other open windows follow favorite changes):
  `juce::PropertiesFile` "CHROME VAULT.settings" in `userApplicationDataDirectory/TrapWin/CHROME VAULT/`
  (macOS: `~/Library/Application Support/TrapWin/CHROME VAULT/`): `favorites` and `recent` (preset ids, one per line),
  `uiScale`, `previewOnClick`.
* Palette (from the approved mockup): ink `#0B0F16`, panels `#111722` / `#151C28`, lines `#222B3A`, text `#E3E8F0`,
  secondary `#A9B6CC`, tertiary `#8593AB` (darkest colour used for text), ice `#9FC3FF` / `#7FA6E8` / deep `#4676C4`
  only for selection, focus and active chips. Fonts: JUCE default sans, bold weights.
* Check it: `xvfb-run -a VaultTests -uitest` (behaviour) and `xvfb-run -a VaultTests -shot out.png [state]` (PNG of the
  real UI; states: `main` (default), `scale:70`, `search:dark bell`, `cat:<n>`, `layers:<n>`,
  `favs`, `recent`, `empty`, `keys`, `pick:<preset name>`, `nowait` (paint at once, while samples load), comma-separated).
  Without a real bank both use a generated 5,000-preset demo bank. v2 look: `VaultTests -writeuidemo /tmp/uidemo.cvb`
  writes 82 v2 presets (sampled / model / synth layers on the library instruments, one instrument deliberately missing:
  "Celesta"), then e.g. `CHROME_VAULT_BANK=/tmp/uidemo.cvb CHROME_VAULT_LIBRARY=/home/user/cv-library xvfb-run -a VaultTests
  -shot out.png "main,pick:Full Orchestra,nowait"` (loading) or with `CHROME_VAULT_LIBRARY=/nonexistent` (library missing).

State (`getStateInformation`): XML `CHROMEVAULT` with `presetId` (restored by id first, so bank re-ordering is safe),
`presetIndex`, `macro1..4`, `volume`.

## Build and test

```
cmake -B build-vault -DCMAKE_BUILD_TYPE=Release [-DKK_JUCE_DIR=/path/to/JUCE]
cmake --build build-vault --target ChromeVault_VST3 ChromeVault_Standalone VaultTests VaultBankGen VaultLibBuild KeysKilla_VST3 KeysKillaTests EvolveFX_VST3 -j4
build-vault/VaultTests_artefacts/Release/VaultTests                    # "vault failures: 0" (real-library checks run when a library is found)
build-vault/VaultTests_artefacts/Release/VaultTests -v3                # only the v3 tests ("vault v3 failures: 0")
CHROME_VAULT_LIBRARY=/home/user/cv-library build-vault/VaultTests_artefacts/Release/VaultTests -convcheck old_v2_bank.cvb [step] [threads]
CHROME_VAULT_LIBRARY=/home/user/cv-library build-vault/VaultTests_artefacts/Release/VaultTests
xvfb-run -a build-vault/VaultTests_artefacts/Release/VaultTests -uitest
CHROME_VAULT_LIBRARY=/home/user/cv-library build-vault/VaultTests_artefacts/Release/VaultTests -libreport /tmp/renders 48 60 72
build-vault/VaultTests_artefacts/Release/VaultTests -writev2demo /tmp/demo_v2.cvb   # 5 v2 presets on the test instruments
CHROME_VAULT_BANK=/tmp/demo_v2.cvb CHROME_VAULT_LIBRARY=/home/user/cv-library build-vault/VaultTests_artefacts/Release/VaultTests -wav 0 piano.wav 60 3
pluginval --strictness-level 8 --skip-gui-tests --validate "build-vault/ChromeVault_artefacts/Release/VST3/CHROME VAULT.vst3"
```

`VaultTests -pitch [--only <text>] [--step n] [--bank f.cvb] [--lo k --hi k] [--csv f] [--wav dir] [--nofx]
[--results file] [--dump] [--library] [--ids file] [--vel n] [--threads n] [-v]` (owner report item 0,
tests/VaultPitchTests.cpp, needs the library) renders EVERY preset of the bank on every key MIDI 24..96 (FL Studio
C2..C8, so both C2..C7 namings are covered; fresh engine per key, the sound as designed incl. FX / arp / gate, 1.2 s at
44.1 kHz, velocity 102 = the loudest velocity layer, round robin 1; `--vel` changes it) and measures each key with
`tools/vault/PitchAnalysis.h` (window from 20 ms after the attack while within 20 dB of the peak, up to 0.74 s; YIN; a
5-cent log-frequency spectrum; the strongest partial; the fundamental partial). Neighbouring keys must be one semitone
apart within +-10 cents by any of: YIN, the spectrum cross-correlation (every partial moves: bells, noisy / layered / FX
sounds; on each key's window and on the same window), the strongest partial, the fundamental partial (tuned bars), the
YIN tracks frame by frame, or - when none agree - short frames on the same window (`frameShift`: risers and pitch
envelopes, whose long-window spectra are smeared and weighted differently by the key-tracked filter). ~0 cents is
reported as SAME PITCH. A layered sound that fails as a whole is checked layer by layer (each layer solo): a layer that
enters at its instrument's range edge (a duet a third apart below the violin's lowest note) or the strongest partial
changing layers is not a wrong pitch, a layer that plays a wrong pitch fails on its own. A pair that fails on a short
note (window < 0.3 s, or a layer below 65 Hz) is measured again with the layers held (sample decay -> sustain 1, synth
sustain 1 unless the mod matrix routes to pitch: a pluck's 150 ms says little about its pitch, a held one shows it); a
gated preset is measured again with the gate open (a TRANCE GATE chops the window). The preset's common YIN tuning must
be the played key's pitch class (within 30 cents, mod octave). The piano roll: C5 right after B4 (touching notes) must
play C5's own pitch (compared with C5 after the same time of silence: glide, per-note random pitch and voice reuse must
not carry B4 into C5; a long-tailed B4 is subtracted - its tail alone, rendered the same way - before C5 is measured).
The instruments a preset uses are pinned while it is measured (the library's memory budget would otherwise evict a
layer half way and make keys silent). Not checked, reported: silent keys (outside an instrument's range), keys where
every sounding layer is below 32.7 Hz, unpitched pairs (noise). `--results f` appends one line per preset and resumes a
run (skips the presets already in the file); `--dump` lists every preset's layers and values; `--library` measures every
library articulation as a plain one-layer preset ("lib.<instrument>/<articulation>"); `--ids f` only the listed preset
ids; `--wav dir` writes each failing pair (and the B4 / C5 sequence) as WAV. About 55 minutes for the bank on 4 cores;
exit code = failing presets.

Result (2026-10-16, final library fixes, final bank): all 3,151 factory presets pass (192,883 key pairs; 36 pairs passed
layer by layer, 117 short-note pairs passed held; silent keys outside an instrument's range: 26,811 on 1,137 presets) and
all 154 library articulations pass (`--library`). Before the integration pass: 3,058 of 3,245 (the run then also evicted instruments half way). What it
took: the library tuning pass (LibraryTuningData.cpp) plus the hand fixes (LibraryZoneFixes.cpp: 6 zone ranges, 62
tuning pins), the swell rule for one-shot articulations (`s.oneShot`), glide only between overlapping notes and
note-seeded synth randomness (Synth.h), the voice models' sung range / scatter / drift (VoiceModels.cpp), and 94
presets taken out of the bank (`tools/vault/bank_drops.tsv`, applied with `VaultBankGen -patch`, ids reserved): 57
whose pitch cannot be one semitone per key - dan tranh bends and tremolo-picked dan tranh (the recordings bend within
the note), timpani variants (inharmonic, the pitch the drum settles on moves with the strike), reversed / mirrored /
pitch-swept FX at the keyboard's ends (Mirror Laser, Backward Bubbles, Pulsing Tension Riser, Seismic / Quake bells),
thumb piano / mbira variants, a few chops whose take is tuned between notes (Clipped High Breathy Chop, 42 cents) - and
37 that fail the v2 bank gate with the retuned library (layers a few cents apart moved: 17 with a weak fundamental at
C5, 8 velocity, 6 mono-loss, 5 harsh 2-5 kHz, 1 listening-phrase peak). A bank regenerated with `-recipe all` must be
patched with the same file.

VaultTests (v2 additions): sampler zone selection (key range, velocity layer, nearest layer), round-robin rotation,
pitch at C5 from zones 3 semitones away (+-5 cents, YIN) at 44.1 and 48 kHz, loop seamlessness on a deliberately
non-periodic loop (max sample jump around 4 wraps <= 1.08x the steady signal), release fade (-60 dB in the release time,
no click), one-shot ignores note-off (OGG file), missing file -> failed + silent, unknown instrument / missing library ->
silent, `libraryFound()` false for a missing folder, silent-while-loading then playing, a used instrument survives a 0-byte
budget while unused ones are evicted, no allocation in `VaultEngine::process` (operator-new counter, 400 blocks with notes,
pedal, bend, macros, sample / synth / model switches), model registry + the sine model in tune at C5, unknown model silent,
CVB v2 round trip with all kinds, unknown sample keys refused, v1 file read (tags dropped, categories mapped, "808"
removed), the embedded v1 bank read; with a library: every instrument / articulation at C5 (or the playable key nearest
to it) clean; in tune - looped notes within 10 cents over 0.35-2.35 s, decaying ones within 15 cents where the builder
measured them (YIN or the fundamental's spectral peak; bars / tines on the spectral peak; bells, glides and detected
drums are not cent-checked, see `pitchCheck` in library.json); loops click-free (jump at each wrap < 1.3x the music
around it, crossfade dip > -3 dB beyond the music's own swing in the 600 ms before it). UI: no component but the search
box takes the keyboard (all components incl. list rows), Return / Escape hand it back, no key is used by the editor,
keyboard labelled C5 at MIDI 60, DRAG WAV "<name> C5.wav", every list line = whole instrument names that fit, plus the
v1 UI checks (speed, sizes, readability, state). `-uitest` then runs `VaultTests -uitestv2` as a child process with the v2
UI demo bank, once with a missing library folder (the "Sound library not found" card + path, sampled layers "not
installed", synth presets audible, model / synth / sample layer names, search by instrument) and once with the real
library when one is installed or named by `CHROME_VAULT_LIBRARY` (picking the biggest unloaded instrument costs < 20 ms on
the UI thread and shows Loading, the state ends when the samples are in, an instrument the library lacks gets the "newer
sound library" card); both at 80 / 100 % with the 14 px minimum text.

## The v3 factory bank (`resources/vault/factory.cvb`, `VaultBankGen -recipe all`)

3,151 presets in 16 categories (3,380 before the dark-strings rework, 3,245 after it; 94 left with the pitch / gate
pass of owner report item 0, `tools/vault/bank_drops.tsv`, and 444 loudness trims re-measured with `-retrim`). Every preset is a finished sound from its designer recipe: an FX chain (5.0 slots on
average, at most 8; the v2-converted bank had 1.75), movement (LFOs on 2,237 presets, mod wheel / velocity / aftertouch
routings) and sound macros named for that sound (four on 3,370 presets, fewer on 10 Plucks / FX); arp and gate are off
except on 31 plain-named rhythm presets (20 arp, 11 gate: "... Arpeggio", "Gated ...", "... Eighths"). `VaultBankGen
-stats <bank.cvb>` prints these numbers per category. Nobody has listened to it yet:
every judgement is measured. QC report: `docs/VAULT_BANK_REPORT.md`.

```
CHROME_VAULT_LIBRARY=/home/user/cv-library build-vault/VaultBankGen_artefacts/Release/VaultBankGen -recipe all -qa -chords \
    -macros /tmp/cv-macrofails.tsv -cache /tmp/cv-cache -threads 4 -out resources/vault/factory.cvb -report /tmp/cv-full-report.md
python3 tools/vault/bank_report_doc.py /tmp/cv-full-report.md docs/VAULT_BANK_REPORT.md
VaultBankGen -macrocheck <bank.cvb> [fails.tsv] [threads] [fixed.cvb]   # the macro check alone on a finished bank
VaultBankGen -pitchfix <in.cvb> <out.cvb>                              # the two-step sequence pitch fix of synth.cpp on a finished bank
VaultBankGen -patch <in.cvb> <out.cvb> <rules.tsv>                     # hand edits: "<id>\tset\t<layer|*>\t<key>\t<value>" or "<id>\tdrop"
VaultBankGen -retrim <in.cvb> <out.cvb> [ids.txt]                      # re-run the gate's loudness trim (cv gain) with the current engine / library
VaultBankGen -listen3 <bank.cvb> <dir> ["id|id|..."]                    # listening clips: phrase + macro-1 sweep (OGG) + picks.tsv
python3 tools/vault/listen3_page.py <dir> index.html                    # the owner's listening page (Czech, votes, summary)
```

Time on 4 cores with an empty cache: about 2 h 30 for the six recipes (keys 24 min, orch 11, perc-voc 8, strings 16,
synth 34, winds 18 with their own v2 caches: `KEYS_V2_CACHE`, `ORCH_V2CACHE`, `PV_V2CACHE`, `WINDS_V2_CACHE`,
`SYNTH_V2_CACHE`), 20 min to measure the 3,431 candidates and about 60 min for the macro check. With `-cache` every stage
is cached (recipe output per recipe, measurements and macro results keyed by a hash of the preset); a re-run with
nothing changed takes 10 s and writes the identical bank.

**Gate**: the v2 gate and assembly QA below, unchanged, on the whole v3 sound (FX chain on, modulation, arp / gate as
designed). **Macro check** (`-macros`, new): every sound macro of every accepted preset rendered at 0 and at 1 (the other
macros at their defaults, C5 at the preset's final gain); an extreme fails when it breaks a rule the designed sound keeps:
clipping at velocity 1.0, 2-5 kHz > 10 %, > 6 kHz > 3 %, > 8 kHz > 1 %, the fundamental > 3 dB under the strongest
harmonic (not Bells & Mallets / FX & Atmos), > 4 dB louder than designed, or a tail longer than the gate's category limit.
A failing side is narrowed (x0.55 per round, the fourth round takes it away) and the knob re-mapped so the designed sound
at its default stays exactly the same (`recipegate::narrowMacro`). The recipes already ran the same check at their own
gain; the assembly re-runs it at the final gain after peak control.

**Result** (2026-10-16): 3,380 of 3,431 candidates (rejected: 46 cross-recipe near-duplicates, 2 near-duplicates, 2
loudness, 1 too quiet). Min pairwise distance 3.50 (Velvet Folk Harp / Hushed Folk Harp Swell), across recipes 3.50.
Peak control on 79 presets. At C5: fundamental within 3 dB of the strongest harmonic (inharmonic bells / FX excepted),
2-5 kHz <= 9.1 % (median <= 0.9 % per category), > 6 kHz <= 0.93 %, > 8 kHz <= 0.59 %, loudness within 1.3 dB of the
category target, the listening phrase at most +1.9 dB over the safety limiter (median -3.5 .. -12.1 dB per category).
Macros: 13,510 checked, 28 extremes broke a rule (7 clip, 7 louder, 7 > 8 kHz, 4 fundamental, 2 tail, 1 2-5 kHz; 15 of
them in Piano / Keys & Organ) and were narrowed (59 narrowings), none left; 2,252 (17 %) change the measured sound only
slightly (slow drift / motion LFOs, reverb amounts on long sounds, pitch wobble the measurement does not see).

| category | presets | | category | presets |
| --- | ---: | --- | --- | ---: |
| Piano | 193 | | Bells & Mallets | 196 |
| Keys & Organ | 210 | | Vocals & Choir | 191 |
| Guitars | 172 | | Plucks | 271 |
| Bass | 200 | | Leads | 184 |
| Strings | 158 | | Pads | 215 |
| Brass | 192 | | Synth | 252 |
| Flutes & Woodwinds | 242 | | World | 212 |
| Orchestral | 140 | | FX & Atmos | 217 |

**Ids against the previous (v2-converted) bank**: 3,164 of its 3,377 presets keep id and name; 208 ids are gone (90
Guitars, 24 Bass, 20 World, 16 Flutes & Woodwinds, 11 Leads, the rest <= 9 per category: the v3 chains made them too
close to a sibling, or the recipe's selection changed), 211 new ids are appended. Five Piano / Keys ids now hold a
different, renamed preset (the keys recipe turned them into its arp / gate presets): cv.piano.0032 Night Drive Tine Piano
-> Gated Dreamwave, cv.piano.0073 Rising FM Piano -> Gated FM Piano, cv.piano.0084 Rising Digital Piano -> Gated Digital
Piano, cv.keys.0091 Echo FM Keys -> FM Keys Arpeggio, cv.keys.0110 Echo Music Box -> Music Box Arpeggio.

**Listening page**: `VaultBankGen -listen3` picks 4 presets per category (the first preset, the category's first arp /
gate preset, then the most different FX chain + macro set + instrument) and renders the listening phrase (C5 E5 G5 C6,
then the C5 triad, 4 s) and sound macro 1 swept 0 -> 1 over 6 s; `tools/vault/listen3_page.py` embeds them as base64 OGG
in one page (about 8 MB) with votes kept in the browser and a summary to copy.

**Dark strings (owner's FL Studio test report #1, item 17; `tools/vault/recipes/orch.cpp` addCinematic /
v3::designCinematic)**: the Strings category now opens on 36 hand-voiced cinematic low-string presets (ids
`cv.str.1001`+, the default is "Cinematic Low Octaves"), the string-heavy Orchestral on 8 more (`cv.orch.1001`+): the
library's viola, cello and contrabass sections stacked in octaves (key C5 sounds C4 / C3 / C2), cellos in octaves, open
fifths, the contrabass vibrato section, as sustains, slow swells, sul tasto / con sordino pads, marcato and sforzando
strokes, spiccato / pizzicato ostinatos, tremolo, and two-style strokes (accent into tremolo, tremolo into sustain,
spiccato over a bowed bass bed); horns, brass stabs, choir, bassoon, timpani and harp over the low sections. Each has
section glue, tape power with clean lows, a warm EQ with the top gently shelved, a long damped hall with a long pre-delay,
slow movement (bow-pressure breathing, drift, tension surges, delayed vibrato on the mod wheel) and 4 named macros (Bow /
Bite / Tone / Snap / Pluck, Hall, Swell / Bloom / Drift / Tension / Power / Weight, Basses / Depth / Echo). The processed
variants of thin / high sounds (violins only, solo violin, high violas, the synth string machine) are left out; their plain
forms stay. Strings: 158 presets (was 256), 123 of them with cellos / basses / the octave stack; median C5 centroid ~206 Hz (the cinematic ones),
2-5 kHz <= 2.4 %. The bank on disk was assembled by swapping only the orch recipe's presets (Strings / Brass / Orchestral,
rebuilt and gated against the current, retuned library: Brass 192, its trumpet / trombone sounds refitted) in the previous
bank (`ORCH_SPLIT` -> `-recipe all -cache <dir> -rebuild orch` -> `ORCH_MERGE=<shipped>,<assembled>,<out>`), so every
other recipe's preset is byte-identical to before: 3,245 presets. The integration pass then re-gated the whole bank
against the retuned library (`-retrim`: 444 trims re-measured; the fundamental now read in the best of three windows, so
layers that merely beat no longer fail it) and took out the 37 that still fail plus the 57 that fail the pitch test
(`-patch tools/vault/bank_drops.tsv`): 3,151 presets. Owner's demos: `ORCH_DEMO=<bank>|<dir>|<names>` (low chord
progression C minor - A flat - F minor - G, then a swell; WAV + OGG + demos.tsv).

## The v2 factory bank (previous; kept for reference) (`resources/vault/factory.cvb`, `VaultBankGen -recipe all`)

3,377 presets in 16 categories, every one built by a sound-design recipe and measured; nothing in the bank has been
listened to by a person yet (the owner's listening page is the first ear). QC report of the shipped bank:
`docs/VAULT_BANK_REPORT.md`.

```
CHROME_VAULT_LIBRARY=/home/user/cv-library build-vault/VaultBankGen_artefacts/Release/VaultBankGen -recipe all -qa \
    -cache /tmp/cv-cache -threads 4 -out resources/vault/factory.cvb -report /tmp/cv-full-report.md [-prints p.tsv] [-chords]
python3 tools/vault/bank_report_doc.py /tmp/cv-full-report.md docs/VAULT_BANK_REPORT.md
VaultBankGen -recipe <name> [...]          # one recipe alone through the same gate (designers)
VaultBankGen -recipe all -cache dir -listen <dir> "<id>|<id>|..."   # the listening phrase as OGG
python3 tools/vault/listen_picks.py full-report.md prints.tsv picks.tsv && python3 tools/vault/listen_page.py picks.tsv <ogg dir> index.html
```

About 25 minutes on 4 cores to measure the 5,321 candidates with an empty cache (each is rendered 7 times plus the
phrase), plus 10-22 minutes per recipe whose output is not cached yet; with `-cache`
only changed presets are measured again (key: a hash of the preset + `gateVersion`).

**Recipes** (`tools/vault/recipes/*.cpp`, one per designer, design notes at the top of each file):

| recipe | categories | candidates |
| --- | --- | ---: |
| `keys` | Piano, Keys & Organ | 454 |
| `strings` | Guitars, Bass | 2,128 (the gate picks the distinct ones) |
| `orch` | Strings, Brass, Orchestral | 629 |
| `winds` | Flutes & Woodwinds, World | 468 |
| `perc-voc` | Bells & Mallets, Vocals & Choir | 398 |
| `synth` | Plucks, Leads, Pads, Synth, FX & Atmos | 1,244 |

**Gate** (`RecipeGate.h`, every candidate; 48 kHz, C5 = MIDI 60 at velocity 0.8, note off after 1.5 s and rendered until the
tail is 60 dB down, plus C4 / C6 and C5 at velocity 0.35 / 1.0): NaN, silence, DC, clicks (note start / end / anywhere),
clipping (velocity-1.0 peak under the safety limiter), mono-sum loss <= 3 dB, no side energy under 120 Hz, tail <= 10 s
(Pads / FX & Atmos 16 s); C5 comfort: the sounding fundamental within 3 dB of the strongest harmonic (not Bells & Mallets /
FX & Atmos; in the best of three 0.7 s windows from 0.05 / 0.4 / 0.75 s - layers a few cents apart beat, and a single
window could catch the fundamental in a beat's trough), 2-5 kHz <= 10 % of the power, > 6 kHz <= 3 %, octave-up parts >= 12 dB under part 0, velocity 0.35 -> 1.0
opens >= 3 dB; names / labels clean (no brands, no "808").

**Loudness**: K-weighted loudest 400 ms at C5 set by `VaultPreset::gain` to the category target of `BankQC.h` minus 6 dB
of headroom (`recipegate::bankHeadroomDb`: one note at velocity 0.8 sits at -20.5 Piano .. -18 Bass, so chords stay under
the safety limiter). A preset whose velocity-1.0 peak would pass 0.84 at that level gets the peak control (`cv.peak`,
threshold iterated, at most 16 dB); one still more than 1.5 dB under the target is rejected (`-spread`).

**Assembly QA** (`-qa`): loudness within 1.5 dB of the category target, > 8 kHz <= 1 % of the power, spectral-centroid
ceilings per category (Bass 700 Hz .. Bells / FX 2,200 Hz, `centroidCeiling`), and the listening phrase (C5 E5 G5 C6 +
the C5 triad at velocity ~0.77) may reach the safety limiter by at most 2 dB (`maxChordOver`). Always: a trim above +22 dB
(`maxTrimDb`: a near-silent render lifted into the noise floor) is rejected as too quiet. Names longer than 22 characters
write "and" as "&"; the browser's name column fits 30 characters (252 px at 100 %, `PresetList::columns`).

**Distinctness**: the gate's fingerprint (log-mel x time at C5, C4, C6, cepstrum, C5 harmonic profile, modulation depth,
stereo width; RMS distance in dB). Candidates are taken in recipe priority order (keys, strings, orch, winds, perc-voc,
synth: real instruments first, generic synth last) and each recipe's own order (plain instruments before treatments); one
closer than 3.5 to any accepted preset, of its own recipe or another, in any category, is dropped. Names are unique
(case-insensitive) across the bank, ids (`cv.<category slug>.<nnnn>`) come from the recipes and are stable.

**Result** (the shipped bank, 2026-10-14): 3,377 of 5,321 candidates. Min pairwise fingerprint distance 3.50 (Double
Basses / Double Bass Vibrato), across recipes 3.51. Peak control on 197 presets. At C5 over the whole bank: fundamental
within 3 dB of the strongest harmonic (60 inharmonic bells / FX excepted), 2-5 kHz <= 8.0 % (median 0.2 %), > 6 kHz
<= 1.8 %, > 8 kHz <= 1.0 %, loudness within 1.3 dB of the category target, trims -15 .. +22 dB, the listening phrase
5.7 dB under the safety limiter in the median (at most 2 dB over).

| category | presets | | category | presets |
| --- | ---: | --- | --- | ---: |
| Piano | 176 | | Bells & Mallets | 196 |
| Keys & Organ | 200 | | Vocals & Choir | 189 |
| Guitars | 208 | | Plucks | 271 |
| Bass | 201 | | Leads | 195 |
| Strings | 245 | | Pads | 214 |
| Brass | 212 | | Synth | 256 |
| Flutes & Woodwinds | 232 | | World | 209 |
| Orchestral | 151 | | FX & Atmos | 222 |

Per recipe (designer's selection -> shipped): keys 454 -> 376 (78 closer than 3.5: the recipe selected at 3.05), strings
421 (gate-selected from 2,128) -> 409, orch 629 -> 608, winds 468 -> 441, perc-voc 398 -> 385, synth 1,244 -> 1,158 (80
too close to a real-instrument preset of another recipe). Rejected: 1,733 near-duplicates inside a recipe (1,655 of them
strings candidates), 120 across recipes, 31 too quiet (trim over +22 dB), 23 weak fundamental, 7 loudness, 6 click,
5 duplicate names, 3 chord headroom, the rest single cases (report).

The target was 3,600: the six recipes delivered 3,614 selected sounds, and one bank-wide definition of "different"
(3.5) plus the cross-recipe pass leaves 3,377. Categories with room to grow by new recipe content (not by loosening the
threshold): Orchestral (151), Piano (176), Vocals & Choir (189), Leads (195), Bells & Mallets (196), Keys & Organ (200).

**VaultTests** (shipped-bank checks, `shippedBankTests`): 3,000..5,000 presets, every category >= 100, ids
`cv.<slug>.<nnnn>` and names unique, names clean and <= 22 characters, labels clean, no "808", most parts sampled /
modelled, and 160 random presets (fixed seed; with the sample library, else the synth / model ones) re-run through this
gate: no failure, the shipped trim within 0.5 dB of the measured one, loudness within 1.5 dB of the target, > 8 kHz,
centroid ceilings, the listening phrase at most 2.5 dB over the limiter, sampled presets >= 3.0 apart.
`CV_BANK_GATE_ALL=1` runs every preset.

## The v1 factory bank (legacy, no longer shipped: `VaultBankGen -v1`)

> v1 synth-only bank, replaced by the v2 bank (above). `BankGen.cpp` still builds: it plans / tags / names in the
> OLD category list internally (`v1c` namespace: old enums, targets, styles / characters only steer the names) and writes
> v2 presets (v2 categories via `vault::v1::toV2Category`, no tags, "808" -> "Sub"). The text below describes the v1 run.


5,000 presets, generated by `tools/vault/BankGen.cpp` (console target `VaultBankGen`) and committed, so builds / CI never run it.
QC report of the shipped bank: `docs/VAULT_BANK_REPORT.md` (written by the generator).

```
cmake --build build-vault --target VaultBankGen
build-vault/VaultBankGen_artefacts/Release/VaultBankGen -v1 [-threads N] [-scale 0.1] [-out file.cvb] [-report file.md] [-dup 3.0]
VaultBankGen -check resources/vault/factory.cvb      # re-render every preset at its final gain through the gate
VaultBankGen -list  resources/vault/factory.cvb      # id, name, layers, tags
VaultBankGen -audit resources/vault/factory.cvb [-csv out.tsv] [-every N]   # independent audio audit, exit code 0 = 0 failures
```

Run it from the repo root (paths are relative). About 15-20 minutes on 4 cores. Deterministic: own RNG (splitmix64,
no std distributions), every candidate seeded by its slot, rendered in parallel but accepted in slot order - same sources
and same compiler give the same bank whatever the thread count.

How it builds the bank:

1. **Source library** (~1,950 one-part patches): the EVOLVE factory sounds (`factoryPresets()`, loudness from
   `PresetGains.h`; drums left out) plus procedural recipes per role (808s with pitch drop / saturation / glide, reese and
   synth basses, FM bells, modal mallets, EPs / pianos / organs, plucks, leads, pads, orchestral strings / brass, flutes / reeds,
   guitars with body resonance, choirs, poly synths, textures, risers / sweeps, world plucks / pipes / drones, braams / hits,
   808 top layers "grit" and "knock"). Every source is rendered alone and its `gain` set so it plays at -15 dB
   (K-weighted): layer levels then mean the same thing for every source.
2. **Plan**: exact counts per category (`vault::categoryTarget`) and an integer category x layer-count matrix fitted to the
   layer mix 1600 / 1900 / 1100 / 400 (iterative proportional fitting + controlled rounding; 808 & Bass leans to single 808s,
   Orchestral to ensembles).
3. **Variants**: the main part (walked evenly through the category's pool) gets 2-4 treatments (1-3 under layers): brightness,
   envelope, era profile (EVOLVE `era`), space, chorus / phaser / flanger + width, unison / detune, drive, lo-fi, motion,
   timbre, octave, a second oscillator layer; for 808s: decay, glide, punch, saturation, sub, tone instead.
4. **Layers** (2-4 parts): per category a table of compatible roles with octave ranges and level ranges, e.g. bell + pad +
   air, pluck + sub, strings + brass, choir + pad, piano + pad, lead + octave double, 808 + grit / knock on top. At most one
   bass-ish part, layer parts panned around the centre (bass layers centred), FX only in part 0 (airy layers lift its reverb).
   Arps & Sequences: a held note plays a tempo-synced two-step sequence (LFO2 square on pitch, octave / fifth / fourth;
   LFO1 ramp plucks the filter every step; synced delay).
5. **Quality gate** (`tools/vault/BankQC.h`, shared with VaultTests), two notes at 48 kHz (808 & Bass MIDI 36 / 48, the rest
   60 / 84, C3 = 60), 1.5 s (longer for slow attacks), note off at 70 %: silence, NaN, peak >= 0.89 (the safety limiter
   must never act), DC, click at note start (first sample, sample jump in the first 2 ms against the next 200 ms) and note end,
   mono-sum loss > 3 dB, side under 90 Hz > -30 dB. Then the **audit render** at the final gain (`tools/vault/BankAudit.h`:
   C4, 808 & Bass C2, note off at 1.05 s, rendered on until the tail is 60 dB down): real release tail > 7.6 s, side power
   under 120 Hz (FFT) > -33 dB of the signal or > -23 dB of the mid under 120 Hz, mono-sum loss > 2.7 dB, DC, clicks at
   note start / end / anywhere in the tail. A failing candidate is repaired (softer attack, longer release, progressively
   shorter tails - cloud reverb size capped, then swapped for a hall - narrower stereo, master glue) and re-rendered, else
   replaced by a new candidate for its slot.
6. **Loudness**: K-weighted (BS.1770 filters) loudest 400 ms, mean of both notes, set by `VaultPreset::gain` to the category
   target (808 & Bass -12, Leads -13.5, Brass / Synth -14, Keys / Plucks / Guitars / Arps / Orchestral -14.5, Bells / Strings /
   Winds / World -15, Vocals -15.5, Pads -16, FX & Atmos -17) within +-1 dB. Presets whose peak would reach 0.89 at the target
   are allowed down to -0.9 dB under it. The gain is then nudged (mean of both notes stays within 0.85 dB) so the low test
   note - the one most people hit - is within 1.5 dB of the target too.
7. **Dedupe**: a fingerprint per note (20 log bands of the held spectrum, 6 bands of the attack, 10-step envelope, width);
   a candidate closer than 3.0 to any accepted preset (any category) is rejected (relaxed to 2.55 / 2.1 for slots that keep
   failing), and also one closer than 2.1 by the audit's fingerprint (24 log bands x 5 time segments of the C4 / C2 note).

**Audit** (`VaultBankGen -audit`, `tools/vault/BankAudit.h`): every preset re-rendered at C4 (808 & Bass C2), velocity 0.8
and 1.0, plus MIDI 24 / 96; fails on NaN, silence, peak > -0.5 dBFS, the safety limiter reached at velocity 0.8, DC, clicks,
mono-sum loss > 3 dB, wide lows under 120 Hz, real tail > 8 s, loudness > 2 dB from the category median, CPU > 3x the median
of presets with the same layer count, near-duplicates (audit fingerprint < 2.0), names (unique, ASCII, brand words /
phrases, offensive words). Must report 0 failures before a bank is committed.
8. **Tags** from the measured sound: Bright / Dark = spectral-centroid percentile in the category, Warm = 150-900 Hz share,
   Wide = side / mid percentile, Hard / Soft = attack time, Dirty / Clean = drive + crush (+ 808 saturation); 1-3 each.
   Styles scored from category + character + space / lo-fi / glide (e.g. 808 with glide -> Drill, soft / warm plucks -> Pluggnb).
   Trap stays a usable filter: in the trap categories (808 & Bass, Bells, Plucks, Strings, Brass, Winds, Orchestral) only the
   most trap-like 70 % (90 % for 808 & Bass) by a hard / dirty / dark vs. soft / roomy score carry it.
9. **Names**: invented "<adjective> <noun> <instrument>" (adjective family follows the character tags), <= 22 characters,
   unique, no word stems repeated, checked against a brand / product word list (`vaultqc::isCleanName`). Part labels are short
   instrument words ("Ice Bell", "Dark Pad", "Sub 808", "Grit"). Ids `cv.<category>.<nnnn>` (808, bell, keys, pluck, lead, pad,
   str, brass, wind, gtr, vox, synth, arp, orch, world, fx).

VaultTests (with a bank embedded) checks today: 1,000..5,000 presets, at least 12 of the 16 categories used, unique ids,
names (99 %) clean and short, labels clean, no "808" in any name / label, and 200 random presets (fixed seed) re-rendered
through the v1 gate (incl. loudness, +-1.5 dB: v1 arps now sit in Synth) and mutually distinct. (v1 originally also checked
exact per-category / per-layer counts and the tags.)

## EVOLVE refactor note

`KeysKillaProcessor::buildVoiceParams` now calls `kk::buildVoiceParamsFrom` with a lookup into its APVTS / morph values
(`Idx::vk` maps `kk::vk` keys to parameter indices). Behaviour is unchanged (same code, same order; EVOLVE unit tests pass).
Never reorder `KK_VOICE_KEYS` - append only.
