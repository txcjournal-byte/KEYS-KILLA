# Sound design research (v0.13)

Web research into how commercial trap / hip-hop presets are built (synthesis techniques only, no preset copies).
Used as the reference for the v0.13+ sound work. Numbers are starting points.

## Engine priorities (by perceived quality)
1. Band-limited oscillators (PolyBLEP/BLAMP, mip-mapped wavetables, oversampled drive / FM) - aliasing is the #1 "cheap" tell.
2. Random unison / free-running start phase (hard reset only for plucks and basses).
3. Unison: JP-style detune curve, centre-vs-side blend, per-voice pan, 1/sqrt(N) gain, mono centre.
4. Per-voice analog drift: +-1-4 cents at 0.1-1 Hz, cutoff +-2-5 %.
5. Output stage: low-cut + mono < 120 Hz, limiter at -1 dBFS, presets loudness-matched within +-2 dB.
6. Velocity to timbre (FM index, filter env, noise transient), amp velocity 30-60 %.
7. Enveloped FM index, modulator decays faster than the amp; key-scaled index and decay.
8. Transient / noise layer (5-40 ms): mallet strike, hammer, bow, chiff, brass blat.
9. Reverb: dense modulated tail, pre-delay, low/high cut on the return, decorrelated stereo.
10. ZDF filters with per-voice nonlinearity and resonance compensation.
11. Stereo: per-voice pan spread, ensemble chorus, mono-safe M/S width, Haas only on the side.
12. Modal tables: bell 0.56/0.92/1.19/1.71/2.0/2.74/3.0/3.76/4.07; bar 1/2.756/5.404/8.933; marimba 1/4/10.
13. Karplus-Strong: allpass fine tuning, velocity-dependent brightness, pick-position comb.
14. Delayed vibrato, smoothed modulation.
15. Macros driving 3-6 targets each (Tone, Space, Movement, Character).

## Recipes (condensed)
- Bells: FM C:M 1:3.5 (tubular), 1:1.4 dark, 1:2.7; glass ~1:9.1 + 3-7 ct detune; music box 1:4.2 index 3-5, mod decay 60-150 ms, amp 1.2-2 s; dark bell index 1-2, LP 2-4 kHz, A 0-2 ms D 1.5-3 s R 1-2 s, sine sub -12 dB. Reverb 2.5-4 s, pre 20-40 ms, return LC 300 Hz.
- Plucks: A 0-3 ms, D 120-350 ms, S 0-35 %, R 150-300 ms; filter env D 80-250 ms +3..5 oct, reso 10-25 %. Plugg: square/pulse + sine, 2-3 voices 8-12 ct, hall 3-5 s 30-40 %.
- Keys: FM EP 1:1 body + 14:1 tine dying in ~100 ms, velocity -> tine; dark piano saw+tri LP 1.2-2 kHz keytrack 50-70 %, hammer noise 5-10 ms -20 dB, tape wow 0.4 Hz +-3-6 ct.
- Pads: attack 0.4-1.5 s, release 1-3 s, 4-8 unison 10-20 ct, LFO 0.05-0.3 Hz on wavetable / cutoff, reverb 4-8 s (cloud 6-10 s), LC 80-120 Hz. Choir: formants ah 800/1150/2900 Hz + breath noise, vibrato 5 Hz +-8 ct delayed 300 ms.
- Leads: supersaw 7 voices (6-12), detune 25-30 %, blend 75 %; rage: 3x9-voice saws + clip, HP 150-250 Hz; whistle: sine + chiff noise, vibrato 5-6 Hz +-10-20 ct fading in 250-500 ms, glide 40-120 ms.
- Brass: 2-3 saws 5-10 ct, 24 dB LP 30-40 %, reso 30 %, filter env A 12 ms D 50 ms, pitch env 30-50 ms scoop, drive; drill horn +-12 octave stack, +3 dB at 1.5-2.5 kHz.
- Strings: detuned saws / PWM, delayed vibrato 5.5 Hz +-8 ct; staccato A 5-10 ms D 120-200 ms + 20 ms bow noise at 3 kHz.
- Basses: reese 2 saws +-5-30 ct LP 0.5-3 kHz mono; moog ladder ~120 Hz reso 25 %, env D ~200 ms, glide 30-60 ms.
- Finished presets: layered transient + body + air, per-preset FX chain, LC 80-150 Hz on non-bass, dip 250-400 Hz 1-3 dB, air +1-3 dB at 10-12 kHz, loudness matched.
