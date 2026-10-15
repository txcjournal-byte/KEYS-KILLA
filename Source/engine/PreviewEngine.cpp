#include "PreviewEngine.h"

#include <algorithm>

namespace ln
{
static constexpr double kTwoPi = 6.283185307179586476925286766559;

void PreviewSynth::prepare (double sampleRate)
{
    sr = sampleRate > 0 ? sampleRate : 48000.0;
    master.reset (sr, 0.010);
    master.setCurrentAndTarget (1.0f);
    wave = pendingWave;
    for (auto& v : voices) v = Voice();
}

void PreviewSynth::reset()
{
    for (auto& v : voices) v = Voice();
    wave = pendingWave;
    master.setCurrentAndTarget (1.0f);
}

void PreviewSynth::noteOn (int pitch, int velocity)
{
    // choose: a free voice, else the quietest releasing one, else the oldest
    Voice* v = nullptr;
    for (auto& x : voices) if (x.stage == Off) { v = &x; break; }
    if (v == nullptr)
        for (auto& x : voices)
            if (x.stage == Release && (v == nullptr || x.level < v->level)) v = &x;
    if (v == nullptr)
        for (auto& x : voices)
            if (v == nullptr || x.age < v->age) v = &x;

    const double freq = 440.0 * std::pow (2.0, (std::clamp (pitch, 0, 127) - 69) / 12.0);
    if (v->stage == Off) { v->phase = 0.0; v->level = 0.0f; }
    // a stolen voice keeps its phase and level: the attack ramps from where it is (no discontinuity)
    v->pitch = pitch;
    v->inc = freq / sr;
    v->stage = Attack;
    v->attackStep = (float) (1.0 / std::max (1.0, attackSec * sr));
    v->decayStep = (float) ((1.0 - sustain) / std::max (1.0, decaySec * sr));
    v->amp = 0.62f + 0.38f * (float) std::clamp (velocity, 1, 127) / 127.0f;
    v->age = ++ageCounter;
}

void PreviewSynth::noteOff (int pitch)
{
    for (auto& v : voices)
        if (v.pitch == pitch && v.stage != Off && v.stage != Release)
        {
            v.stage = Release;
            v.releaseStep = (float) (std::max (v.level, 1.0e-4f) / std::max (1.0, releaseSec * sr));
        }
}

void PreviewSynth::allNotesOff()
{
    for (auto& v : voices)
        if (v.stage != Off && v.stage != Release)
        {
            v.stage = Release;
            v.releaseStep = (float) (std::max (v.level, 1.0e-4f) / std::max (1.0, releaseSec * sr));
        }
}

bool PreviewSynth::isSilent() const
{
    for (const auto& v : voices) if (v.stage != Off) return false;
    return true;
}

double PreviewSynth::polyBlep (double t, double dt)
{
    if (t < dt) { t /= dt; return t + t - t * t - 1.0; }
    if (t > 1.0 - dt) { t = (t - 1.0) / dt; return t * t + t + t + 1.0; }
    return 0.0;
}

float PreviewSynth::renderVoice (Voice& v)
{
    switch (v.stage)
    {
        case Attack:
            v.level += v.attackStep;
            if (v.level >= 1.0f) { v.level = 1.0f; v.stage = Decay; }
            break;
        case Decay:
            v.level -= v.decayStep;
            if (v.level <= sustain) { v.level = sustain; v.stage = Sustain; }
            break;
        case Sustain: break;
        case Release:
            v.level -= v.releaseStep;
            if (v.level <= 0.0f) { v.level = 0.0f; v.stage = Off; v.pitch = -1; return 0.0f; }
            break;
        case Off: default: return 0.0f;
    }

    double s = 0.0, gain = 0.30;
    const double t = v.phase;
    switch (wave)
    {
        case Wave::Sine:     s = std::sin (kTwoPi * t); gain = 0.30; break;
        case Wave::Square:
        {
            s = t < 0.5 ? 1.0 : -1.0;
            s += polyBlep (t, v.inc);
            double t2 = t + 0.5; if (t2 >= 1.0) t2 -= 1.0;
            s -= polyBlep (t2, v.inc);
            gain = 0.15;
            break;
        }
        case Wave::Triangle:
        {
            double tt = t + 0.25; if (tt >= 1.0) tt -= 1.0;   // starts at 0 like the sine
            s = 1.0 - 4.0 * std::abs (tt - 0.5);
            gain = 0.34;
            break;
        }
        case Wave::EnvelopeOnly: s = 1.0; gain = 0.30; break;
    }
    v.phase += v.inc;
    if (v.phase >= 1.0) v.phase -= 1.0;
    return (float) (s * gain) * v.level * v.amp;
}

void PreviewSynth::render (float* out, int numSamples)
{
    for (int i = 0; i < numSamples; ++i)
    {
        // a sound change fades out, switches, fades in (no click when switching mid-note)
        if (pendingWave != wave)
        {
            if (master.getTarget() != 0.0f) master.setTarget (0.0f);
            if (! master.isSmoothing() && master.getCurrent() <= 0.0f) { wave = pendingWave; master.setTarget (1.0f); }
        }
        float sum = 0.0f;
        for (auto& v : voices)
            if (v.stage != Off) sum += renderVoice (v);
        sum *= master.next();
        out[i] = std::clamp (sum, -kPeakLimit, kPeakLimit);
    }
}

// ---------------------------------------------------------------------------------------------------------------
void PreviewSequencer::setPattern (const Note* notes, int n, double lengthBeats)
{
    count = 0;
    length = lengthBeats;
    n = std::min (n, kMaxNotes);
    for (int i = 0; i < n; ++i)
    {
        const double s = std::clamp (notes[i].start, 0.0, lengthBeats);
        const double e = std::clamp (notes[i].start + notes[i].length, s, lengthBeats);
        events[count++] = { s, notes[i].pitch, notes[i].velocity, true };
        events[count++] = { e, notes[i].pitch, 0, false };
    }
    std::sort (events, events + count, [] (const Ev& a, const Ev& b)
    {
        if (a.beat != b.beat) return a.beat < b.beat;
        return ! a.on && b.on;
    });
}

int PreviewSequencer::stop (NoteEvent* out, int maxEvents)
{
    expected = -1.0e9;
    if (activePitch < 0 || maxEvents <= 0) { activePitch = -1; return 0; }
    out[0] = { 0, activePitch, 0, false };
    activePitch = -1;
    return 1;
}

int PreviewSequencer::process (double b0, double bps, int numSamples, NoteEvent* out, int maxEvents)
{
    int n = 0;
    if (numSamples <= 0 || bps <= 0) return 0;

    // a jump (host loop / relocate / first block): end the sounding note, continue from the new position
    if (std::abs (b0 - expected) > bps * 4.0 + 1.0e-7)
    {
        if (activePitch >= 0 && n < maxEvents) { out[n++] = { 0, activePitch, 0, false }; activePitch = -1; }
    }
    else
    {
        b0 = expected; // continuous: start exactly where the last block ended (no event is played twice or skipped)
    }
    const double b1 = b0 + bps * numSamples;
    expected = b1;

    if (! hasPattern()) return n;

    const long long k0 = (long long) std::floor (b0 / length) - 1;
    const long long k1 = (long long) std::floor (b1 / length);
    for (long long k = k0; k <= k1; ++k)
    {
        const double base = (double) k * length;
        for (int i = 0; i < count && n < maxEvents; ++i)
        {
            const double t = base + events[i].beat;
            if (t < b0 || t >= b1) continue;
            const int off = std::clamp ((int) ((t - b0) / bps), 0, numSamples - 1);
            if (events[i].on)
            {
                if (activePitch >= 0) { out[n++] = { off, activePitch, 0, false }; if (n >= maxEvents) break; }
                out[n++] = { off, events[i].pitch, events[i].vel, true };
                activePitch = events[i].pitch;
            }
            else if (activePitch == events[i].pitch)
            {
                out[n++] = { off, activePitch, 0, false };
                activePitch = -1;
            }
        }
    }
    std::stable_sort (out, out + n, [] (const NoteEvent& a, const NoteEvent& b) { return a.sampleOffset < b.sampleOffset; });
    return n;
}

// ---------------------------------------------------------------------------------------------------------------
int PreviewEngine::render (float* out, int numSamples, double beatStart, double bps, bool playing,
                           NoteEvent* evOut, int maxEvents)
{
    int n = 0;
    if (playing)
        n = seq.process (beatStart, bps, numSamples, scratch, 256);
    else if (wasPlaying)
        n = seq.stop (scratch, 256);
    if (! playing && wasPlaying) synth.allNotesOff();
    wasPlaying = playing;

    int pos = 0;
    for (int i = 0; i <= n; ++i)
    {
        const int until = i < n ? std::clamp (scratch[i].sampleOffset, pos, numSamples) : numSamples;
        if (until > pos) { synth.render (out + pos, until - pos); pos = until; }
        if (i < n)
        {
            if (scratch[i].on) synth.noteOn (scratch[i].pitch, scratch[i].velocity);
            else synth.noteOff (scratch[i].pitch);
        }
    }
    if (pos < numSamples) synth.render (out + pos, numSamples - pos);

    const int copied = evOut != nullptr ? std::min (n, maxEvents) : 0;
    for (int i = 0; i < copied; ++i) evOut[i] = scratch[i];
    return copied;
}
} // namespace ln
