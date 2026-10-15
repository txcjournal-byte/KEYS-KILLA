// Lucky Notes engine: plain C++ (no JUCE / GUI dependencies).
// Basic types, seeded RNG and scale / chord helpers.
#pragma once

#include <algorithm>
#include <array>
#include <iterator>
#include <cstdint>
#include <string>
#include <vector>

namespace ln
{
constexpr int kNumMelodies = 12;

struct Note
{
    int pitch = 60;        // MIDI note number
    double start = 0.0;    // beats (quarter notes) from the start of the melody
    double length = 0.5;   // beats
    int velocity = 100;    // 1..127
};

struct Key
{
    int root = 9;       // 0 = C ... 11 = B
    bool minor = true;
    bool operator== (const Key& o) const { return root == o.root && minor == o.minor; }
};

// Deterministic 64-bit RNG (splitmix64). Same seed -> same sequence on every platform / compiler.
class Rng
{
public:
    explicit Rng (uint64_t seed) : state (seed) {}

    uint64_t next()
    {
        uint64_t z = (state += 0x9E3779B97F4A7C15ull);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
        return z ^ (z >> 31);
    }

    // uniform integer in [lo, hi] (inclusive)
    int range (int lo, int hi)
    {
        if (hi <= lo) return lo;
        const auto span = (uint64_t) (hi - lo + 1);
        return lo + (int) (next() % span);
    }

    // uniform double in [0, 1)
    double uniform() { return (double) (next() >> 11) * (1.0 / 9007199254740992.0); }
    bool chance (double p) { return uniform() < p; }

    // index chosen with integer weights
    int weighted (const int* weights, int count)
    {
        int total = 0;
        for (int i = 0; i < count; ++i) total += weights[i] > 0 ? weights[i] : 0;
        if (total <= 0) return 0;
        int r = range (0, total - 1);
        for (int i = 0; i < count; ++i)
        {
            const int w = weights[i] > 0 ? weights[i] : 0;
            if (r < w) return i;
            r -= w;
        }
        return count - 1;
    }

private:
    uint64_t state;
};

inline uint64_t mixSeed (uint64_t a, uint64_t b)
{
    Rng r (a * 0x100000001B3ull ^ (b + 0x632BE59BD9B4E019ull));
    r.next();
    return r.next();
}

inline const std::array<int, 7>& scaleIntervals (bool minor)
{
    static const std::array<int, 7> majorScale { 0, 2, 4, 5, 7, 9, 11 };
    static const std::array<int, 7> minorScale { 0, 2, 3, 5, 7, 8, 10 }; // natural minor (aeolian)
    return minor ? minorScale : majorScale;
}

inline int floorDiv (int a, int b) { return a >= 0 ? a / b : -((-a + b - 1) / b); }
inline int floorMod (int a, int b) { const int m = a % b; return m < 0 ? m + b : m; }

// MIDI pitch of the tonic that degree 0 maps to: between G3 (55) and F#4 (66), so melodies sit around C4..C6.
inline int tonicMidi (const Key& k) { return 60 + k.root - (k.root >= 7 ? 12 : 0); }

// absolute scale degree (0 = tonic, 7 = tonic an octave up, -1 = degree below the tonic...) -> MIDI pitch
inline int degreeToPitch (const Key& k, int degree)
{
    const auto& s = scaleIntervals (k.minor);
    return tonicMidi (k) + 12 * floorDiv (degree, 7) + s[(size_t) floorMod (degree, 7)];
}

inline bool pitchInScale (const Key& k, int pitch)
{
    const int pc = floorMod (pitch - k.root, 12);
    for (int i : scaleIntervals (k.minor))
        if (i == pc) return true;
    return false;
}

// chord built on scale degree `chordRoot` (0..6): is absolute degree d a chord tone (root / 3rd / 5th)?
inline bool isChordTone (int d, int chordRoot)
{
    const int rel = floorMod (d - chordRoot, 7);
    return rel == 0 || rel == 2 || rel == 4;
}

inline const char* noteName (int pc)
{
    static const char* names[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    return names[floorMod (pc, 12)];
}

// "Fmin" / "C#maj" (used in file names)
inline std::string keyShortName (const Key& k) { return std::string (noteName (k.root)) + (k.minor ? "min" : "maj"); }
// "F MIN" (UI)
inline std::string keyDisplayName (const Key& k) { return std::string (noteName (k.root)) + (k.minor ? " MIN" : " MAJ"); }
} // namespace ln
