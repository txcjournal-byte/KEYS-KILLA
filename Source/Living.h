#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <cmath>
#include "MelodyGen.h"

// v0.43 LIVING GENERATORS - no piano roll: melodies come out of physics and behaviour.
//  GRAVITY  : impulses thrown into a field with gravity, friction and collisions. mass = velocity, bounces = rhythm,
//             friction = the natural speeding up / slowing down (rubato).
//  PREDATOR : a creature hunts its prey. the chase climbs, circling the prey plays around one chord,
//             the pounce is the final accent with a pitch-bend slide.
//  SWARM    : a flock of birds holds the harmony. together = a tight chord in the key, startled = a cascade of
//             notes across the octaves.
//  METABOLISM: a motif ages every time it repeats - the timing breathes, the notes oxidise, and if nobody touches it
//             it starts to fall apart (so the arrangement moves on).
// Everything is deterministic (seeded): the same field always plays the same music, so the animation and the sound match.
namespace kk::live
{
using kk::mel::Note;
using kk::mel::Melody;

struct LNote { float start = 0, len = 0.25f; int pitch = 60; float vel = 0.8f; float bend = 0; };   // bend: semitones it slides in from
struct Frame { float t; std::vector<juce::Point<float>> pos; };                                         // what the field looked like (for drawing)
struct Result { std::vector<LNote> notes; std::vector<Frame> frames; float beats = 16; };

inline Melody toMelody (const Result& r, int key, int scale, const juce::String& name)
{
    Melody m; m.key = key; m.scale = scale; m.bars = std::max (1, (int) std::round (r.beats / 4.0f)); m.name = name;
    for (auto& n : r.notes) m.notes.push_back ({ n.start, n.len, n.pitch, n.vel });
    return m;
}

// a MIDI file with the slides written as pitch-bend (range +-2 semitones, the common default)
inline juce::MidiFile toMidi (const Result& r, double bpm)
{
    juce::MidiMessageSequence seq;
    const double tpq = 960.0;
    seq.addEvent (juce::MidiMessage::tempoMetaEvent ((int) std::round (60000000.0 / std::max (30.0, bpm))), 0);
    for (auto& n : r.notes)
    {
        const double t0 = n.start * tpq, t1 = (n.start + std::max (0.05f, n.len)) * tpq;
        const int vel = juce::jlimit (1, 127, (int) std::round (n.vel * 127.0f));
        if (std::abs (n.bend) > 0.01f)
        {
            for (int k = 0; k <= 8; ++k)
            {
                const float u = (float) k / 8.0f, semis = n.bend * (1.0f - u) * (1.0f - u);
                const int pw = juce::jlimit (0, 16383, 8192 + (int) std::round (semis / 2.0f * 8191.0f));
                seq.addEvent (juce::MidiMessage::pitchWheel (1, pw), t0 + u * 0.18 * tpq - (k == 0 ? 1.0 : 0.0));
            }
        }
        seq.addEvent (juce::MidiMessage::noteOn (1, n.pitch, (juce::uint8) vel), t0);
        seq.addEvent (juce::MidiMessage::noteOff (1, n.pitch), t1 - 1.0);
    }
    seq.updateMatchedPairs(); seq.sort();
    juce::MidiFile f; f.setTicksPerQuarterNote ((int) tpq); f.addTrack (seq);
    return f;
}

inline void tidy (Result& r, int maxPoly)
{
    std::sort (r.notes.begin(), r.notes.end(), [] (auto& a, auto& b) { return a.start < b.start || (a.start == b.start && a.pitch < b.pitch); });
    std::vector<LNote> out;
    for (auto& n : r.notes)
    {
        if (n.start < 0 || n.start >= r.beats) continue;
        int same = 0; bool dup = false;
        for (auto it = out.rbegin(); it != out.rend() && it->start > n.start - 0.03f; ++it) { ++same; if (it->pitch == n.pitch) dup = true; }
        if (dup || same >= maxPoly) continue;
        n.len = std::min (n.len, r.beats - n.start);
        out.push_back (n);
    }
    r.notes = std::move (out);
}

// ------------------------------------------------------------------ GRAVITY ------------------------------------------------------------------
struct Throw { float x = 0.2f, y = 0.2f, vx = 0.3f, vy = 0.0f, mass = 0.6f, at = 0.0f; };   // field units 0..1 (y down), velocity per beat, at = beat
struct Ledge { float x0, y0, x1, y1; };                                                    // lines drawn into the field
struct GravityField
{
    std::vector<Throw> throws;
    std::vector<Ledge> ledges;
    float gravity = 0.5f, friction = 0.3f, bounce = 0.75f;
};

inline Result gravity (const GravityField& f, int key, int scale, int bars, bool keepFrames = true)
{
    Result r; r.beats = (float) bars * 4.0f;
    struct Ball { float x, y, vx, vy, m; bool live; float at; int lastHit = -1; float lastT = -9; };
    std::vector<Ball> balls;
    for (auto& t : f.throws) balls.push_back ({ t.x, t.y, t.vx, t.vy, t.mass, false, t.at });
    const float g = 1.2f + 6.0f * f.gravity;                    // field heights per beat^2
    const float drag = 0.05f + 1.6f * f.friction;               // air friction
    const float e = juce::jlimit (0.2f, 0.97f, f.bounce * (1.0f - 0.35f * f.friction));
    const float dt = 1.0f / 192.0f;
    int frame = 0;
    auto pitchAt = [&] (float x, float y)
    {
        const int deg = (int) std::round (x * 13.0f) - 3 + (int) std::round ((1.0f - y) * 4.0f);
        return kk::mel::degreeToPitch (deg, key, scale, 0.5f);
    };
    for (float t = 0; t < r.beats; t += dt, ++frame)
    {
        for (int bi = 0; bi < (int) balls.size(); ++bi)
        {
            auto& b = balls[(size_t) bi];
            if (! b.live) { if (t >= b.at) b.live = true; else continue; }
            b.vy += g * dt;
            b.vx -= b.vx * drag * dt; b.vy -= b.vy * drag * 0.3f * dt;
            const float px = b.x, py = b.y;
            b.x += b.vx * dt; b.y += b.vy * dt;
            auto hit = [&] (int id, float speed, float x, float y)
            {
                if (speed < 0.18f || (id == b.lastHit && t - b.lastT < 0.06f)) return;
                b.lastHit = id; b.lastT = t;
                LNote n; n.start = std::round (t * 48.0f) / 48.0f;      // tiny grid so the file is clean - the rubato stays
                n.vel = juce::jlimit (0.15f, 1.0f, (0.25f + 0.75f * b.m) * std::min (1.0f, speed / 2.2f + 0.25f));
                n.len = juce::jlimit (0.08f, 1.5f, 0.15f + speed * 0.25f);
                n.pitch = pitchAt (x, y);
                r.notes.push_back (n);
            };
            if (b.x < 0.0f) { b.x = -b.x; b.vx = -b.vx * e; hit (-2, std::abs (b.vx), 0.0f, b.y); }
            if (b.x > 1.0f) { b.x = 2.0f - b.x; b.vx = -b.vx * e; hit (-3, std::abs (b.vx), 1.0f, b.y); }
            if (b.y > 1.0f) { b.y = 2.0f - b.y; const float sp = std::abs (b.vy); b.vy = -sp * e; b.vx *= 1.0f - 0.25f * f.friction; hit (-1, sp, b.x, 1.0f); }
            if (b.y < -0.5f) { b.y = -0.5f; b.vy = std::abs (b.vy) * e; }
            for (int li = 0; li < (int) f.ledges.size(); ++li)
            {
                const auto& L = f.ledges[(size_t) li];
                const float lx = L.x1 - L.x0, ly = L.y1 - L.y0, len2 = lx * lx + ly * ly;
                if (len2 < 1.0e-6f) continue;
                const float side0 = (px - L.x0) * ly - (py - L.y0) * lx, side1 = (b.x - L.x0) * ly - (b.y - L.y0) * lx;
                if ((side0 > 0) == (side1 > 0)) continue;
                const float u = ((b.x - L.x0) * lx + (b.y - L.y0) * ly) / len2;
                if (u < 0 || u > 1) continue;
                float nx = -ly, ny = lx; const float nl = std::sqrt (nx * nx + ny * ny); nx /= nl; ny /= nl;
                const float vn = b.vx * nx + b.vy * ny;
                b.vx -= (1.0f + e) * vn * nx; b.vy -= (1.0f + e) * vn * ny;
                b.x = px; b.y = py;
                hit (li, std::abs (vn), L.x0 + u * lx, L.y0 + u * ly);
            }
        }
        // balls hitting each other: both ring (a two-note chord)
        for (size_t i = 0; i < balls.size(); ++i)
            for (size_t j = i + 1; j < balls.size(); ++j)
            {
                auto& A = balls[i]; auto& B = balls[j];
                if (! A.live || ! B.live) continue;
                const float dx = B.x - A.x, dy = B.y - A.y, d2 = dx * dx + dy * dy, rad = 0.035f * (1.5f + A.m + B.m);
                if (d2 > rad * rad || d2 < 1.0e-8f) continue;
                const float d = std::sqrt (d2), nx = dx / d, ny = dy / d;
                const float rv = (B.vx - A.vx) * nx + (B.vy - A.vy) * ny;
                if (rv > 0) continue;
                const float imp = -(1.0f + e) * rv / (1.0f / std::max (0.1f, A.m) + 1.0f / std::max (0.1f, B.m));
                A.vx -= imp / std::max (0.1f, A.m) * nx; A.vy -= imp / std::max (0.1f, A.m) * ny;
                B.vx += imp / std::max (0.1f, B.m) * nx; B.vy += imp / std::max (0.1f, B.m) * ny;
                if (std::abs (rv) > 0.2f && t - A.lastT > 0.05f)
                {
                    A.lastT = B.lastT = t;
                    for (auto* b : { &A, &B }) r.notes.push_back ({ std::round (t * 48.0f) / 48.0f, 0.3f, pitchAt (b->x, b->y), juce::jlimit (0.2f, 1.0f, 0.3f + 0.3f * std::abs (rv) * b->m), 0 });
                }
            }
        if (keepFrames && frame % 8 == 0)
        {
            Frame fr; fr.t = t;
            for (auto& b : balls) fr.pos.push_back (b.live ? juce::Point<float> (b.x, b.y) : juce::Point<float> (-1, -1));
            r.frames.push_back (std::move (fr));
        }
    }
    tidy (r, 6);
    return r;
}

inline GravityField randomField (uint32_t seed)
{
    Rng rr; rr.seed (hash32 (seed + 31u));
    GravityField f;
    const int n = 2 + (int) (rr.next() % 3);
    for (int i = 0; i < n; ++i)
        f.throws.push_back ({ 0.1f + 0.8f * rr.uni(), 0.05f + 0.35f * rr.uni(), (rr.uni() - 0.5f) * 1.2f, -0.6f * rr.uni(), 0.2f + 0.8f * rr.uni(), (float) (rr.next() % 4) * 0.5f + (float) i * 2.0f });
    const int ln = (int) (rr.next() % 3);
    for (int i = 0; i < ln; ++i)
    {
        const float cx = 0.15f + 0.7f * rr.uni(), cy = 0.45f + 0.35f * rr.uni(), w = 0.1f + 0.15f * rr.uni(), tilt = (rr.uni() - 0.5f) * 0.2f;
        f.ledges.push_back ({ cx - w, cy - tilt, cx + w, cy + tilt });
    }
    f.gravity = 0.3f + 0.4f * rr.uni(); f.friction = 0.15f + 0.35f * rr.uni(); f.bounce = 0.65f + 0.25f * rr.uni();
    return f;
}

// ------------------------------------------------------------------ PREDATOR ------------------------------------------------------------------
struct Hunt { float hunger = 0.6f, preySpeed = 0.5f, rate = 0.5f; bool escape = false; uint32_t seed = 1; };
enum HuntState { hsChase, hsCircle, hsPounce, hsRest };
inline const char* huntStateName (int s) { static const char* n[] { "CHASE", "CIRCLE", "POUNCE", "REST" }; return n[juce::jlimit (0, 3, s)]; }

struct HuntResult : Result { std::vector<int> stateAt; };   // per frame: what the creature did

inline HuntResult predator (const Hunt& h, int key, int scale, int bars)
{
    HuntResult r; r.beats = (float) bars * 4.0f;
    Rng rr; rr.seed (hash32 (h.seed * 7u + 3u));
    juce::Point<float> me { 0.15f, 0.8f }, prey { 0.7f, 0.4f }, vel { 0, 0 }, preyVel { 0.2f, 0.1f };
    int state = hsChase; float stateT = 0, circleA = 0;
    const float dt = 1.0f / 96.0f;
    const float step = h.rate > 0.66f ? 0.25f : h.rate > 0.33f ? 0.5f : 1.0f;        // how often it "sings"
    float nextNote = 0; int frame = 0;
    const int chordRoot = (int) (rr.next() % 3) * 2;                                  // i, iii or v as the circled chord
    auto deg = [&] (float y) { return (int) std::round ((1.0f - y) * 12.0f) - 2; };
    for (float t = 0; t < r.beats; t += dt, ++frame)
    {
        stateT += dt;
        // the prey wanders (and flees when the hunter is close)
        const auto away = prey - me; const float dist = away.getDistanceFromOrigin();
        if (rr.uni() < 0.02f) preyVel = { (rr.uni() - 0.5f) * h.preySpeed, (rr.uni() - 0.5f) * h.preySpeed };
        if (dist < 0.25f && state != hsPounce) preyVel += away / std::max (0.05f, dist) * 0.02f * h.preySpeed;
        preyVel *= 0.99f;
        prey += preyVel * dt * 2.0f;
        prey.x = juce::jlimit (0.05f, 0.95f, prey.x); prey.y = juce::jlimit (0.08f, 0.92f, prey.y);
        if (prey.x <= 0.05f || prey.x >= 0.95f) preyVel.x = -preyVel.x;
        if (prey.y <= 0.08f || prey.y >= 0.92f) preyVel.y = -preyVel.y;
        const float speed = 0.4f + 1.6f * h.hunger;
        switch (state)
        {
            case hsChase:
            {
                vel = vel * 0.9f + away / std::max (0.02f, dist) * speed * 0.1f;
                if (dist < 0.12f) { state = hsCircle; stateT = 0; circleA = std::atan2 (-away.y, -away.x); }
                break;
            }
            case hsCircle:
            {
                circleA += dt * (3.0f + 4.0f * h.hunger);
                const auto target = prey + juce::Point<float> (std::cos (circleA), std::sin (circleA)) * 0.1f;
                vel = (target - me) / dt * 0.15f;
                if (stateT > 2.0f + 2.0f * (1.0f - h.hunger)) { state = hsPounce; stateT = 0; }
                break;
            }
            case hsPounce:
            {
                vel = away / std::max (0.02f, dist) * speed * 2.2f;
                if (dist < 0.03f || stateT > 0.5f)
                {
                    // the accent: a long loud note that slides up into place
                    const int d = deg (me.y) + 4;
                    r.notes.push_back ({ std::round (t * 4.0f) / 4.0f, 1.5f, kk::mel::degreeToPitch (d, key, scale, 0.5f), 1.0f, -2.0f });
                    state = hsRest; stateT = 0; nextNote = std::round (t * 4.0f) / 4.0f + 1.75f;
                    prey = { 0.1f + 0.8f * rr.uni(), 0.1f + 0.8f * rr.uni() };          // the prey gets away to a new place
                }
                break;
            }
            default:
                vel *= 0.9f;
                if (stateT > 1.0f) { state = hsChase; stateT = 0; }
                break;
        }
        me += vel * dt;
        me.x = juce::jlimit (0.02f, 0.98f, me.x); me.y = juce::jlimit (0.02f, 0.98f, me.y);
        if (t >= nextNote && state != hsRest && state != hsPounce)
        {
            LNote n; n.start = nextNote;
            if (state == hsChase)
            {
                // the chase climbs: the closer, the higher - running motifs
                const float close = juce::jlimit (0.0f, 1.0f, 1.0f - dist / 0.8f);
                int d = deg (me.y) / 2 + (int) std::round (close * 7.0f);
                if (h.escape) d = 7 - d;                                                   // escaping: it falls instead
                n.pitch = kk::mel::degreeToPitch (d, key, scale, 0.5f); n.len = step * 0.9f; n.vel = 0.55f + 0.35f * close;
            }
            else
            {
                // circling: around one chord (root, third, fifth, octave) - variations on a fixed chord
                static const int tones[] { 0, 2, 4, 7, 4, 2 };
                const int k = (int) std::floor (std::fmod (circleA / juce::MathConstants<float>::twoPi * 6.0f + 600.0f, 6.0f));
                n.pitch = kk::mel::degreeToPitch (chordRoot + tones[k], key, scale, 0.5f); n.len = step * 0.8f; n.vel = 0.6f + 0.2f * std::sin (circleA);
            }
            r.notes.push_back (n);
            nextNote += step;
        }
        if (frame % 4 == 0) { r.frames.push_back ({ t, { me, prey } }); r.stateAt.push_back (state); }
    }
    tidy (r, 2);
    return r;
}

// ------------------------------------------------------------------ SWARM ------------------------------------------------------------------
struct Flock { int birds = 12; float cohesion = 0.7f, calm = 0.6f; std::vector<float> startles; uint32_t seed = 1; };   // startles: beats when something frightens it

struct SwarmResult : Result { std::vector<float> spreadAt; };

inline SwarmResult swarm (const Flock& fl, int key, int scale, int bars)
{
    SwarmResult r; r.beats = (float) bars * 4.0f;
    Rng rr; rr.seed (hash32 (fl.seed * 13u + 5u));
    struct Bird { juce::Point<float> p, v; float phase; };
    std::vector<Bird> b;
    for (int i = 0; i < fl.birds; ++i) b.push_back ({ { 0.4f + 0.2f * rr.uni(), 0.4f + 0.2f * rr.uni() }, { (rr.uni() - 0.5f) * 0.2f, (rr.uni() - 0.5f) * 0.2f }, rr.uni() });
    const float dt = 1.0f / 96.0f;
    float nextChord = 0, panic = 0; int frame = 0;
    const float wander = rr.uni() * 6.28f;
    size_t nextStartle = 0;
    std::vector<float> startles = fl.startles; std::sort (startles.begin(), startles.end());
    for (float t = 0; t < r.beats; t += dt, ++frame)
    {
        if (nextStartle < startles.size() && t >= startles[nextStartle])
        {
            ++nextStartle; panic = 1.0f;
            for (auto& x : b) x.v += juce::Point<float> (rr.uni() - 0.5f, rr.uni() - 0.5f) * 3.0f;
        }
        panic = std::max (0.0f, panic - dt * (0.15f + 0.6f * fl.calm));
        juce::Point<float> c; for (auto& x : b) c += x.p; c /= (float) b.size();
        const juce::Point<float> home (0.5f + 0.3f * std::sin (t * 0.41f + wander), 0.45f + 0.3f * std::sin (t * 0.27f + 2.0f * wander));   // the flock travels - the harmony moves
        for (auto& x : b)
        {
            juce::Point<float> sep, ali;
            for (auto& y : b)
            {
                if (&x == &y) continue;
                const auto d = x.p - y.p; const float dd = d.getDistanceFromOrigin();
                if (dd < 0.06f && dd > 1.0e-5f) sep += d / (dd * dd) * 0.0004f;
                if (dd < 0.2f) ali += y.v;
            }
            const float coh = fl.cohesion * (1.0f - panic);
            x.v += ((c - x.p) * coh * 1.2f + sep * (1.0f + 3.0f * panic) + ali * 0.004f + (home - x.p) * 0.6f) * dt * 6.0f;
            const float sp = x.v.getDistanceFromOrigin(), maxSp = 0.5f + 1.5f * panic;
            if (sp > maxSp) x.v *= maxSp / sp;
            x.p += x.v * dt;
            if (x.p.x < 0.02f || x.p.x > 0.98f) x.v.x = -x.v.x;
            if (x.p.y < 0.02f || x.p.y > 0.98f) x.v.y = -x.v.y;
            x.p.x = juce::jlimit (0.02f, 0.98f, x.p.x); x.p.y = juce::jlimit (0.02f, 0.98f, x.p.y);
        }
        float spread = 0; for (auto& x : b) spread += x.p.getDistanceFrom (c); spread /= (float) b.size();
        const bool scattered = spread > 0.16f || panic > 0.35f;
        if (! scattered && t >= nextChord)
        {
            // together: a tight chord in the key, voiced around where the flock flies
            const int root = (int) std::round ((1.0f - c.y) * 7.0f) - 2;
            const int voices = 3 + (spread > 0.08f ? 1 : 0);
            for (int k = 0; k < voices; ++k)
                r.notes.push_back ({ nextChord, 1.9f, kk::mel::degreeToPitch (root + k * 2, key, scale, 0.5f), 0.55f + 0.1f * (float) (k == 0), 0 });
            nextChord += 2.0f;
        }
        else if (scattered)
        {
            nextChord = std::ceil (t / 2.0f) * 2.0f;
            // scattered: every bird sings on its own clock (its speed) across three octaves - a cascade
            for (auto& x : b)
            {
                x.phase += dt * (0.25f + 1.1f * x.v.getDistanceFromOrigin());
                if (x.phase >= 1.0f)
                {
                    x.phase -= 1.0f;
                    const int d = (int) std::round ((1.0f - x.p.y) * 18.0f) - 6;
                    r.notes.push_back ({ std::round (t * 8.0f) / 8.0f, 0.2f, kk::mel::degreeToPitch (d, key, scale, 0.5f), 0.4f + 0.5f * std::min (1.0f, x.v.getDistanceFromOrigin()), 0 });
                }
            }
        }
        if (frame % 4 == 0)
        {
            Frame f; f.t = t; for (auto& x : b) f.pos.push_back (x.p);
            r.frames.push_back (std::move (f)); r.spreadAt.push_back (spread);
        }
    }
    tidy (r, 6);
    return r;
}

// ------------------------------------------------------------------ METABOLISM ------------------------------------------------------------------
// age = how many times the motif has repeated untouched. the motif breathes (timing), oxidises (notes slip to neighbours,
// velocities fade) and after its LIFESPAN it decays (notes drop out) - time to move on.
struct Metabolism { float rate = 0.5f; int lifespan = 8; uint32_t seed = 1; };
inline float lifeLeft (const Metabolism& m, int age) { return juce::jlimit (0.0f, 1.0f, 1.0f - (float) age / (float) std::max (1, m.lifespan)); }

inline std::vector<LNote> age (const std::vector<LNote>& src, int age, const Metabolism& m, int key, int scale, float beats)
{
    if (age <= 0 || m.rate <= 0.0f) return src;
    Rng rr; rr.seed (hash32 (m.seed * 977u + (uint32_t) age * 131u));
    const float a = (float) age * m.rate;
    const float decay = std::max (0.0f, (float) age - (float) m.lifespan) / (float) std::max (1, m.lifespan);   // past its life: falls apart
    std::vector<LNote> out;
    for (auto& n0 : src)
    {
        auto n = n0;
        // breathing: the timing swells and relaxes like a slow breath
        n.start += std::sin (n.start / 4.0f * juce::MathConstants<float>::twoPi + (float) age * 0.7f) * std::min (0.12f, 0.012f * a);
        n.start += (rr.uni() - 0.5f) * std::min (0.06f, 0.006f * a);
        n.start = juce::jlimit (0.0f, beats - 0.05f, n.start);
        // oxidation: a note may slip to a neighbour of the scale, velocities fade and blur
        if (rr.uni() < std::min (0.35f, 0.03f * a))
        {
            const int d = kk::mel::pitchToDegree (n.pitch, key, scale, 0.5f) + (rr.uni() < 0.5f ? -1 : 1);
            n.pitch = kk::mel::degreeToPitch (d, key, scale, 0.5f);
        }
        n.vel = juce::jlimit (0.1f, 1.0f, n.vel * (1.0f - std::min (0.4f, 0.025f * a)) + (rr.uni() - 0.5f) * 0.1f);
        if (decay > 0.0f && rr.uni() < std::min (0.9f, decay * 0.6f)) continue;          // decay: notes fall away
        out.push_back (n);
    }
    std::sort (out.begin(), out.end(), [] (auto& x, auto& y) { return x.start < y.start; });
    return out;
}
} // namespace kk::live
