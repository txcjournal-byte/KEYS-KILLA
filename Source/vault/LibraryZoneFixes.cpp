// CHROME VAULT: hand-made fixes of the sample library's zones (LibraryTuning.h, owner report item 0), applied to every
// library.json at load time. Found with `VaultTests -pitch --library` (every articulation as a plain one-layer sound:
// neighbouring keys one semitone apart +-10 cents) and `VaultTests -pitch` (every factory preset). Each line says why.
#include "LibraryTuning.h"

namespace vault
{
namespace
{
// zones taken out / re-ranged (the old range as built, the new one; -1 = not used)
const ZoneRangeFix rangeFixList[] = {
    { "strings.contrabass/spiccato_036_v2_r1.ogg", 36, 37, 64, 127, -1, -1, -1, -1 },   // holds a loud E3 over the C (a second string ringing): the keys play a C-E dyad
    { "strings.contrabass/spiccato_036_v1_r1.ogg", 36, 37, 1, 63, 36, 37, 1, 127 },   // covers the loud velocities too (its v2 take is out)
    { "strings.contrabass/spiccato_036_v2_r2.ogg", 36, 37, 64, 127, -1, -1, -1, -1 },   // the same take, second round robin
    { "strings.contrabass/spiccato_036_v1_r2.ogg", 36, 37, 1, 63, 36, 37, 1, 127 },   // covers the loud velocities too
    { "brass.horn/staccato_036_v2_r1.ogg", 32, 37, 43, 84, 29, 37, 43, 84 },   // loud notes below G#2 used the soft take (no loud one there) - 20 cents off the loud takes above; the loud take covers them
    { "brass.horn/staccato_036_v2_r2.ogg", 32, 37, 43, 84, 29, 37, 43, 84 },   // loud notes below G#2 used the soft take (no loud one there) - 20 cents off the loud takes above; the loud take covers them
};

// zones tuned by hand after the tuning pass: root, fine (the change against the pass, why)
const ZoneTunePin pinList[] = {
    { "strings.contrabass/tremolo_052_v2_r1.ogg", 52, 1.9f },   // -28 c: contrabass tremolo: YIN +28.5 c on every key of the zone, both neighbours in tune
    { "strings.contrabass/tremolo_056_v2_r1.ogg", 56, 7.6f },   // -17 c: contrabass tremolo 056: centre the 054/055 and 057/058 boundaries
    { "brass.trombone.vintage/buzz_062_v2_r1.ogg", 63, 0.0f },   // -100 c: the take labelled D5 sounds D#5 (every measurement: +200 cents from C#5)
    { "brass.trombone.vintage/fall_057_v1_r1.ogg", 57, 18.2f },   // +32 c: the A4 fall take is 30-35 cents flat against both neighbours (spectrum, partials, YIN tracks)
    { "winds.ocarina.small/staccato_081_v1_r1.ogg", 81, 0.5f },   // -6 c: the A6 take, set between its neighbours and its own pitch: YIN at F6 within 15 cents (VaultTests library check), steps to B6 one semitone
    { "winds.sax.tenor/vibrato_086_v1_r1.ogg", 86, -2.6f },   // -24 c: the top take (D7..F#7) plays 28-32 cents sharp (YIN), 20-45 cents above C#7
    { "world.dantranh/vibrato_049_v2_r1.ogg", 50, 13.0f },   // -87 c: dan tranh vibrato: re-fit to the chain of neighbour steps anchored on the YIN-steady takes (047, 068, 078, 083)
    { "world.dantranh/vibrato_051_v2_r1.ogg", 51, -50.0f },   // -50 c: dan tranh vibrato: re-fit to the chain of neighbour steps anchored on the YIN-steady takes (047, 068, 078, 083)
    { "world.dantranh/vibrato_054_v2_r1.ogg", 54, -48.5f },   // -58 c: dan tranh vibrato: re-fit to the chain of neighbour steps anchored on the YIN-steady takes (047, 068, 078, 083)
    { "world.dantranh/vibrato_056_v2_r1.ogg", 56, -30.2f },   // -29 c: boundary fit for cv.world.0341 (world.dantranh/vibrato)
    { "world.dantranh/vibrato_059_v2_r1.ogg", 59, -21.5f },   // -1 c: dan tranh vibrato: re-fit to the chain of neighbour steps anchored on the YIN-steady takes (047, 068, 078, 083)
    { "world.dantranh/vibrato_061_v2_r1.ogg", 61, -16.5f },   // -30 c: dan tranh vibrato: re-fit to the chain of neighbour steps anchored on the YIN-steady takes (047, 068, 078, 083)
    { "world.dantranh/vibrato_063_v2_r1.ogg", 63, -39.2f },   // -2 c: dan tranh vibrato: re-fit to the chain of neighbour steps anchored on the YIN-steady takes (047, 068, 078, 083)
    { "world.dantranh/vibrato_066_v2_r1.ogg", 66, -10.8f },   // -3 c: dan tranh vibrato: re-fit to the chain of neighbour steps anchored on the YIN-steady takes (047, 068, 078, 083)
    { "world.dantranh/vibrato_075_v2_r1.ogg", 75, -3.8f },   // -11 c: dan tranh vibrato: re-fit to the chain of neighbour steps anchored on the YIN-steady takes (047, 068, 078, 083)
    { "world.dantranh/vibrato_078_v2_r1.ogg", 78, -14.1f },   // -11 c: dan tranh vibrato: re-fit to the chain of neighbour steps anchored on the YIN-steady takes (047, 068, 078, 083)
    { "world.dantranh/vibrato_080_v2_r1.ogg", 81, 44.6f },   // -42 c: boundary fit for cv.world.0365 (world.dantranh/vibrato)
    { "bells.chimes.chromatic/hit_070_v1_r1.ogg", 70, -3.4f },   // -9 c: boundary fit: the smallest move that lets every neighbour step of the take measure one semitone (pitch test, bells.chimes.chromatic/hit)
    { "world.mbira/pluck_081_v1_r1.ogg", 81, 44.9f },   // +52 c: boundary fit: the smallest move that lets every neighbour step of the take measure one semitone (pitch test, world.mbira/pluck)
    { "voice.male.low/ah_040_v1_r1.ogg", 40, -10.4f },   // -25 c: boundary fit: the smallest move that lets every neighbour step of the take measure one semitone (pitch test, voice.male.low/ah)
    { "voice.male.low/ah_054_v1_r1.ogg", 54, -25.6f },   // -7 c: boundary fit: the smallest move that lets every neighbour step of the take measure one semitone (pitch test, voice.male.low/ah)
    { "voice.choir.classic/aahs_042_v1_r1.ogg", 42, 9.0f },   // -3 c: the E3 take, between the dry articulation's fit and the low-passed choir presets (E3-F3)
    { "perc.timpani/hit_052_v3_r1.ogg", 52, -9.0f },   // +5 c: boundary fit for cv.orch.0001 (perc.timpani/hit)
    { "keys.harpsichord.english/lute_088_v1_r1.ogg", 88, -10.0f },   // -9 c: the E7 take: between the dry / swelled and the plain lute stops' boundary fits (D#7-E7)
    { "world.kalimba.low/pluck_074_v1_r1.ogg", 74, 8.1f },   // -4 c: boundary fit for cv.world.0405 and the plain world.kalimba.low/pluck
    { "strings.contrabass/pizzicato_056_v1_r1.ogg", 56, 0.6f },   // -7 c: boundary fit for cv.bass.0382 (strings.contrabass/pizzicato)
    { "strings.cellos.ensemble/tremolo_036_v2_r1.ogg", 36, -26.3f },   // +4 c: boundary fit for cv.bass.0383 (strings.cellos.ensemble/tremolo)
    { "strings.violin.solo/pizzicato_088_v2_r1.ogg", 88, -17.2f },   // +7 c: boundary fit for cv.str.0025 (strings.violin.solo/pizzicato)
    { "strings.cellos.ensemble/tremolo_050_v2_r1.ogg", 50, -14.0f },   // -5 c: boundary fit for cv.str.0020 and the plain strings.cellos.ensemble/tremolo
    { "strings.cellos.ensemble/tremolo_043_v2_r1.ogg", 43, -22.0f },   // -26 c: boundary fit for cv.str.0283 (strings.cellos.ensemble/tremolo)
    { "strings.cellos.ensemble/pizzicato_043_v2_r1.ogg", 43, 6.5f },   // +7 c: boundary fit for cv.str.0350 (strings.cellos.ensemble/pizzicato)
    { "strings.cellos.ensemble/pizzicato_047_v2_r1.ogg", 47, -1.2f },   // -11 c: boundary fit for cv.fx.0176 (strings.cellos.ensemble/pizzicato)
    { "perc.timpani/hit_039_v3_r1.ogg", 39, 21.1f },   // -7 c: boundary fit for cv.orch.0221 (perc.timpani/hit)
    { "perc.timpani/hit_044_v3_r1.ogg", 43, -39.8f },   // -5 c: boundary fit for cv.orch.0221 (perc.timpani/hit)
    { "perc.timpani/hit_053_v3_r1.ogg", 53, -21.8f },   // -7 c: boundary fit for cv.orch.0233 (perc.timpani/hit)
    { "perc.timpani/roll_042_v2_r1.ogg", 42, -31.1f },   // -11 c: boundary fit for cv.orch.0232 (perc.timpani/roll)
    { "perc.timpani/hit_048_v3_r1.ogg", 48, -2.0f },   // -9 c: boundary fit for cv.orch.0244 (perc.timpani/hit)
    { "perc.timpani/hit_049_v3_r1.ogg", 49, -8.6f },   // +7 c: boundary fit for cv.orch.0246 (perc.timpani/hit)
    { "perc.timpani/hit_050_v3_r1.ogg", 50, -0.4f },   // -20 c: boundary fit for cv.orch.0244 (perc.timpani/hit)
    { "perc.timpani/roll_048_v2_r1.ogg", 48, 46.1f },   // +9 c: boundary fit for cv.orch.0248 (perc.timpani/roll)
    { "perc.timpani/roll_049_v2_r1.ogg", 50, 7.7f },   // -10 c: boundary fit for cv.orch.0248 (perc.timpani/roll)
    { "bells.chimes/hit_068_v2_r1.ogg", 68, 0.7f },   // -6 c: boundary fit for cv.bell.0545 (bells.chimes/hit)
    { "strings.violas.ensemble/pizzicato_079_v2_r1.ogg", 79, 15.9f },   // +5 c: boundary fit for cv.pluck.0172 (strings.violas.ensemble/pizzicato)
    { "strings.cellos.ensemble/tremolo_053_v2_r1.ogg", 53, 0.9f },   // -5 c: boundary fit for cv.pad.0174 (strings.cellos.ensemble/tremolo)
    { "strings.cellos.ensemble/tremolo_057_v2_r1.ogg", 57, -1.1f },   // -2 c: boundary fit for cv.bass.0383 and the plain strings.cellos.ensemble/tremolo
    { "strings.cellos.ensemble/tremolo_074_v2_r1.ogg", 74, 16.5f },   // +9 c: boundary fit for cv.pad.0174 (strings.cellos.ensemble/tremolo)
    { "world.dantranh/pluck_051_v3_r1.ogg", 51, -39.9f },   // +25 c: boundary fit for cv.world.0338 (world.dantranh/pluck)
    { "world.kalimba/pluck_075_v1_r1.ogg", 75, -32.4f },   // -5 c: boundary fit for cv.world.0350 (world.kalimba/pluck)
    { "world.kalimba/pluck_084_v1_r1.ogg", 84, 38.9f },   // -9 c: boundary fit for cv.world.0374 (world.kalimba/pluck)
    { "world.kalimba/pluck_081_v1_r1.ogg", 81, 31.8f },   // -5 c: boundary fit for cv.world.0374 (world.kalimba/pluck)
    { "world.kalimba/pluck_066_v1_r1.ogg", 66, -9.6f },   // -14 c: boundary fit for cv.world.0403 (world.kalimba/pluck)
    { "world.kalimba/pluck_063_v1_r1.ogg", 63, 1.9f },   // -5 c: boundary fit for cv.world.0677 (world.kalimba/pluck)
    { "world.dantranh/pluck_054_v3_r1.ogg", 54, -3.1f },   // +22 c: boundary fit for cv.world.0722 (world.dantranh/pluck)
    { "world.dantranh/pluck_068_v3_r1.ogg", 68, 8.0f },   // +4 c: boundary fit for cv.world.0722 (world.dantranh/pluck)
    { "strings.cellos.ensemble/pizzicato_040_v2_r1.ogg", 40, 17.7f },   // -3 c: boundary fit for cv.fx.0016 (strings.cellos.ensemble/pizzicato)
    { "world.kalimba.low/pluck_055_v1_r1.ogg", 55, -42.8f },   // -12 c: boundary fit for cv.fx.0020 (world.kalimba.low/pluck)
    { "world.kalimba.low/pluck_073_v1_r1.ogg", 73, -11.4f },   // -7 c: boundary fit for cv.fx.0020 (world.kalimba.low/pluck)
    { "world.kalimba.low/pluck_075_v1_r1.ogg", 75, -11.4f },   // -9 c: boundary fit for cv.fx.0179 (world.kalimba.low/pluck)
    { "strings.cellos.ensemble/pizzicato_057_v2_r1.ogg", 57, -2.3f },   // +4 c: boundary fit for cv.fx.0176 (strings.cellos.ensemble/pizzicato)
    { "world.kalimba.low/pluck_049_v1_r1.ogg", 49, 9.9f },   // +8 c: boundary fit for cv.bass.0289 and the plain world.kalimba.low/pluck
    { "world.kalimba.low/pluck_077_v1_r1.ogg", 77, -33.0f },   // -11 c: boundary fit for cv.fx.0179 (world.kalimba.low/pluck)
    { "strings.cellos.ensemble/tremolo_059_v2_r1.ogg", 59, -17.3f },   // +12 c: boundary fit for cv.bass.0383 and the plain strings.cellos.ensemble/tremolo
};
} // namespace

const ZoneRangeFix* libraryRangeFixes (int& count)
{
    count = (int) (sizeof (rangeFixList) / sizeof (rangeFixList[0]));
    return rangeFixList;
}

const ZoneTunePin* libraryTunePins (int& count)
{
    count = (int) (sizeof (pinList) / sizeof (pinList[0]));
    return pinList;
}
} // namespace vault
