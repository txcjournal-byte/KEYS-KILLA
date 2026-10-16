#pragma once
// CHROME VAULT: the built-in tuning patch of the sample library (owner report item 0). The library tuning pass
// (VaultLibBuild -retune, tools/vault/Retune.h) tunes every zone against its neighbours on the keyboard; a library.json
// written before the pass (its articulations carry no "retuned" flag) is corrected at load time with these values, so an
// installed library plays every key at its own pitch without a new download. A zone is changed only while it still holds
// the old root / fine (a rebuilt library keeps its own values). Data: LibraryTuningData.cpp (generated).
#include <cstdint>

namespace vault
{
struct ZoneTuneFix
{
    const char* file;            // the zone's "file" (relative to Library/audio)
    int oldRoot; float oldFine;  // the values before the pass
    int root; float fine;        // the tuned values
};
const ZoneTuneFix* libraryTuneFixes (int& count);

// zones whose recording cannot be played in tune with its neighbours (a sample that holds a second note, a take whose
// pitch wanders) are taken out, or their key / velocity range changed so a clean neighbour covers the keys. Keyed on the
// zone's file AND its range as built (a library with other ranges is left alone); applied to every library.json.
// Data: LibraryZoneFixes.cpp (hand-written, with the reason for each).
struct ZoneRangeFix
{
    const char* file;
    int oldLo, oldHi, oldVlo, oldVhi;    // the range as built
    int lo, hi, vlo, vhi;                // the new range; lo < 0 = the zone is not used
};
const ZoneRangeFix* libraryRangeFixes (int& count);

// zones tuned by hand after the tuning pass (a mislabelled take, a take whose pitch sits well off its neighbours - found
// with the pitch test, see LibraryZoneFixes.cpp): their root / fine, keyed on the file, applied to every library.json
// after the generated patch (so an installed library, a rebuilt one and a re-tuned one all play them the same)
struct ZoneTunePin
{
    const char* file;
    int root; float fine;
};
const ZoneTunePin* libraryTunePins (int& count);
} // namespace vault
