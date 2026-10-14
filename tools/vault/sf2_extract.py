#!/usr/bin/env python3
"""Extract the samples of chosen presets from a SoundFont 2 bank as WAV files VaultLibBuild can map.

  python3 -I tools/vault/sf2_extract.py <bank.sf2> --list                       # presets (bank:program name)
  python3 -I tools/vault/sf2_extract.py <bank.sf2> --preset 0:52 --out DIR --prefix Choir_aahs

Each sample used by the preset's instrument zones is written once as <prefix>_<Note>_<rr>.wav, where <Note> is the zone's
root key (overridingRootKey, else the sample header's original pitch; MIDI 60 = C4 in the file name, the VaultLibBuild
convention) and rr counts samples sharing a root (stereo halves are merged into one stereo file). A manifest
(<prefix>_manifest.json) records the key / velocity ranges, the SF2 loop points and the fine tuning. VaultLibBuild then
re-measures pitch / loudness and finds its own loop. stdlib + numpy only.
"""
import argparse, json, os, struct, sys, wave
import numpy as np

NAMES = 'C C# D D# E F F# G G# A A# B'.split()
def note_name(m): return f"{NAMES[m % 12]}{m // 12 - 1}"

def chunks(data, off, end):
    while off + 8 <= end:
        cid, size = data[off:off + 4].decode('latin1'), struct.unpack('<I', data[off + 4:off + 8])[0]
        yield cid, off + 8, size
        off += 8 + size + (size & 1)

def parse(path):
    data = open(path, 'rb').read()
    assert data[:4] == b'RIFF' and data[8:12] == b'sfbk', 'not a SoundFont 2 file'
    smpl, pdta = None, {}
    for cid, o, s in chunks(data, 12, len(data)):
        if cid != 'LIST': continue
        kind = data[o:o + 4].decode('latin1')
        for sid, so, ss in chunks(data, o + 4, o + s):
            if kind == 'sdta' and sid == 'smpl': smpl = np.frombuffer(data[so:so + ss], dtype='<i2')
            if kind == 'pdta': pdta[sid] = data[so:so + ss]
    def recs(name, fmt):
        n = struct.calcsize(fmt); b = pdta[name]
        return [struct.unpack(fmt, b[i:i + n]) for i in range(0, len(b), n)]
    phdr = recs('phdr', '<20sHHHIII'); pbag = recs('pbag', '<HH'); pgen = recs('pgen', '<HH')
    inst = recs('inst', '<20sH'); ibag = recs('ibag', '<HH'); igen = recs('igen', '<HH')
    shdr = recs('shdr', '<20sIIIIIBbHH')
    return smpl, phdr, pbag, pgen, inst, ibag, igen, shdr

def cstr(b): return b.split(b'\0', 1)[0].decode('latin1').strip()

def gens(genlist, a, b):
    g = {}
    for op, amt in genlist[a:b]: g[op] = amt
    return g

def rng(v): return v & 0xFF, v >> 8

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('sf2'); ap.add_argument('--list', action='store_true')
    ap.add_argument('--preset'); ap.add_argument('--out'); ap.add_argument('--prefix', default='sf2')
    ap.add_argument('--skip', help='leave out samples whose name contains this text (e.g. a noise layer)')
    ap.add_argument('--roots', help='keep only zones whose root key is in lo-hi (MIDI), e.g. 36-90')
    a = ap.parse_args()
    smpl, phdr, pbag, pgen, inst, ibag, igen, shdr = parse(a.sf2)
    if a.list:
        for p in phdr[:-1]: print(f"{p[2]}:{p[1]:3d} {cstr(p[0])}")
        return 0
    bank, prog = (int(v) for v in a.preset.split(':'))
    pi = next(i for i, p in enumerate(phdr[:-1]) if p[1] == prog and p[2] == bank)
    os.makedirs(a.out, exist_ok=True)
    zones = []
    for bz in range(phdr[pi][3], phdr[pi + 1][3]):
        pg = gens(pgen, pbag[bz][0], pbag[bz + 1][0])
        if 41 not in pg: continue                         # global preset zone
        ii = pg[41]
        pk = rng(pg.get(43, 0x7F00)); pv = rng(pg.get(44, 0x7F00))
        for iz in range(inst[ii][1], inst[ii + 1][1]):
            g = gens(igen, ibag[iz][0], ibag[iz + 1][0])
            if 53 not in g: continue                      # global instrument zone
            k = rng(g.get(43, 0x7F00)); v = rng(g.get(44, 0x7F00))
            lo, hi = max(k[0], pk[0]), min(k[1], pk[1]); vlo, vhi = max(v[0], pv[0]), min(v[1], pv[1])
            if lo > hi or vlo > vhi: continue
            sh = shdr[g[53]]
            root = g.get(58, 0xFFFF)
            root = sh[6] if root in (0xFFFF, -1) or root > 127 else root
            fine = struct.unpack('<h', struct.pack('<H', g.get(52, 0)))[0] + sh[7]
            coarse = struct.unpack('<h', struct.pack('<H', g.get(51, 0)))[0]
            zones.append({'inst': cstr(inst[ii][0]), 'sample': g[53], 'link': sh[8], 'type': sh[9], 'name': cstr(sh[0]),
                          'root': root - coarse, 'fineCents': fine, 'lo': lo, 'hi': hi, 'vlo': vlo, 'vhi': vhi,
                          'start': sh[1], 'end': sh[2], 'loopStart': sh[3] - sh[1], 'loopEnd': sh[4] - sh[1], 'rate': sh[5]})
    written, perRoot, manifest = {}, {}, []
    for z in zones:
        key = z['sample']
        if key in written: continue
        if a.skip and a.skip.lower() in z['name'].lower(): continue
        if a.roots and not (int(a.roots.split('-')[0]) <= z['root'] <= int(a.roots.split('-')[1])): continue
        x = smpl[z['start']:z['end']].astype(np.float64) / 32768.0
        ch = [x]
        # stereo pair: the sample link, else (some banks leave the link at 0) the same name with (L) <-> (R)
        partner = None
        if z['type'] & 6:
            partner = next((q for q in zones if q['sample'] == z['link'] and q['sample'] != key), None)
            if partner is None:
                other = z['name'].replace('(L)', '(?)').replace('(R)', '(L)').replace('(?)', '(R)')
                partner = next((q for q in zones if q['name'] == other and q['sample'] != key), None)
            if partner is not None and partner['sample'] in written: continue
        if partner is not None:
            y = smpl[partner['start']:partner['end']].astype(np.float64) / 32768.0
            n = min(len(x), len(y)); ch = [x[:n], y[:n]] if z['type'] & 4 else [y[:n], x[:n]]   # 4 = left, 2 = right
            written[partner['sample']] = True
        written[key] = True
        name = note_name(z['root'])
        perRoot[name] = perRoot.get(name, 0) + 1
        fn = f"{a.prefix}_{name}_{perRoot[name]}.wav"
        data = np.clip(np.round(np.stack(ch, 1) * 32767), -32768, 32767).astype('<i2')
        with wave.open(os.path.join(a.out, fn), 'wb') as w:
            w.setnchannels(data.shape[1]); w.setsampwidth(2); w.setframerate(z['rate']); w.writeframes(data.tobytes())
        manifest.append(dict(z, file=fn, channels=data.shape[1], seconds=round(len(data) / z['rate'], 3)))
        print(f"{fn:28s} {z['name']:20s} root {z['root']} fine {z['fineCents']:+d} c keys {z['lo']}-{z['hi']} vel {z['vlo']}-{z['vhi']} "
              f"{len(data)/z['rate']:.2f} s loop {z['loopStart']}-{z['loopEnd']} ch {data.shape[1]}")
    with open(os.path.join(a.out, f"{a.prefix}_manifest.json"), 'w') as fh: json.dump(manifest, fh, indent=1)
    return 0

if __name__ == '__main__':
    sys.exit(main())
