#include "VaultMod.h"

namespace vault
{
namespace
{
// one pass of a pitch order over m notes sorted low -> high: indices into them
int orderIndices (int mode, int m, int* out)
{
    int c = 0;
    if (m <= 0) return 0;
    switch (mode)
    {
        case VaultArp::down:      for (int i = m - 1; i >= 0; --i) out[c++] = i; break;
        case VaultArp::upDown:    for (int i = 0; i < m; ++i) out[c++] = i; for (int i = m - 2; i >= 1; --i) out[c++] = i; break;
        case VaultArp::downUp:    for (int i = m - 1; i >= 0; --i) out[c++] = i; for (int i = 1; i <= m - 2; ++i) out[c++] = i; break;
        case VaultArp::upAndDown: for (int i = 0; i < m; ++i) out[c++] = i; for (int i = m - 1; i >= 0; --i) out[c++] = i; break;
        case VaultArp::converge: case VaultArp::diverge:
        {
            for (int lo = 0, hi = m - 1; lo <= hi; ++lo, --hi) { out[c++] = lo; if (lo != hi) out[c++] = hi; }
            if (mode == VaultArp::diverge) std::reverse (out, out + c);
            break;
        }
        case VaultArp::thumb:     if (m == 1) out[c++] = 0; else for (int i = 1; i < m; ++i) { out[c++] = 0; out[c++] = i; } break;
        case VaultArp::pinky:     if (m == 1) out[c++] = 0; else for (int i = 0; i < m - 1; ++i) { out[c++] = i; out[c++] = m - 1; } break;
        default:                  for (int i = 0; i < m; ++i) out[c++] = i; break;   // up, as played, random, chord
    }
    return c;
}

// the octave of chord-mode pass `pass` (and of the Up-Down octave passes)
int passOctave (const VaultArp& s, int pass)
{
    const int n = juce::jlimit (1, 4, (int) s.octaves);
    if (n <= 1) return 0;
    switch (s.octMode)
    {
        case VaultArp::octDown:   return -(pass % n);
        case VaultArp::octUpDown: { const int p = pass % (2 * n - 2); return p < n ? p : 2 * n - 2 - p; }
        default:                  return pass % n;
    }
}

// the whole sequence (one cycle) for held notes `notes` / `vels` (played order): pitch + velocity per entry
int buildOrder (const VaultArp& s, const int* notes, const float* vels, int n, int16_t* outN, float* outV, int maxOut,
                int* sorted, float* sortedV, int* idx)
{
    if (n <= 0) return 0;
    const int octs = juce::jlimit (1, 4, (int) s.octaves);
    const bool pitchOrder = s.mode != VaultArp::asPlayed;
    int c = 0;
    auto add = [&] (int note, float v) { if (note >= 0 && note <= 127 && c < maxOut) { outN[c] = (int16_t) note; outV[c] = v; ++c; } };
    // the held notes: by pitch (or as played)
    int m = 0;
    for (int i = 0; i < n; ++i) { sorted[m] = notes[i]; sortedV[m] = vels[i]; ++m; }
    auto sortByPitch = [&] (int count)
    {
        for (int i = 1; i < count; ++i)   // insertion sort (stable, no allocation)
        {
            const int kn = sorted[i]; const float kv = sortedV[i]; int j = i - 1;
            while (j >= 0 && sorted[j] > kn) { sorted[j + 1] = sorted[j]; sortedV[j + 1] = sortedV[j]; --j; }
            sorted[j + 1] = kn; sortedV[j + 1] = kv;
        }
    };
    if ((s.octMode == VaultArp::octUp || s.octMode == VaultArp::octDown) && pitchOrder)
    {
        // the order runs over the held notes copied over the octave range (one sorted set, no repeated pitch)
        const int lo = s.octMode == VaultArp::octDown ? -(octs - 1) : 0;
        int e = 0;
        for (int o = 0; o < octs; ++o)
            for (int i = 0; i < m; ++i)
            {
                const int note = notes[i] + 12 * (lo + o);
                if (note < 0 || note > 127) continue;
                bool dup = false;
                for (int j = 0; j < e && ! dup; ++j) dup = sorted[m + j] == note;
                if (! dup) { sorted[m + e] = note; sortedV[m + e] = vels[i]; ++e; }
            }
        // move the expanded set to the front and sort it
        for (int i = 0; i < e; ++i) { sorted[i] = sorted[m + i]; sortedV[i] = sortedV[m + i]; }
        sortByPitch (e);
        const int c2 = orderIndices (s.mode, e, idx);
        for (int i = 0; i < c2; ++i) add (sorted[idx[i]], sortedV[idx[i]]);
        return c;
    }
    if (pitchOrder) sortByPitch (m);
    const int nb = orderIndices (s.mode, m, idx);   // one pass in the base octave
    if (s.octMode == VaultArp::octAlternate)
    {
        for (int i = 0; i < nb; ++i) for (int o = 0; o < octs; ++o) add (sorted[idx[i]] + 12 * o, sortedV[idx[i]]);
        return c;
    }
    // Up-Down (and As Played with Up / Down): one pass per octave
    const int passes = s.octMode == VaultArp::octUpDown ? std::max (1, 2 * octs - 2) : octs;
    for (int p = 0; p < passes; ++p)
    {
        const int o = s.octMode == VaultArp::octUpDown ? passOctave (s, p) : s.octMode == VaultArp::octDown ? -p : p;
        for (int i = 0; i < nb; ++i) add (sorted[idx[i]] + 12 * o, sortedV[idx[i]]);
    }
    return c;
}
} // namespace

int Arp::orderOf (const VaultArp& s, const int* notes, int n, int* out, int maxOut)
{
    n = juce::jlimit (0, 128, n);
    std::vector<float> v ((size_t) std::max (1, n), 1.0f);
    std::vector<int16_t> on ((size_t) maxSeq);
    std::vector<float> ov ((size_t) maxSeq);
    std::vector<int> sorted (128 * 8), idx ((size_t) maxSeq);
    std::vector<float> sv (128 * 8);
    const int c = buildOrder (s, notes, v.data(), n, on.data(), ov.data(), maxSeq, sorted.data(), sv.data(), idx.data());
    const int r = std::min (c, maxOut);
    for (int i = 0; i < r; ++i) out[i] = on[(size_t) i];
    return r;
}

void Arp::reset()
{
    numHeld = 0; numPhys = 0; numSounding = 0; running = false; seqLen = 0; seqPos = 0; chordPass = 0; lastNote = -1;
    seqDirty = true; restartPending = false; newChordPending = false; pedal = false; anyJoin = false;
    std::fill (std::begin (physDown), std::end (physDown), false);
    std::fill (std::begin (joinPending), std::end (joinPending), false);
    std::fill (std::begin (offAt), std::end (offAt), 0.0);
    std::fill (std::begin (sounding), std::end (sounding), false);
    origin = 0; gridLen = divisionBeats (set.rate); lastBeatEnd = 0; now = 0; haveClock = false; lastPlaying = false;
    freeBarOrigin = 0; lastBarDone = -1.0e18; k = 0; laneBase = 0; lastStep = -1; lastStepBeat = -1.0e18; lastStepOff = 0;
    lastStepChord = false; lastStepShift = 0; lastStepLane = 0; orderCounter = 0; ++heldVer;
    rng.seed (4711);
}

void Arp::setSettings (const VaultArp& s)
{
    if (set.latch && ! s.latch && ! pedal)   // latch off: let go of the keys that are no longer down
        for (int i = numHeld - 1; i >= 0; --i) if (! physDown[held[i].note]) removeHeld (held[i].note);
    if (s.mode != set.mode || s.octaves != set.octaves || s.octMode != set.octMode) seqDirty = true;
    set = s;
}

void Arp::removeHeld (int note)
{
    for (int i = 0; i < numHeld; ++i)
        if (held[i].note == note)
        {
            for (int j = i + 1; j < numHeld; ++j) held[j - 1] = held[j];   // keeps the played order
            --numHeld; seqDirty = true; ++heldVer;
            return;
        }
}

void Arp::keyDown (int note, float velocity)
{
    note = juce::jlimit (0, 127, note);
    velocity = juce::jlimit (0.01f, 1.0f, velocity);
    const bool newChord = numPhys == 0;
    if (newChord)
    {
        if (set.latch && set.latchMode == VaultArp::latchReplace && ! pedal) { numHeld = 0; seqDirty = true; ++heldVer; }   // a new chord replaces the latched one
        if (set.retrig == VaultArp::retrigChord) restartPending = true;
        if (set.retrig != VaultArp::retrigOff) newChordPending = true;
    }
    if (set.retrig == VaultArp::retrigNote) restartPending = true;
    if (! physDown[note]) { physDown[note] = true; ++numPhys; }
    bool known = false;
    for (int i = 0; i < numHeld; ++i) if (held[i].note == note) { held[i].vel = velocity; known = true; }
    if (! known && numHeld < 128) held[numHeld++] = { note, velocity, ++orderCounter };
    seqDirty = true; ++heldVer;
    // Chord: a key that comes a moment after the chord step (a chord played slightly late) joins that step at once
    const double window = std::min (0.45 * divisionBeats (set.rate), 0.0625);
    if (! known && ! newChord && running && set.mode == VaultArp::chord && lastStepChord && now - lastStepBeat <= window && now < lastStepOff)
    {
        joinPending[note] = true; anyJoin = true;
    }
}

void Arp::keyUp (int note)
{
    note = juce::jlimit (0, 127, note);
    if (physDown[note]) { physDown[note] = false; --numPhys; }
    joinPending[note] = false;
    if (set.latch || pedal) return;
    removeHeld (note);
}

void Arp::setPedal (bool down)
{
    if (down == pedal) return;
    pedal = down;
    if (! down && ! set.latch)
        for (int i = numHeld - 1; i >= 0; --i) if (! physDown[held[i].note]) removeHeld (held[i].note);
}

int Arp::heldNotes (int* notes, float* velocities, int max) const
{
    const int c = std::min (numHeld, max);
    for (int i = 0; i < c; ++i) { notes[i] = held[i].note; if (velocities != nullptr) velocities[i] = held[i].vel; }
    return c;
}

void Arp::buildSequence()
{
    seqDirty = false;
    int notes[128]; float vels[128];
    for (int i = 0; i < numHeld; ++i) { notes[i] = held[i].note; vels[i] = held[i].vel; }
    seqLen = buildOrder (set, notes, vels, numHeld, seq, seqVel, maxSeq, sortedNotes, sortedVel, base);
    if (seqLen <= 0) { seqPos = 0; return; }
    // a changed chord carries on from where the pattern was: after the note played last (Up: the next one above it)
    if (lastNote >= 0)
    {
        int at = -1;
        for (int i = 0; i < seqLen && at < 0; ++i) if (seq[i] == lastNote) at = i;
        if (at >= 0) { seqPos = (at + 1) % seqLen; return; }
        if (set.mode == VaultArp::up || set.mode == VaultArp::down)
        {
            for (int i = 0; i < seqLen; ++i)
                if (set.mode == VaultArp::up ? seq[i] > lastNote : seq[i] < lastNote) { seqPos = i; return; }
            seqPos = 0; return;
        }
    }
    seqPos %= seqLen;
}

int Arp::pushEvent (Event* out, int count, int sample, int note, float vel, bool on)
{
    if (count >= eventCap) return count;
    out[count] = { sample, juce::jlimit (0, 127, note), vel, on };
    return count + 1;
}

int Arp::endSounding (Event* out, int count, int sample)
{
    if (numSounding > 0)
        for (int i = 0; i < 128; ++i)
            if (sounding[i]) { count = pushEvent (out, count, sample, i, 0.0f, false); sounding[i] = false; }
    numSounding = 0;
    return count;
}

int Arp::allNotesOff (Event* out, int sample) { eventCap = maxEvents; return endSounding (out, 0, sample); }

void Arp::realign (double beat, bool host, bool catchLate)
{
    gridLen = divisionBeats (set.rate);
    const double window = catchLate ? std::min (0.45 * gridLen, 0.0625) : 1.0e-9;
    laneBase = 0;
    if (host)
    {
        // the song's grid: a step line that passed a moment ago (a key or a loop start a few ms late) still plays now
        origin = 0;
        k = StepGrid::indexAt (beat, origin, gridLen, set.swing);
        if (beat - StepGrid::start (k, origin, gridLen, set.swing) > window) ++k;
        lastBarDone = beat - 1.0e-7;
    }
    else
    {
        origin = beat; k = 0; freeBarOrigin = beat; lastBarDone = beat;
    }
}

int Arp::process (int n, const Clock& c, Event* out)
{
    if (n <= 0 || ! (c.bps > 0.0)) return 0;
    // a host loop that wraps inside this block (hosts that do not split the block at the loop end): what comes after the
    // loop end plays from the loop start, on the sample
    if (c.hostPlaying && c.loopEnd > c.loopStart + 1.0e-6)
    {
        const double end = c.beat0 + c.bps * n;
        if (c.beat0 < c.loopEnd - 1.0e-9 && end > c.loopEnd + 1.0e-9)
        {
            const int n1 = (int) std::ceil ((c.loopEnd - c.beat0) / c.bps - 1.0e-9);
            if (n1 > 0 && n1 < n)
            {
                Clock a = c; a.loopStart = a.loopEnd = 0.0;
                const int c1 = processSpan (n1, a, out, maxEvents);
                Clock b = a; b.beat0 = c.loopStart + (c.beat0 + c.bps * n1 - c.loopEnd);
                const int c2 = processSpan (n - n1, b, out + c1, maxEvents - c1);
                for (int i = c1; i < c1 + c2; ++i) out[i].sample += n1;
                return c1 + c2;
            }
        }
    }
    return processSpan (n, c, out, maxEvents);
}

int Arp::processSpan (int n, const Clock& c, Event* out, int cap)
{
    int count = 0;
    eventCap = std::max (0, cap);
    const double bps = c.bps, beat0 = c.beat0, beatEnd = beat0 + bps * n;
    const bool host = c.hostPlaying;
    const double stepBeats = divisionBeats (set.rate);
    const double window = std::min (0.45 * stepBeats, 0.0625);
    auto sampleAt = [&] (double beat) { return juce::jlimit (0, n - 1, (int) std::floor ((beat - beat0) / bps + 1.0e-6)); };

    // a jump of the beat clock (loop, relocation, transport start / stop): the sounding notes keep their lengths; on the
    // host's clock the steps re-align to its grid and the pattern starts again (a loop plays the same every time) - a
    // step line just passed plays at once unless a step played a moment ago (no double); the free clock carries on in
    // the same rhythm
    if (haveClock && (host != lastPlaying || std::abs (beat0 - lastBeatEnd) > std::max (bps * 8.0, 1.0e-9)))
    {
        const double delta = beat0 - lastBeatEnd, since = lastBeatEnd - lastStepBeat;
        for (int i = 0; i < 128; ++i) if (sounding[i]) offAt[i] += delta;
        lastStepBeat += delta; lastStepOff += delta;
        if (running)
        {
            if (host) { realign (beat0, true, since > window); seqPos = 0; chordPass = 0; lastNote = -1; restartPending = false; }
            else { origin += delta; freeBarOrigin += delta; lastBarDone += delta; }
        }
        newChordPending = false;
    }
    haveClock = true; lastPlaying = host;
    lastBeatEnd = beatEnd;

    // the rate changed while running: the step that is due still comes, then the new length
    if (running && std::abs (stepBeats - gridLen) > 1.0e-12)
    {
        if (host) realign (beat0, true, false);
        else
        {
            const double due = std::max (beat0, StepGrid::start (k, origin, gridLen, set.swing));
            const int64_t lane = k - laneBase;
            gridLen = stepBeats; origin = due; k = 0; laneBase = -lane;
        }
    }
    if (! running && numHeld > 0)
    {
        // the first keys: host playing - on its grid (bar-aligned lanes); stopped - at once, from the key
        running = true; seqPos = 0; chordPass = 0; lastNote = -1; restartPending = false; newChordPending = false;
        rng.seed (4711);
        realign (beat0, host, true);
    }
    else if (running && newChordPending && ! host)
    {
        // free clock: a new chord starts the pattern from the key (key sync)
        realign (beat0, false, false);
        restartPending = set.retrig == VaultArp::retrigChord || set.retrig == VaultArp::retrigNote || restartPending;
    }
    newChordPending = false;
    const float swing = set.swing;
    const int steps = juce::jlimit (1, (int) numSteps, (int) set.steps);
    auto velFor = [&] (float played, float laneVel)
    {
        switch (set.velMode)
        {
            case VaultArp::velPattern: return laneVel;
            case VaultArp::velPlayed:  return played;
            default:                   return played * laneVel;
        }
    };
    auto startNote = [&] (int sample, int note, float vel, double off)
    {
        if (note < 0 || note > 127 || vel <= 0.0f) return false;
        if (sounding[note]) { count = pushEvent (out, count, sample, note, 0.0f, false); sounding[note] = false; --numSounding; }   // retrigger
        count = pushEvent (out, count, sample, note, juce::jlimit (0.01f, 1.0f, vel), true);
        sounding[note] = true; offAt[note] = off; ++numSounding;
        return true;
    };

    // Chord: keys that joined just after the step play now, until the step's notes end
    if (anyJoin)
    {
        if (running && set.mode == VaultArp::chord && lastStepChord && beat0 < lastStepOff)
            for (int i = 0; i < numHeld; ++i)
                if (joinPending[held[i].note])
                    startNote (0, held[i].note + lastStepShift, velFor (held[i].vel, set.vel[(size_t) lastStepLane]), lastStepOff);
        std::fill (std::begin (joinPending), std::end (joinPending), false);
        anyJoin = false;
    }

    for (;;)
    {
        // the next thing that happens: a note end, a bar line (Every bar), a step
        int offNote = -1; double offBeat = 1.0e300;
        if (numSounding > 0)
            for (int i = 0; i < 128; ++i) if (sounding[i] && offAt[i] < offBeat) { offBeat = offAt[i]; offNote = i; }
        const double fire = running ? std::max (beat0, StepGrid::start (k, origin, gridLen, swing)) : 1.0e300;
        double bar = 1.0e300;
        if (running && set.retrig == VaultArp::retrigBar)
        {
            const double bo = host ? c.barStart : freeBarOrigin, bl = host ? std::max (0.25, c.barBeats) : 4.0;
            const double from = std::max (beat0, lastBarDone + 1.0e-7);
            bar = bo + std::ceil ((from - bo) / bl - 1.0e-9) * bl;
        }
        if (offNote >= 0 && offBeat < beatEnd && offBeat <= fire && offBeat <= bar)
        {
            count = pushEvent (out, count, sampleAt (offBeat), offNote, 0.0f, false);
            sounding[offNote] = false; --numSounding;
            continue;
        }
        if (bar < beatEnd && bar <= fire)
        {
            // Every bar: the order and the lanes start again on the bar line
            lastBarDone = bar;
            restartPending = true;
            const int64_t kb = StepGrid::indexAt (bar, origin, gridLen, swing);
            if (std::abs (StepGrid::start (kb, origin, gridLen, swing) - bar) < 1.0e-6) { laneBase = kb; k = std::max (k, kb); }
            else { origin = bar; k = 0; laneBase = 0; }   // the rate does not divide the bar: the grid starts on it
            continue;
        }
        if (! running || fire >= beatEnd) break;

        // a step
        const int at = sampleAt (fire);
        if (numHeld == 0)
        {
            // every key is up: the pattern stops on this step line and nothing rings past it
            running = false;
            count = endSounding (out, count, at);
            break;
        }
        const int lane = (int) ((((k - laneBase) % steps) + steps) % steps);
        lastStep = lane;
        if (restartPending) { seqPos = 0; chordPass = 0; lastNote = -1; restartPending = false; }
        if (set.type[(size_t) lane] == VaultArp::play)
        {
            if (seqDirty) buildSequence();
            int ties = 0;
            for (int j = 1; j < steps && set.type[(size_t) ((lane + j) % steps)] == VaultArp::tie; ++j) ++ties;
            const double s0 = StepGrid::start (k + ties, origin, gridLen, swing), s1 = StepGrid::start (k + ties + 1, origin, gridLen, swing);
            const double g = juce::jlimit (0.01, 1.0, (double) set.gate * (double) set.len[(size_t) lane]);
            double off = set.slide[(size_t) lane] != 0 ? s1 + std::min (0.25 * gridLen, 0.0625)   // slide: into the next note
                                                    : s0 + (s1 - s0) * g - 1.0e-9;                 // (a same-pitch next note retriggers)
            off = std::max (off, fire + bps * 2.0);
            const int shift = 12 * juce::jlimit (-2, 2, (int) set.oct[(size_t) lane]);
            const float laneVel = set.vel[(size_t) lane];
            lastStepChord = false;
            if (set.mode == VaultArp::chord)
            {
                const int sh = shift + 12 * passOctave (set, chordPass++);
                for (int i = 0; i < numHeld; ++i) startNote (at, held[i].note + sh, velFor (held[i].vel, laneVel), off);
                lastStepChord = true; lastStepShift = sh; lastStepLane = lane;
            }
            else if (seqLen > 0)
            {
                int pick = seqPos % seqLen;
                if (set.mode == VaultArp::random)
                {
                    pick = (int) (rng.uni() * (float) seqLen) % seqLen;
                    if (seqLen > 1 && seq[pick] == lastNote) pick = (pick + 1 + (int) (rng.uni() * (float) (seqLen - 1)) % (seqLen - 1)) % seqLen;
                }
                seqPos = (seqPos + 1) % seqLen;
                startNote (at, seq[pick] + shift, velFor (seqVel[pick], laneVel), off);
                lastNote = seq[pick];
            }
            lastStepBeat = fire; lastStepOff = off;
        }
        ++k;
    }
    now = beatEnd;
    if (! running && numSounding == 0) lastStep = -1;
    return count;
}

//==============================================================================
void Gate::process (float* L, float* R, int n, const VaultGate& g, double beat0, double bps, float depth, float mixFrom, float mixTo, float smooth,
                    double loopStart, double loopEnd)
{
    const bool loop = loopEnd > loopStart + 1.0e-6 && beat0 < loopEnd;   // a host loop that wraps inside the block
    const double stepBeats = divisionBeats (g.rate);
    const int steps = juce::jlimit (1, (int) numSteps, (int) g.steps);
    depth = juce::jlimit (0.0f, 1.0f, depth); smooth = juce::jlimit (0.0f, 1.0f, smooth);
    mixFrom = juce::jlimit (0.0f, 1.0f, mixFrom); mixTo = juce::jlimit (0.0f, 1.0f, mixTo);
    const float mixStep = (mixTo - mixFrom) / (float) std::max (1, n);
    const double stepSec = stepBeats / std::max (1.0e-9, bps) / sr;
    const float tau = (float) std::max (0.0005, smooth * stepSec * 0.35);
    const float a = 1.0f - std::exp (-1.0f / (tau * (float) sr));
    for (int i = 0; i < n; ++i)
    {
        double beat = beat0 + bps * i;
        if (loop && beat >= loopEnd) beat -= loopEnd - loopStart;
        const int64_t idx = StepGrid::indexAt (beat, 0.0, stepBeats, g.swing);   // the arpeggiator's step clock
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
    Arp::Clock clock;   // the host's grid from bar 1 (4/4), like the engine with the transport running
    clock.bps = bps; clock.hostPlaying = true; clock.barStart = 0.0; clock.barBeats = 4.0;
    juce::MidiMessageSequence seq;
    seq.addEvent (juce::MidiMessage::tempoMetaEvent ((int) std::round (60000000.0 / bpm)), 0);
    seq.addEvent (juce::MidiMessage::timeSignatureMetaEvent (4, 4), 0);
    std::vector<Arp::Event> ev (Arp::maxEvents);
    const int total = (int) std::floor (totalBeats / bps + 1.0e-9);   // the pattern's own length: a step on the end line belongs to the next loop
    for (int s0 = 0; s0 < total; s0 += block)
    {
        const int len = std::min (block, total - s0);
        clock.beat0 = s0 * bps;
        const int c = arp->process (len, clock, ev.data());
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
