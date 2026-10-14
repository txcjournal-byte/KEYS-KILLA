# CHROME VAULT - sample credits

The sampled instruments of CHROME VAULT are built from two public-domain (CC0 1.0 Universal) sample collections, plus a
few vocal recordings under CC BY 4.0 and MIT (see "Vocal and choir samples" - those licences require the credit).
CC0 asks for no credit at all - we give it gladly, because these libraries are a gift to every musician and developer.

## VSCO 2 Community Edition (VS Chamber Orchestra: Community Edition)

* By **Versilian Studios LLC** - recorded by **Sam Gossner** and **Simon Dalzell** (Ivy Audio), sample cutting by
  **Elan Hickler** (Soundemote).
* License: CC0 1.0 Universal (public domain dedication).
* Home: https://github.com/sgossner/VSCO-2-CE - more information: http://vis.versilstudios.net/vsco-community.html
* Used for: string sections and solo strings, brass, woodwinds, orchestral percussion (for example the violin section
  sustain and the concert flute in the first library build).

## VCSL - Versilian Community Sample Library

* By **Versilian Studios LLC** (Sam Gossner) and its contributors.
* License: CC0 1.0 Universal (public domain dedication).
* Home: https://github.com/sgossner/VCSL
* Used for: grand and upright pianos, harpsichords, organs, recorders, ocarinas, harmonicas, saxophones, harps, plucked
  and struck instruments from around the world, mallets, bells, chimes and percussion (for example the concert grand
  piano and the marimba in the first library build).

## Vocal and choir samples

These licences **require** attribution (CC BY 4.0) or that the notice ships with the product (MIT). Keep this section
in the installer's licence / credits page. Details and the search that found them: `docs/VAULT_VOCAL_SOURCES.md`.

### Fluid (R3) SoundFont - "Classic Choir" (Choir Aahs, Voice Oohs, Solo Vox)

* By **Frank Wen**, with Suren M. Seron, Scott Hanan, Steve Aupperle, Chris Gillman, Alex Taubr, Chris Prola, Andrew
  Klenk, Winfried Hubbe, Dylan, Tim, Gort, Uros Katic and Ethan Winer.
* Source: FluidR3_GM.sf2, GitHub release https://github.com/pianobooster/fluid-soundfont (v3.1).
* Changes: the samples of three programs were extracted, stereo pairs merged, trimmed, pitch-checked, re-looped,
  level-matched and encoded as OggVorbis.
* License: MIT -

  > Copyright (c) 2000-2002, 2008 Frank Wen <getfrank@gmail.com>
  >
  > Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
  > documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
  > rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
  > persons to whom the Software is furnished to do so, subject to the following conditions:
  >
  > The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
  > Software.
  >
  > THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  > WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
  > COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
  > OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

### VocalSet - "Female Vowel Chops"

* **VocalSet: A Singing Voice Dataset** - Julia Wilkins, Prem Seetharaman, Alison Wahl, Bryan Pardo (Northwestern
  University Interactive Audio Lab), ISMIR 2018. https://doi.org/10.5281/zenodo.1442513
* Material: singer "f2", scales on "e" (straight and breathy tone), from the lab's demo page
  https://github.com/interactiveaudiolab/interactiveaudiolab.github.io (`demos/samples`).
* Changes: sliced into single notes, trimmed, faded, pitch-corrected (fine tuning), level-matched, encoded as OggVorbis.
* License: Creative Commons Attribution 4.0 International (CC BY 4.0) - https://creativecommons.org/licenses/by/4.0/

### MTG sms-tools sounds - "Female Solo Voice"

* `singing-female.wav` and `soprano-E4.wav` from https://github.com/MTG/sms-tools-materials (`sounds/`), by **Xavier
  Serra** / Music Technology Group, Universitat Pompeu Fabra, published on freesound.org (pack 13038,
  https://freesound.org/people/xserra/packs/13038/).
* Changes: the first 2.4 s of the phrase / the whole note, trimmed, looped, tuned, level-matched, encoded as OggVorbis.
* License: Creative Commons Attribution 4.0 International (CC BY 4.0) - https://creativecommons.org/licenses/by/4.0/

### vocobox Human Voice Dataset - "Male Low Voice"

* https://github.com/vocobox/human-voice-dataset - singer "martin", recorded 2014.
* Changes: notes renamed by their measured pitch, trimmed, looped, tuned, level-matched, encoded as OggVorbis.
* License: MIT -

  > Copyright (c) 2014 vocobox
  >
  > Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
  > documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
  > rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
  > persons to whom the Software is furnished to do so, subject to the following conditions:
  >
  > The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
  > Software.
  >
  > THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  > WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
  > COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
  > OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

## How the samples are used

The recordings are processed by the CHROME VAULT library builder (`tools/vault/LibBuild.cpp`, map
`tools/vault/library_map.json`): trimmed, pitch-checked, loop-pointed, level-matched across velocity layers and encoded
as OggVorbis. Instrument names in the plugin are generic instrument nouns ("Concert Grand", "Violin Section"); the
makers of the recorded instruments are not named.

Thank you, Sam, Simon, Elan and everyone who contributed to VSCO and VCSL.
