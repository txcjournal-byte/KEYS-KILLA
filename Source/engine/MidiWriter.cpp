#include "MidiWriter.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace ln
{
namespace
{
void put32 (std::vector<uint8_t>& v, uint32_t x)
{
    v.push_back ((uint8_t) (x >> 24)); v.push_back ((uint8_t) (x >> 16));
    v.push_back ((uint8_t) (x >> 8));  v.push_back ((uint8_t) x);
}
void put16 (std::vector<uint8_t>& v, uint32_t x) { v.push_back ((uint8_t) (x >> 8)); v.push_back ((uint8_t) x); }

void putVar (std::vector<uint8_t>& v, uint32_t x)
{
    uint8_t buf[5];
    int n = 0;
    buf[n++] = (uint8_t) (x & 0x7f);
    while ((x >>= 7) != 0) buf[n++] = (uint8_t) ((x & 0x7f) | 0x80);
    while (n > 0) v.push_back (buf[--n]);
}

void putText (std::vector<uint8_t>& t, uint8_t type, const std::string& s)
{
    putVar (t, 0);
    t.push_back (0xff); t.push_back (type);
    putVar (t, (uint32_t) s.size());
    t.insert (t.end(), s.begin(), s.end());
}

void appendTrack (std::vector<uint8_t>& file, const std::vector<uint8_t>& track)
{
    file.push_back ('M'); file.push_back ('T'); file.push_back ('r'); file.push_back ('k');
    put32 (file, (uint32_t) track.size());
    file.insert (file.end(), track.begin(), track.end());
}

uint32_t toTicks (double beats) { return (uint32_t) std::max (0.0, std::round (beats * kTicksPerQuarter)); }
} // namespace

std::vector<uint8_t> writeMidiFile (const std::vector<Note>& notes, double bpm, double lengthBeats,
                                    const std::string& trackName, int channel)
{
    bpm = std::clamp (bpm, 20.0, 999.0);
    channel = std::clamp (channel, 0, 15);
    const uint32_t endTick = toTicks (lengthBeats);

    std::vector<uint8_t> file;
    file.push_back ('M'); file.push_back ('T'); file.push_back ('h'); file.push_back ('d');
    put32 (file, 6);
    put16 (file, 1);  // format 1
    put16 (file, 2);  // tracks
    put16 (file, kTicksPerQuarter);

    // track 0: tempo map
    {
        std::vector<uint8_t> t;
        putText (t, 0x03, "Lucky Notes");
        putVar (t, 0); t.push_back (0xff); t.push_back (0x58); t.push_back (4);  // 4/4
        t.push_back (4); t.push_back (2); t.push_back (24); t.push_back (8);
        const uint32_t usPerQuarter = (uint32_t) std::round (60000000.0 / bpm);
        putVar (t, 0); t.push_back (0xff); t.push_back (0x51); t.push_back (3);
        t.push_back ((uint8_t) (usPerQuarter >> 16)); t.push_back ((uint8_t) (usPerQuarter >> 8)); t.push_back ((uint8_t) usPerQuarter);
        putVar (t, endTick); t.push_back (0xff); t.push_back (0x2f); t.push_back (0);
        appendTrack (file, t);
    }

    // track 1: notes
    {
        struct Ev { uint32_t tick; bool on; int pitch; int vel; };
        std::vector<Ev> ev;
        for (const auto& n : notes)
        {
            const uint32_t s = std::min (toTicks (n.start), endTick);
            const uint32_t e = std::min (std::max (s + 1, toTicks (n.start + n.length)), endTick);
            if (e <= s) continue;
            ev.push_back ({ s, true, std::clamp (n.pitch, 0, 127), std::clamp (n.velocity, 1, 127) });
            ev.push_back ({ e, false, std::clamp (n.pitch, 0, 127), 0 });
        }
        std::stable_sort (ev.begin(), ev.end(), [] (const Ev& a, const Ev& b)
        {
            if (a.tick != b.tick) return a.tick < b.tick;
            return ! a.on && b.on; // note-offs before note-ons at the same tick
        });

        std::vector<uint8_t> t;
        putText (t, 0x03, trackName);
        uint32_t last = 0;
        for (const auto& e : ev)
        {
            putVar (t, e.tick - last);
            last = e.tick;
            t.push_back ((uint8_t) ((e.on ? 0x90 : 0x80) | channel));
            t.push_back ((uint8_t) e.pitch);
            t.push_back ((uint8_t) (e.on ? e.vel : 64));
        }
        putVar (t, endTick - std::min (last, endTick));
        t.push_back (0xff); t.push_back (0x2f); t.push_back (0);
        appendTrack (file, t);
    }
    return file;
}

std::string bpmText (double bpm)
{
    char buf[32];
    const double r = std::round (bpm);
    if (std::abs (bpm - r) < 0.05) std::snprintf (buf, sizeof (buf), "%d", (int) r);
    else std::snprintf (buf, sizeof (buf), "%.1f", bpm);
    return buf;
}

std::string midiFileName (int row, double bpm, const Key& key, int bars, bool chords)
{
    char num[8];
    std::snprintf (num, sizeof (num), "%02d", std::clamp (row + 1, 1, 99));
    return std::string ("LuckyNotes_") + num + "_" + bpmText (bpm) + "BPM_" + keyShortName (key) + "_"
         + std::to_string (bars) + "bars" + (chords ? "_Chords" : "") + ".mid";
}

std::vector<uint8_t> melodyMidi (const Melody& m, double bpm, const Key& key)
{
    char name[64];
    std::snprintf (name, sizeof (name), "Lucky Notes %02d %s", m.index + 1, keyShortName (key).c_str());
    return writeMidiFile (m.notes, bpm, m.lengthBeats, name);
}

std::vector<uint8_t> chordsMidi (const Melody& m, double bpm, const Key& key)
{
    char name[64];
    std::snprintf (name, sizeof (name), "Lucky Notes %02d Chords %s", m.index + 1, keyShortName (key).c_str());
    return writeMidiFile (m.chords, bpm, m.lengthBeats, name);
}
} // namespace ln
