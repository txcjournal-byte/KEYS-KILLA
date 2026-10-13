# CHROME VAULT by TrapWin

A separate plugin product (target `ChromeVault`, VST3 / AU / Standalone) next to EVOLVE (`KeysKilla`) and
EVOLVE FX PRO (`EvolveFX`). A fast preset-bank instrument: open, click a sound, play. 5,000 presets in 16
categories, each preset is 1-4 layers ("parts"). UI 100 % English, no real brand / product / company names anywhere.

Identity (never change after release): company `TrapWin`, bundle `com.trapwin.chromevault`, manufacturer code `Trpw`,
plugin code `Cvlt`, product name `CHROME VAULT`, version 1.0.0 (`set(CV_VERSION ...)` in `CMakeLists.txt`, separate
from the EVOLVE project version). Installers, CI artifacts and end-user install notes: `docs/CHROME_VAULT_INSTALL.md`.

## Files

| File | What |
| --- | --- |
| `Source/ParamKeys.h` | `kk::vk` - the synth parameters as small integer keys (frozen order), with range / default / integer flag |
| `Source/VoiceBuild.h` | `kk::buildVoiceParamsFrom` - parameter values -> `kk::VoiceParams` + `kk::FxParams` (shared with EVOLVE) |
| `Source/vault/VaultTypes.h/.cpp` | `VaultPart`, `VaultPreset`, name tables, the `CVB1` bank format (light: juce_core only in the header) |
| `Source/vault/VaultEngine.h/.cpp` | `VaultEngine` - 4 synth parts -> master FX -> macros -> mono lows -> trim -> volume -> limiter; offline `render()` |
| `Source/vault/VaultProcessor.h/.cpp` | `VaultProcessor` - the `juce::AudioProcessor` (parameters, bank loading, state) |
| `Source/vault/VaultEditor.h/.cpp` | the editor: browser UI (see "Editor UI" below); `VaultMainView` / `VaultInfoPanel` are implemented here too |
| `Source/vault/ui/*.h` | header-only UI parts: `VaultTheme.h` (palette, fonts, icons, LookAndFeel), `VaultBrowserModel.h` (search / filters / favorites / recent, no GUI), `VaultSettings.h` (user settings file), `VaultThumbs.h` (background waveform thumbnails), `VaultWidgets.h` (pills, category list, chips, preset list, keyboard, DRAG WAV), `VaultMainView.h` |
| `tests/VaultTests.cpp` | console target `VaultTests` |
| `resources/vault/*.cvb` | the preset bank(s) embedded into the plugin; `factory.cvb` = the generated 5,000-preset factory bank |
| `tools/vault/BankGen.cpp` | console target `VaultBankGen`: generates `factory.cvb` + `docs/VAULT_BANK_REPORT.md` |
| `tools/vault/BankQC.h` | the quality gate (analysis, loudness, fingerprint, name check) shared by the generator and VaultTests |

Every `Source/vault/*.cpp` is compiled into `ChromeVault` and `VaultTests` automatically (CMake `file(GLOB ... CONFIGURE_DEPENDS)`),
so new UI files only need to live in `Source/vault/`. Shared EVOLVE code used: `Synth.h`, `Fx.h`, `Studio.h`, `DspUtil.h`,
`Wavetable.h`, `Params.h`, `Presets.h/.cpp` (header-only DSP; `Presets.cpp` for the starter bank). The EVOLVE editor / processor
are NOT part of CHROME VAULT.

## Architecture

```
MIDI ─┬─> part 0: kk::SynthEngine (32 voices, 2 osc layers, filter, envs, LFOs)  x level, pan ─┐
      ├─> part 1  (transposed by octave*12+semi)                                             ├─> sum ─> kk::FxRack (FX values of PART 0)
      ├─> part 2                                                                              │        drive, chorus, phaser, delay, reverb,
      └─> part 3                                                                              ┘        EQ, width, MASTER glue + limiter
   ─> side high-pass 160 Hz 8th order (everything under ~120 Hz is mono: side -20 dB at 120 Hz) + 5 Hz DC blocker on the mid
   (preset gain trim per sound slot; during a preset switch two slots - each parts + own FxRack - crossfade, 40 ms equal power)
   ─> master volume ─> safety limiter (-1 dBFS, instant attack, 120 ms release) + soft ceiling 0.98 ─> out
```

Preset switch: a real crossfade. The engine has two pre-allocated sound slots (program + 4 parts with their voices and
LFOs + its own `kk::FxRack`). On a switch the free slot takes the new sound, the keys still held are struck on it, and it
fades in on sin(pi/2 t) while the old slot - its voices AND its own chorus / delay / reverb tails - keeps running and fades
out on cos(pi/2 t), over 40 ms (`VaultEngine::xfadeSeconds`). Nothing is cut hard, so no cut can enter an FX line and come
back later as a click (the cause of the old 10 ms fade-out / fade-in clicks: the voice cut went through the chorus / delay
and returned ~9 ms later). When the fade ends the old slot's voices are killed and its FX rack cleared
(`kk::FxRack::clearState()`, no allocation; done then rather than in the switch block), so the next sound in that slot
starts in a clean rack. Only one slot renders outside a crossfade; during the 40 ms both do (about 2x CPU for 8 blocks of
256). A switch that arrives during a crossfade waits for it to end (only the latest one is kept, at most 40 ms later).
`setPreset (p, true)` is instant (both slots cleared). Offline renders (`render` / `renderMidi`) prepare one slot only.

* Each part is a complete EVOLVE synth patch (`VaultPart::values`, real parameter values with EVOLVE parameter IDs,
  anything missing = the EVOLVE default). The conversion values -> `VoiceParams` is the exact EVOLVE one
  (`kk::buildVoiceParamsFrom`), so an EVOLVE factory preset copied into one part sounds the same.
* FX: one shared master `kk::FxRack`, built from **part 0's** FX values (drive, crush, chorus, delay*, rev*, eq*, width,
  ghost, body, punch, master, `gain`, ...). FX values in parts 1-3 are ignored, except `bassMode` (any part) which
  protects the lows from drive. So a generator should put the preset's FX into part 0.
* Gain: part 0's `gain` (dB) is the FX output gain (as in EVOLVE). Parts 1-3 are scaled by `dB(gain_k - gain_0)` on top of
  their `level`. `VaultPreset::gain` (linear) is the loudness trim after the FX.
* Macros (0..1, **0.5 = exactly as designed**; below 0.5 scales the designed amount down to 0, above 0.5 adds):
  * TONE: cutoff of every part x 2^((2t-1)*1.5) and a master tilt (high shelf +-5 dB, low shelf -+2.5 dB)
  * SPACE: reverb mix / delay mix (scaled, then +0.45 / +0.25), reverb size +0.3, width
  * DRIVE: FX drive (scaled, then +0.7)
  * MOTION: LFO1 -> filter / pitch / amp depth, chorus, phaser, flanger, + pitch drift above 0.5
* Preset switch: `setPreset` is allocation-free (presets are pre-compiled to `VaultCompiledPreset`); the new sound starts
  at once in the second slot (held keys re-struck) and crossfades with the old one over 40 ms, equal power (see above).
* CPU: about 3 % of one core for a held 4-layer note (10 s of a 4-layer preset renders in ~0.3 s). Memory: ~4 MB per prepared part, x2 for the
  crossfade slot (plus a second FxRack, ~7 MB) in the plugin.

## Data model (`Source/vault/VaultTypes.h`)

```cpp
struct VaultPart {
    std::vector<std::pair<juce::String, float>> values;   // REAL param values, EVOLVE IDs (Params.h); missing = default
    float level = 1.0f;     // 0..2 linear
    float pan = 0.0f;       // -1..1, equal power (unity at centre)
    int octave = 0;         // -3..3 (MIDI transpose of this part)
    int semi = 0;           // -12..12
    juce::String label;     // UI layer name, e.g. "Ice Bell"
};
struct VaultPreset {
    juce::String id;        // stable unique id, e.g. "cv.808.0001" - saved in projects, never change
    juce::String name;      // English display name, invented (no real brands / products / companies)
    int category = 0;       // vault::CategoryIndex
    uint32_t styles = 0;    // bit i = vault::styleNames()[i]
    uint32_t chars = 0;     // bit i = vault::characterNames()[i]
    std::vector<VaultPart> parts;          // 1..4
    float gain = 1.0f;                     // loudness trim, linear, after the FX
    float macroDefaults[4] { 0.5f, 0.5f, 0.5f, 0.5f };   // TONE SPACE DRIVE MOTION
};
namespace vault {
  enum { numCategories = 16, numStyles = 8, numCharacters = 8, maxParts = 4, numMacros = 4 };
  enum Macro { macroTone, macroSpace, macroDrive, macroMotion };
  enum CategoryIndex { cat808Bass, catBellsMallets, catPianoKeys, catPlucks, catLeads, catPads, catStrings, catBrassHorns,
                       catFlutesWinds, catGuitars, catVocalsChoir, catSynth, catArpsSequences, catOrchestral, catWorld, catFxAtmos };
  enum StyleIndex { styleTrap, styleRage, styleDrill, stylePluggnb, styleDark, styleEmo, styleLoFi, styleAmbient };
  enum CharIndex  { charBright, charDark, charWarm, charWide, charHard, charSoft, charDirty, charClean };
  const juce::StringArray& categoryNames();   // "808 & Bass", "Bells & Mallets", ... "FX & Atmos"
  int categoryTarget (int category);          // 520, 480, 420, ... (sum 5000)
  int layerCountTarget (int numParts);        // 1: 1600, 2: 1900, 3: 1100, 4: 400
  const juce::StringArray& styleNames();      // Trap Rage Drill Pluggnb Dark Emo Lo-Fi Ambient
  const juce::StringArray& characterNames();  // Bright Dark Warm Wide Hard Soft Dirty Clean
  const juce::StringArray& macroNames();      // TONE SPACE DRIVE MOTION
  juce::StringArray namesOfMask (uint32_t mask, const juce::StringArray& names);
  uint32_t maskOfNames (const juce::StringArray& tags, const juce::StringArray& names);

  juce::MemoryBlock serializeBank (const std::vector<VaultPreset>& bank, bool compress = true);
  bool deserializeBank (const void* data, size_t size, std::vector<VaultPreset>& out, juce::String* error = nullptr);
  bool saveBankFile (const juce::File& file, const std::vector<VaultPreset>& bank);
  bool loadBankFile (const juce::File& file, std::vector<VaultPreset>& out, juce::String* error = nullptr);
  bool validatePreset (const VaultPreset& p, juce::String* why = nullptr);   // 1..4 parts, ranges, known param IDs, finite
}
```

Parameter IDs and real ranges: `Source/Params.h` (`createLayout()`); the sound-relevant subset with min / max / default
is `kk::vk::info (key)` in `Source/ParamKeys.h` (`kk::vk::keyOf (id)`, `kk::vk::count`). Useful IDs: `engine` (0 VA, 1 FM,
2 Pluck, 3 Vox, 4 Organ, 5 Flute, 6 Sub 808, 7 Wavetable, 8 Orchestral, 9 Modal), `wave`, `unison`, `detune`, `fmAmt`,
`fmAlgo`, `layerB` + `...B` keys (second oscillator layer inside one part), `sub`, `filterType`, `cutoff`, `reso`, `fenv`,
`attack` `decay` `sustain` `release`, `lfoRate` `lfoPitch` `lfoFilter`, `mono` `glide` `bassMode` (808 / bass: mono,
clean lows), `drive` `driveType`, `chorus`, `delayMix`, `revMix` `revSize` `revType`, `width`, `gain` (dB), `macro1..8`
(EVOLVE macros, leave at default), `mmSrc1..8 / mmDst1..8 / mmAmt1..8` (mod matrix). Performance IDs (`arp`, `chord`,
`keyLock`, ...) are not sound parameters and are refused by `validatePreset`.

## Bank file format `CVB1`

```
"CVB1" | uint16 version = 1 | uint16 flags (bit0 = gzip/zlib payload) | uint32 presetCount | uint32 rawPayloadSize | payload
payload:  uint16 keyCount, keyCount x UTF-8 zero-terminated param-ID strings (the file's own key table; written in kk::vk order,
          then any other ID used)
          per preset: id, name (UTF-8 z-strings), uint8 category, uint32 styles, uint32 chars, float gain, 4 x float macroDefaults,
                      uint8 partCount, per part: label (z-string), float level, float pan, int8 octave, int8 semi,
                                                 uint16 valueCount, valueCount x (uint8 keyIndex, float32 value)
all little-endian (juce::MemoryOutputStream). Round trips bit-exactly. ~760 one-layer presets = 30 KB compressed.
```

## How a bank is produced and loaded

1. The bank generator builds `std::vector<VaultPreset>`, validates each with `vault::validatePreset`, renders with
   `VaultEngine::render` to check / calibrate, and writes `vault::saveBankFile (file, bank)`.
2. Put the file(s) in `resources/vault/` (any name ending in `.cvb`, e.g. `chrome_vault.cvb`, or one file per category:
   `01_808_bass.cvb`, ...). CMake globs `resources/vault/*.cvb` (CONFIGURE_DEPENDS: a new file re-runs the configure step);
   when there is at least one file it creates the binary-data target `ChromeVaultData` (namespace `VaultData`, header
   `VaultData.h`), links it into `ChromeVault` and `VaultTests` and defines `CV_HAS_BANK=1`.
3. At start the plugin loads (once per process, shared by all instances):
   1. the file named by the environment variable `CHROME_VAULT_BANK` (absolute path) - for development / generator checks
      without rebuilding;
   2. else every embedded `.cvb`, concatenated in file-name order;
   3. else the **starter bank**: one 1-layer preset per EVOLVE factory sound (757 now), categorised into the 16 vault
      categories (`VaultProcessor::buildFallbackBank()`, ids `cv.fb.<slug>`).
   At most 5,000 presets are used (`VaultProcessor::maxPresets`).
4. Check a bank: `VaultTests -bankinfo` (per-category / per-layer counts vs targets), `VaultTests` (all tests over the
   loaded bank: every preset validates, ids unique, two renders per category clean), `VaultTests -wav <index> out.wav [note] [seconds]`.
   With `CHROME_VAULT_BANK=/abs/path/bank.cvb` these use that file. `VaultTests -writestarter out.cvb` writes the starter
   bank as a sample `.cvb`; `xvfb-run -a VaultTests -snapshot editor.png` paints the editor into a PNG.

## The factory bank (`resources/vault/factory.cvb`, generator `VaultBankGen`)

5,000 presets, generated by `tools/vault/BankGen.cpp` (console target `VaultBankGen`) and committed, so builds / CI never run it.
QC report of the shipped bank: `docs/VAULT_BANK_REPORT.md` (written by the generator).

```
cmake --build build-vault --target VaultBankGen
build-vault/VaultBankGen_artefacts/Release/VaultBankGen [-threads N] [-scale 0.1] [-out file.cvb] [-report file.md] [-dup 3.0]
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

VaultTests (with the bank embedded) checks: exactly 5,000 presets, per-category and per-layer counts, unique ids and names,
names / labels clean and short, 1-3 styles and characters, and 200 random presets (fixed seed) re-rendered at their final gain
through the same gate (incl. loudness) and mutually distinct.

## API for the bank generator (`Source/vault/VaultEngine.h`)

```cpp
// Offline, no GUI / plugin needed, thread-safe (each call has its own engine - render presets in parallel).
// Note on at 0, note off at 70 % of `seconds`, release tail to the end. Stereo, 2 channels. Macro defaults of the preset, volume 1.
static juce::AudioBuffer<float> VaultEngine::render (const VaultPreset& preset, int midiNote, float velocity, double seconds, double sampleRate);
static juce::AudioBuffer<float> VaultEngine::render (const VaultPreset& preset, int midiNote, float velocity, double seconds, double sampleRate,
                                                     const float* macros /* 4 values or nullptr */);
static juce::AudioBuffer<float> VaultEngine::renderMidi (const VaultPreset& preset, const juce::MidiBuffer& midi, int numSamples,
                                                         double sampleRate, const float* macros = nullptr);
static float VaultEngine::measureLoudnessDb (const juce::AudioBuffer<float>& buffer, double sampleRate);   // loudest 400 ms RMS, dBFS
static constexpr float VaultEngine::targetLoudnessDb = -15.0f;
// loudness: render at gain 1, then preset.gain = juce::Decibels::decibelsToGain (targetLoudnessDb - measured)
static VaultCompiledPreset VaultEngine::compile (const VaultPreset& preset);
```

Rendering is deterministic (same preset + note = same samples), so near-duplicate detection can compare renders.
Use note 36-48 for 808 & Bass, 60 for the rest. The safety limiter holds every output under 0.98; a preset that hits
it is too hot - lower its gain (check the render peak < 0.89 before the limiter would act, i.e. keep loudness at the target).

Engine (real-time) API, used by the processor:

```cpp
struct VaultTransport { double bpm = 120.0; double ppq = 0.0; bool playing = false; };
class VaultEngine {
    void prepare (double sampleRate, int maxBlockSize, int numPartsToPrepare = vault::maxParts,
                  bool crossfadeSlot = true);                            // allocates; false: one slot, switches are instant
    void releaseResources();
    bool isPrepared() const;
    void setPreset (const VaultCompiledPreset& p, bool instant = false);   // audio thread, no allocation, 40 ms crossfade
    void setMacro (int index, float value01);  float getMacro (int index) const;   // any thread
    void setVolume (float linearGain);                                           // any thread, smoothed
    void process (juce::AudioBuffer<float>& buffer, const juce::MidiBuffer& midi, const VaultTransport& t = VaultTransport());
    void allNotesOff (bool hard);
    static constexpr int chunk = 128;  static constexpr float monoBelowHz = 120.0f, sideHighPassHz = 160.0f;
};
```

## API for the editor (`Source/vault/VaultProcessor.h`)

The editor class must stay `class VaultEditor : public juce::AudioProcessorEditor` with constructor
`explicit VaultEditor (VaultProcessor&)` in `Source/vault/VaultEditor.h/.cpp` (created by `VaultProcessor::createEditor`).
Everything below is message-thread API:

```cpp
class VaultProcessor : public juce::AudioProcessor {
    static constexpr const char* idPreset = "presetIndex";                        // AudioParameterInt 0..4999, automatable
    static constexpr const char* idMacro[4] { "macro1", "macro2", "macro3", "macro4" };   // TONE SPACE DRIVE MOTION 0..1
    static constexpr const char* idVolume = "volume";                             // dB -60..+6, default 0 (-60 = mute)
    static constexpr int maxPresets = 5000;
    enum class BankSource { embedded, file, factoryFallback };

    const std::vector<VaultPreset>& bank() const;    // immutable for the plugin's lifetime (safe to keep indices / pointers)
    BankSource bankSource() const;
    juce::String bankDescription() const;            // "757 presets (starter bank)"
    void selectPreset (int index);                   // instant (next audio block), resets the 4 macros to the preset defaults,
                                                     // notifies the host (sets the presetIndex parameter)
    int currentPreset() const;                       // index into bank()
    const VaultPreset* currentPresetData() const;
    int findPreset (const juce::String& id) const;   // -1 if unknown
    float getMacro (int index) const;                // 0..1
    void setMacro (int index, float value01);        // with host notification (or attach sliders to apvts / idMacro[i])
    float getVolumeDb() const;  void setVolumeDb (float dB);
    void previewNote (int midiNote, float velocity = 0.8f, double seconds = 0.9);   // audition on click (any thread)
    float getOutputPeak() const;                     // peak of the last block (meter)
    juce::MidiKeyboardState& getKeyboardState();     // for a juce::MidiKeyboardComponent
    juce::AudioProcessorValueTreeState apvts;        // SliderAttachment etc.
    static std::vector<VaultPreset> buildFallbackBank();
    static int vaultCategoryOfFactory (int kkCategory);
};
```

## Editor UI (`VaultEditor`)

Laid out at the base size **1180 x 760** and scaled as one piece with an affine transform (vector drawing, crisp at any
scale): 80 / 90 / 100 / 115 / 130 / 150 / 175 / 200 % from the scale pill in the status bar (sizes that would not fit
the screen are greyed out), or by dragging the corner (aspect kept). Minimum 80 % = 944 x 608.
First open: the largest preset up to 100 % (never more, even on 4K) that fits 85 % of the screen's usable area (the
display showing the editor, else the main display): 1366 x 768 laptop -> 80 %, 1920 x 1080 and up -> 100 %. Afterwards the
user's choice is remembered in the settings file and restored, shrunk to the largest preset that fits the screen it opens
on (200 % chosen on 4K opens at 90 % on a 1366 x 768 laptop; the 200 % choice stays saved).
Readability: no text is smaller than 14 px in the base layout (11.2 px at 80 %), text colours are never darker than
#8593AB, primary text #E3E8F0; `VaultTests -uitest` paints the editor at 80 / 100 / 150 / 200 % and checks the smallest
font, and that every bank name and "category · styles" line fits its list column without "...".

* Top bar: chrome logo, search (every word must match the name, category, layer labels, style / character tags or
  "n layers"; filters as you type; Up / Down still step while typing, Esc clears, Return goes back to the list),
  current sound with ◀ ▶ (wrap through the filtered list), RANDOM (from the filtered list), RECENT (last 30 played,
  newest first), FAVORITES.
* Left: All Sounds + 16 categories with the real counts of the loaded bank.
* Centre: chips STYLE / CHARACTER (one per row, click again = off) / LAYERS (All 1 2 3 4), "Clear filters";
  the list is a `juce::ListBox` (only visible rows exist). Row: selected dot, name, category · styles, layer badge, heart.
  Single click = load + a short audition note (C4, C3 for 808 & Bass) when "Preview on click" is on (default).
* Right: name, category, tag pills, waveform thumbnail (rendered by `VaultThumbs` on a low-priority thread with
  `VaultEngine::render`, cached; never blocks), LAYERS · n stack (A-D, label, transpose, level %), DRAG WAV (renders the
  current sound with the current macros, C4, 2 s, 44.1 kHz 24-bit stereo, into the temp folder `CHROME VAULT/` and starts an
  external file drag), the macro knobs TONE SPACE DRIVE MOTION (ring = distance from "as designed", double click = preset
  default) and VOLUME, all attached to the APVTS parameters.
* Bottom: status bar (Showing N of total, hints, temporary messages), Preview on click, scale; 4-octave keyboard C2-B5
  (`juce::MidiKeyboardComponent` restyled) feeding `getKeyboardState()`.
* Keys (when the search box does not have focus): Up / Down = next / previous sound (clamped), Left / Right = same with
  wrap, Space / Return = audition again, PC keys `A W S E D F T G Y H U J K O L P ;` play from C4, Z / X octave down / up.
  Clicking anywhere except the search box gives the keys back to the browser. `EDITOR_WANTS_KEYBOARD_FOCUS` is TRUE for
  ChromeVault (needed for typing on macOS).
* Settings (`VaultSettings`, one shared object per process, other open windows follow favorite changes):
  `juce::PropertiesFile` "CHROME VAULT.settings" in `userApplicationDataDirectory/TrapWin/CHROME VAULT/`
  (macOS: `~/Library/Application Support/TrapWin/CHROME VAULT/`): `favorites` and `recent` (preset ids, one per line),
  `uiScale`, `previewOnClick`.
* Palette (from the approved mockup): ink `#0B0F16`, panels `#111722` / `#151C28`, lines `#222B3A`, text `#E3E8F0`,
  secondary `#A9B6CC`, tertiary `#8593AB` (darkest colour used for text), ice `#9FC3FF` / `#7FA6E8` / deep `#4676C4`
  only for selection, focus and active chips. Fonts: JUCE default sans, bold weights.
* Check it: `xvfb-run -a VaultTests -uitest` (behaviour) and `xvfb-run -a VaultTests -shot out.png [state]` (PNG of the
  real UI; states: `main` (default), `scale:70`, `search:dark bell`, `cat:<n>`, `style:<n>`, `char:<n>`, `layers:<n>`,
  `favs`, `recent`, `empty`, `keys`, comma-separated). Without a real bank both use a generated 5,000-preset demo bank.

State (`getStateInformation`): XML `CHROMEVAULT` with `presetId` (restored by id first, so bank re-ordering is safe),
`presetIndex`, `macro1..4`, `volume`.

## Build and test

```
cmake -B build-vault -DCMAKE_BUILD_TYPE=Release [-DKK_JUCE_DIR=/path/to/JUCE]
cmake --build build-vault --target ChromeVault_VST3 ChromeVault_Standalone VaultTests -j8
build-vault/VaultTests_artefacts/Release/VaultTests          # "vault failures: 0"
pluginval --strictness-level 8 --skip-gui-tests --validate "build-vault/ChromeVault_artefacts/Release/VST3/CHROME VAULT.vst3"
```

VaultTests covers: key table == `createLayout()`, serializer round trip (gzip + raw, unknown IDs, UTF-8), damaged files refused,
render of hand-made 1-4 layer presets + two presets per category (non-silent, finite, peak < 0.99, side under 90 Hz < -30 dB,
clean note start / end, deterministic, macros at 0 / 1), loudness range, preset switch with a held note (no click, note
continues), processBlock, state save / restore, macro reset on select, presetIndex automation.

## EVOLVE refactor note

`KeysKillaProcessor::buildVoiceParams` now calls `kk::buildVoiceParamsFrom` with a lookup into its APVTS / morph values
(`Idx::vk` maps `kk::vk` keys to parameter indices). Behaviour is unchanged (same code, same order; EVOLVE unit tests pass).
Never reorder `KK_VOICE_KEYS` - append only.
