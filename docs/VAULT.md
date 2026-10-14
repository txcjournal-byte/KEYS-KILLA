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

* Done (core): part kinds + `CVB` v2 (v1 still read), the multi-zone sampler, the sample library format + loader + LRU
  cache, the library builder `VaultLibBuild` with 4 test instruments in `/home/user/cv-library`, the model interface +
  registry (+ the `sine` example), the 16 v2 categories, Style / Character tags removed everywhere, C5 + keyboard-focus
  fixes in the editor, tests.
* Not done here (parallel / later work): the v2 factory bank (the embedded `resources/vault/factory.cvb` is still the v1
  synth bank, read through the v1 path: categories mapped, tags dropped, "808" replaced by "Sub" in names / labels), the
  full instrument map (all VSCO / VCSL instruments in `tools/vault/library_map.json`), the physical models themselves,
  installer packaging of the library.

## Files

| File | What |
| --- | --- |
| `Source/vault/VaultTypes.h/.cpp` | `VaultPart` (kind, source, articulation, values), `VaultPreset`, the 16 categories, sample-parameter table, home note, the `CVB` bank format (v2 written, v1 read) |
| `Source/vault/VaultLibrary.h/.cpp` | `vault::SampleLibrary` (library.json index, background loader, LRU cache, location search), `InstrumentHandle`, `SampleInstrumentData` (decoded zones, `pick()`) |
| `Source/vault/VaultSampler.h/.cpp` | `vault::SamplerPart` (64-voice multi-zone sampler) and `vault::SincTables` (16-tap windowed-sinc interpolation) |
| `Source/vault/models/ModelVoice.h` | the model interface (`vault::models::ModelVoice`, `ModelInfo`, `ModelRegistry`) |
| `Source/vault/models/ModelRegistry.cpp` | the registry + `registerBuiltinModels()` (one line per model) + the `sine` example model |
| `Source/vault/VaultEngine.h/.cpp` | `VaultEngine` - 4 parts of any kind -> master FX -> macros -> mono lows -> trim -> volume -> limiter; offline `render()` |
| `Source/vault/VaultProcessor.h/.cpp` | `VaultProcessor` - the `juce::AudioProcessor` (parameters, bank loading, library pins, state) |
| `Source/vault/VaultEditor.h/.cpp` | the editor (browser UI); `VaultMainView` / `VaultInfoPanel` are implemented here too |
| `Source/vault/ui/*.h` | header-only UI parts: `VaultTheme.h`, `VaultBrowserModel.h` (search / filters / favorites / recent), `VaultSettings.h`, `VaultThumbs.h` (thumbnails at C5), `VaultWidgets.h` (pills, categories, LAYERS chips, preset list, keyboard, DRAG WAV), `VaultMainView.h` |
| `tools/vault/LibBuild.cpp` | console target `VaultLibBuild`: builds the sample library from the CC0 sources |
| `tools/vault/library_map.json` | the builder's map: instruments -> source folders, file-name patterns, articulations, options |
| `tools/vault/BankGen.cpp` | console target `VaultBankGen`: the v1 synth-bank generator (legacy; ported to compile with v2 types) |
| `tools/vault/BankQC.h`, `BankAudit.h` | v1 quality gate / audit (shared with VaultTests) |
| `tests/VaultTests.cpp` | console target `VaultTests` |
| `resources/vault/*.cvb` | the preset bank(s) embedded into the plugin |

Every `Source/vault/*.cpp` and `Source/vault/models/*.cpp` is compiled into `ChromeVault` and `VaultTests` automatically
(CMake `file(GLOB ... CONFIGURE_DEPENDS)`); console tools use `CV_ENGINE_SOURCES` + `CV_MODEL_SOURCES`.
`JUCE_USE_OGGVORBIS=1` is set for ChromeVault, VaultTests and VaultLibBuild.

## Architecture

```
MIDI ─┬─> part 0  synth:  kk::SynthEngine (32 voices)                      ─┐  x level, pan (equal power), part gain
      ├─> part 1  sample: vault::SamplerPart (64 voices) <- InstrumentHandle ├─> sum ─> kk::FxRack (FX values of PART 0)
      ├─> part 2  model:  pool of vault::models::ModelVoice (ModelInfo::maxVoices)
      └─> part 3  (any kind; transposed by octave*12+semi)                   ─┘
   ─> side high-pass 160 Hz 8th order (mono below ~120 Hz) + 5 Hz DC blocker on the mid
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
| `s.oneShot` | -1/0/1 | -1 | -1 articulation default, 0 note-off releases, 1 plays to the end |

`validatePreset`: synth parts take EVOLVE keys only and no `source`; sample parts need a `source` and take EVOLVE keys +
the `s.*` table (range-checked); model parts need a `source` and take EVOLVE keys + any `m.*` key (the model's table is
applied at compile time, unknown `m.*` keys are ignored, values clamped to the model's ranges).

## Bank file format `CVB` (v2; v1 still read)

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
```

Run from the repo root. Output defaults to the map's `output` (`/home/user/cv-library`, outside the repo; never commit
audio). Instruments not rebuilt in a run are kept in `library.json`. `build_report.txt` lists every file (detected pitch,
cents, aperiodicity, loudness, gain, key / velocity range, loop points and correlation, warnings, dropped takes).

Map format: `roots` (name -> folder), `output`, `defaults`, `instruments[]` = `{ id, name, family, credit, articulations[] }`,
articulation = `{ name, sources[], options }`, source = `{ "files": "<root>:<folder>/<glob>", "pattern": "...", "trigger":
"release", "levelDb": -22 }`. Pattern placeholders: `{note}` (C4, F#3, Bb2, A#-1), `{vel}` (any token: 1, vl3, soft), `{rr}`
(1-based number), `*` anything. Options (defaults in `defaults`, override per articulation): `loop`, `oneShot`, `release`,
`releaseDecayDb`, `maxSeconds` (16), `tailFadeMs` (250, when capped), `preRollMs` (4), `silenceDb` (-62), `peakDb` (-1, the
instrument's loudest peak), `minLoopMs` (300), `maxLoopMs` (2600), `xfadeMs` (100; 150 for weakly correlated loops),
`extendKeys` (6: the outermost zones reach this many semitones beyond their root), `quality` (6 = Ogg q0.6),
`velOrder` (layer names soft -> loud, else numeric order), `layerDb` (imposed layer levels for sources that were
normalised per layer, e.g. `{ "2": -15, "3": -7.5, "4": 0 }`), `tune` (`"correct"` default / `"keep"`: keep the natural
tuning, fix only > 30 cents), `pitchFrom` (s, where the pitch of a one-shot is measured; 0.5 for pianos - the attack glides sharp;
set explicitly it also applies to loops, which otherwise are measured over the whole sustain), `pitchSpan` (s, length of
the pitch analysis; 0 = 0.35 s for one-shots / the sustain for loops; voices use 0.15 + 0.8 s). Map-level keys: `report`
(report file name, default `build_report.txt`) and `credits` (a line such as "Vocals: ..." merged into `library.json`'s
`credits`; a later build of the same map replaces its line). A second map, `tools/vault/library_map_vocals.json`, adds
the vocal instruments (prepared by `tools/vault/vocal_prep.py`; see `docs/VAULT_VOCAL_SOURCES.md`).

Per file: read; resample to 44.1 kHz (windowed sinc) if needed; identical channels -> mono; mean + 5 Hz DC removal; trim to
the first / last sample above `silenceDb` of the peak with a pre-roll before the onset (-40 dB) and a raised-cosine fade-in
over it; length cap with a tail fade; YIN (threshold 0.15, parabolic interpolation, median of the frames every half window
(7-64; only the clearly periodic ones when there are at least 3), search 0.45-2.2 x the named pitch) -> the naming
convention offset per articulation (VSCO / VCSL names are an octave off: corrected and reported), single mismatches within
2 semitones corrected, larger ones reported, the residual detune written as `fine` (a corrected root that collides with a
take of the same note / layer / round robin gets the next free round robin - the output name would otherwise clash);
loudness (BS.1770 K-weighting, loudest 400 ms); loops for `loop` articulations (rising zero crossings of the mid in the
stable sustain, normalized cross-correlation of [-xfade, +12 ms] around both points minus level mismatch, bonus for
length; >= 300 ms). Per articulation: velocity layers matched to a smoothed per-register loudness curve of the top layer
plus the layer offset (natural median spacing kept, at least 2.5 dB apart, whole range <= 18 dB, or `layerDb`); takes more
than 12 dB off their layer (vs the top layer of the same note) are dropped as broken; release samples set `levelDb` under
their note; one gain normalises the instrument peak to `peakDb`; looped files are cut 300 ms after the loop end; encoded
OggVorbis q 0.6. The builder finally reads its own output with `vault::SampleLibrary`.

### The first build (`/home/user/cv-library`, 4 instruments, 42 MB)

| id | name | source | articulations | notes |
| --- | --- | --- | --- | --- |
| `piano.grand.concert` | Concert Grand | VCSL grand piano, close mics, pedal up | `sustain` (3 velocity layers, whole-tone zones A#0-G#7, 99 release samples) | layers imposed -15 / -7.5 / 0 dB (source normalised) |
| `strings.violins.ensemble` | Violin Section | VSCO 2 CE violin section sustain vibrato | `sustain` (looped, 2 layers, 11 notes G3-D6) | low notes have a weak fundamental (at C5 the 2nd harmonic is +20 dB, as in the recording) |
| `winds.flute.concert` | Concert Flute | VSCO 2 CE flute | `sustain` (straight, looped, 3 layers), `vibrato` (looped, 1 layer, round robins) | |
| `mallets.marimba` | Marimba | VCSL marimba, outrigger mallets | `hit` (one-shot, 3 layers soft / med / loud, 10 notes) | 2 broken soft takes dropped; tuning kept |

Measured at C5 (velocity 100, `VaultTests -libreport`): in tune within 2.5 cents, peaks -7..-14 dBFS, 2-5 kHz share
0-8.5 %, loops: 31 wraps of the violins within 1.23x the surrounding sample jumps, crossfade level dip -1 dB.

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

## Editor UI (`VaultEditor`) - v2 changes

* Categories: the 16 v2 categories. Filters: LAYERS (All 1 2 3 4) + Clear filters (STYLE / CHARACTER rows removed).
  List info column: the category (All Sounds / search) or, inside a category, the first layer's name. Search matches
  name, category, layer labels, "sampled" / "modelled", instrument ids and articulations.
* Info panel pills: Sampled / Modelled / Synth, "Loading..." / "Samples missing" for the current sound, "Plays at C5".
* Preview on click, DRAG WAV (file "<name> C5.wav") and thumbnails: MIDI 60 for every category.
* Keyboard: `setOctaveForMiddleC (5)`, labels C3 C4 **C5** (bold, blue, dot) C6.
* **Keyboard focus**: the editor never uses a key. Nothing takes the keyboard on click except the search box
  (`setWantsKeyboardFocus (false)` + `setMouseClickGrabsKeyboardFocus (false)` on every component, list rows included as
  the list creates them; the editor itself too); Return, Escape (also clears the text) and a click anywhere else hand the
  keyboard back (`giveFocusBack()`: `giveAwayKeyboardFocus` + `unfocusAllComponents`), so the DAW's computer-keyboard piano
  and shortcuts always work. The PC-key note playing (A-L, Z / X) and arrow / Page / Home / End key browsing are removed;
  ◀ ▶ and Up / Down while typing in the search box still step. `EDITOR_WANTS_KEYBOARD_FOCUS` stays TRUE (macOS needs it
  for typing into the search box).
* Status bar: "Click a sound to hear it at C5 · ◀ ▶ next sound · Instant load", or "Loading samples..." / "Sample library
  not found ..." for the current sound.

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
  `favs`, `recent`, `empty`, `keys`, comma-separated). Without a real bank both use a generated 5,000-preset demo bank.

State (`getStateInformation`): XML `CHROMEVAULT` with `presetId` (restored by id first, so bank re-ordering is safe),
`presetIndex`, `macro1..4`, `volume`.

State (`getStateInformation`): XML `CHROMEVAULT` with `presetId` (restored by id first), `presetIndex`, `macro1..4`, `volume`.

## Build and test

```
cmake -B build-vault -DCMAKE_BUILD_TYPE=Release [-DKK_JUCE_DIR=/path/to/JUCE]
cmake --build build-vault --target ChromeVault_VST3 ChromeVault_Standalone VaultTests VaultLibBuild KeysKilla_VST3 KeysKillaTests -j4
build-vault/VaultTests_artefacts/Release/VaultTests                    # "vault failures: 0" (real-library checks run when a library is found)
CHROME_VAULT_LIBRARY=/home/user/cv-library build-vault/VaultTests_artefacts/Release/VaultTests
xvfb-run -a build-vault/VaultTests_artefacts/Release/VaultTests -uitest
CHROME_VAULT_LIBRARY=/home/user/cv-library build-vault/VaultTests_artefacts/Release/VaultTests -libreport /tmp/renders 48 60 72
build-vault/VaultTests_artefacts/Release/VaultTests -writev2demo /tmp/demo_v2.cvb   # 5 v2 presets on the test instruments
CHROME_VAULT_BANK=/tmp/demo_v2.cvb CHROME_VAULT_LIBRARY=/home/user/cv-library build-vault/VaultTests_artefacts/Release/VaultTests -wav 0 piano.wav 60 3
pluginval --strictness-level 8 --skip-gui-tests --validate "build-vault/ChromeVault_artefacts/Release/VST3/CHROME VAULT.vst3"
```

VaultTests (v2 additions): sampler zone selection (key range, velocity layer, nearest layer), round-robin rotation,
pitch at C5 from zones 3 semitones away (+-5 cents, YIN) at 44.1 and 48 kHz, loop seamlessness on a deliberately
non-periodic loop (max sample jump around 4 wraps <= 1.08x the steady signal), release fade (-60 dB in the release time,
no click), one-shot ignores note-off (OGG file), missing file -> failed + silent, unknown instrument / missing library ->
silent, `libraryFound()` false for a missing folder, silent-while-loading then playing, a used instrument survives a 0-byte
budget while unused ones are evicted, no allocation in `VaultEngine::process` (operator-new counter, 400 blocks with notes,
pedal, bend, macros, sample / synth / model switches), model registry + the sine model in tune at C5, unknown model silent,
CVB v2 round trip with all kinds, unknown sample keys refused, v1 file read (tags dropped, categories mapped, "808"
removed), the embedded v1 bank read; with a library: every instrument / articulation at C5 clean, in tune within 5 cents,
loops click-free (jump at each wrap < 1.3x the music around it, crossfade dip > -3 dB). UI: no component but the search
box takes the keyboard (all components incl. list rows), Return / Escape hand it back, no key is used by the editor,
keyboard labelled C5 at MIDI 60, DRAG WAV "<name> C5.wav", plus the v1 UI checks (speed, sizes, readability, state).

## The v1 factory bank (legacy: `resources/vault/factory.cvb`, generator `VaultBankGen`)

> v1 synth-only bank, still embedded until the v2 bank replaces it. `BankGen.cpp` still builds: it plans / tags / names in the
> OLD category list internally (`v1c` namespace: old enums, targets, styles / characters only steer the names) and writes
> v2 presets (v2 categories via `vault::v1::toV2Category`, no tags, "808" -> "Sub"). The text below describes the v1 run.


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

VaultTests (with a bank embedded) checks today: 1,000..5,000 presets, at least 12 of the 16 categories used, unique ids,
names (99 %) clean and short, labels clean, no "808" in any name / label, and 200 random presets (fixed seed) re-rendered
through the v1 gate (incl. loudness, +-1.5 dB: v1 arps now sit in Synth) and mutually distinct. (v1 originally also checked
exact per-category / per-layer counts and the tags.)

## EVOLVE refactor note

`KeysKillaProcessor::buildVoiceParams` now calls `kk::buildVoiceParamsFrom` with a lookup into its APVTS / morph values
(`Idx::vk` maps `kk::vk` keys to parameter indices). Behaviour is unchanged (same code, same order; EVOLVE unit tests pass).
Never reorder `KK_VOICE_KEYS` - append only.
