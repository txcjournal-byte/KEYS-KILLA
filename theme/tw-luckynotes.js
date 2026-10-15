/* TrapWin Lucky Notes product page: platform picker, sticky buy bar, in-browser "try it" generator. */
(function () {
  'use strict';

  function init(root) {
    /* platform picker -> hidden variant input + prices */
    const form = root.querySelector('.twln-form');
    const idInput = root.querySelector('[data-variant-input]');
    const prices = root.querySelectorAll('[data-price]');
    root.querySelectorAll('input[name="twln-os"]').forEach(function (r) {
      r.addEventListener('change', function () {
        if (!r.checked) return;
        idInput.value = r.value;
        prices.forEach(function (p) { p.textContent = r.dataset.price; });
      });
    });

    /* sticky bar when the buy box scrolls away */
    const sticky = root.querySelector('[data-sticky]');
    const buy = root.querySelector('.twln-buy');
    if (sticky && buy && 'IntersectionObserver' in window) {
      new IntersectionObserver(function (es) {
        const show = !es[0].isIntersecting && es[0].boundingClientRect.top < 0;
        sticky.classList.toggle('is-on', show);
        sticky.setAttribute('aria-hidden', show ? 'false' : 'true');
        sticky.querySelector('button').tabIndex = show ? 0 : -1;
      }).observe(buy);
      sticky.querySelector('[data-sticky-buy]').addEventListener('click', function () {
        if (form.requestSubmit) form.requestSubmit(); else form.submit();
      });
    }

    const tryBox = root.querySelector('[data-try]');
    if (tryBox) tryIt(tryBox);
  }

  /* ---------------- try it ---------------- */
  const NAMES = ['STEPWISE', 'ARPEGGIO', 'HIGH HOOK', 'LOW MOTIF', 'SPARSE', 'BUSY RUN', 'TRIPLETS', 'DOTTED', 'SYNCOPATED', 'CALL+ANSWER', 'BOUNCE', 'ZIGZAG'];
  const ROOTS = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'];
  const MINOR = [0, 2, 3, 5, 7, 8, 10], MAJOR = [0, 2, 4, 5, 7, 9, 11];
  const BPM = 150, BARS = 4, STEPS = BARS * 16;

  function rng(seed) { let a = seed >>> 0; return function () { a = (a + 0x6D2B79F5) >>> 0; let t = Math.imul(a ^ (a >>> 15), 1 | a); t = (t + Math.imul(t ^ (t >>> 7), 61 | t)) ^ t; return ((t ^ (t >>> 14)) >>> 0) / 4294967296; }; }

  function makeMelody(r, ch, root, scale) {
    // degree-based 1-bar motif, answered and varied: A A' A B ; notes in scale, chord tones on beats 1 & 3
    const prog = [0, 5, 2, 6];                       // i - VI - III - VII (minor) / I - vi - iii - vii° shape
    const lo = 60 + root - (root > 6 ? 12 : 0);
    const deg = function (d) { const o = Math.floor(d / 7); return lo + 12 * o + scale[((d % 7) + 7) % 7]; };
    const dens = [.55, .7, .5, .45, .25, .9, .6, .45, .55, .5, .65, .75][ch];
    const rhythms = [];
    for (let s = 0; s < 16; s++) {
      const w = s % 8 === 0 ? 1 : s % 4 === 0 ? .7 : s % 2 === 0 ? .45 : (ch === 8 || ch === 10 ? .5 : .15);
      if (r() < w * dens * 1.4) rhythms.push(s);
    }
    if (!rhythms.length || rhythms[0] !== 0) rhythms.unshift(0);
    const base = ch === 3 ? -3 : ch === 2 ? 5 : 2;
    let cur = base; const notes = [];
    for (let bar = 0; bar < BARS; bar++) {
      const cd = prog[bar % 4];
      const tones = [cd, cd + 2, cd + 4];
      const rh = bar === 3 ? rhythms.filter(function (s) { return s < 12; }) : rhythms;
      for (let i = 0; i < rh.length; i++) {
        const s = rh[i], next = i + 1 < rh.length ? rh[i + 1] : 16;
        if (s % 8 === 0) { let best = tones[0], bd = 99; for (const t of tones) for (const o of [-7, 0, 7]) { const d = Math.abs(t + o - cur); if (d < bd) { bd = d; best = t + o; } } cur = best; }
        else if (ch === 1) cur += 2;
        else if (ch === 11) cur += (i % 2 ? -3 : 3);
        else cur += [-2, -1, -1, 1, 1, 2][Math.floor(r() * 6)];
        if (cur > base + 9) cur -= 7; if (cur < base - 5) cur += 7;
        const len = Math.max(1, Math.min(next - s, ch === 4 ? 6 : ch === 7 ? 3 : 2));
        notes.push({ s: bar * 16 + s, l: len, m: deg(cur) });
      }
    }
    return notes;
  }

  function makeSet(seed) {
    const r = rng(seed);
    const root = Math.floor(r() * 12), minor = r() < .7;
    const scale = minor ? MINOR : MAJOR;
    const set = [];
    for (let i = 0; i < 12; i++) set.push(makeMelody(rng(seed * 31 + i * 977), i, root, scale));
    return { root: root, minor: minor, key: ROOTS[root] + (minor ? ' MIN' : ' MAJ'), keyFile: ROOTS[root].replace('#', 's') + (minor ? 'min' : 'maj'), lines: set };
  }

  function drawRoll(cv, notes, playStep) {
    const dpr = Math.min(2, window.devicePixelRatio || 1);
    const w = cv.clientWidth || 600, h = cv.clientHeight || 34;
    cv.width = Math.round(w * dpr); cv.height = Math.round(h * dpr);
    const g = cv.getContext('2d'); g.setTransform(dpr, 0, 0, dpr, 0, 0); g.clearRect(0, 0, w, h);
    g.fillStyle = 'rgba(51,255,102,.12)';
    for (let b = 1; b < BARS; b++) g.fillRect(Math.round(b / BARS * w), 0, 1, h);
    let lo = 127, hi = 0; notes.forEach(function (n) { lo = Math.min(lo, n.m); hi = Math.max(hi, n.m); });
    if (hi - lo < 6) { hi = lo + 6; }
    g.fillStyle = '#33ff66'; g.shadowColor = 'rgba(51,255,102,.7)'; g.shadowBlur = 4;
    notes.forEach(function (n) {
      const x = n.s / STEPS * w, y = 3 + (hi - n.m) / (hi - lo) * (h - 9);
      g.fillRect(Math.round(x), Math.round(y), Math.max(2, n.l / STEPS * w - 1), 3);
    });
    g.shadowBlur = 0;
    if (playStep != null) { g.fillStyle = '#ffb000'; g.fillRect(Math.round(playStep / STEPS * w), 0, 2, h); }
  }

  /* minimal MIDI file (format 0) */
  function midiFile(notes) {
    const ppq = 96, tick = ppq / 4, ev = [];
    notes.forEach(function (n) { ev.push([n.s * tick, 0x90, n.m, 96]); ev.push([(n.s + n.l) * tick - 2, 0x80, n.m, 0]); });
    ev.sort(function (a, b) { return a[0] - b[0] || (a[1] === 0x80 ? -1 : 1); });
    const vlq = function (v) { const b = [v & 127]; while ((v >>= 7)) b.unshift((v & 127) | 128); return b; };
    const us = Math.round(60000000 / BPM);
    let tr = [0, 0xFF, 0x51, 3, (us >> 16) & 255, (us >> 8) & 255, us & 255, 0, 0xFF, 0x58, 4, 4, 2, 24, 8], last = 0;
    ev.forEach(function (e) { tr = tr.concat(vlq(e[0] - last), [e[1], e[2], e[3]]); last = e[0]; });
    tr = tr.concat(vlq(STEPS * tick - last), [0xFF, 0x2F, 0]);
    const L = tr.length;
    return new Uint8Array([0x4D, 0x54, 0x68, 0x64, 0, 0, 0, 6, 0, 0, 0, 1, 0, ppq, 0x4D, 0x54, 0x72, 0x6B, (L >>> 24) & 255, (L >> 16) & 255, (L >> 8) & 255, L & 255].concat(tr));
  }

  function tryIt(box) {
    const list = box.querySelector('[data-rows]');
    const keyEl = box.querySelector('[data-key]');
    let set = null, seed = 7, ac = null, playing = -1, startT = 0, voices = [], raf = 0;
    const rowsEls = [];

    function audio() {
      if (!ac) { const C = window.AudioContext || window.webkitAudioContext; if (!C) return null; ac = new C(); }
      if (ac.state === 'suspended') ac.resume();
      return ac;
    }
    function stop() {
      const t = ac ? ac.currentTime : 0;
      voices.forEach(function (v) { try { v.g.gain.cancelScheduledValues(t); v.g.gain.setTargetAtTime(0, t, .01); v.o.stop(t + .08); } catch (e) {} });
      voices = []; cancelAnimationFrame(raf);
      if (playing >= 0 && rowsEls[playing]) { rowsEls[playing].li.classList.remove('is-on'); rowsEls[playing].play.textContent = '▶'; drawRoll(rowsEls[playing].cv, set.lines[playing]); }
      playing = -1;
    }
    function play(i) {
      const a = audio(); if (!a) return;
      stop(); playing = i;
      const spb = 60 / BPM, step = spb / 4, t0 = a.currentTime + .06, loops = 2;
      const out = a.createGain(); out.gain.value = .18; out.connect(a.destination);
      for (let k = 0; k < loops; k++) set.lines[i].forEach(function (n) {
        const t = t0 + (k * STEPS + n.s) * step, d = Math.max(.06, n.l * step * .9);
        const o = a.createOscillator(), g = a.createGain();
        o.type = 'sine'; o.frequency.value = 440 * Math.pow(2, (n.m - 69) / 12);
        g.gain.setValueAtTime(0, t); g.gain.linearRampToValueAtTime(1, t + .004); g.gain.setTargetAtTime(.6, t + .004, .05);
        g.gain.setTargetAtTime(0, t + d, .02);
        o.connect(g).connect(out); o.start(t); o.stop(t + d + .2); voices.push({ o: o, g: g });
      });
      startT = t0;
      const r = rowsEls[i]; r.li.classList.add('is-on'); r.play.textContent = '■';
      const total = loops * STEPS * step;
      (function tick() {
        if (playing !== i) return;
        const el = a.currentTime - startT;
        if (el > total + .1) { stop(); return; }
        drawRoll(r.cv, set.lines[i], el < 0 ? null : (el / step) % STEPS);
        raf = requestAnimationFrame(tick);
      })();
    }
    function download(i) {
      const blob = new Blob([midiFile(set.lines[i])], { type: 'audio/midi' });
      const a = document.createElement('a');
      a.href = URL.createObjectURL(blob);
      a.download = 'LuckyNotes_Demo_' + String(i + 1).padStart(2, '0') + '_' + BPM + 'BPM_' + set.keyFile + '_' + BARS + 'bars.mid';
      document.body.appendChild(a); a.click(); setTimeout(function () { URL.revokeObjectURL(a.href); a.remove(); }, 500);
    }
    function build() {
      stop(); set = makeSet(seed); keyEl.textContent = set.key;
      list.textContent = ''; rowsEls.length = 0;
      set.lines.forEach(function (notes, i) {
        const li = document.createElement('li'); li.className = 'twln-r';
        const n = document.createElement('span'); n.className = 'twln-r__n'; n.textContent = String(i + 1).padStart(2, '0');
        const p = document.createElement('button'); p.type = 'button'; p.className = 'twln-r__play'; p.textContent = '▶'; p.setAttribute('aria-label', 'Play melody ' + (i + 1));
        const nm = document.createElement('span'); nm.className = 'twln-r__name'; nm.textContent = NAMES[i];
        const cv = document.createElement('canvas'); cv.className = 'twln-r__roll'; cv.setAttribute('aria-hidden', 'true');
        const dl = document.createElement('button'); dl.type = 'button'; dl.className = 'twln-r__dl'; dl.textContent = 'GET MIDI'; dl.setAttribute('aria-label', 'Download melody ' + (i + 1) + ' as a MIDI file');
        p.addEventListener('click', function () { if (playing === i) stop(); else play(i); });
        dl.addEventListener('click', function () { download(i); });
        li.append(n, p, nm, cv, dl); list.appendChild(li);
        rowsEls.push({ li: li, play: p, cv: cv });
      });
      requestAnimationFrame(function () { rowsEls.forEach(function (r, i) { drawRoll(r.cv, set.lines[i]); }); });
    }
    box.querySelector('[data-generate]').addEventListener('click', function () { seed = (seed * 1103515245 + 12345) >>> 0; build(); });
    let rt; window.addEventListener('resize', function () { clearTimeout(rt); rt = setTimeout(function () { if (set) rowsEls.forEach(function (r, i) { if (i !== playing) drawRoll(r.cv, set.lines[i]); }); }, 150); });
    build();
  }

  document.querySelectorAll('.twln').forEach(init);
})();
