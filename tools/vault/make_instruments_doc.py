#!/usr/bin/env python3
"""Writes docs/VAULT_INSTRUMENTS.md from a built CHROME VAULT library.

    VaultLibBuild -out /home/user/cv-library                       # build
    VaultLibBuild -out /home/user/cv-library -qc -demo <dir>       # measure (qc.json, qc_report.txt, C5 demo WAVs)
    python3 tools/vault/make_instruments_doc.py /home/user/cv-library tools/vault/library_map.json docs/VAULT_INSTRUMENTS.md

Ranges use FL Studio note names (MIDI 60 = C5), like the plugin. The curation notes (what was left out and why) are
kept in NOTES below - edit them when the map changes.
"""
import json
import os
import re
import sys

NAMES = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']


def fl(m):
    return f"{NAMES[m % 12]}{m // 12}"


def mb(b):
    return f"{b / 1048576:.1f}"


def source_of(inst_map):
    roots = set()
    for a in inst_map.get('articulations', []):
        for s in a.get('sources', []):
            roots.add(s['files'].split(':')[0])
    return ' + '.join(sorted({'vsco': 'VSCO 2 CE', 'vcsl': 'VCSL'}.get(r, r) for r in roots))


NOTES = """
## Curation notes

**Mapped** - every musically useful pitched instrument and articulation of VSCO 2 CE and VCSL, plus the tuned
percussion (timpani, slit drums) and the orchestral effects the sources have (tremolo, trombone falls and buzz, bowed
vibraphone / psaltery, harmon and straight mutes, didgeridoo drones).

**Left out on purpose**

| source | why |
| --- | --- |
| VSCO `Keys/Organ`, `Keys/Upright Piano`, `Keys/Upright Nr1`, `Strings/Harp`, `Percussion/Glock`, `Percussion/Marimba`, `Percussion/Xylo`, `Percussion/Timpani` | the same recordings are in VCSL with note names (organ, the second upright piano), remastered (upright no. 1) or with more takes (harp, mallets, timpani); the VCSL copies are used |
| VCSL `Idiophones/.../Tubular Glockenspiel` | the bars sound at G6-A7 (files named six octaves low); 2-3.5 kHz partials only - piercing in exactly the 2-5 kHz band the owner rejected, and reaching C5 would stretch it two octaves |
| VCSL `Bell Tree/Individual` | inharmonic bells with their strongest partials at 4-6.5 kHz and a very quiet recording (+35 dB normalisation lifts the noise) - fails "nothing may hurt the ears" |
| VCSL `Dan Tranh/Gliss`, `Dan Tranh/FX`, VSCO `Miscellania Raw` (glock glisses, zaps, Nepalese bells), sirens, ball / train whistles, `Didgeridoo` barks / taps | not playable as a pitched instrument (no single pitch per file) - candidates for a later FX / one-shot pack |
| all unpitched drums and kit percussion (snares, kicks, toms, congas, bongos, cajon, cymbals, gongs, triangles, shakers, tambourines, claves, cowbells, agogo, anvil, ratchet, ...) | out of scope for this pitched library (brief) |
| `Timpani7F` (VCSL timpani 2) and `Didgeridoo1_Phrase6` | no stable pitch: the spectrum gives a principal tone that does not fit the drum's tuning sequence / the drone (excluded in the map) |
| the second upright piano's `Pedal` folder | pedal noises only |

**Corrections the builder made** (all listed in `build_report.txt` of the library)

* File names one octave off (VSCO and VCSL name C4 as "C3" in most sets) are found per articulation from the median YIN
  pitch and corrected. Where YIN cannot see the octave the map sets it by hand after a spectrum check (`"octave"`):
  pipe organ +1 (the loud ranks have a weak 8' fundamental), FM clav +2 (the patch sounds two octaves above its file
  names), chamber organ 4' +2 (a 4' rank sounds an octave above the key; mapped to its sounding pitch - layer it with
  the 8' a part an octave down for the organ registration), concert chimes +1, chromatic chimes +3, trombone falls /
  buzz and dan tranh vibrato +1.
* Inharmonic bells and glides keep the names as roots (`"tune": "name"`): tubular bells (the three sets agree: the
  same partials sound at the same key), trombone falls and buzz, dan tranh vibrato (bends).
* A single file whose YIN pitch disagrees with its name gets a second opinion from a harmonic-sum spectrum; the root
  only moves when both agree (YIN alone mis-hears bright piano trebles, organ ranks without fundamental and weak bass
  fundamentals) or when YIN is unusable and the spectrum is clear. Example: the soft hard-mallet xylophone "G3" take is
  an F#; the loud one a G.
* Struck bars and tines (marimba, vibraphone, xylophone, glockenspiel, balafon, hand chimes, kalimbas, mbiras;
  `"pitchMethod": "spectrum"`) are tuned from the fundamental's spectral peak - YIN is pulled by their inharmonic
  upper partials. Keys of the traditionally tuned mbiras / kalimbas that sit between two semitones go to the nearest one.
* Looped notes are tuned on what a held note plays (0.35 s to the loop end), one-shots right after the attack (pianos
  from 0.5 s: their attack glides sharp; the piano top octave, which has decayed by then, right after the attack).
* Timpani, slit drums and didgeridoo have no note names: their pitch is measured (harmonic sum with the instrument's
  partial ratios, timpani 1 : 1.5 : 1.98; files of one drum tuning share the median).
* Takes more than 12 dB away from their velocity layer are dropped as broken; round robins are renumbered per key /
  layer so no take is lost when two files land on the same note.
* Velocity layers of sources that were normalised per layer get imposed spacing (`layerDb`: concert grand -15 / -7.5 /
  0 dB, chamber grand -18 / -12 / -6 / 0 dB).
* Key ranges: neighbouring roots split the keys between them, but a zone plays at most 4 semitones from its root
  where another velocity layer has a closer sample (the sampler then uses that layer); the outermost zones reach 4
  semitones past their root (the didgeridoo 6). Only the French horn has keys no sample reaches within 4 semitones:
  the source has no notes between C5 and D6 (FL names).
* Files that start without a lead-in get a 1.5 ms fade-in (no click); rubbed glass, timpani rolls and long tremolos get
  400 ms loop crossfades.

**Known limits (the flags in the QC table)**

* French horn: the source has no notes between C5 and D6 (FL names), so keys there play a sample up to 7-9 semitones
  away. Presets should keep horn lines below C5 or above D6, or layer the muted horn there.
* Loop level dips on contrabass vibrato, piccolo, bowed psaltery and slow rubbed glass: these loops are weakly
  correlated (vibrato, bow changes, beating) and the sampler's crossfade is linear. They do not click (every wrap jump
  is at or below the music's own); a held note can breathe by 6-11 dB at the wrap. An equal-power crossfade for weakly
  correlated loops in the sampler would halve it (engine work, not library data).
* Contrabass tremolo below E2: the bow noise hides the pitch (no reliable measurement); the builder's fine tune stays
  at 0 there. Use it as a rumble / texture, or the sustain articulation for pitched bass lines.
* Vintage trombone buzz starts on a hard growl (the "start jump" is the attack itself, faded in).
* Tuba staccato: one low take has no measurable pitch (a short blat); it stays at its named note.
"""

METHOD = """
## How the QC numbers are measured (`VaultLibBuild -qc`)

The library is read the way the plugin reads it (`vault::SampleLibrary`, decoded OggVorbis), then per zone:

* **tuning** - YIN (threshold 0.15, median of 7 frames) on the stretch the builder tuned (0.35 s to the loop end, or
  from `pitchFrom` for one-shots), expected pitch = the zone root, the zone `fine` added back: the cents a key plays off
  its equal-tempered pitch. Where YIN is unclear or more than half a semitone off, the harmonic-sum spectrum decides;
  `spectrum` articulations (bars, tines) use the fundamental's spectral peak. Below E1 (MIDI 28) nothing is measured. `n/a (name)` = inharmonic bells (names trusted), `n/a (detect)` = pitch measured by the builder (timpani, slit
  drum, didgeridoo).
* **noise** - the builder's measurement: RMS of the lead-in before the onset (>= 20 ms of it) vs the file's peak.
  Files trimmed without a lead-in have no value.
* **start** - first-frame level vs peak and the largest sample jump in the first 3 ms vs the next 50 ms (> 3x would
  be a click; the builder fades in over a 4 ms pre-roll).
* **loop** - the sampler's linear crossfade rebuilt around the wrap: the sample jump at the wrap vs the largest jump
  in the 200 ms around it (1.0 = as smooth as the music itself) and the quietest 20 ms inside the crossfade vs the 50 ms
  before it, minus the dip the music has on its own in the 600 ms before the crossfade (tremolo, bow changes and the
  beating of rubbed glass breathe by themselves) - the level dip the loop adds.
* **velocity** - loudness (BS.1770 K-weighting, loudest 400 ms) of round robin 1 of every layer at every root that has
  several layers: the smallest step between neighbouring layers (a step under 0.5 dB counts as an inversion).
* **flags** - tuning: mean |error| > 8 cents or more than 10 % of the zones > 25 cents; loop: wrap > 1.5x or added dip
  < -6 dB; velocity step < 0.5 dB; key gaps; stretch > 4; noise > -45 dB; start jump > 3x.
* **keys** - per velocity layer, keys inside the range no zone covers (gaps), the farthest a zone plays from its root
  inside the range (stretch) and at the outer edges.
* **C5 demo** - every instrument's first articulation rendered through the engine (`VaultEngine::renderMidi`, no
  preset effects) at C5 (MIDI 60) or the covered key nearest to it, velocity 100, 2 s (note-off at 1.6 s): peak,
  loudness, pitch, fundamental vs the strongest of harmonics 2-16 (dB; positive = the played note is the loudest
  partial) and the 2-5 kHz share of the energy.
"""


def main():
    lib, mapf, out = sys.argv[1], sys.argv[2], sys.argv[3]
    L = json.load(open(os.path.join(lib, 'library.json')))
    Q = json.load(open(os.path.join(lib, 'qc.json')))
    M = json.load(open(mapf))
    mp = {i['id']: i for i in M['instruments']}
    qi = {i['id']: i for i in Q['instruments']}
    report = open(os.path.join(lib, 'build_report.txt')).read() if os.path.exists(os.path.join(lib, 'build_report.txt')) else ''
    order = [i['id'] for i in M['instruments'] if i['id'] in {x['id'] for x in L['instruments']}]
    lj = {i['id']: i for i in L['instruments']}

    fams = {}
    for iid in order:
        fams.setdefault(lj[iid]['family'], []).append(iid)
    n_art = sum(len(lj[i]['articulations']) for i in order)
    n_zones = sum(sum(1 for a in lj[i]['articulations'] for z in a['zones']) for i in order)
    n_files = sum(1 for _ in os.scandir(os.path.join(lib, 'audio'))) if os.path.isdir(os.path.join(lib, 'audio')) else 0
    disk_total = 0
    for dp, dn, fn in os.walk(lib):
        for f in fn:
            disk_total += os.path.getsize(os.path.join(dp, f))
    corrected = len(re.findall(r'-> root corrected', report)) + len(re.findall(r'-> root from the spectrum', report)) \
        + len(re.findall(r'fundamental at MIDI', report))
    named_ok = len(re.findall(r'-> the name is right', report))
    octaves = len(re.findall(r'octave\(s\) off \(corrected\)', report))
    dropped = len(re.findall(r'!! dropped', report))
    excluded = len(re.findall(r'excluded by the map', report))

    o = []
    o.append('# CHROME VAULT - sample instruments\n')
    o.append('The sampled instruments of the CHROME VAULT library (`Library/`, built by `VaultLibBuild` from '
             '`tools/vault/library_map.json`; format and builder in `docs/VAULT.md`, credits in '
             '`docs/CHROME_VAULT_CREDITS.md`). Generated by `tools/vault/make_instruments_doc.py` from the built library '
             'and its QC run - do not edit the tables by hand.\n')
    o.append('Note names follow FL Studio: **MIDI 60 = C5** (the plugin\'s home note).\n')
    o.append('## Totals\n')
    o.append(f'* **{len(order)} instruments**, {n_art} articulations, {n_zones} zones (incl. release samples) in {len(fams)} families')
    # this map's instruments only; instruments from other maps (library_map_vocals.json) get one line of their own
    disk_map = sum(qi[i]['diskBytes'] for i in order if i in qi)
    ram_map = sum(qi[i]['ramBytes'] for i in order if i in qi)
    others = [x for x in L['instruments'] if x['id'] not in mp]
    o.append(f'* **{disk_map / 1048576:.0f} MB on disk** (audio, OggVorbis q0.6, 44.1 kHz) - target <= 1800 MB')
    o.append(f'* {ram_map / 1048576:.0f} MB if every instrument were decoded at once (16-bit RAM; the plugin loads only what presets use, 1.5 GB budget)')
    if others:
        o.append(f'* plus {len(others)} instruments from other maps ({", ".join(x["id"] for x in others)}; vocals: '
                 f'`docs/VAULT_VOCAL_SOURCES.md`) - the whole library: {len(L["instruments"])} instruments, '
                 f'{disk_total / 1048576:.0f} MB on disk, {Q["ramBytes"] / 1048576:.0f} MB decoded')
    o.append(f'* builder corrections: {octaves} articulations with octave-off file names, {corrected} roots moved to the measured pitch, '
             f'{named_ok} YIN doubts overruled by the spectrum (name kept), {dropped} broken takes dropped, {excluded} files excluded by the map\n')

    o.append('## Instruments\n')
    o.append('| id | name | family | articulations | range | zones | size MB | source |')
    o.append('| --- | --- | --- | --- | --- | ---: | ---: | --- |')
    for fam in fams:
        for iid in fams[fam]:
            ins = lj[iid]
            q = qi.get(iid, {})
            qa = {a['name']: a for a in q.get('articulations', [])}
            arts = []
            lo, hi = 127, 0
            for a in ins['articulations']:
                x = qa.get(a['name'], {})
                lay = x.get('layers', 1)
                rel = x.get('releaseZones', 0)
                bits = []
                if a.get('oneShot'):
                    bits.append('one-shot')
                elif any(z.get('loopEnd', 0) > 0 for z in a['zones']):
                    bits.append('looped')
                if lay > 1:
                    bits.append(f'{lay} vel')
                if rel:
                    bits.append('release samples')
                arts.append(a['name'] + (f' ({", ".join(bits)})' if bits else ''))
                if x:
                    lo, hi = min(lo, x['lo']), max(hi, x['hi'])
            zones = sum(len(a['zones']) for a in ins['articulations'])
            o.append(f'| `{iid}` | {ins["name"]} | {fam} | {"; ".join(arts)} | {fl(lo)}-{fl(hi)} | {zones} | {mb(q.get("diskBytes", 0))} | {source_of(mp.get(iid, {}))} |')
    o.append('')

    o.append('## QC per articulation\n')
    o.append('Tuning in cents after the zone fine tune (mean / max |error| over the measured zones; `name` = names trusted '
             '(bells, glides), `detect` = pitch measured by the builder, "(spectrum)" = bars / tines measured on the '
             'fundamental\'s spectral peak); stretch = semitones a zone plays from its root inside the range / at the edges; '
             'loops = looped zones: worst wrap jump vs the music around it, level dip the crossfade adds; vel = smallest '
             'loudness step between neighbouring layers; noise = lead-in noise vs peak (average / worst). Flags: see "How '
             'the QC numbers are measured".\n')
    o.append('| instrument / articulation | zones | layers | keys | stretch | tuning c | loops | vel step dB | noise dB | flags |')
    o.append('| --- | ---: | ---: | --- | --- | --- | --- | --- | --- | --- |')
    for iid in order:
        q = qi.get(iid)
        if not q:
            continue
        for a in q['articulations']:
            tun = a['pitchCheck'] if a['pitchCheck'] in ('name', 'detect') else f"{a['centsMean']:.1f} / {a['centsMax']:.1f}" + (' (spectrum)' if a['pitchCheck'] == 'spectrum' else '')
            loops = f"{a['loops']}: {a['loopWrap']:.2f}x, {a['loopDip']:.1f} dB" if a['loops'] else '-'
            vel = f"{a['velMinStep']:.1f}" if a['layers'] > 1 else '-'
            noise = f"{a['noiseAvg']:.0f} / {a['noiseWorst']:.0f}" if a['noiseWorst'] else '-'
            o.append(f"| {iid} / {a['name']} | {a['zones']} | {a['layers']} | {fl(a['lo'])}-{fl(a['hi'])} | {a['stretch']} / {a['edge']} | {tun} | {loops} | {vel} | {noise} | {a['flags'] or 'ok'} |")
    o.append('')

    o.append('## C5 test notes\n')
    o.append('First articulation, velocity 100, 2 s, no effects (`VaultLibBuild -qc -demo <dir>` writes them as `<id>.wav`). '
             '"fund" = the played fundamental vs the strongest of harmonics 2-16 (>= -3 dB meets the C5 comfort rule; '
             'inharmonic bells / drums excepted). Presets fix level and tone; these are the raw instruments.\n')
    o.append('| id | key | peak dBFS | loudness dB | pitch c | fund dB | 2-5 kHz % |')
    o.append('| --- | --- | ---: | ---: | ---: | ---: | ---: |')
    for iid in order:
        d = qi.get(iid, {}).get('demo')
        if not d:
            continue
        o.append(f"| {iid} | {fl(d['key'])} | {d['peakDb']:.1f} | {d['loudDb']:.1f} | {d['cents']:+.1f} | {d['fundVsStrongestDb']:+.1f} | {d['presence'] * 100:.1f} |")
    o.append('')
    o.append(NOTES.strip() + '\n')
    o.append(METHOD.strip() + '\n')
    open(out, 'w').write('\n'.join(o))
    print('wrote', out, len(order), 'instruments')


if __name__ == '__main__':
    main()
