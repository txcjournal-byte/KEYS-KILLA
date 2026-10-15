// Lucky Notes engine: Standard MIDI File writer (format 1) + file naming (plain C++).
#pragma once

#include "MelodyGenerator.h"

namespace ln
{
constexpr int kTicksPerQuarter = 480;

// Format 1: track 0 = tempo map (track name "Lucky Notes", 4/4, tempo = bpm), track 1 = the notes (named trackName).
// The end-of-track events sit exactly at lengthBeats, so the clip loops cleanly in a DAW.
std::vector<uint8_t> writeMidiFile (const std::vector<Note>& notes, double bpm, double lengthBeats,
                                    const std::string& trackName, int channel = 0);

// "142.5" / "150"
std::string bpmText (double bpm);

// LuckyNotes_07_150BPM_Fmin_8bars.mid  (row 0-based -> "01"..)  /  ..._8bars_Chords.mid
std::string midiFileName (int row, double bpm, const Key& key, int bars, bool chords = false);

std::vector<uint8_t> melodyMidi (const Melody& m, double bpm, const Key& key);
std::vector<uint8_t> chordsMidi (const Melody& m, double bpm, const Key& key);
} // namespace ln
