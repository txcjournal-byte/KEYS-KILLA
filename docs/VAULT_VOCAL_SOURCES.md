# CHROME VAULT - recorded vocal sources (search, licences, build, QC)

Goal: real recorded singing voice / choir material, legally usable in a commercial plugin (CC0, public domain, CC BY, or an
equally permissive licence such as MIT), downloadable from GitHub (the only reachable host). Result: **four small vocal
instruments** are built into the library; **no professional multi-sampled choir exists under such a licence on GitHub**,
so ensembles, hums, whispers and legato remain the job of the voice model. Design guidance: `docs/VAULT_VOCAL_DESIGN.md`.

## 1. What is used

| Instrument (`id`) | Articulations | Source | Licence (verified where) |
| --- | --- | --- | --- |
| **Classic Choir** (`voice.choir.classic`) | `aahs` (mixed choir, stereo, 15 zones D#2-A5 in minor thirds, looped), `oohs` (7 zones C3-C6, looped), `solo_vox` (solo "doo/ooh" voice, 4 zones C3-C6) | Fluid (R3) GM SoundFont, programs 52 "Ahh Choir", 53 "Ohh Voices", 85 "Solo Vox" (the "Bottle Chiff" noise layer and the C7 zone - 99.7 % of its energy in 2-5 kHz - left out) | **MIT**, (c) 2000-2002, 2008 Frank Wen. `COPYING` + `README` in github.com/pianobooster/fluid-soundfont (the .sf2 is the v3.1 release asset). README: built "in part from samples found in the public domain ... and largely from recordings of my own". Shipped in Debian main / Ubuntu / MuseScore. |
| **Female Vowel Chops** (`voice.female.chops`) | `eh` (straight tone, 10 one-shots C#4-D#5), `eh_breathy` (12 one-shots C#4-D5) | VocalSet, singer "f2", demo clips `f2_scales_straight_e.wav`, `f2_scales_breathy_e.wav`, sliced into single notes | **CC BY 4.0** (VocalSet, Wilkins / Seetharaman / Wahl / Pardo, ISMIR 2018, Zenodo record 1442513). The clips are in github.com/interactiveaudiolab/interactiveaudiolab.github.io (`demos/samples/`), the authors' lab site; that repository has no licence file of its own, the licence is the dataset's (the Zenodo page could not be opened from here - it is confirmed by the derived Annotated-VocalSet record and dataset cards; see section 4). |
| **Female Solo Voice** (`voice.female.solo`) | `oo` (one sustained note G#4, looped), `ah` (soprano, one sustained note E4, looped) | MTG `sms-tools-materials/sounds/singing-female.wav` (first 2.4 s) and `soprano-E4.wav` | **CC BY 4.0** - `sounds/LICENSE` in github.com/MTG/sms-tools-materials ("All sounds in this directory are released under CC BY 4.0 ... from freesound.org pack 13038 by xserra"). |
| **Male Low Voice** (`voice.male.low`) | `ah` (21 notes D2-A#3, 2-7 takes per note as round robins, looped where possible) | github.com/vocobox/human-voice-dataset, singer "martin" (2014, Roland R-05 at 20 cm) | **MIT**, `LICENSE` in that repository, (c) 2014 vocobox. |

Attribution text for all of them is in `docs/CHROME_VAULT_CREDITS.md` and in the `credits` field of `library.json`
(CC BY 4.0 requires credit, a licence link and a note that the material was changed - it was: sliced, trimmed, looped,
level-matched, re-encoded). MIT requires the copyright + permission notice to ship with the product (installer licence
page / credits file).

## 2. How it is built (reproducible)

```sh
# sources (outside the repo; ~160 MB, the .sf2 is 148 MB)
mkdir -p /home/user/vocal-src && cd /home/user/vocal-src
git clone --depth 1 https://github.com/MTG/sms-tools-materials.git
git clone --depth 1 https://github.com/vocobox/human-voice-dataset.git
git clone --depth 1 --filter=blob:none --sparse https://github.com/interactiveaudiolab/interactiveaudiolab.github.io.git ial
git -C ial sparse-checkout set demos/samples
mkdir fluid && curl -L -o fluid/FluidR3_GM.sf2 https://github.com/pianobooster/fluid-soundfont/releases/download/v3.1/FluidR3_GM.sf2
# prepare (numpy only): slice / rename / extract into /home/user/vocal-src/prep
python3 -I tools/vault/vocal_prep.py
# build into the library (other instruments are kept; the credits line "Vocals: ..." is merged)
VaultLibBuild -map tools/vault/library_map_vocals.json      # report: <library>/build_report_vocals.txt
```

- `tools/vault/vocal_prep.py`: slices the VocalSet scales into notes (YIN pitch track, stable >= 0.25 s within 0.6
  semitone, spread <= 45 cents, not sung 42-50 cents between two semitones - 8 ambiguous notes and 2 gliding ones left out),
  renames the vocobox notes by their heard pitch (their octave numbers start at A, and the singer is up to 60 cents flat
  in the upper notes), extracts the SoundFont programs with `tools/vault/sf2_extract.py` (stereo pairs merged; this bank
  leaves the SF2 sample links at 0, pairs are found by their "(L)"/"(R)" names), links single recordings with a note name.
- `VaultLibBuild` changes made for this material (they apply to every map): an explicit `pitchFrom` now also applies to
  looped articulations and `pitchSpan` sets the analysis length (voices drift; they are tuned on 0.15-0.95 s, the part the
  ear hears first and the part the C5 test measures); the pitch median uses frames every half window (7-64) and only the
  clearly periodic ones when there are enough; **a root correction that lands on a note already holding a take with the
  same layer / round robin now gets the next free round robin** (before, both wrote the same output file and one zone
  played the other's audio with the wrong fine tune - this also affects earlier maps); the map may set `report` (report
  file name) and `credits` (merged into `library.json`).

## 3. QC (measured, `VaultTests -libreport`, velocity 100; C5 = MIDI 60)

| Instrument / articulation | C4 (48) | C5 (60) | C6 (72) | Loops |
| --- | --- | --- | --- | --- |
| choir `aahs` | -0.1 c, fund -26.8 dB, 2-5 kHz 2.6 % | -4.5 c, **fund -14.2 dB**, 1.3 % | -0.9 c, fund -4.7 dB, 2.3 % | 15/15 zones looped, wrap jump <= 1.01x, crossfade dip >= -1.3 dB at C5 |
| choir `oohs` | +1.7 c, fund 0 dB, 0.0 % | -0.1 c, 0 dB, 0.1 % | +0.1 c, 0 dB, 0.1 % | 7/7, jump 0.57x |
| choir `solo_vox` | +0.8 c, 0 dB, 0.4 % | +0.4 c, 0 dB, 0.1 % | +0.6 c, 0 dB, 0.0 % | 3/4 (C6 zone plays to its end, 0.65 s) |
| female chops `eh` | (out of range) | +0.5 c, 0 dB, 3.1 % | +0.8 c, 0 dB, 3.7 % | one-shots 0.5-0.8 s |
| female chops `eh_breathy` | (out of range) | +0.5 c, 0 dB, 0.8 % | +0.3 c, 0 dB, 0.9 % | one-shots 0.3-0.6 s |
| female solo `oo` | (out of range) | -3.5 c, 0 dB, 0.1 % | +0.4 c, 0 dB, 0.0 % | 1 zone, jump 0.67x, no dip |
| female solo `ah` | (out of range) | +0.1 c, fund -2.3 dB, 4.2 % | +7.1 c, fund -5.2 dB, 9.4 % | 1 zone, jump 0.96x, no dip |
| male `ah` | -0.1 c, fund -1.0 dB, 0.0 % | -0.8 c, 0 dB, 0.1 % | (out of range) | 62/68 takes looped; jump 0.97x, dip -0.2 dB at C5 |

`VaultTests -unit` with only these four instruments installed: 0 failures (every articulation plays clean at C5, is in tune
within 5 cents and loops without clicks or dips); with no library: 0 failures. Source noise floors: VocalSet -67 dBFS
(about 45 dB under the singing), MTG / Fluid / vocobox clips are trimmed to the voiced part (no measurable floor).

Known weaknesses (for the sound designers):

- **Choir `aahs` fails the C5 comfort rule** (the fundamental is 14 dB under H2/H3 at MIDI 60 - an open "ah" vowel has its
  first formant around 600-800 Hz). It is not shrill (2-5 kHz 1.3 %), but a preset must add body: layer `oohs` or the
  voice model an octave down at -10 dB, or a low shelf; `oohs` and `solo_vox` pass.
- **Female solo `ah` at C6**: H2 5 dB over the fundamental and 9.4 % in 2-5 kHz - use TONE (low-pass ~4 kHz) when
  playing it high; ranges are single samples (`oo` keys 59-77, `ah` 55-73), so they serve as lead / attack layers, not as
  a full instrument.
- **Male Low Voice** is an amateur recording: dark (almost no energy above 1 kHz - centroid ~250 Hz), 1-1.3 s takes with
  0.25-0.4 s loops, intonation corrected per take. Use it as a warm low "hum / ah" layer under the model, never solo.
- **Female chops** cover C#4-D#5 (+5 keys); out of range below MIDI 56 - pitch them with the part octave, not further down.
- The Fluid samples are GM-grade (2-3 s, 17-40 kHz sample rates, single velocity layer): good bodies for "Choir Aahs /
  Voice Oohs" presets, not a replacement for a 20 GB choir library.

## 4. Search log (what was checked, why it was rejected)

Only github.com (+ raw / release downloads) is reachable; zenodo.org, freesound.org, huggingface.co, archive.org,
codeberg.org, freepats.zenvoid.org and pianobook.co.uk are blocked (proxy 403 / no DNS).

| Candidate | Content | Licence | Verdict |
| --- | --- | --- | --- |
| VSCO 2 CE, VCSL (already local) | no voices (only whistles / sirens) | CC0 | nothing to use |
| **VocalSet** (Zenodo 1442513, 10.1 h, 20 singers, 5 vowels, long tones / scales / arpeggios) | ideal content | CC BY 4.0 | the full set is only on Zenodo (blocked); 10 demo clips of one singer are on GitHub -> used (straight + breathy "eh"). Left out: belt "ah" (21 % of its energy in 2-5 kHz - piercing), vibrato "ah" (notes only 0.3 s, tuning not measurable), vocal fry, lip trill, trill / trillo (not notes), the inhale and the spoken clip (unpitched FX; the C5 tuning test cannot pass them). |
| **MTG sms-tools-materials** `sounds/` | soprano E4, female sung phrase, speech | CC BY 4.0 (`sounds/LICENSE`) | used: soprano E4 + the first 2.4 s of `singing-female` (a held G#4). Speech not used. (The `sms-tools` repo itself is AGPL code, no sounds.) |
| **vocobox human-voice-dataset** | 1 amateur male singer, 75 notes + 6 vowels at C3 | MIT | used (notes). The vowel set (one pitch, C3) was built and then dropped: playing it at C5 needs +12 semitones (chipmunk) and its tuning cannot be pinned within 5 cents. |
| **Fluid (R3) GM SoundFont** (pianobooster/fluid-soundfont release) | GM Choir Aahs / Voice Oohs / Solo Vox / Synth Voice | MIT | used (52, 53, 85). Provenance is the author's statement (public-domain samples + own recordings); accepted because the bank has shipped in Debian main and MuseScore for 15+ years. 54 "Synth Voice" / 91 "Space Voice" are synthetic - not used. |
| **Sonic Pi** `etc/samples` | `ambi_choir` (1.6 s choir cluster), `tbd_voctone` (vocal tone) | CC0 (`LICENSE.md` "Samples" section; sources listed in `etc/samples/README.md`) | licence fine, built and then dropped: single short samples, the choir is a cluster (no defined pitch: fails the C5 tuning test), the vocal tone has H3 4.8 dB over the fundamental. Could return as an FX one-shot if the tests get an "unpitched" flag. |
| GeneralUser GS (mrbumpy409/GeneralUser-GS) | GM choir / voice | custom: free for music, but "cannot be 100% sure where all of the samples originated" | rejected: unclear sample provenance for redistribution in a commercial instrument. |
| MuseScore "MS Basic" / "MuseScore_General" | GM choir (derived from Fluid) | MIT, but MuseScore_General's licence notes some samples still "under discussion" | not needed (Fluid used directly). |
| Dave Choir (Decent Samples) | own-voice "ah" / "oh" choir, 352 notes | no licence stated, e-mail-gated, not on GitHub | rejected. |
| Vowel Ensemble choir | SFZ / Kontakt vowel choir | personal, non-transferable; commercial sample libraries forbidden | rejected. |
| Sonatina Symphonic Orchestra | no choir | CC Sampling Plus 1.0 | rejected (licence, no choir). |
| Signature Sounds "Angelic Choir" / "Children's Choirs" | choir recordings | CC0 (site) | not on GitHub (host blocked) - **worth downloading by hand** for a later build. |
| FreePats "Choir" pad | synthesised / sampled pad | CC0 | host blocked; not on GitHub. |
| Freesound CC0 choir / vocal packs (e.g. curated list gist.github.com/PtrMan/b3ff...) | many | per-file CC0 / CC BY | host blocked - **worth downloading by hand** (verify each file's licence). |
| Phonation Modes Dataset (sustained vowels, ~900 per set) | ideal vowel material | CC BY-NC-SA | rejected (non-commercial). |
| GTSinger, CSD, M4Singer, Opencpop, OpenSinger, PopCS, NUS-48E, JVS-music, Kiritan / Namine Ritsu / Ofuton UTAU banks | singing corpora / voicebanks | NC, research-only or custom terms | rejected (non-commercial or restrictive). |
| UTAU / DiffSinger voicebanks on GitHub | syllable recordings | per-bank terms; no CC0 / CC BY bank found | rejected (none verified). |
| Dagstuhl ChoirSet, Choral Singing Dataset, Erkomaishvili | SATB choir recordings | CC BY (Zenodo / AudioLabs) | not on GitHub (hosts blocked); they are song recordings, would need note slicing - candidates for a later manual download. |
| NSynth (vocal family) | 4-s notes incl. "vocal" family | CC BY 4.0 | Google Storage / TF datasets - blocked; mostly synthetic "vocal" patches. |

## 5. What the voice model must cover

Choir sections (S / A / T / B, women / men / children), ensemble sizes, Mm / Nn hums, whispers, swells, legato,
syllables, any range outside the recorded zones, and "ear-safe" bright vowels. The recorded samples above are best
used as attack transients, short chops and texture layers on top of the model.
