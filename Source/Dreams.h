#pragma once
#include "PluginProcessor.h"

// v0.45 DREAM: while FL is idle (no mouse, no playback) the plugin dreams. After a while of quiet it takes the sounds you used
// lately (or the one on the keys) and dreams 3 variations of them - a slower, frozen dream, a hot torn nightmare, a small
// bright daydream ... (SCULPT mutations, no audio rendered, no memory).  When you come back, 3 dreams are waiting.
// tick() is called from a 1 Hz timer by the editor: tick (true) = the user is away this second.
class DreamEngine
{
public:
    using Genome = KeysKillaProcessor::Genome;
    struct Dream { Genome g; juce::String from, how; uint32_t seed = 0; bool kept = false; };
    static constexpr int perNight = 3;

    explicit DreamEngine (KeysKillaProcessor& p) : proc (p) {}

    // a sound you just used (FEED keep, ALCHEMY, WORDS, GRID ...): it may come back in a dream
    void remember (const Genome& g)
    {
        if (! g.valid()) return;
        recent.erase (std::remove_if (recent.begin(), recent.end(), [&] (const Genome& x) { return x.name == g.name && x.v == g.v; }), recent.end());
        recent.insert (recent.begin(), g);
        if (recent.size() > 8) recent.pop_back();
    }
    void setIdleSeconds (int s) { idleNeeded = juce::jmax (1, s); }
    int idleSeconds() const { return idle; }
    bool dreaming() const { return toDream > 0; }

    void tick (bool userIdle)
    {
        if (! userIdle)
        {
            if (idle > 0 && made > 0) fresh = true;   // you are back: the dreams are waiting
            idle = 0; toDream = 0; return;
        }
        ++idle;
        if (idle == idleNeeded) { toDream = perNight; made = 0; ++night; nightDreams.clear(); }
        if (toDream > 0) { dreamOne(); --toDream; }     // one dream per second - the CPU never notices
    }
    const std::vector<Dream>& dreams() const { return nightDreams; }
    bool hasNewDreams() const { return fresh && ! nightDreams.empty(); }
    void markSeen() { fresh = false; }
    void keep (int i) { if (juce::isPositiveAndBelow (i, (int) nightDreams.size())) nightDreams[(size_t) i].kept = true; }

    // one variation of a source sound (deterministic: the same seed = the same dream)
    static Dream dreamOf (const KeysKillaProcessor& p, const Genome& src, uint32_t seed)
    {
        Dream d; d.seed = seed; d.from = src.name;
        kk::Rng r; r.seed (kk::hash32 (seed * 2654435761u + 13u));
        const int kind = (int) (seed % 4u);
        auto sgn = [&] { return r.uni() < 0.5f ? -1.0f : 1.0f; };
        float st = 0, br = 0, he = 0, co = 0, sp = 0;
        switch (kind)
        {
            case 0: st = 0.5f + 0.4f * r.uni(); co = 0.4f + 0.5f * r.uni(); br = -0.2f - 0.4f * r.uni(); d.how = "a slow, frozen dream"; break;
            case 1: he = 0.5f + 0.5f * r.uni(); sp = 0.3f + 0.5f * r.uni(); st = sgn() * 0.3f; d.how = "a hot, torn nightmare"; break;
            case 2: br = 0.4f + 0.5f * r.uni(); st = -0.4f - 0.4f * r.uni(); d.how = "a small, bright daydream"; break;
            default: st = sgn() * (0.3f + 0.5f * r.uni()); br = sgn() * (0.3f + 0.4f * r.uni()); sp = 0.4f * r.uni(); co = 0.3f * r.uni(); d.how = "a strange dream"; break;
        }
        d.g = p.sculpt (src, st, br, he, co, sp);
        static const char* words[] { "Dream of ", "Nightmare of ", "Daydream of ", "Echo of " };
        d.g.name = (juce::String (words[kind]) + src.name).substring (0, 40);
        return d;
    }
private:
    void dreamOne()
    {
        std::vector<Genome> pool = recent;
        if (pool.empty()) { auto cur = proc.currentGenome(); if (cur.valid()) pool.push_back (cur); }
        const uint32_t seed = kk::hash32 ((uint32_t) night * 7919u + (uint32_t) made * 104729u + 3u) * 4u + (uint32_t) made;   // made 0, 1, 2 -> 3 different kinds
        Genome src;
        if (pool.empty()) src = proc.alchemy ((int) (seed % 5u), (int) ((seed >> 5) % 5u), 0.4f, 0.5f, seed | 1u);   // nothing used yet: dream from the matter itself
        else src = pool[(size_t) (made % (int) pool.size())];
        if (! src.valid()) return;
        nightDreams.push_back (dreamOf (proc, src, seed));
        ++made;
    }
    KeysKillaProcessor& proc;
    std::vector<Genome> recent;
    std::vector<Dream> nightDreams;
    int idle = 0, idleNeeded = 90, toDream = 0, made = 0, night = 0;
    bool fresh = false;
};
