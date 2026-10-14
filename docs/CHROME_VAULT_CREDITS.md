# CHROME VAULT - sample credits

The sampled instruments of CHROME VAULT are built from two public-domain (CC0 1.0 Universal) sample collections.
CC0 asks for no credit at all - we give it gladly, because these libraries are a gift to every musician and developer.

## VSCO 2 Community Edition (VS Chamber Orchestra: Community Edition)

* By **Versilian Studios LLC** - recorded by **Sam Gossner** and **Simon Dalzell** (Ivy Audio), sample cutting by
  **Elan Hickler** (Soundemote).
* License: CC0 1.0 Universal (public domain dedication).
* Home: https://github.com/sgossner/VSCO-2-CE - more information: http://vis.versilstudios.net/vsco-community.html
* Used for: the violin, viola and cello sections, solo violin and contrabass (sustain, vibrato, tremolo, pizzicato,
  spiccato), trumpet (open, straight and harmon mutes), French horn, tenor trombone, a 1950s tenor trombone (with falls
  and buzz), tuba, concert flute, piccolo, clarinet, oboe, bassoon, and the slit drums of the VSCO 1 percussion set.

## VCSL - Versilian Community Sample Library

* By **Versilian Studios LLC** (Sam Gossner) and its contributors.
* License: CC0 1.0 Universal (public domain dedication).
* Home: https://github.com/sgossner/VCSL
* Used for: three grand pianos and two upright pianos, FM electric piano / piano / clav patches, five harpsichords, the
  pipe organ and a renaissance chamber organ, four baroque recorders, two ocarinas, three harmonicas, tenor and curved
  soprano saxophones, concert and folk harps, stick dulcimer, psaltery, dan tranh, marimba, vibraphone, xylophone,
  glockenspiel, three sets of tubular bells, hand chimes, wine glasses, timpani, balafon, two kalimbas, three mbiras,
  slit drums and didgeridoo. The full list is in `docs/VAULT_INSTRUMENTS.md`.
* Some VCSL / VSCO sets were recorded by **Simon Dalzell (Ivy Audio)** for Versilian Studios (the pipe organ and the
  "parlor" upright piano); their info files encourage crediting both him and Versilian Studios - gladly done here.

## How the samples are used

The recordings are processed by the CHROME VAULT library builder (`tools/vault/LibBuild.cpp`, map
`tools/vault/library_map.json`): trimmed, pitch-checked, loop-pointed, level-matched across velocity layers and encoded
as OggVorbis. Instrument names in the plugin are generic instrument nouns ("Concert Grand", "Violin Section"); the
makers of the recorded instruments are not named.

Thank you, Sam, Simon, Elan and everyone who contributed to VSCO and VCSL.
