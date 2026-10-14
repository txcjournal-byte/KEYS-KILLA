# CHROME VAULT - vocal & choir preset design guide

For the sound designers of the **Vocals & Choir** category (and vocal layers in Pads / FX & Atmos). It summarises how
professional vocal and choir instruments organise their content, what that means for our 4-part presets, and the
ear-safety rules that apply to voices in particular. The available recorded material is listed in
`docs/VAULT_VOCAL_SOURCES.md`; everything it does not cover comes from the voice model (model part).

Home note: MIDI 60 = **C5** (FL Studio naming). Every rule below is checked at C5.

## 1. How the big libraries are organised

### Choir libraries (orchestral / cinematic)

| Axis | What they offer | Notes |
| --- | --- | --- |
| **Sections** | Soprano, Alto, Tenor, Bass; Women (S+A), Men (T+B), Full (SATB); often Boys / Children; Solo voices | Each section is a separate keyrange: B ~E2-E4, T ~C3-A4, A ~F3-D5, S ~C4-A5. Playing a section far outside its range is the most common "fake" tell. |
| **Ensemble size** | Solo, chamber (4-8 per section), symphonic (20-60) | Big = slower attack, wider stereo, natural chorus (no single pitch). Small = intimate, clear consonants. |
| **Vowels** | Ah, Oh, Oo, Ee, Eh, Mm (closed hum), Nn | Ah/Oh = bright and big, Oo/Mm = dark and soft (pads), Ee = most piercing (strong 2-3.5 kHz, use sparingly). Many libraries morph vowels with the mod wheel (Oo <-> Ah). |
| **Sustains** | Non-vibrato and vibrato; dynamic layers pp-ff crossfaded on CC1 | The vowel colour changes with dynamics (ff opens the formants), so a crossfade is better than plain volume. |
| **Shorts** | Staccato / marcato on syllables ("ta", "pa", "da", "ka", "ah"), round robins | Used for rhythmic ostinati in trailers. |
| **Swells / dynamics** | Crescendo, diminuendo, sforzando-piano, "breath swells" of 2-8 s | Tempo-free; the best-selling "epic" patches are swells layered under strings. |
| **Legato** | True-legato transitions on Ah/Oh (sometimes Oo) | Hard to fake with samples; our models can do it (glide + formant continuity). |
| **Phrase / word building** | Syllable sequencers ("WordBuilder" style, 40+ syllables, long + short versions); Latin text phrases | Words are easiest on one held note; legato and words rarely combine well (user reports). |
| **Effects** | Clusters, glissandi (up/down), whispers, shouts, aleatoric murmurs, breaths, "ghost" choirs | These live in FX & Atmos. |
| **Mic positions** | Close / stage / hall, mixed with faders | We emulate with the SPACE macro (dry -> hall). |

Sources: reviews of large SATB choir libraries (Sound On Sound, MusicTech, ask.audio; Wikipedia "Symphonic Choirs"), VSL
"Studio Choir" product page (syllable staccatos: aa / pa / ra / sa / ta, uu / pu / ru / su / tu), VI-Control user reports.

### Pop / modern vocal instruments

The leading "vocal engine" instruments split their library into three play modes (reviews of Output Exhale, MusicRadar /
ask.audio / Dubspot / Produce Like A Pro):

1. **Notes** - chromatic one-shots and sustained "pads" (an "ooh", "ah", "hey"), two sources layered per preset, each
   with its own volume / pan / tune / reverse / EQ / ADSR.
2. **Loops** - tempo-synced phrases, one per key (banks of 13).
3. **Slices** - a phrase chopped into 13 slices across the keys (REX-style) = "vocal chops".

Presets are found with tag filters (genre / mood / type). For CHROME VAULT the owner removed Style and Character tags, so
the **name and the category carry everything**: the name must say what the sound is (see section 4).

### General MIDI / romplers

GM has three voice programs: **Choir Aahs** (53), **Voice Oohs** (54), **Synth Voice** (55). Romplers expand these into
families: Aahs / Oohs / Mmms in Men / Women / Mixed, "Vox pads", "breathy vox", "vocoder" leads, "angel" pads,
"female ah" one-shots (the trap / hip-hop staple), "choir hits" (orchestral stab), whisper pads.

## 2. Translation to CHROME VAULT presets (4 parts, macros TONE / SPACE / DRIVE / MOTION)

Every preset should be clearly different from its neighbours: change **section, vowel, articulation and treatment**
together, not just the reverb.

| Preset family | Parts (suggested) | Character rule |
| --- | --- | --- |
| **Female "Ha" / "Eh" chop** | 1 sample part (Female Vowel Chops `eh` or `eh_breathy`, one-shot) + short room | Dry, short, punchy, played in the C4-C6 range; the classic trap / pop chop. |
| **Female solo vowel** | model voice (soprano, Ah) + sample `voice.female.solo` ah/oo as attack | Slow vibrato (MOTION), soft attack 40-80 ms, SPACE = plate. |
| **Choir Aahs / Oohs (mixed)** | 3-4 model voices: S+A+T+B (detuned 3-8 c, panned L-R, attack 150-300 ms) | Each section limited to its range (key-split by part octave / range); Oo for dark, Ah for bright. |
| **Men's choir** | T + B model voices, Oh vowel, octave doubled with bass at -10 dB | Under 1.5 kHz most of the energy: warm, never shrill. |
| **Women's choir** | S + A model voices, Oo / Ah, airy noise layer -24 dB | Watch 2-5 kHz: S at C6 with Ee is the classic ear-piercer. |
| **Children / boys choir** | S model with less vibrato, brighter formants, smaller ensemble | Straight tone, no vibrato, short release. |
| **Male "Hum" / "Mm"** | `voice.male.low` `ah` (low-passed) + model Mm | Closed-mouth: almost only harmonics 1-4. |
| **Whisper / breath** | noise model through vowel formants (no recorded breath in the library yet) | Unpitched; in FX & Atmos or as an attack layer. |
| **Vocal pad (ambient)** | 2 model voices (Oo / Ah) + Classic Choir `oohs` / `aahs` or Female Solo `oo`, attack 0.8-2 s, long release, big hall | The owner's "very ambient" pads: slow vowel morph on MOTION, TONE opens the formants. |
| **Choir hit / stab** | full SATB "Ah" short + brass layer | Orchestral; one-shot with a fast release. |
| **Swell** | SATB with a 2-4 s crescendo envelope | Layer under strings presets. |
| **Vox lead / vocoder-like** | model voice with glide (portamento 60-120 ms), mono | In Leads, not Vocals. |
| **Vocal FX** | Classic Choir `aahs` reversed / pitched far down, clusters (several parts a semitone apart), glissandi (model) | In FX & Atmos. |

Velocity: map velocity to brightness (formant opening / TONE) more than to level - a softly sung note is darker, not just
quieter. Mod wheel: dynamics crossfade or vowel morph. Release: voices need 0.25-0.6 s release tails (breath) - cutting
them makes the preset sound like a synth.

## 3. Ear-safety rules for voices (all measured at C5, velocity 100)

1. **Fundamental first**: the sung fundamental is the loudest partial or within 3 dB. Bright vowels (Ee, Eh, belted Ah)
   violate this easily - low-pass (TONE) or choose Oo / Oh at high pitches.
2. **2-5 kHz share**: keep it under ~10 % of the energy. The singer's formant (F3-F5 cluster at 2.5-3.5 kHz) is what makes
   operatic voices carry - and what hurts on headphones. A gentle -3..-6 dB dip at 3 kHz is the standard fix.
3. **Sibilants / breath noise**: 5-10 kHz breath noise in chops must stay at least 20 dB below the voiced part.
4. **Octave-up layers** (e.g. a soprano over a choir) at least 12 dB under the main part.
5. **No pitch shifting beyond +-5 semitones of a recorded note** for natural voices: further up gives "chipmunk"
   formants, further down a "monster" (zones are limited by `extendKeys`). For wider ranges use the model.
6. **Belted / shouted material** (VocalSet `belt`, 2-5 kHz share 21 %) is not used: it fails rule 2.

## 4. Naming

Instrument nouns only, no maker / library names in the UI: "Female Ah Chop", "Choir Oohs", "Men's Choir Oh",
"Soprano Solo", "Whisper Pad", "Angel Pad", "Choir Swell", "Boys Choir", "Low Male Hum". Each name unique and
descriptive (the owner has no tags to fall back on).

## 5. What the recorded material can and cannot do

See `docs/VAULT_VOCAL_SOURCES.md`. In short: a GM-grade mixed choir (Aahs D#2-A5, Oohs C3-C6, a solo "vox" C3-C6), one
female singer's scale notes (Eh straight / breathy, C#4-D#5), two single sustained female notes (Oo at G#4, Ah at E4) and
an amateur low male voice (Ah, D2-A#3, dark and muffled). There is **no free multi-sampled choir** (SATB sections,
vowels, dynamics) reachable from GitHub under a commercial licence, so choirs, ensembles, Mm / Nn hums, whispers with
pitch, and anything that needs legato come from the voice model; the samples serve as attack transients, chops and
texture layers.
