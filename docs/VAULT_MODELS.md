# CHROME VAULT physical models

Our own modelled instruments for what the CC0 sample libraries (VSCO 2 CE, VCSL) do not have: guitars, electric basses,
electric pianos, a struck-string "funk keys", a felt toy piano, solo voices, choirs and a vocal pad. They are `model` parts
(`VaultPart::kind = model`, `source` = the model id, parameters `m.*`), registered in `registerBuiltinModels()`
(`Source/vault/models/ModelRegistry.cpp`, one line per model). The interface and the engine's side (voice pools, pedal,
bend, MOTION vibrato, TONE low-pass) are described in `docs/VAULT.md` ("Models").

Design rules (same as the whole product): at C5 (MIDI 60, the FL Studio naming used everywhere) every model is
comfortable - the played fundamental is the loudest partial (or within 3 dB), octave-up content at least 12 dB under the
main string, tame 2-5 kHz, nothing piercing; velocity 1.0 at C5 peaks about -9 dBFS and never above -1 dBFS; no
allocation / locks on the audio thread; deterministic renders; < 0.3 % of one core per sounding voice at 48 kHz.

## Files

| File | What |
| --- | --- |
| `Source/vault/models/ModelDsp.h` | shared DSP: power-of-two delay line with 3rd-order Lagrange fractional read, first-order allpass (+ phase delay), RBJ biquads, Klatt formant resonator, one-pole / DC blocker, anti-derivative anti-aliased soft clipper, ramps, xorshift noise |
| `Source/vault/models/BuiltinModels.h` | the family info factories (`stringModelInfo`, `keysModelInfo`, `voiceModelInfo`) |
| `Source/vault/models/StringModels.cpp` | waveguide strings: 7 guitars, 4 electric basses, struck-string keys (`ep.clav`) - one profile row per instrument |
| `Source/vault/models/KeysModels.cpp` | tine (mellow / bell) and reed electric pianos, felt toy piano - one profile row per instrument |
| `Source/vault/models/VoiceModels.cpp` | LF glottal tables, formant tables, solo voice, choir / pad |
| `tools/vault/ModelLab.cpp` | console target `VaultModelLab`: `-demos <dir>` (2 s demos at C5 through the engine), `-measure <dir>` (renders for the script), `-cpu [s]`, `-sweep`, `-one <model> <out.wav> [note vel secs hold m.k=v ...]`, `-bend <model> <out.wav>` |
| `tools/vault/models_measure.py` | the measurement script (numpy only): literature / physics targets, C5 comfort rules, pitch C2..C7, formants, HNR, comparisons with real recordings |
| `tests/VaultModelTests.cpp` | unit tests, run by `VaultTests` (alone: `VaultTests -models`) |

Workflow: `VaultModelLab -measure /tmp/mm && python3 -I tools/vault/models_measure.py /tmp/mm --sources /home/user/sgossner`
(exit code = number of FAILs). Demos: `VaultModelLab -demos <dir>`.

## The models

| id | name | method | voices |
| --- | --- | --- | --- |
| `guitar.acoustic` | Steel Acoustic Guitar | 2-polarisation waveguide, steel string, commuted dreadnought body | 12 |
| `guitar.nylon` | Nylon Classical Guitar | 2-polarisation waveguide, nylon damping, finger pluck, classical body | 12 |
| `guitar.banjo` | Banjo | waveguide, metal finger pick near the bridge, membrane (head) body, fast decay | 12 |
| `guitar.twelve` | 12-String Guitar | 2 courses: octave string (-12 dB, picked 6 ms earlier) below G4, unison pair (+3 c) above | 10 |
| `guitar.electric` | Clean Electric Guitar | waveguide, pickup combs (neck / bridge blend), coil resonance, clean amp + open-back cabinet | 12 |
| `guitar.crunch` | Crunch Electric Guitar | as above, driven amp (anti-aliased asymmetric soft clip), closed-back cabinet | 12 |
| `guitar.jazz` | Jazz Archtop Guitar | flatwound (short high-partial decay), neck humbucker, archtop body, tone rolled back | 12 |
| `bass.finger` | Finger Bass | 4-allpass stiff string, 2 polarisations, two-pickup blend, DI amp | 10 |
| `bass.pick` | Pick Bass | as finger, pick excitation, neck pickup | 10 |
| `bass.slap` | Slap Bass | thumb strike + fret-collision limiter in the loop (first 150 ms), scooped amp | 10 |
| `bass.fretless` | Fretless Bass | flesh termination (short high-partial T60), "mwah" formant sweep, slide-in | 10 |
| `ep.tine.mellow` | Tine Electric Piano | modal tine + tone bar, electromagnetic pickup voiced for warmth | 16 |
| `ep.tine.bell` | Bell Tine Electric Piano | same, pickup voiced closer to the pole (2nd harmonic), long bell partial | 16 |
| `ep.reed` | Reed Electric Piano | modal reed, electrostatic pickup (asymmetric -> "bark"), preamp drive | 16 |
| `ep.clav` | Funk Keys | struck string (tangent), two pickups near the end, "medium" filter, yarn-damped release | 12 |
| `keys.toy` | Felt Toy Piano | struck steel rod (clamped-free bar modes), soft felt hammer, small box, no dampers | 16 |
| `voice.female` | Female Voice | LF pulse + aspiration, alto formants, vibrato / jitter / shimmer | 12 |
| `voice.male` | Male Voice | same, tenor formants | 12 |
| `voice.choir` | Choir | 8-24 singers in soprano / alto / tenor / bass sections, pitch scatter, own formants per section | 8 |
| `voice.pad` | Vocal Pad | choir engine: breathy, slow attack (1.2 s) / release (2.5 s), slow vowel drift, wide | 8 |

### Parameters

Strings (`guitar.*`, `bass.*`, `ep.clav`) - defaults per instrument:

| key | range | meaning |
| --- | --- | --- |
| `m.artic` | 0..2 | 0 pluck, 1 palm mute (T60 <= 0.45 s, highs <= 0.06 s, darker pluck, +3 cents of pressure), 2 harmonic (sine-like pluck, 1.4x sustain) |
| `m.tone` | 0..1 | pluck hardness and output tone (0.5 = as designed) |
| `m.decay` | 0.2..4 | string decay multiplier |
| `m.release` | 0.02..3 s | T60 after note-off |
| `m.pickPos` | 0.03..0.5 | pluck position (fraction of the string from the bridge) |
| `m.body` | 0..1.5 | body / resonance amount |
| `m.pickup` | 0..1 | electric instruments: neck .. bridge (the bridge setting keeps 30 % neck) |
| `m.drive` | 0..1 | amp drive |
| `m.stiff` | 0..4 | inharmonicity multiplier |
| `m.detune` | 0..20 c | second polarisation / 12-string unison course detune |
| `m.level`, `m.velSens` | | output level, velocity -> level |

Keys (`ep.tine.*`, `ep.reed`, `keys.toy`): `m.tone`, `m.decay`, `m.release`, `m.drive`, `m.bell` (upper-mode level),
`m.trem` / `m.tremRate` (stereo pan tremolo on tines, level tremolo on reed / toy), `m.voicing` (-1 rounder .. +1 more
2nd harmonic / bark), `m.level`, `m.velSens`.

Voices (`voice.*`): `m.vowel` (0 ah, 1 oh, 2 oo, 3 ee, morph in between), `m.artic` (0 sung, 1 "ha", 2 breathy,
3 whisper), `m.breath`, `m.vibrato` (1 = 60 cents), `m.vibRate`, `m.attack`, `m.release`, `m.bright` (0 lax / breathy ..
1 pressed), `m.formant` (-1..+1 = -15..+15 % formant shift), `m.level`, `m.velSens`; choir / pad: `m.voices` (1..24),
`m.gender` (0 men .. 1 women), `m.detune` (pitch scatter SD, cents), `m.spread`, `m.motion` (slow vowel drift).

## Research and design

### Plucked and struck strings (`StringModels.cpp`)

* **Digital waveguide / extended Karplus-Strong** (Karplus & Strong 1983; Jaffe & Smith 1983; Smith, "Physical Audio
  Signal Processing"; Karjalainen, Valimaki & Tolonen 1998, "Plucked-string models: from the Karplus-Strong algorithm to
  digital waveguides and beyond", CMJ 22(3)): a single delay loop per string polarisation.
* **Tuning**: 3rd-order Lagrange fractional delay (smooth under bends). The delay-line length is the period minus the
  phase delays of every loop filter at f0 (loss one-pole and allpasses, computed analytically), so the fundamental is
  exact: measured within +-1.3 cents from C2 to C7 for every string model (most within +-0.5). Bends move the read point with a per-block ramp;
  the pickup taps move with it (a bend changes the tension, not where the pickup sits).
* **Losses**: one-pole loss filter `g (1 + a) / (1 + a z^-1)` fitted per note to two targets - the T60 of the
  fundamental and the T60 at 2 kHz (or 2 f0) - the method of Valimaki, Huopaniemi, Karjalainen & Janosy (1996, "Physical
  modeling of plucked string instruments with application to real-time sound synthesis", JAES 44(5)). Targets are
  key-tracked (x0.70-0.80 per octave). A strongly low-passing one-pole is itself dispersive, so its pole is limited more
  at high notes (else partials go sharp - found by measurement at C7).
* **Stiffness / inharmonicity**: 2 (guitars) or 4 (basses) identical first-order allpasses; the coefficient is solved per
  note (bisection on the analytic loop phase) so a reference partial (~2.5 kHz) lands on f_k = k f0 sqrt((1 + B k^2) /
  (1 + B)) (Van Duyne & Smith 1994; Rauhala & Valimaki 2006, "Tunable dispersion filter design for piano synthesis").
  B targets at C5: steel guitar 6e-5, nylon 2e-5, banjo 8e-5, electric bass 1.2e-4 at E2 rising x2 per octave
  (shorter fretted length). Measured B (least-squares fit of the partials): steel 6.9e-5, banjo 8.9e-5, electric 5.8e-5,
  basses at C5 3.3-4.5e-4, struck-string keys 9.7e-5 - and the VCSL steel psaltery (523 Hz) measures 6.1e-5.
* **Commuted synthesis** (Smith 1993; Karjalainen, Valimaki & Janosy 1993): the excitation is the body impulse response
  (85 ms, synthesised from body modes) filtered by the plucking finger / pick (two one-poles, cutoff rising with velocity
  squared: 0.7-2.6 kHz nylon finger, 1.4-5 kHz steel pick, 3-8 kHz slap thumb) and a 1/k pluck tilt (bridge force of a
  plucked string falls ~1/k, Fletcher & Rossing ch. 2); the pluck-position comb (zeros at multiples of 1 / position) is
  applied per loop. Body modes (`steelModes`, `nylonModes`, `archModes`, `banjoModes`): low modes from Fletcher & Rossing
  ch. 9-10 (A0 ~100 Hz, T(1,1) ~200 Hz, A1 ~375 Hz), Q ~10-15; the coupled top / back mode is voiced at ~255 Hz so C5 gets
  body support. Banjo: membrane head with many modes from ~260 Hz, admittance 20-30 dB above a guitar's, louder start and
  faster decay (Woodhouse, Politzer & Mansour 2021, Acta Acustica 5:15/16).
* **Two polarisations** (the vertical one couples to the body and decays fast, the horizontal one slower and a little
  detuned): the double decay and gentle beating of real strings. 12-string: the second loop is the course partner.
* **Electric**: magnetic pickup = string displacement at the pickup, a comb `y[n] - y[n - P]` (neck P = 0.25 of the
  string, bridge 0.09), coil inductance + cable resonance (2-3.5 kHz, Q 0.8-1.1), amp = soft clip `x / sqrt(1 + x^2)` with
  first-order anti-derivative anti-aliasing (Parker et al. 2016) and asymmetric bias for crunch, tone stack and cabinet
  biquads (high-pass 70-90 Hz, low-mid body, 2-pole low-passes at 3.5-6 kHz, a 2.8 kHz dip) - the cabinet keeps 2-5 kHz
  tame.
* **Bass extras**: slap = a fret-collision limiter inside the loop (energy-reducing piecewise gain above a threshold that
  opens over 150 ms; cf. fret / string collision models, Bilbao & Torin 2015) + a 4 ms thumb thud; fretless = short
  high-partial T60 (flesh termination), a 7 dB "mwah" formant gliding 1.3 kHz -> 500 Hz, a 25-cent slide-in.
* **Struck-string keys** (`ep.clav`, after Gabrielli, Valimaki, Penttinen, Squartini & Bilbao 2013, "A digital waveguide-
  based approach for Clavinet modeling and synthesis", EURASIP JASP 2013:103; pickup: Remaggi et al. DAFx 2012): hard
  tangent strike, two pickups near the string end (combs), "medium" filter, release = yarn damping (T60 0.06 s) and an
  8-cent pitch drop (the delay line lengthens when the tangent leaves the string).

### Electric and toy pianos (`KeysModels.cpp`)

* Modes as complex rotators (exact frequency, exponential decay, no aliasing). Tine: fundamental + tone-bar partner
  (+1.2 cents, slower decay -> slow beating) + cantilever modes 6.27 and 17.55 x f0 (clamped-free beam ratios, Fletcher &
  Rossing ch. 2) for the attack "bell" (T60 0.12 s mellow, 0.30 s bell). Hammer contact smoothing ~0.7 ms (neoprene tip).
* Tine pickup: `Phi(x) = 1 / sqrt(1 + ((x + offset) / h)^2)`, output dPhi/dt normalised by the fundamental's angular
  step. The offset of the tine against the pole sets the 2nd-harmonic balance; a harder strike pushes into the non-linear
  region (Pfeifle 2017, "Real-time physical model of a Wurlitzer and Rhodes electric piano", DAFx-17). Mellow voicing
  offset 1.3 h (H2 -15 dB at C5), bell 0.45 h (H2 -4 dB).
* Reed pickup: electrostatic, `C ~ 1 / (gap - x)`: asymmetric, so even harmonics grow with amplitude ("bark"), then a
  preamp with velocity-dependent drive.
* Toy piano: clamped-free rod modes (1, 6.27, 17.55) + an octave component, soft felt hammer (1.2 ms contact), small box
  resonance (650 Hz peak), no dampers (notes ring, `m.release` sets the fade after the key is up).
* Comparison target: the VCSL TX81Z FM electric piano at 262 Hz: H2 -17 dB, T60 of the fundamental 4.9 s; our mellow tine
  H2 -15 dB, T60 5.3 s.

### Voices (`VoiceModels.cpp`)

* **Source-filter model** (Fant 1960; Klatt 1980). Source: Liljencrants-Fant flow-derivative pulse (Fant, Liljencrants &
  Lin 1985) driven by the single voice-quality parameter Rd (Fant 1995: Rap = (-1 + 4.8 Rd)/100, Rkp = (22.4 + 11.8 Rd)/100,
  Rgp = Rkp / (4 (0.11 Rd / (0.5 + 1.2 Rkp) - Rap))); alpha and epsilon solved numerically (zero net flow). Six Rd tables
  (0.5 .. 2.8) x 10 octave-band-limited levels (FFT-truncated, 2048 points) - no aliasing at any pitch.
* **Aspiration**: high-passed noise gated by the open phase (pulsed breath); breathy raises Rd to >= 2.3 and the noise;
  whisper = noise only (+10 % voiced), wide F1, low-passed at 3.5 kHz.
* **Tract**: 5 Klatt resonators in cascade (unity DC gain), formants from the Csound manual appendix "Formant values"
  (female = alto, male = tenor; choir sections soprano / alto / tenor / bass). Choral / pop voices do not use the operatic
  singer's formant (Sundberg 1987, "The Science of the Singing Voice"): F3-F5 bandwidths widened, a 3 kHz dip. Formant
  tuning: F1 >= 1.25 f0. At high pitch every formant bandwidth >= 0.15 f0 and the formants keep apart (no runaway
  resonance gain), plus an output guard (transparent below -6 dBFS).
* **C5 comfort for voices**: an "ah" at C5 has F1 (~800 Hz) on the 3rd harmonic. The model predicts the harmonic levels
  (pulse spectrum x tract response) and widens the F1 / F2 bandwidths until H1 leads (by 1 dB solo, 3 dB in ensembles) -
  only above 150 Hz (low voices keep their formant peaks). The same prediction normalises the level (equal RMS for every
  vowel / pitch).
* **Life**: vibrato with delayed onset (250 ms), 5.3 Hz, 24 cents by default; jitter (low-passed random, ~3 cents),
  shimmer, slow drift.
* **"ha"**: 70 ms of aspiration through a wide-F1 tract, voicing from 50 ms, voiced decay (T 0.32 s), Rd rising (pressed
  -> breathy), a 50-cent fall after 0.42 s, ends by itself at ~0.75 s.
* **Choir**: each singer has its own pulse phase, Gaussian pitch scatter (default SD 12 cents - within the 14 cents
  listeners accept in unison choir sound, Ternstrom 1993, J. Voice 7(4); measured singer-to-singer dispersion 25-30 cents,
  Jers & Ternstrom), vibrato rate 4.6-6 Hz / depth / phase, jitter, level (+-2 dB) and a ragged onset (20-40 ms). The
  level-weighted mean of the scatter is removed so the section is in tune. Sections have their own formant filter
  (+-3 % formant scatter) and stereo place. The ensemble runs at half the sample rate with a 16-tap polyphase
  interpolator (formants and pulses stay below ~10 kHz, the post filter rolls off at 4.5-6.5 kHz).

## Measurements

`python3 -I tools/vault/models_measure.py <renders> --sources /home/user/sgossner` - all at 48 kHz, direct voice renders
(3 s, held 2.5 s), C5 = MIDI 60. Status: **0 FAIL, 0 WARN**.

| variant (C5, velocity 1.0) | peak dBFS | pitch c | H1 - strongest dB | H2 - H1 dB | 2-5 kHz % | centroid Hz | B | T60 H1 s | T60 H6 s |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| bass_finger_mute | -10.8 | +2.9 | 0.0 | -9.4 | 0.0 | 285 | 0.0e+00 | 0.45 | - |
| bass_finger_pluck | -8.2 | +0.0 | -0.9 | 0.9 | 0.0 | 427 | 4.4e-04 | 3.86 | 0.88 |
| bass_fretless_harmonic | -13.0 | +0.0 | 0.0 | -52.1 | 0.0 | 268 | 0.0e+00 | 3.80 | - |
| bass_fretless | -8.4 | +0.0 | -1.3 | 1.3 | 0.0 | 422 | 3.4e-04 | 2.72 | 0.88 |
| bass_pick_mute | -10.1 | +2.9 | 0.0 | -7.4 | 0.0 | 298 | 0.0e+00 | 0.45 | - |
| bass_pick_pluck | -9.4 | +0.0 | -2.4 | 2.4 | 0.0 | 489 | 4.3e-04 | 3.86 | 1.86 |
| bass_slap_mute | -8.6 | +2.9 | 0.0 | -29.5 | 0.0 | 262 | 0.0e+00 | 0.45 | - |
| bass_slap | -8.2 | -0.0 | 0.0 | -19.8 | 0.0 | 265 | 4.5e-04 | 3.31 | 1.19 |
| choir_female8_ah | -8.1 | +0.5 | 0.0 | -3.4 | 0.0 | 447 | 0.0e+00 | >99 | >99 |
| choir_full24_ah | -8.3 | +3.8 | -0.3 | 0.3 | 0.0 | 573 | 0.0e+00 | >99 | >99 |
| choir_male24_oh | -9.0 | +3.8 | 0.0 | -1.1 | 0.0 | 438 | 0.0e+00 | >99 | 92.06 |
| choir_mixed16_ah | -8.1 | -1.7 | 0.0 | -3.5 | 0.0 | 445 | 0.0e+00 | 93.18 | >99 |
| choir_mixed16_oo | -8.1 | +0.6 | 0.0 | -8.2 | 0.0 | 300 | 0.0e+00 | >99 | - |
| ep_clav_mute | -9.9 | +2.9 | 0.0 | -7.8 | 0.0 | 295 | 0.0e+00 | 0.45 | - |
| ep_clav | -9.0 | +0.0 | -2.4 | 2.4 | 0.1 | 505 | 9.7e-05 | 3.00 | 1.32 |
| ep_reed_bark | -7.4 | +0.1 | 0.0 | -14.4 | 0.0 | 282 | 0.0e+00 | 3.45 | 0.86 |
| ep_reed_soft | -7.3 | +0.1 | 0.0 | -11.8 | 0.0 | 281 | 0.0e+00 | 3.27 | 0.68 |
| ep_reed | -7.3 | +0.1 | 0.0 | -11.8 | 0.0 | 281 | 0.0e+00 | 3.27 | 0.68 |
| ep_tine_bell_trem | -9.4 | +0.2 | 0.0 | -4.1 | 0.0 | 345 | 0.0e+00 | 6.55 | 1.27 |
| ep_tine_bell | -8.3 | +0.2 | 0.0 | -4.1 | 0.0 | 345 | 0.0e+00 | 6.55 | 1.27 |
| ep_tine_mellow_soft | -9.2 | +0.1 | 0.0 | -15.3 | 0.0 | 268 | 0.0e+00 | 5.27 | 0.97 |
| ep_tine_mellow | -9.2 | +0.1 | 0.0 | -15.3 | 0.0 | 268 | 0.0e+00 | 5.27 | 0.97 |
| guitar_12string_harmonic | -12.6 | -0.0 | 0.0 | -20.5 | 0.0 | 267 | 0.0e+00 | 6.30 | 2.03 |
| guitar_12string_mute | -12.2 | +3.0 | 0.0 | -10.4 | 0.0 | 282 | 0.0e+00 | 0.45 | - |
| guitar_12string_pluck | -9.0 | -0.0 | 0.0 | -3.3 | 0.0 | 397 | 6.8e-05 | 4.50 | 1.55 |
| guitar_banjo_harmonic | -12.1 | -0.0 | 0.0 | -55.4 | 0.0 | 262 | 0.0e+00 | 2.24 | - |
| guitar_banjo_mute | -12.0 | +2.9 | 0.0 | -15.5 | 0.0 | 268 | 2.6e-04 | 0.45 | - |
| guitar_banjo_pluck | -9.0 | -0.0 | 0.0 | -5.8 | 0.0 | 351 | 8.9e-05 | 1.60 | 0.55 |
| guitar_crunch_harmonic | -10.9 | -0.0 | 0.0 | -47.7 | 0.0 | 269 | 6.5e-05 | 9.81 | - |
| guitar_crunch_mute | -10.5 | +2.9 | 0.0 | -8.1 | 0.0 | 293 | 0.0e+00 | 0.45 | - |
| guitar_crunch_pluck | -9.0 | -0.0 | -2.3 | 2.3 | 0.0 | 523 | 6.0e-05 | 7.00 | 2.26 |
| guitar_electric_bridge | -7.3 | -0.0 | -2.4 | 2.4 | 0.1 | 731 | 5.6e-05 | 7.00 | 2.28 |
| guitar_electric_harmonic | -10.9 | -0.0 | 0.0 | -48.5 | 0.0 | 269 | 6.5e-05 | 9.80 | 7.04 |
| guitar_electric_mute | -10.2 | +2.9 | 0.0 | -8.8 | 0.0 | 289 | 0.0e+00 | 0.45 | - |
| guitar_electric_neck | -8.9 | -0.0 | -1.6 | 1.6 | 0.1 | 523 | 5.8e-05 | 7.00 | 2.28 |
| guitar_jazz_harmonic | -6.7 | -0.0 | 0.0 | -51.9 | 0.0 | 262 | 0.0e+00 | 4.90 | - |
| guitar_jazz_mute | -10.2 | +2.9 | 0.0 | -12.0 | 0.0 | 275 | 0.0e+00 | 0.45 | - |
| guitar_jazz_pluck | -9.0 | -0.0 | 0.0 | -3.2 | 0.0 | 336 | 0.0e+00 | 3.50 | - |
| guitar_nylon_harmonic | -10.1 | +0.2 | 0.0 | -60.8 | 0.0 | 262 | 0.0e+00 | 5.61 | - |
| guitar_nylon_mute | -11.6 | +3.6 | 0.0 | -20.7 | 0.0 | 264 | 0.0e+00 | 0.65 | - |
| guitar_nylon_pluck | -9.0 | +0.3 | 0.0 | -12.3 | 0.0 | 276 | 2.8e-05 | 4.23 | - |
| guitar_steel_harmonic | -12.4 | +0.3 | 0.0 | -54.9 | 0.0 | 264 | 0.0e+00 | 7.07 | - |
| guitar_steel_mute | -12.0 | +3.8 | 0.0 | -12.8 | 0.0 | 275 | 0.0e+00 | 0.70 | - |
| guitar_steel_pluck | -9.0 | +0.3 | 0.0 | -5.2 | 0.0 | 383 | 6.9e-05 | 5.44 | 2.55 |
| keys_toy_felt | -8.9 | -0.0 | 0.0 | -28.1 | 0.0 | 263 | 0.0e+00 | 1.90 | - |
| vocalpad_ah | -7.2 | +0.9 | 0.0 | -4.0 | 0.0 | 415 | 0.0e+00 | >99 | 65.41 |
| vocalpad_oo | -8.8 | +2.1 | 0.0 | -20.1 | 0.0 | 267 | 0.0e+00 | >99 | - |
| voice_female_ah | -8.9 | +0.2 | 0.0 | -0.2 | 0.1 | 572 | 0.0e+00 | >99 | 60.78 |
| voice_female_breathy | -10.0 | +0.2 | 0.0 | -10.0 | 0.0 | 593 | 0.0e+00 | >99 | >99 |
| voice_female_ee | -10.4 | +0.8 | 0.0 | -11.4 | 0.0 | 282 | 0.0e+00 | >99 | 60.31 |
| voice_female_ha | -7.3 | -40.5 | 0.0 | -5.0 | 0.1 | 574 | 0.0e+00 | 1.64 | 1.72 |
| voice_female_oh | -8.9 | +0.4 | -0.5 | 0.5 | 0.0 | 437 | 0.0e+00 | >99 | 60.78 |
| voice_female_oo | -9.9 | +0.5 | 0.0 | -9.8 | 0.0 | 290 | 0.0e+00 | >99 | - |
| voice_female_whisper | -11.1 | +0.8 | -9.4 | -10.4 | 0.4 | 964 | 0.0e+00 | >99 | >99 |
| voice_male_ah | -8.5 | +0.1 | -0.1 | 0.1 | 0.9 | 564 | 0.0e+00 | >99 | 61.38 |
| voice_male_breathy | -11.2 | +0.3 | 0.0 | -6.5 | 0.4 | 437 | 0.0e+00 | >99 | >99 |
| voice_male_ee | -11.1 | +0.1 | 0.0 | -14.0 | 0.1 | 280 | 0.0e+00 | >99 | 61.11 |
| voice_male_ha | -9.7 | -41.0 | 0.0 | -1.3 | 0.4 | 509 | 0.0e+00 | 1.84 | 2.56 |
| voice_male_oh | -9.4 | +0.5 | 0.0 | -0.3 | 0.0 | 455 | 0.0e+00 | >99 | 61.43 |
| voice_male_oo | -9.4 | +0.7 | 0.0 | -1.1 | 0.0 | 376 | 0.0e+00 | >99 | 61.44 |
| voice_male_whisper | -10.0 | +2.1 | -6.7 | -8.7 | 5.6 | 982 | 0.0e+00 | >99 | 52.96 |

Literature / physics checks (all pass):

* Pitch: every model within +-1.3 cents from C2 to C7 (voices: power centre of the vibrato band; choir / pad: the
  section's centre within +-10 cents - scatter is intended).
* String decay (T60 of the fundamental at C5): steel 5.4 s, nylon 4.2 s, banjo 1.6 s, 12-string 4.5 s, electric 7.0 s,
  jazz (flatwound) 3.5 s, basses 2.7-3.9 s; high partials decay faster everywhere (nylon H4 1.5 s vs steel 3.9 s -
  nylon's internal damping, Erkut et al. 2000; jazz flatwound H3 1.5 s). Targets in `TARGETS` of the script.
* Inharmonicity: see above (B within the target ranges; psaltery reference 6.1e-5).
* Formants (vowels at C2, harmonic envelope): every F1 / F2 within 6 % of the tables (e.g. female ah 800 / 1124 Hz vs
  800 / 1150, male ee 277 / 1872 vs 290 / 1870).
* HNR at C5: sung vowels 30-34 dB, breathy 12-14 dB, whisper -5 dB, choir 24 dB, pad 24 dB (the VCSL alto recorder's
  breathy tone: 34 dB).
* Comparisons with real recordings (same script, `--sources`): folk harp 262 Hz H2 -14 dB (our nylon -12 dB), TX81Z EP
  262 Hz H2 -17 dB / T60 4.9 s (our mellow tine -15 dB / 5.3 s), steel psaltery B 6.1e-5 (our steel guitar 6.9e-5).

CPU (`VaultModelLab -cpu`, this machine: Xeon 2.1 GHz, one core, 48 kHz, 128-sample blocks, setPitch every block):

| model | ns / sample | % of one core (two runs) |
| --- | --- | --- |
| guitar.acoustic | 25.9 / 26.9 | 0.124 / 0.129 |
| guitar.nylon | 26.1 / 24.7 | 0.125 / 0.119 |
| guitar.banjo | 18.9 / 19.5 | 0.091 / 0.094 |
| guitar.twelve | 30.7 / 33.0 | 0.148 / 0.158 |
| guitar.electric | 37.2 / 53.6 | 0.179 / 0.257 |
| guitar.crunch | 45.4 / 50.6 | 0.218 / 0.243 |
| guitar.jazz | 31.3 / 31.9 | 0.150 / 0.153 |
| bass.finger | 32.8 / 34.5 | 0.157 / 0.166 |
| bass.pick | 32.1 / 34.6 | 0.154 / 0.166 |
| bass.slap | 36.7 / 39.4 | 0.176 / 0.189 |
| bass.fretless | 37.3 / 36.9 | 0.179 / 0.177 |
| ep.tine.mellow | 34.5 / 47.5 | 0.165 / 0.228 |
| ep.tine.bell | 32.6 / 39.7 | 0.157 / 0.190 |
| ep.reed | 26.4 / 30.5 | 0.127 / 0.146 |
| ep.clav | 32.2 / 35.0 | 0.155 / 0.168 |
| keys.toy | 21.8 / 23.4 | 0.104 / 0.112 |
| voice.female | 26.4 / 30.9 | 0.127 / 0.148 |
| voice.male | 28.6 / 29.4 | 0.137 / 0.141 |
| voice.choir | 58.1 / 57.2 | 0.279 / 0.275 |
| voice.pad | 48.9 / 52.1 | 0.235 / 0.250 |
| voice.choir x8 | 45.1 / 47.2 | 0.217 / 0.227 |
| voice.choir x24 | 68.5 / 76.5 | 0.329 / 0.367 |

## Unit tests (`tests/VaultModelTests.cpp`)

For every model: pitch within +-5 cents at C2, C3, C4, C5, C6, C7 (no vibrato / scatter); ensembles' centre within 12
cents with their natural scatter; at 44.1 and 96 kHz C5 in tune and within 3 dB of the 48 kHz level; plays through
`VaultEngine::render` as a preset part; C5 velocity-1 peak below -1 dBFS; 10-second held notes at C2 / C5 / C8 with default,
all-maximum and all-minimum parameters stay finite, bounded and free of denormals; no click at note-off (step <= 2x the
steps before), at the end of the voice (last 64 samples < -70 dBFS) or at the onset; +-2 semitone 5 Hz bends add no steps;
deterministic; no allocation in noteOn / setParams / setPitch / render / noteOff / kill.

## Limits / next steps

* The models are tuned by measurement only (nobody listened). Preset designers should audition and adjust `m.tone`,
  `m.body`, `m.drive`, `m.bright` per preset; the macros (TONE, SPACE, DRIVE, MOTION) work on model parts as on samples.
* A choir note with 24 singers costs ~0.33-0.37 % of a core (above the 0.3 % budget; 16 singers ~0.28 %, 8 singers ~0.22 %).
* Body impulse responses are synthesised from generic mode tables, not measured from a specific instrument.
* No strumming / chord engine (the engine plays what MIDI sends); no string-to-string sympathetic coupling.
