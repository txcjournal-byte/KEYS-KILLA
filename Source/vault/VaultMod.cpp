#include "VaultMod.h"

namespace vault
{
void Arp::reset()
{
    numHeld = 0; numSounding = 0; running = false; seqLen = 0; seqPos = 0; seqDirty = true;
    std::fill (std::begin (physDown), std::end (physDown), false);
    nextStepBeat = gridOrigin = 0; lastBeatEnd = -1; stepCounter = 0; lastStep = -1; orderCounter = 0;
    rng.seed (4711);
}

void Arp::keyDown (int note, float velocity)
{
    note = juce::jlimit (0, 127, note);
    bool anyDown = false;
    for (bool d : physDown) anyDown |= d;
    if (set.latch && ! anyDown) numHeld = 0;                 // latch: a new chord replaces the latched one
    physDown[note] = true;
    for (int i = 0; i < numHeld; ++i) if (held[i].note == note) { held[i].vel = velocity; seqDirty = true; return; }
    if (numHeld < 128) held[numHeld++] = { note, juce::jlimit (0.01f, 1.0f, velocity), ++orderCounter };
    seqDirty = true;
}

void Arp::keyUp (int note)
{
    note = juce::jlimit (0, 127, note);
    physDown[note] = false;
    if (set.latch) return;
    for (int i = 0; i < numHeld; ++i)
        if (held[i].note == note) { held[i] = held[--numHeld]; seqDirty = true; break; }
}

void Arp::buildSequence()
{
    seqDirty = false;
    // held notes sorted by pitch, or in the order they were pressed
    Held sorted[128];
    std::copy (held, held + numHeld, sorted);
    if (set.mode == VaultArp::asPlayed) std::sort (sorted, sorted + numHeld, [] (const Held& a, const Held& b) { return a.order < b.order; });
    else std::sort (sorted, sorted + numHeld, [] (const Held& a, const Held& b) { return a.note < b.note; });
    const int oct = juce::jlimit (1, 4, (int) set.octaves);
    int up[128 * 4]; int nUp = 0;
    for (int o = 0; o < oct; ++o)
        for (int i = 0; i < numHeld; ++i)
            if (sorted[i].note + 12 * o <= 127) up[nUp++] = sorted[i].note + 12 * o;
    seqLen = 0;
    switch (set.mode)
    {
        case VaultArp::down: for (int i = nUp - 1; i >= 0; --i) seq[seqLen++] = up[i]; break;
        case VaultArp::upDown:
            for (int i = 0; i < nUp; ++i) seq[seqLen++] = up[i];
            for (int i = nUp - 2; i >= 1; --i) seq[seqLen++] = up[i];
            break;
        default: for (int i = 0; i < nUp; ++i) seq[seqLen++] = up[i]; break;
    }
    if (seqLen > 0) seqPos %= seqLen; else seqPos = 0;
}

int Arp::pushEvent (Event* out, int count, int sample, int note, float vel, bool on)
{
    if (count >= maxEvents) return count;
    out[count] = { sample, juce::jlimit (0, 127, note), vel, on };
    return count + 1;
}

int Arp::allNotesOff (Event* out, int sample)
{
    int c = 0;
    for (int i = 0; i < numSounding; ++i) c = pushEvent (out, c, sample, sounding[i].note, 0.0f, false);
    numSounding = 0;
    return c;
}

int Arp::process (int n, double beat0, double bps, bool hostPlaying, Event* out)
{
    int count = 0;
    if (n <= 0) return 0;
    const double stepBeats = divisionBeats (set.rate);
    const double beatEnd = beat0 + bps * n;
    auto sampleAt = [&] (double beat) { return juce::jlimit (0, n - 1, (int) std::floor ((beat - beat0) / bps + 1.0e-6)); };
    // a jump of the beat clock (host relocated / started / stopped): end the notes, re-align the steps
    if (lastBeatEnd >= 0 && std::abs (beat0 - lastBeatEnd) > bps * 4 + 1.0e-9)
    {
        count = allNotesOff (out, 0);
        if (running) nextStepBeat = hostPlaying ? std::ceil (beat0 / stepBeats - 1.0e-6) * stepBeats : beat0;
    }
    lastBeatEnd = beatEnd;
    if (! running && numHeld > 0)
    {
        running = true;
        stepCounter = 0;
        // host playing: start on its grid (the next step line); free: at once
        nextStepBeat = hostPlaying ? std::ceil (beat0 / stepBeats - 1.0e-6) * stepBeats : beat0;
        gridOrigin = nextStepBeat;
        seqPos = 0;
        if (set.mode == VaultArp::random) rng.seed (4711);
    }
    const int steps = juce::jlimit (1, (int) numSteps, (int) set.steps);
    for (;;)
    {
        // the next thing that happens: a note end or a step
        int offIdx = -1; double offBeat = beatEnd;
        for (int i = 0; i < numSounding; ++i) if (sounding[i].offBeat < offBeat) { offBeat = sounding[i].offBeat; offIdx = i; }
        double stepTime = beatEnd + 1.0;
        int64_t idx = 0;
        if (running)
        {
            idx = hostPlaying ? (int64_t) std::llround (nextStepBeat / stepBeats) : stepCounter;
            stepTime = nextStepBeat + ((idx & 1) ? set.swing * 0.5 * stepBeats : 0.0);
        }
        if (offIdx >= 0 && offBeat <= stepTime)
        {
            count = pushEvent (out, count, sampleAt (offBeat), sounding[offIdx].note, 0.0f, false);
            sounding[offIdx] = sounding[--numSounding];
            continue;
        }
        if (! running || stepTime >= beatEnd) break;
        // a step
        if (numHeld == 0) { running = false; break; }
        const int lane = (int) (((idx % steps) + steps) % steps);
        lastStep = lane;
        const uint8_t type = set.type[(size_t) lane];
        if (type == VaultArp::play)
        {
            if (seqDirty) buildSequence();
            int ties = 0;
            for (int k = 1; k < steps && set.type[(size_t) ((lane + k) % steps)] == VaultArp::tie; ++k) ++ties;
            const double off = stepTime + stepBeats * (ties + juce::jlimit (0.01f, 1.0f, set.gate)) - 1.0e-9;
            const int shift = 12 * juce::jlimit (-2, 2, (int) set.oct[(size_t) lane]);
            const float laneVel = set.vel[(size_t) lane];
            auto play = [&] (int note, float vel)
            {
                note += shift;
                if (note < 0 || note > 127 || laneVel <= 0.0f) return;
                for (int i = 0; i < numSounding; ++i)   // the same note still sounding: end it first
                    if (sounding[i].note == note) { count = pushEvent (out, count, sampleAt (stepTime), note, 0.0f, false); sounding[i] = sounding[--numSounding]; break; }
                count = pushEvent (out, count, sampleAt (stepTime), note, juce::jlimit (0.01f, 1.0f, vel * laneVel), true);
                if (numSounding < 64) sounding[numSounding++] = { note, off };
            };
            auto velOf = [&] (int note)
            {
                for (int i = 0; i < numHeld; ++i) if (held[i].note == note) return held[i].vel;
                for (int i = 0; i < numHeld; ++i) if (note > held[i].note && (note - held[i].note) % 12 == 0) return held[i].vel;
                return numHeld > 0 ? held[0].vel : 0.8f;
            };
            if (set.mode == VaultArp::chord)
            {
                for (int i = 0; i < numHeld; ++i) play (held[i].note, held[i].vel);
            }
            else if (seqLen > 0)
            {
                const int pick = set.mode == VaultArp::random ? (int) (rng.uni() * (float) seqLen) % seqLen : seqPos % seqLen;
                seqPos = (seqPos + 1) % seqLen;
                play (seq[pick], velOf (seq[pick]));
            }
        }
        nextStepBeat += stepBeats;
        ++stepCounter;
    }
    if (! running && numSounding == 0) lastStep = -1;
    return count;
}

//==============================================================================
void Gate::process (float* L, float* R, int n, const VaultGate& g, double beat0, double bps, float depth, float mixFrom, float mixTo, float smooth)
{
    const double stepBeats = divisionBeats (g.rate);
    const int steps = juce::jlimit (1, (int) numSteps, (int) g.steps);
    depth = juce::jlimit (0.0f, 1.0f, depth); smooth = juce::jlimit (0.0f, 1.0f, smooth);
    mixFrom = juce::jlimit (0.0f, 1.0f, mixFrom); mixTo = juce::jlimit (0.0f, 1.0f, mixTo);
    const float mixStep = (mixTo - mixFrom) / (float) std::max (1, n);
    const double stepSec = stepBeats / std::max (1.0e-9, bps) / sr;
    const float tau = (float) std::max (0.0005, smooth * stepSec * 0.35);
    const float a = 1.0f - std::exp (-1.0f / (tau * (float) sr));
    const double split = stepBeats * (1.0 + g.swing * 0.5);
    for (int i = 0; i < n; ++i)
    {
        const double beat = beat0 + bps * i;
        const double pair = std::floor (beat / (2.0 * stepBeats));
        const double pos = beat - pair * 2.0 * stepBeats;
        const int64_t idx = (int64_t) pair * 2 + (pos < split ? 0 : 1);
        const int lane = (int) (((idx % steps) + steps) % steps);
        const float target = 1.0f - depth * (1.0f - g.level[(size_t) lane]);
        level += (target - level) * a;
        const float mix = mixFrom + mixStep * (float) (i + 1);
        const float gain = 1.0f - mix + mix * level;
        L[i] *= gain; R[i] *= gain;
    }
}

//==============================================================================
juce::MidiFile arpPatternToMidi (const VaultArp& settings, const std::vector<std::pair<int, float>>& notes, double bars, double bpm)
{
    constexpr double sr = 48000.0;
    constexpr int ppq = 960, block = 256;
    bpm = juce::jlimit (20.0, 999.0, bpm);
    const double bps = bpm / 60.0 / sr;
    const double totalBeats = juce::jlimit (0.25, 256.0, bars * 4.0);
    auto arp = std::make_unique<Arp>();
    auto s = settings; s.on = true; s.latch = false;
    arp->setSettings (s);
    arp->reset();
    for (auto& [k, v] : notes) arp->keyDown (k, v);
    juce::MidiMessageSequence seq;
    seq.addEvent (juce::MidiMessage::tempoMetaEvent ((int) std::round (60000000.0 / bpm)), 0);
    seq.addEvent (juce::MidiMessage::timeSignatureMetaEvent (4, 4), 0);
    std::vector<Arp::Event> ev (Arp::maxEvents);
    const int total = (int) std::floor (totalBeats / bps + 1.0e-9);   // the pattern's own length: a step on the end line belongs to the next loop
    for (int s0 = 0; s0 < total; s0 += block)
    {
        const int len = std::min (block, total - s0);
        const int c = arp->process (len, s0 * bps, bps, false, ev.data());
        for (int i = 0; i < c; ++i)
        {
            const double tick = std::round ((s0 + ev[(size_t) i].sample) * bps * ppq);   // whole ticks (the step grid)
            const auto vel = (juce::uint8) juce::jlimit (1, 127, (int) std::round (ev[(size_t) i].velocity * 127.0f));
            seq.addEvent (ev[(size_t) i].on ? juce::MidiMessage::noteOn (1, ev[(size_t) i].note, vel) : juce::MidiMessage::noteOff (1, ev[(size_t) i].note), tick);
        }
    }
    const int c = arp->allNotesOff (ev.data());
    for (int i = 0; i < c; ++i) seq.addEvent (juce::MidiMessage::noteOff (1, ev[(size_t) i].note), totalBeats * ppq);
    seq.updateMatchedPairs();
    juce::MidiFile mf;
    mf.setTicksPerQuarterNote (ppq);
    mf.addTrack (seq);
    return mf;
}
} // namespace vault
