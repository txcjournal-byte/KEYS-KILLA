# KEYS KILLA

*rare & modern trap melodies & basses* – VST3 / AU / Standalone syntezátor (JUCE 8, C++20, CMake) od výrobce 808 KILLA.
Všechny zvuky generuje plugin sám syntézou, žádné samply.

![BLOOD](docs/screenshot_blood.png)

## Co umí (verze 0.2)

| Oblast | Obsah |
|---|---|
| **Enginy** (vrstva A + B) | VA (PolyBLEP, unison až 8), FM (4 operátory, 6 algoritmů, feedback), Wavetable (8 tabulek, mip-mapping, warp Bend/Sync/Mirror/Quantize/FM), Pluck (Karplus-Strong), Modal (kalimba / marimba / zvon), Vox (formanty A-E-I-O-U), Organ (additive), Flute, Orchestral (brass / strings / choir hity), Sub 808 |
| **Filtr** | Clean, Ladder, Dirty, High Pass, Band Pass; key tracking; obálka 2 |
| **Modulace** | 3 ADSR obálky, 2 LFO (sync, 5 tvarů), mod matrix 8 slotů (Env 2/3, LFO 1/2, velocity, mod wheel, aftertouch, key track, random → pitch, cutoff, reso, wave A/B, FM A/B, level A/B, amp, pan, sub, detune) |
| **Hraní** | 16 hlasů, Mono / Legato + glide, pitch bend range, sustain, aftertouch (channel i poly), **Key Lock** (tónina + 8 stupnic, zobrazeno na klaviatuře), **CHORD** (9 typů vč. diatonického, strum), **ARP** (Up/Down/Up-Down/Random/As Played, 1–3 oktávy, swing, gate, sync) |
| **Bass režim** | mono, sub vrstva, mono pod 120 Hz, subsonic filtr 22 Hz, Clean Low distorze, makra SUB/WOBBLE/DIRT/GLIDE/TONE/PUNCH, wobble cíl filtr/hlasitost/wave/pitch, varování při přebuzení |
| **FX rack** (pořadí přetahovatelné) | Drive (Soft/Tape/Hard/Blown/Fold, 2× oversampling), Body Swap, Lo-Fi (bitcrush, SR, wow/flutter, vinyl), Circuit Bend, Chorus, Phaser, Flanger, EQ, Delay (ping-pong/stereo/tape), Reverb (hall/plate/cloud, freeze), Reverse, Width |
| **EXCLUSIVE** | DICE + CHAOS (zámky sekcí, historie 20 hodů, uložení jako preset – pravý klik), ERA MORPH (XY pad, do rohů lze vložit presety a míchat je, jinak mění charakter), GHOST (obrácený stín ±oktáva, blur), BEND (dive/rise/dip/octave jump/random + broken tape), CIRCUIT (glitche v tempu, deterministické), BODY SWAP (6 těles) |
| **Presety** | **408 factory presetů**: 10 kategorií, éry 2010–2026, Experimental, Exclusive (48), Bass (99); varianty Lo-Fi / Dark / Blown; hlasitost srovnaná na −15 dB |
| **Správa presetů** | prohlížeč s vyhledáváním a filtry (kategorie, éra, exclusive, oblíbené, user), Save / Save As / Rename / Delete, `*` při změně + Revert, A/B, Undo/Redo, Init, import/export packů (.zip), JSON formát s verzí |
| **GUI** | skiny CHROME / BLOOD (☀/☾, výchozí pro všechny instance), 100–200 %, ADVANCED se záložkami ENGINE A/B, FILTER, MOD, FX, EXCLUSIVE, PLAY, SETTINGS, vizualizace vlny, obálek a LFO, tooltip u každého prvku, dvojklik = default, Ctrl + tah = jemně |
| **Technika** | žádné alokace v audio threadu, deterministický render (export = přehrávání), bypass s fade, tail 10 s, 44,1–192 kHz, Eco režim |

![ADVANCED](docs/screenshot_advanced.png)

## Test ve FL Studiu (Windows)

1. GitHub → **Actions** → poslední běh `build` → stáhni **KEYS-KILLA-Windows-Installer** (instalátor) nebo **KEYS-KILLA-Windows-VST3**.
2. Instalátor dá plugin do `C:\Program Files\Common Files\VST3\`. Ručně: zkopíruj složku `KEYS KILLA.vst3` tamtéž.
3. FL Studio → *Options → Manage plugins → Find installed plugins* → KEYS KILLA (výrobce 808 KILLA) → Channel Rack.

## Build a testy

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release        # JUCE se stáhne automaticky (nebo -DKK_JUCE_DIR=cesta)
cmake --build build --config Release
./build/KeysKillaTests_artefacts/Release/KeysKillaTests          # unit testy + render všech presetů
./build/KeysKillaTests_artefacts/Release/KeysKillaTests -bench   # CPU, 8 not
pluginval --strictness-level 10 --validate "build/KeysKilla_artefacts/Release/VST3/KEYS KILLA.vst3"
python3 tests/calibrate.py build/KeysKillaTests_artefacts/Release/KeysKillaTests   # přepočet hlasitosti presetů
```

## Distribuce

- Windows: `installer/win/keyskilla.iss` (Inno Setup) – staví CI.
- macOS: `installer/mac/build_pkg.sh` – `.pkg` s VST3 + AU + Standalone, Universal Binary. Podepíše a notarizuje,
  když jsou v GitHubu nastavené secrets `MAC_APP_SIGN_ID`, `MAC_INST_SIGN_ID`, `NOTARY_APPLE_ID`, `NOTARY_TEAM_ID`, `NOTARY_PASSWORD`.
- Identita: výrobce **808 KILLA**, kód výrobce **Kila** (stejný jako 808 KILLA), kód pluginu **Kkey**. Nikdy neměnit.
- Verze je jen v `CMakeLists.txt` (`project(KEYS_KILLA VERSION …)`).
