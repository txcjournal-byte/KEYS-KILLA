#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <atomic>
#include <memory>
#include <vector>
#include <cmath>
#include <algorithm>

// v0.23 CHOP / SLICE: DIGGA's sample cut into slices you can move, play on the keys (MPC style, first slice = C5),
// drag out as WAV, send to PAIR / the bank, and drag the chop pattern into FL as MIDI.
namespace kk
{
// per slice controls (MPC style): pitch, reverse, volume, pan, fade in / out
struct SliceFx { int semi = 0; bool rev = false; float vol = 1.0f, pan = 0.0f, fade = 0.004f; };
struct ChopData
{
    std::shared_ptr<const juce::AudioBuffer<float>> src;
    std::vector<SliceFx> fx;  // one per slice
    double rate = 44100.0;
    std::vector<int> marks;   // slice starts, sorted, marks[0] == 0
    juce::String name;
    int numSlices() const { return (int) marks.size(); }
    SliceFx fxOf (int i) const { return i >= 0 && i < (int) fx.size() ? fx[(size_t) i] : SliceFx {}; }
    int sliceStart (int i) const { return marks[(size_t) i]; }
    int sliceEnd (int i) const { return i + 1 < (int) marks.size() ? marks[(size_t) i + 1] : (src ? src->getNumSamples() : 0); }
};
using ChopPtr = std::shared_ptr<const ChopData>;

class ChopLab
{
public:
    static constexpr int firstNote = 60, maxSlices = 64;

    // ---------- message thread ----------
    void setSource (std::shared_ptr<const juce::AudioBuffer<float>> src, double rate, const juce::String& name)
    {
        auto d = std::make_shared<ChopData>();
        d->src = std::move (src); d->rate = rate; d->name = name;
        d->marks = d->src != nullptr ? transients (*d->src, rate, 16) : std::vector<int> {};
        if (d->marks.empty() && d->src != nullptr) d->marks = { 0 };
        publish (d);
    }
    ChopPtr current() const { const juce::SpinLock::ScopedLockType l (lock); return data; }
    bool hasSource() const { auto d = current(); return d != nullptr && d->src != nullptr && d->src->getNumSamples() > 0; }
    void clear() { publish (std::make_shared<ChopData>()); }   // v0.35: CLEAR - the sampler is empty again
    // v0.35: new audio of the same length (MUTATE / KILL / UNDO) - the cuts and the slice settings stay
    void replaceAudio (std::shared_ptr<const juce::AudioBuffer<float>> src)
    {
        auto d = current(); if (d == nullptr || src == nullptr) return;
        auto n = std::make_shared<ChopData> (*d);
        n->src = std::move (src);
        publish (n);
    }
    // v0.35: play any part of the sample (the selection), once or as a loop
    void playRegion (int start, int end, bool loop) { regStart = start; regEnd = end; regLoop = loop; regReq = true; }
    void stopAll() { stopReq = true; }
    bool regionPlaying() const { return regActive.load(); }
    bool anyPlaying() const { return regActive.load() || playhead() >= 0; }

    // mode 0 = transients, -1 = note changes (vocals / melodies), gridMode: 1/4 1/8 1/16 at the tempo, >0 = equal slices
    void autoSlice (int mode, double bpm = 0.0, int gridDiv = 0)
    {
        auto d = current(); if (d == nullptr || d->src == nullptr) return;
        auto n = std::make_shared<ChopData> (*d);
        if (gridDiv > 0 && bpm > 0)
        {
            n->marks.clear();
            const double step = d->rate * 60.0 / bpm * 4.0 / gridDiv;   // gridDiv 4 = 1/4 note
            for (double x = 0; x < d->src->getNumSamples() - d->rate * 0.02 && (int) n->marks.size() < maxSlices; x += step) n->marks.push_back ((int) x);
        }
        else if (mode == 0) n->marks = transients (*d->src, d->rate, 16);
        else if (mode < 0) n->marks = noteChanges (*d->src, d->rate, 16);
        else
        {
            n->marks.clear();
            const int len = d->src->getNumSamples();
            for (int i = 0; i < mode; ++i) n->marks.push_back ((int) ((juce::int64) len * i / mode));
        }
        if (n->marks.empty()) n->marks = { 0 };
        publish (n);
    }
    void setMarks (std::vector<int> m)
    {
        auto d = current(); if (d == nullptr || d->src == nullptr) return;
        const int len = d->src->getNumSamples();
        std::sort (m.begin(), m.end());
        std::vector<int> clean { 0 };
        const int minGap = (int) (d->rate * 0.02);
        for (int x : m) if (x > clean.back() + minGap && x < len - minGap && (int) clean.size() < maxSlices) clean.push_back (x);
        auto n = std::make_shared<ChopData> (*d); n->marks = std::move (clean);
        publish (n);
    }
    void setFx (int i, const SliceFx& f)
    {
        auto d = current(); if (d == nullptr || i < 0 || i >= d->numSlices()) return;
        auto n = std::make_shared<ChopData> (*d); n->fx.resize (n->marks.size()); n->fx[(size_t) i] = f;
        const juce::SpinLock::ScopedLockType l (lock);
        data = std::move (n);   // voices keep playing
    }
    // FLIP: a new trap pattern from the slices (1/8 grid, accents on the beat, some 1/16 doubles) as MIDI
    juce::File flipMidi (double bpm, uint32_t seed, int bars = 2) const
    {
        auto d = current(); if (d == nullptr || d->src == nullptr || d->numSlices() == 0) return {};
        juce::Random r ((juce::int64) seed);
        const int ppq = 960;
        juce::MidiMessageSequence seq;
        auto tempo = juce::MidiMessage::tempoMetaEvent ((int) std::round (60000000.0 / bpm)); tempo.setTimeStamp (0); seq.addEvent (tempo);
        const int ns = std::min (d->numSlices(), 128 - firstNote);
        for (int step = 0; step < bars * 8; ++step)
        {
            if (step % 8 != 0 && r.nextFloat() < 0.22f) continue;            // rests
            const int sl = step % 8 == 0 ? 0 : r.nextInt (ns);               // the 1 keeps the first chop
            const double b = step * 0.5;
            const bool dbl = r.nextFloat() < 0.18f;
            const double len = dbl ? 0.25 : 0.5;
            const int vel = step % 4 == 0 ? 115 : 85 + r.nextInt (20);
            seq.addEvent (juce::MidiMessage::noteOn (1, firstNote + sl, (juce::uint8) vel), std::round (b * ppq));
            seq.addEvent (juce::MidiMessage::noteOff (1, firstNote + sl), std::round ((b + len * 0.95) * ppq));
            if (dbl)
            {
                seq.addEvent (juce::MidiMessage::noteOn (1, firstNote + sl, (juce::uint8) (vel - 20)), std::round ((b + 0.25) * ppq));
                seq.addEvent (juce::MidiMessage::noteOff (1, firstNote + sl), std::round ((b + 0.49) * ppq));
            }
        }
        seq.updateMatchedPairs();
        juce::MidiFile mf; mf.setTicksPerQuarterNote (ppq); mf.addTrack (seq);
        auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("KEYS KILLA Chops");
        dir.createDirectory();
        auto f = dir.getChildFile (juce::File::createLegalFileName (d->name + " - flip " + juce::String ((int) (seed % 1000))) + ".mid");
        f.deleteFile();
        if (juce::FileOutputStream os { f }; os.openedOk()) mf.writeTo (os, 1);
        return f;
    }
    juce::AudioBuffer<float> slice (int i) const
    {
        juce::AudioBuffer<float> out;
        auto d = current();
        if (d == nullptr || d->src == nullptr || i < 0 || i >= d->numSlices()) return out;
        const int s0 = d->sliceStart (i), len = std::max (1, d->sliceEnd (i) - s0);
        out.setSize (2, len);
        for (int c = 0; c < 2; ++c) out.copyFrom (c, 0, *d->src, std::min (c, d->src->getNumChannels() - 1), s0, len);
        const auto fx = d->fxOf (i);
        if (fx.rev) out.reverse (0, len);
        if (fx.semi != 0)   // pitch: resample
        {
            const double ratio = std::exp2 (fx.semi / 12.0);
            const int nl = std::max (2, (int) (len / ratio));
            juce::AudioBuffer<float> o (2, nl);
            for (int c = 0; c < 2; ++c)
                for (int k = 0; k < nl; ++k)
                {
                    const double p = k * ratio; const int i0 = std::min (len - 2, (int) p); const float fr = (float) (p - i0);
                    o.setSample (c, k, out.getSample (c, i0) + (out.getSample (c, i0 + 1) - out.getSample (c, i0)) * fr);
                }
            out = std::move (o);
        }
        const int L = out.getNumSamples();
        const int fade = std::min (L / 3, std::max (8, (int) (d->rate * fx.fade)));
        for (int c = 0; c < 2; ++c) { out.applyGainRamp (c, 0, fade, 0.0f, 1.0f); out.applyGainRamp (c, L - fade, fade, 1.0f, 0.0f); }
        const float gl = fx.vol * std::cos ((fx.pan + 1.0f) * 0.25f * juce::MathConstants<float>::pi) * 1.41421356f;
        const float gr = fx.vol * std::sin ((fx.pan + 1.0f) * 0.25f * juce::MathConstants<float>::pi) * 1.41421356f;
        out.applyGain (0, 0, L, gl); out.applyGain (1, 0, L, gr);
        return out;
    }
    juce::File exportSlice (int i) const
    {
        auto d = current();
        auto b = slice (i);
        if (b.getNumSamples() < 2 || d == nullptr) return {};
        auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("KEYS KILLA Chops");
        dir.createDirectory();
        auto f = dir.getChildFile (juce::File::createLegalFileName (d->name + " - chop " + juce::String (i + 1)) + ".wav");
        f.deleteFile();
        juce::WavAudioFormat wav;
        if (auto os = std::unique_ptr<juce::OutputStream> (new juce::FileOutputStream (f)))
            if (auto w = std::unique_ptr<juce::AudioFormatWriter> (wav.createWriterFor (os.get(), d->rate, 2, 24, {}, 0)))
            { os.release(); w->writeFromAudioSampleBuffer (b, 0, b.getNumSamples()); }
        return f;
    }
    // the chop as it was played in the sample: slice i at its own time (host tempo = the sample's tempo)
    juce::File exportMidi (double bpm) const
    {
        auto d = current(); if (d == nullptr || d->src == nullptr) return {};
        const int ppq = 960;
        juce::MidiMessageSequence seq;
        auto tempo = juce::MidiMessage::tempoMetaEvent ((int) std::round (60000000.0 / bpm)); tempo.setTimeStamp (0); seq.addEvent (tempo);
        const double beatsPerSample = bpm / 60.0 / d->rate;
        for (int i = 0; i < d->numSlices() && i < 128 - firstNote; ++i)
        {
            const double b0 = d->sliceStart (i) * beatsPerSample, b1 = d->sliceEnd (i) * beatsPerSample;
            seq.addEvent (juce::MidiMessage::noteOn (1, firstNote + i, (juce::uint8) 100), std::round (b0 * ppq));
            seq.addEvent (juce::MidiMessage::noteOff (1, firstNote + i), std::round (std::max (b0 + 0.05, b1) * ppq));
        }
        seq.updateMatchedPairs();
        juce::MidiFile mf; mf.setTicksPerQuarterNote (ppq); mf.addTrack (seq);
        auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("KEYS KILLA Chops");
        dir.createDirectory();
        auto f = dir.getChildFile (juce::File::createLegalFileName (d->name + " - chop pattern") + ".mid");
        f.deleteFile();
        if (juce::FileOutputStream os { f }; os.openedOk()) mf.writeTo (os, 1);
        return f;
    }
    std::vector<float> overview (int cols) const
    {
        std::vector<float> pk ((size_t) cols, 0.0f);
        auto d = current(); if (d == nullptr || d->src == nullptr) return pk;
        const auto& a = *d->src; const int len = a.getNumSamples();
        for (int c = 0; c < cols && len > 0; ++c)
        {
            const int s0 = (int) ((juce::int64) len * c / cols), s1 = std::max (s0 + 1, (int) ((juce::int64) len * (c + 1) / cols));
            pk[(size_t) c] = a.getMagnitude (s0, s1 - s0);
        }
        const float m = *std::max_element (pk.begin(), pk.end());
        if (m > 1.0e-6f) for (auto& v : pk) v /= m;
        return pk;
    }

    // ---------- audio thread ----------
    void prepare (double sampleRate) { outRate = sampleRate; }
    void noteOn (int note, float vel, int offset, bool rev = false, int semi = 0)
    {
        const int i = note - firstNote;
        if (i < 0) return;
        // MONO pads (choke, on by default): a new pad stops the one before - like an MPC / FL slicer in mono.
        // Either way the old voice fades out in 8 ms (never a hard cut = no click)
        for (auto& x : voices)
            if (x.active && ! x.rel && (choke.load() || x.slice == i)) { x.rel = true; x.fast = true; }
        Voice* v = nullptr;
        for (auto& x : voices) if (! x.active) { v = &x; break; }
        if (v == nullptr) { v = &voices[0]; for (auto& x : voices) if (x.rel && x.relGain < v->relGain) v = &x; }
        *v = {};
        v->active = true; v->slice = i; v->note = note; v->vel = 0.3f + 0.7f * vel; v->delay = offset; v->vrev = rev; v->vsemi = semi;
        lastHit = i;
    }
    std::atomic<bool> choke { true };
    void noteOff (int note) { for (auto& v : voices) if (v.active && v.note == note) v.rel = true; }
    void allOff() { for (auto& v : voices) v.rel = true; }
    void render (float* L, float* R, int n)
    {
        const juce::SpinLock::ScopedTryLockType tl (lock);
        if (! tl.isLocked() || data == nullptr || data->src == nullptr) { regActive = false; return; }
        const auto& d = *data;
        const auto& a = *d.src;
        const float* l = a.getReadPointer (0); const float* r = a.getReadPointer (a.getNumChannels() > 1 ? 1 : 0);
        const double inc = d.rate / outRate;
        if (stopReq.exchange (false)) { for (auto& v : voices) v.rel = true; regActive = false; reg.on = false; }
        if (regReq.exchange (false))
        {
            reg = {}; reg.on = true; reg.s0 = juce::jlimit (0, a.getNumSamples() - 2, regStart.load()); reg.s1 = juce::jlimit (reg.s0 + 2, a.getNumSamples(), regEnd.load());
            reg.loop = regLoop.load(); regActive = true;
        }
        if (reg.on)   // the selection player (with 6 ms fades so a loop never clicks)
        {
            const double slen = reg.s1 - reg.s0, fade = std::min (slen * 0.25, d.rate * 0.006);
            for (int i = 0; i < n; ++i)
            {
                if (reg.pos >= slen - 1) { if (reg.loop) reg.pos = 0; else { reg.on = false; break; } }
                const double p = reg.s0 + reg.pos;
                const int i0 = std::min ((int) p, a.getNumSamples() - 2);
                const float fr = (float) (p - std::floor (p));
                const float g = (float) std::min (1.0, std::min (reg.pos / fade, (slen - reg.pos) / fade));
                L[i] += (l[i0] + (l[i0 + 1] - l[i0]) * fr) * g;
                R[i] += (r[i0] + (r[i0 + 1] - r[i0]) * fr) * g;
                reg.pos += inc;
            }
            playPos = (float) ((reg.s0 + reg.pos) / std::max (1, a.getNumSamples()));
            regActive = reg.on;
        }
        const float relStep = 1.0f / (0.012f * (float) outRate), fastStep = 1.0f / (0.008f * (float) outRate);
        for (auto& v : voices)
        {
            if (! v.active) continue;
            if (v.slice >= d.numSlices()) { v.active = false; continue; }
            const double s0 = d.sliceStart (v.slice), s1 = d.sliceEnd (v.slice), slen = s1 - s0;
            auto fx = d.fxOf (v.slice);
            fx.rev = fx.rev != v.vrev;
            const double vinc = inc * std::exp2 ((fx.semi + v.vsemi) / 12.0);
            const double fadeS = std::max (8.0, d.rate * fx.fade);
            const float gl = fx.vol * std::cos ((fx.pan + 1.0f) * 0.25f * juce::MathConstants<float>::pi) * 1.41421356f;
            const float gr = fx.vol * std::sin ((fx.pan + 1.0f) * 0.25f * juce::MathConstants<float>::pi) * 1.41421356f;
            for (int i = 0; i < n; ++i)
            {
                if (v.delay > 0) { --v.delay; continue; }
                if (v.pos + 1 >= slen) { v.active = false; break; }
                const double p = fx.rev ? s1 - 1 - v.pos : s0 + v.pos;
                const int i0 = std::max ((int) s0, std::min ((int) s1 - 2, (int) p));
                const float fr = (float) (p - std::floor (p));
                float g = v.vel * (float) std::min (1.0, std::min (v.pos / fadeS, (slen - v.pos) / fadeS));
                if (v.rel) { v.relGain -= v.fast ? fastStep : relStep; if (v.relGain <= 0) { v.active = false; break; } g *= v.relGain; }
                L[i] += (l[i0] + (l[i0 + 1] - l[i0]) * fr) * g * gl;
                R[i] += (r[i0] + (r[i0 + 1] - r[i0]) * fr) * g * gr;
                v.pos += vinc;
            }
            playPos = (float) ((s0 + v.pos) / std::max (1, a.getNumSamples()));
        }
    }
    float playhead() const { if (regActive.load()) return playPos.load(); for (auto& v : voices) if (v.active) return playPos.load(); return -1.0f; }

    // v0.37 MUTATE / KILL: musical, in time. The part is cut into 1/8 notes (at the project tempo); each piece gets a move -
    // played backwards, stuttered, slowed like a tape stop, an octave down / up, filtered, gated. The pieces stay where they
    // were, so the melody and the groove survive. MUTATE: a few pieces.  KILL: most pieces, bigger moves, a little crush.
    static void mutate (juce::AudioBuffer<float>& a, int start, int end, double rate, bool kill, uint32_t seed, double bpm = 140.0)
    {
        start = juce::jlimit (0, a.getNumSamples(), start); end = juce::jlimit (start, a.getNumSamples(), end);
        const int len = end - start, ch = a.getNumChannels();
        if (len < 256) return;
        juce::AudioBuffer<float> src (ch, len);
        for (int c = 0; c < ch; ++c) src.copyFrom (c, 0, a, c, start, len);
        juce::Random rnd ((juce::int64) seed);
        const int piece = std::max (256, (int) (rate * 60.0 / juce::jlimit (60.0, 200.0, bpm) * 0.5));   // 1/8 note
        const float pChange = kill ? 0.72f : 0.34f;
        enum { keep, reverse, stutter, tape, octDown, octUp, filter, gate };
        const int xf = std::max (16, (int) (rate * 0.003));
        for (int p0 = 0; p0 < len; p0 += piece)
        {
            const int n = std::min (piece, len - p0);
            if (n < 64) break;
            int mv = keep;
            if (rnd.nextFloat() < pChange)
            {
                static const int mild[] { reverse, stutter, filter, gate, reverse, stutter };
                static const int wild[] { reverse, stutter, tape, octDown, octUp, filter, gate, stutter, reverse, tape };
                mv = kill ? wild[rnd.nextInt (10)] : mild[rnd.nextInt (6)];
            }
            if (mv == keep) continue;
            juce::AudioBuffer<float> o (ch, n);
            auto at = [&] (int c, double pos) { const int i0 = juce::jlimit (0, len - 2, (int) pos); const float fr = (float) (pos - std::floor (pos)); const float* x = src.getReadPointer (c); return x[i0] + (x[i0 + 1] - x[i0]) * fr; };
            const float cut = 600.0f + rnd.nextFloat() * 1400.0f;
            const int rep = rnd.nextBool() ? 4 : 8;
            for (int c = 0; c < ch; ++c)
            {
                float lp = 0, held = 0;
                const float k = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * cut / (float) rate);
                double tp = 0;
                for (int i = 0; i < n; ++i)
                {
                    float y = src.getSample (c, p0 + i);
                    switch (mv)
                    {
                        case reverse: y = src.getSample (c, p0 + n - 1 - i); break;
                        case stutter: { const int sl = std::max (32, n / rep); const int ph = i % sl; const float w = std::min (1.0f, std::min ((float) ph, (float) (sl - ph)) / (float) xf); y = src.getSample (c, p0 + ph) * w; break; }
                        case tape: { tp += std::max (0.0, 1.0 - (double) i / n * 1.1); y = at (c, p0 + tp) * std::min (1.0f, (float) (n - i) / (float) (n / 6 + 1)); break; }
                        case octDown: y = at (c, p0 + i * 0.5); break;
                        case octUp: y = at (c, p0 + (i * 2) % n); break;
                        case filter: lp += (y - lp) * k; y = lp; break;
                        case gate: y *= (i % (n / 2) < n / 4) ? 1.0f : 0.0f; break;
                        default: break;
                    }
                    if (kill && (mv == octDown || mv == stutter)) { if (i % 3 == 0) held = std::round (y * 256.0f) / 256.0f; y = y * 0.5f + held * 0.5f; }
                    o.setSample (c, i, y);
                }
            }
            if (mv == gate)   // soften the gate edges
                for (int c = 0; c < ch; ++c) { float sm = o.getSample (c, 0); const float g = 1.0f / (float) xf; for (int i = 0; i < n; ++i) { sm += (o.getSample (c, i) - sm) * std::min (1.0f, g * 4.0f); o.setSample (c, i, sm); } }
            // match the piece's loudness, crossfade the edges into the neighbours
            const float rIn = src.getRMSLevel (0, p0, n) + 1.0e-6f, rOut = o.getRMSLevel (0, 0, n) + 1.0e-6f;
            const float gm = juce::jlimit (0.3f, 2.5f, rIn / rOut);
            for (int c = 0; c < ch; ++c)
            {
                float* out = a.getWritePointer (c, start + p0);
                for (int i = 0; i < n; ++i)
                {
                    const float e = std::min (1.0f, std::min ((float) i / (float) xf, (float) (n - 1 - i) / (float) xf));
                    const float y = juce::jlimit (-1.0f, 1.0f, o.getSample (c, i) * gm);
                    out[i] = src.getSample (c, p0 + i) + (y - src.getSample (c, p0 + i)) * e;
                }
            }
        }
    }
    std::atomic<int> lastHit { -1 };

    // energy-flux onsets, at most maxCount slices, at least 60 ms apart
    static std::vector<int> transients (const juce::AudioBuffer<float>& a, double rate, int maxCount)
    {
        const int hop = 256, len = a.getNumSamples();
        if (len < hop * 4) return { 0 };
        std::vector<float> e;
        for (int s = 0; s + hop <= len; s += hop)
        {
            float acc = 0;
            for (int c = 0; c < a.getNumChannels(); ++c) { const float* x = a.getReadPointer (c, s); for (int k = 1; k < hop; ++k) { const float d = x[k] - x[k - 1]; acc += d * d; } }
            e.push_back (std::log1p (acc * 100.0f));
        }
        std::vector<std::pair<float, int>> cand;
        const int minGap = std::max (1, (int) (rate * 0.06 / hop));
        for (int i = 2; i + 1 < (int) e.size(); ++i)
        {
            float mean = 0; int cnt = 0;
            for (int k = std::max (0, i - 12); k < i; ++k) { mean += e[(size_t) k]; ++cnt; }
            mean /= (float) std::max (1, cnt);
            const float flux = e[(size_t) i] - mean;
            if (flux > 0.25f && e[(size_t) i] >= e[(size_t) i - 1] && e[(size_t) i] >= e[(size_t) i + 1]) cand.push_back ({ flux, i });
        }
        std::sort (cand.begin(), cand.end(), [] (auto& x, auto& y) { return x.first > y.first; });
        std::vector<int> picks { 0 };
        for (auto& [f, i] : cand)
        {
            if ((int) picks.size() >= maxCount) break;
            bool ok = true;
            for (int p : picks) if (std::abs (p / hop - i) < minGap) { ok = false; break; }
            if (ok) picks.push_back (i * hop);
        }
        std::sort (picks.begin(), picks.end());
        return picks;
    }

    // NOTES: cut where the pitch moves (vocal syllables, melodies) - autocorrelation pitch per 23 ms frame
    static std::vector<int> noteChanges (const juce::AudioBuffer<float>& a, double rate, int maxCount)
    {
        const int win = 2048, hop = 512, len = a.getNumSamples();
        std::vector<float> midi;
        for (int s = 0; s + win <= len; s += hop)
        {
            const float* x = a.getReadPointer (0, s);
            float e = 0; for (int k = 0; k < win; ++k) e += x[k] * x[k];
            if (e < 1.0e-3f) { midi.push_back (-1); continue; }
            const int lo = (int) (rate / 1000.0), hi = std::min (win / 2, (int) (rate / 70.0));
            int best = -1; float bv = 0;
            for (int lag = lo; lag < hi; ++lag)
            {
                float c = 0; for (int k = 0; k < win - lag; k += 2) c += x[k] * x[k + lag];
                if (c > bv) { bv = c; best = lag; }
            }
            midi.push_back (best > 0 && bv > 0.3f * e * 0.5f ? (float) (69.0 + 12.0 * std::log2 (rate / best / 440.0)) : -1.0f);
        }
        std::vector<int> picks { 0 };
        float cur = -1; int since = 0;
        const int minGap = std::max (1, (int) (rate * 0.08 / hop));
        for (int f = 0; f < (int) midi.size() && (int) picks.size() < maxCount; ++f, ++since)
        {
            const float m = midi[(size_t) f];
            const bool change = (m >= 0 && cur < 0) || (m >= 0 && cur >= 0 && std::abs (m - cur) > 0.8f);
            if (change && since >= minGap && f > 0) { picks.push_back (f * hop); since = 0; }
            if (m >= 0) cur = m; else if (since > minGap) cur = -1;
        }
        if (picks.size() < 3) return transients (a, rate, maxCount);   // not melodic: fall back to the hits
        return picks;
    }

private:
    void publish (std::shared_ptr<ChopData> d)
    {
        if (d->fx.size() != d->marks.size()) d->fx.resize (d->marks.size());
        const juce::SpinLock::ScopedLockType l (lock);
        data = std::move (d);
        for (auto& v : voices) v.active = false;
    }
    struct Voice { bool active = false, rel = false, fast = false, vrev = false; int slice = 0, note = 60, delay = 0, vsemi = 0; double pos = 0; float vel = 1, relGain = 1; };
    struct Region { bool on = false, loop = false; int s0 = 0, s1 = 0; double pos = 0; } reg;
    std::atomic<int> regStart { 0 }, regEnd { 0 };
    std::atomic<bool> regLoop { false }, regReq { false }, stopReq { false }, regActive { false };
    mutable juce::SpinLock lock;
    ChopPtr data;
    std::array<Voice, 12> voices {};
    unsigned rr = 0;
    double outRate = 44100.0;
    std::atomic<float> playPos { 0 };
};
} // namespace kk
