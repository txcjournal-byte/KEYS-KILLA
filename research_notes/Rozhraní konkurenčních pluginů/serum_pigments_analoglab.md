# UI of Xfer Serum 2 (vs Serum 1), Arturia Pigments (v5–v7) and Arturia Analog Lab (V / Pro)

Research note: in this session, WebFetch and direct HTTP were blocked for all target domains (DNS / egress proxy refusal), so I couldn't open full pages. Every finding below comes from search-engine extracts of the cited pages, not from reading the pages in full. Items marked "(unverified, from prior knowledge)" in Inferences have no source found in this session. Treat them as hypotheses to check against screenshots or manuals.

Versions as of Oct 2026: Serum 2 (released 2025, 2.0.x builds in late 2025). Pigments 7 shipped 16 Dec 2025 as a free update, so the current version is 7, not 5/6. Analog Lab V was renamed Analog Lab Pro in late 2023, alongside the free Analog Lab Play.

## Q1. Tab/page structure: what is on each page

### Takeaway
Serum 2 uses top tabs: OSC (the default view: 3 wavetable/hybrid oscillators, sub, noise, 2 filters), MIX (a new graphical routing and mixer page), FX, MATRIX/MOD and GLOBAL, plus an integrated ARP and CLIP sequencer. Serum 1 had OSC / FX / MATRIX / GLOBAL. Pigments separates a simplified Play view (added in v4, redesigned in v7) from the full Synth view, an FX page and a Seq page (sequencer/arp). The browser is a separate full-window view. Analog Lab Pro has three views: Library (the default browser), Studio (mix, FX, two-part combos) and Stage (live playlists). A newer "Main"/Play view shows a large instrument image with macros.

### Cited Findings
- Serum 2 OSC is "the default view that you see on startup": 3 main oscillators, sub osc (far left), noise osc and 2 filters — [EDMProd Serum 2 guide](https://www.edmprod.com/serum-2-guide/)
- Serum 2 MIX is a new routing view showing "which oscillator is going into which filter, into which effect", with graphical routing, levels, panning and balance — [EDMProd](https://www.edmprod.com/serum-2-guide/); [The Producer School](https://theproducerschool.com/blogs/featured-blogs/serum-2-synth-plugin-features-review-free-presets-download) (a "brand-new mixer… channel faders and pan controls for each source")
- Serum 2 FX: the effects view (EQ, chorus, distortion…), with new Bus options. MATRIX "lets you quickly visualize all the modulations currently active". GLOBAL holds voicing, audio quality and similar settings — [EDMProd](https://www.edmprod.com/serum-2-guide/)
- Serum 2 has "a cleaner, wider layout with tabbed pages for oscillators, mixer, FX, and modulation" — [Dubspot](https://blog.dubspot.com/xfer-records-releases-serum-2)
- Serum 2 adds an integrated Arpeggiator and Clip Sequencer — [Starsamples](https://starsamples.com/blogs/music-articles/xfer-serum-2-history-features-presets-and-beginner-s-guide); [MusicTech review (search extract)](https://musictech.com/reviews/software-instruments/xfer-records-serum-2-review)
- Serum 1's tab set was OSC, FX, MATRIX and GLOBAL, with no MIX page — [EDMProd Serum 1 tutorial](https://www.edmprod.com/how-to-use-serum-tutorial/)
- Pigments Play view was "introduced in version 4 as a simplified interface that provides the essential controls for playing and editing presets". Each control has a counterpart in the Synth view, and changes carry over between views — [Pigments 5 manual](https://dl.arturia.net/products/pigments/manual/pigments_Manual_5_0_0_EN.pdf); [Arturia support: Play View](https://support.arturia.com/hc/en-us/articles/6946292004380-Pigments-Play-View)
- Pigments' Seq button holds two modes, a step sequencer and an arpeggiator — [Pigments manuals](https://dl.arturia.net/products/pigments/manual/pigments_Manual_5_0_0_EN.pdf)
- Pigments 7 (Dec 2025) headline UI change: a redesigned Play View, "a highly visual workspace focused on real-time feedback and hands-on sound manipulation". Macros are "more integrated into the interface and… visually redesigned" — [Mixdown](https://mixdownmag.com.au/news/arturia-launches-pigments-7-redesigned-interface-and-powerful-new-sound-shaping-tools/); [Synth Anatomy review](https://synthanatomy.com/2025/12/arturia-pigments-7-review-a-character-upgrade-for-the-flagship-synthesizer-plugin.html)
- Pigments 7 also added in-app tutorials and 150 presets — [KVR news](https://www.kvraudio.com/news/arturia-releases-pigments-7-65693)
- Analog Lab: Library View is the default (browse by type, instrument, soundbank, sound designer; sidebar with Home, banks, Sound Store, Playlists). Studio View is for mixing, panning and adjusting presets and effects. Stage View is for live performance, recalling sounds and multis via program change — [Analog Lab V manual](https://dl.arturia.net/products/analoglab-v/manual/analog-lab-v_Manual_1_0_EN.pdf)
- Analog Lab Pro Studio view covers "effects and combining two instruments" — [Arturia Analog Lab Pro page](https://www.arturia.com/products/software-instruments/analoglab/overview)
- In late 2023 Analog Lab V was renamed Analog Lab Pro ($199, 2,000+ presets), and an update gave it the simplified "Play" view. The free Analog Lab Play launched at the same time — [MusicRadar](https://www.musicradar.com/news/arturia-analog-lab-play-free-plugin)

### Inferences
- All three converge on the same split: a simple "play" surface (Pigments Play, Analog Lab Main/Play, Serum's main OSC page plus macros) and deeper edit pages behind tabs. For a trap preset instrument, the pattern to copy is "Play page by default, plus an Edit/FX/ARP tab".
- Serum 2's MIX page is a routing overview. A preset player probably needs only a simplified signal-flow strip (Sound → FX → Out), not a full matrix.

### Gaps
- I couldn't confirm the exact Serum 2 tab labels (e.g. "MATRIX" vs "MOD", or whether ARP and CLIP are top-level tabs or panels). The official "What's New" PDF (https://static.xferrecords.com/Serum%202%20What's%20New.pdf) was found but couldn't be fetched.
- I couldn't fetch the Analog Lab Pro (2023+) manual for the "Edit"/instrument-panel tab. (Unverified, from prior knowledge: owners of the full V Collection can open the full instrument editor from Analog Lab.)

## Q2. Preset browser

### Takeaway
Pigments and Analog Lab have full tag browsers: Types (instrument category), Styles (character tags), Genres (Analog Lab), Banks, Playlists, a Sound Store, a preset-info panel and favorites. Serum 2's browser has search, category/tag filters, 1–5 star ratings, favorites and free-form comma-separated tags. Ratings now survive rescans and import from Serum 1.

### Cited Findings
- Pigments browser has four areas: search and results; a sidebar for banks, Playlists and Arturia Sound Store purchases; a Preset Info panel; and a Quick Graphics panel. All views draw on the same preset banks — [Pigments 5 manual](https://dl.arturia.net/products/pigments/manual/pigments_Manual_5_0_0_EN.pdf)
- Pigments uses two tag kinds, Types and Styles. The Styles button opens three further subdivisions, and Bank selection sits next to Types/Styles. The save dialog sets Author, Bank and Type, adds tags, and can create your own Bank, Type and Styles — [Pigments manual 3.5](https://dl.arturia.net/products/pigments/manual/pigments_Manual_3_5_0_EN.pdf); [Pigments 5 manual](https://dl.arturia.net/products/pigments/manual/pigments_Manual_5_0_0_EN.pdf)
- Pigments 6 improved the file browser — [Stars Music listing](https://www.stars-music.com/arturia-pigments-6-box_219884.html)
- Pigments has a sequencer lock and a sequencer-preset browser (load a sequence onto any sound) — [KVR Pigments 7 listing](https://www.kvraudio.com/marketplace/pigments-7-by-arturia)
- Analog Lab Pro browser: "find sounds quickly: by Type (Bass, Pads, Leads…), by Style (bright, dark…), by Genre (Ambient, Disco, Techno…)" — [Arturia Analog Lab Pro page](https://www.arturia.com/store/analoglab-v)
- Analog Lab Library view filters by type, instrument, soundbank and sound designer, with sidebar Home, banks, Sound Store and Playlists — [Analog Lab V manual](https://dl.arturia.net/products/analoglab-v/manual/analog-lab-v_Manual_1_0_EN.pdf)
- Serum: each preset can be rated 1–5 stars, which can serve as favorites — [Gearspace thread](https://gearspace.com/threads/can-you-tag-favorites-in-serum.1344551/)
- Serum 2 changelog: preset ratings and comments are "now preserved forever", even when presets are removed and the browser is rebuilt. Serum 1 rating imports run on each rescan — [Serum changelog gist](https://gist.github.com/0xdevalias/a537a59d1389d5aed3bc63b544c70c8d)
- Serum 2 tags are free-form, comma-separated descriptors, and the TAGS filter is "only as good as the tags people typed" — [monosounds: saving presets in Serum 2](https://monosounds.studio/how-to-save-presets-in-serum-2/)
- Serum 2 browser supports search, category/tag filtering and favorites — [EDMProd Serum 2 presets](https://www.edmprod.com/serum-2-presets/); [r-loops Serum 1 vs 2](https://r-loops.com/blog/serum-1-vs-serum-2-differences-preset-compatibility-upgrade-guide/)
- Serum 2 macro names are stored in the preset file and appear in the browser — [monosounds macros tutorial](https://monosounds.studio/serum-2-macros-tutorial/)

### Inferences
- Industry-standard browser fields: Type/Category (single), Style/character tags (multi), Genre (Analog Lab only), Bank/Pack, Author/Designer, Favorite (heart) or Rating (stars), and free-text search. A trap-oriented instrument should add genre/sub-genre tags (trap, drill, rage, plugg…) and key/BPM fields where relevant.
- (Unverified, from prior knowledge) All three put the preset name in the header with prev/next arrows for click-through auditioning without opening the browser. Pigments and Analog Lab use a heart for "liked". Analog Lab shows a large instrument photo per preset.
- Free-form tags (Serum) cause inconsistent filtering. A fixed tag vocabulary (Arturia) is better for a curated bank.

### Gaps
- I found no source on audio preview or demo snippets in any of the three browsers. Auditioning appears to be play-with-MIDI after a click-load, but this is unverified.
- I couldn't confirm whether Serum 2 has a dedicated favorite flag separate from star ratings.

## Q3. Main/performance view and macros

### Takeaway
Serum 1 had 4 macros; Serum 2 has 8, assigned by dragging the macro knob onto a parameter, with names saved per preset. Pigments has 4 macros, visible in every view, and Pigments 7 redesigned them into the Play view. Analog Lab Pro has 4 macros per preset (brightness / timbre / time / movement style), which can be renamed by double-click in a macro settings window.

### Cited Findings
- Serum 1 modulation panel: 4 macros, 3 envelopes, 4 LFOs — [Sleepfreaks Serum 1 basics](https://sleepfreaks-dtm.com/en/softsynth/xfer-records-serum-1)
- Serum 2 has eight macros (up from four). To assign one, drag the macro knob onto any parameter and release when the "+" appears. Names are stored in the preset and shown in the browser, the MACROS panel and as DAW parameter names — [monosounds](https://monosounds.studio/serum-2-macros-tutorial/); [Xfer What's New PDF (search extract)](https://static.xferrecords.com/Serum%202%20What's%20New.pdf)
- Pigments has "4 dynamic, assignable Macro controls" — [Adorama](https://www.adorama.com/ar240601down.html); [Sound On Sound](https://www.soundonsound.com/reviews/arturia-pigments)
- Pigments macros appear in every view, not only Play — [Pigments manual](https://dl.arturia.net/products/pigments/manual/pigments_Manual_5_0_0_EN.pdf)
- Pigments 7 visually redesigned the macros and integrated them more tightly into the Play view — [Mixdown](https://mixdownmag.com.au/news/arturia-launches-pigments-7-redesigned-interface-and-powerful-new-sound-shaping-tools/)
- Analog Lab Pro: "4 new Macro controls on all the sounds for instant sound edition" — [Arturia](https://www.arturia.com/store/analoglab-v)
- Analog Lab macros are described as controlling "brightness, timbre, time, and movement" (marketing copy) — [Thomann](https://www.thomann.co.uk/arturia_analog_lab_v.htm)
- Analog Lab macro names can be changed by double-clicking the name box in the macro settings window (gear icon → Macros). Which parameters are assignable depends on whether you own the full V Collection — [Gearspace Analog Lab thread](https://gearspace.com/threads/arturia-analog-lab.1420050/); [KVR thread](https://www.kvraudio.com/forum/viewtopic.php?t=603176&start=15)
- Analog Lab release notes (quoted on KVR): "macros are now correctly named on all presets", and renaming a macro now updates the control panel — [KVR thread](https://www.kvraudio.com/forum/viewtopic.php?t=603176&start=15)

### Inferences
- 4 macros is the Arturia norm for preset players. 8 is Serum 2's sound-designer standard. For click-and-play trap users, 4–8 labelled macros with preset-specific names (not "Macro 1") is the expected baseline. Wrong or unlabelled macro names were a visible bug that Arturia had to fix.

### Gaps
- I didn't find a description of the exact Pigments 7 Play View layout (e.g. whether it shows a waveform/visualiser plus macros plus key params).

## Q4. Modulation (LFOs, envelopes, drag-and-drop, matrix, visualisation)

### Takeaway
Serum 2 has up to 10 LFOs with drawable shapes and modes (envelope one-shot, chaos Lorenz/Rössler, S&H, path), 4 envelopes, drag-and-drop assignment and a matrix with reorderable, bypassable slots, live curves and a full-screen mode. Serum 1 had 4 LFOs and 3 envelopes. Pigments has a bottom modulation strip with 3 envelopes, 3 LFOs, 3 functions, 3 random sources, 4 macros and combinators. Sources animate live, unused ones stay dark, and drag-and-drop shows colour-coded rings (colour-coded paths in v7).

### Cited Findings
- Serum 2 has up to 10 LFOs and eight macros — [Xfer What's New (search extract)](https://static.xferrecords.com/Serum%202%20What's%20New.pdf); [unison.audio](https://unison.audio/xfer-serum-2/)
- Serum 2 LFO modes include an envelope-style one-shot (plays the drawn shape once per note), Chaos (Lorenz, Rössler), Random S&H and Path — [monosounds LFO modes](https://monosounds.studio/serum-2-lfo-modes/); [unison.audio](https://unison.audio/xfer-serum-2/)
- Serum 2 has 4 envelopes with BPM and invert/legato options (Serum 1 had 3) — [Serum 2 manual excerpt](https://images.equipboard.com/uploads/item/manual/127411/xfer-records-serum-2-advanced-wavetable-synthesizer-manual.pdf); [Serum 1 manual](https://s3.amazonaws.com/decembercymatics/Serum_Manual.pdf)
- Serum 2 matrix: drag to reorder slots, bypass individual mods, expanded full-screen mode, curves and live values per LFO/envelope. Aux source scales a mod — [The Producer School](https://theproducerschool.com/blogs/featured-blogs/serum-2-synth-plugin-features-review-free-presets-download); [Sonic Weaponry](https://sonic-weaponry.com/blogs/free-production-tutorials-and-resources/serum-2-released)
- Pigments mod sources: three envelopes, three LFOs, three Functions (multi-point tempo-synced envelopes), three random sources (Turing, S&H, Binary), four macros, plus "Combinate" math sources — [Sound On Sound](https://www.soundonsound.com/reviews/arturia-pigments)
- Pigments LFOs morph smoothly between shapes, all the way into S&H randomizers — [AudioTechnology](https://www.audiotechnology.com/reviews/arturia-pigments)
- Pigments visualisation: "whatever is happening always on show and animated". Unused sources stay dark, and a used source shows an animated colourful scope. Clicking a block label turns the strip into an assignment window — [AudioTechnology](https://www.audiotechnology.com/reviews/arturia-pigments); [Databroth](https://www.databroth.com/blog/pigments-review)
- Pigments Synth view uses drag-and-drop mod routing ("easy to animate almost any parameter") — [MusicTech Pigments 7 review (extract)](https://musictech.com/reviews/software-instruments/arturia-pigments-7-review/)
- Pigments 7 adds voice-randomization modulators, an envelope follower, enhanced function generators and colour-coded modulation paths — [MusicTech Pigments 7 review (extract)](https://musictech.com/reviews/software-instruments/arturia-pigments-7-review/)
- Criticism: Pigments envelopes don't restart from the current level between adjacent notes (minor). Advanced modulation needs more technical understanding — [Synth and Software Pigments 5 review](https://synthandsoftware.com/2024/04/arturia-pigments-5-0-polychrome-software-synthesizer-the-synth-and-software-review/); [gearnews](https://www.gearnews.com/arturia-pigments-a-swirling-polyrhythmic-modulating-and-randomising-mega-synth/)

### Inferences
- (Unverified, from prior knowledge) Both Serum and Pigments show modulation depth as a coloured arc or ring around the target knob, and a moving dot or indicator for the live value. This is the de-facto visual language users expect.
- For a preset player, 2–4 LFOs with a few shapes, tempo sync and a drag-to-assign handle would cover "quick tweaking". Drawable shapes plus 10 LFOs is sound-designer territory.

### Gaps
- I didn't confirm Serum 2's default LFO count visible on screen (the commonly cited figure is "up to 10" LFOs, with fewer initially shown).

## Q5. Effects rack

### Takeaway
Serum 2 has 13 FX modules plus 3 splitter modules (L/H, L/M/H, M/S), routed across MAIN, BUS 1 and BUS 2 lanes from the MIX page. It allows multiple instances of the same effect and drag-and-drop module reordering. New effects include Convolve (convolution reverb), Bode (frequency shifter), Utility and new reverb types. Pigments has two insert buses (A, B) plus an Aux send bus, each with 3 slots, switched by vertical tabs. FX are dragged between slots (dragging onto an occupied slot swaps them), with A>B, B>A or A//B routing and about 18–20 FX types.

### Cited Findings
- Serum 2: 13 effects and 3 splitter modules across multiple busses. Improvements include drag-and-drop module rearranging and two separate FX busses — [Xfer What's New (search extract)](https://static.xferrecords.com/Serum%202%20What's%20New.pdf)
- Serum 2 lanes are MAIN, BUS 1 and BUS 2. Example routing: OSC A+B → Filter 1 → Main FX, and OSC C → Filter 2 → Bus 1 — [EDMProd](https://www.edmprod.com/serum-2-guide/); [CFA-Sound](https://www.cfa-sound.com/xfer-serum-2-review-and-insights/)
- Splitters are L/H, L/M/H and M/S. Multiple instances of the same effect are allowed — [Splice](https://splice.com/blog/serum-2-advanced-features/); [CDM](https://cdm.link/serum-2/)
- Source conflict: Dubspot describes "dual FX buses" in series or parallel, while CDM describes "three effects channels". These reconcile as Main plus 2 buses — [Dubspot](https://blog.dubspot.com/xfer-records-releases-serum-2); [CDM](https://cdm.link/serum-2/)
- New Serum 2 FX include Bode (frequency shifter), Convolve (convolution reverb) and Utility, plus reverb types Vintage, Nitrous and Basin — [RouteNote](https://create.routenote.com/blog/serum-2-everything-you-need-to-know/); [Splice](https://splice.com/blog/serum-2-advanced-features/)
- Serum 2 FX is also available as a standalone FX plugin, and "Hyper/Dimension" is described as being for massive stereo width — [Slooply](https://slooply.com/blog/how-to-use-serum-effects-on-vocals-drums-in-fl-studio-serum-2-fx-guide/)
- MusicRadar: "the effects rack in particular can feel a little cramped". The reviewer would like options to hide or resize elements — [MusicRadar Serum 2 review](https://www.musicradar.com/music-tech/plugins/a-copy-of-serum-might-be-the-smartest-investment-any-budding-producer-makes-xfer-serum-2-review)
- Pigments FX: vertical tabs for Bus A, Bus B and Aux Bus, each with up to 3 effects. You drag FX between slots in the bus overview on the left, and dragging onto an occupied slot swaps them. Insert routing is A>B, B>A or A//B (serial modes only when filter routing is Pre-FX Sum). The Send feeds the Aux directly — [Arturia support: Pigments Effects](https://support.arturia.com/hc/en-us/articles/4405741074962-Pigments-Effects); [Pigments 7 manual](https://dl.arturia.net/products/pigments/manual/pigments_Manual_7_0_1_EN.pdf)
- Pigments FX types include filter, EQ, compressor, distortion, bit-crusher, chorus, flanger, phaser, auto-pan, reverb, delay, Pitch-Shift Delay, Jun-6 Chorus, Multiband Compressor, BL-20 Flanger, Shimmer Reverb and Vocoder (v6). Pigments 7 added Corroder and Reverb/Ripple/Rage filters. The count is 18 in older docs and 20 on current product pages — [SOS](https://www.soundonsound.com/reviews/arturia-pigments); [Arturia Pigments](https://www.arturia.com/products/software-instruments/pigments/overview); [Mixdown](https://mixdownmag.com.au/news/arturia-launches-pigments-7-redesigned-interface-and-powerful-new-sound-shaping-tools/)
- Analog Lab Studio view handles effects and signal-chain order — [Analog Lab V manual](https://dl.arturia.net/products/analoglab-v/manual/analog-lab-v_Manual_1_0_EN.pdf); [Arturia](https://www.arturia.com/products/software-instruments/analoglab/overview)

### Inferences
- (Unverified, from prior knowledge) Serum 1's FX page had about 10 modules (Hyper/Dimension, Distortion, Flanger, Phaser, Chorus, Delay, Compressor, Reverb, EQ, Filter), each with a power toggle on its tab and drag-to-reorder in a vertical list, plus a per-module Mix knob. This is a widely copied "fixed module list + on/off + reorder" pattern.
- A standard FX set for preset players is reverb, delay, chorus, distortion/saturation, filter, compressor, EQ, plus a stereo widener (hyper/dimension). Each needs on/off, mix (wet/dry) and 1–3 key knobs. Fixed slots with drag-reorder (Pigments, 3 slots per bus) look simpler than Serum's long list.

### Gaps
- I couldn't confirm Serum 2's full list of 13 FX names or whether a per-FX preset menu exists. (Unverified, from prior knowledge: Serum 1 had FX-rack presets saved separately.)
- No source found for Analog Lab's FX slot count.

## Q6. Arp / sequencer

### Takeaway
Serum 2 has an arpeggiator and a piano-roll clip sequencer, each with 12 slots launchable from keyboard keys (like Ableton clips), MIDI import, real-time recording and banks. Pigments has a 16-step sequencer / arp with lanes for pitch, octave, velocity, probability, gate and slide, plus polyrhythm (per-lane length), randomisation and a sequence browser/lock. Analog Lab's arp exists only where the source instrument has one.

### Cited Findings
- Serum 2: each of the 12 clips in the ARP and CLIP sections can be triggered via the respective ARP or CLIP keyboards (Ableton-like clip launching), and saved individually or as an ARP/CLIP bank. The clip sequencer is a full piano roll with real-time MIDI recording and MPE editing — [MusicTech (search extract)](https://musictech.com/reviews/software-instruments/xfer-records-serum-2-review)
- The clip sequencer has 12 slots. Set the trigger octave on the CLIP label, and C1 then plays clip 1, C#1 clip 2. Loop length is adjustable, and you can record, quantise and import MIDI — [EDMProd clip sequencer guide](https://www.edmprod.com/serum-2-clip-sequencer/); [Dubspot](https://blog.dubspot.com/xfer-records-releases-serum-2)
- Serum 2 arp modes include up, down, up/down and converge/diverge. A custom pattern editor uses numbered rows to select notes from the input chord. You can store 12 arp patterns and switch them live — [EDMProd arp guide](https://www.edmprod.com/serum-2-arpeggiator/); [RouteNote](https://create.routenote.com/blog/serum-2-everything-you-need-to-know/)
- Step-limit conflict: one blog claims 64 steps and another 16, while EDMProd describes free-length piano-roll loops — [Metalverse](https://www.themetalverse.net/xfer-serum-review/) vs [EDMProd](https://www.edmprod.com/serum-2-clip-sequencer/)
- Pigments: 16-step sequencer with lanes for pitch, octave, velocity, probability, gate length and slide. Polyrhythm mode gives each lane its own length, per-row loop points and "intelligent randomising" — [Sound On Sound](https://www.soundonsound.com/reviews/arturia-pigments); [CDM Pigments 5](https://cdm.link/arturia-pigments-5-hands-on/)
- Pigments arp mode uses all lanes except pitch, which comes from held keys — [Pigments manual](https://dl.arturia.net/products/pigments/manual/pigments_Manual_5_0_0_EN.pdf)
- Pigments sequencer presets can be saved and browsed, and a "sequencer lock" keeps the sequence while changing sounds — [KVR Pigments 7](https://www.kvraudio.com/marketplace/pigments-7-by-arturia)

### Inferences
- Lessons for trap: the Pigments-style lanes (gate, velocity, probability, slide) map directly to "arp/gate" tweaking. A sequencer lock (keep the pattern, swap the sound) is a high-value feature for click-and-play browsing.
- Serum 2's key-triggered clips resemble trap "loop starter" workflows, but the piano roll adds complexity.

### Gaps
- I found no reliable details on Analog Lab Pro's own arp or sequencer. (Unverified: it appears to depend on the underlying V Collection instrument.)

## Q7. Window sizing / scaling

### Takeaway
Serum 2 is resizable (reportedly up to 200%), but the skins only have 100% and 200% bitmaps, so text is blurry at intermediate zooms. This is a recurring KVR complaint. Serum 1 started with a fixed, small GUI, and resizing was added later after high-DPI complaints.

### Cited Findings
- Serum 2 UI is reportedly resizable up to 200% (single source) — [unison.audio](https://unison.audio/xfer-serum-2/)
- In the KVR Official Serum 2 thread, users report blurry text at zoom levels other than 100% or 200%. The developers say skins contain graphics only for 100% and 200%, so perfect sharpness is impossible elsewhere. Users ask why text isn't rendered dynamically — [KVR Serum 2 thread p.59](https://www.kvraudio.com/forum/viewtopic.php?t=619315&start=870)
- Serum 2.0.24 fixed UI sizing in JUCE hosts when Windows display scaling isn't 100%, but some blurry fonts remained (Dec 2025 post) — [Serum changelog gist](https://gist.github.com/0xdevalias/a537a59d1389d5aed3bc63b544c70c8d); [KVR thread](https://www.kvraudio.com/forum/viewtopic.php?t=619315&start=870)
- Serum 1: user threads on the GUI being too small on hi-res screens. The developer announced a resizable-UI update — [Xfer forum: resizable GUI](https://xferrecords.com/forums/general/serum-resizable-gui); [Xfer forum: hi-res screens](https://xferrecords.com/forums/general/serum-on-hi-res-screens)

### Inferences
- Lesson: render text and vector elements dynamically (or ship multiple bitmap scales) so any zoom percentage stays sharp. Offer resize steps or free resize with a remembered size.

### Gaps
- I found no default pixel dimensions for any of the three plugins. (Unverified, from prior knowledge: Serum 1's default was about 1000×700 px, and Arturia plugins are freely resizable by dragging a corner.)

## Q8. User praise and complaints about the UI

### Takeaway
Serum gets praise for its sleek, navigable UI. Complaints are about a cramped FX rack, blurry scaled text, a steeper learning curve in v2 and CPU use. Pigments gets praise for animated, visual modulation and drag-and-drop. Complaints are a learning curve for advanced modulation, and v7 was seen as a small update. Analog Lab's 2023 visual "Main" view was criticised as pretty but useless, hiding the browser behind a small button.

### Cited Findings
- MusicRadar on Serum 2: the UI is "sleeker and easier to navigate", the effects rack "can feel a little cramped", and the reviewer wants options to hide or resize elements. "Hard to find much in the way of faults" — [MusicRadar](https://www.musicradar.com/music-tech/plugins/a-copy-of-serum-might-be-the-smartest-investment-any-budding-producer-makes-xfer-serum-2-review)
- Dubspot: the "gentle on-ramp got a little steeper", newcomers will spend more time in the manual, and the granular/spectral oscillators are CPU-hungry — [Dubspot](https://blog.dubspot.com/xfer-records-releases-serum-2)
- MusicRadar synth showdown: Serum's included patch library "seems less generous than some" — [MusicRadar](https://www.musicradar.com/music-tech/soft-synth-showdown-repub)
- Pigments: praise for the animated, always-visible modulation ("wonderfully visualised") and simple function editing — [AudioTechnology](https://www.audiotechnology.com/reviews/arturia-pigments); [Databroth](https://www.databroth.com/blog/pigments-review)
- Pigments 7 felt "cleaner and more like a virtual instrument that you enjoy tweaking" — [Synth Anatomy](https://synthanatomy.com/2025/12/arturia-pigments-7-review-a-character-upgrade-for-the-flagship-synthesizer-plugin.html)
- Pigments 7 got 9/10 from Audiofanzine, which noted "fewer new features than previous updates" and possible changes to old sounds — [Audiofanzine](https://en.audiofanzine.com/virtual-hybrid-synth/arturia/pigments-7/editorial/reviews/arturia-pigments-7-test.html)
- Analog Lab Pro on KVR: the new Main view is "gorgeous but functionally useless", and you can't see anything else until you click a small library button in the header. Clicking that brings back the "old Analog Lab" — [KVR thread](https://www.kvraudio.com/forum/viewtopic.php?t=603176)
- Analog Lab Pro rated 4.8 from 637 customer reviews on Plugin Boutique (customer, not press) — [Plugin Boutique](https://www.pluginboutique.com/products/7149-Analog-Lab-Pro)

### Inferences
- Design takeaways: (1) keep the browser one click away or always visible beside the play surface, since Analog Lab's hidden-browser backlash shows this matters; (2) avoid cramming the FX rack and allow collapsing; (3) make modulation visible and animated; (4) keep text sharp at all scales.

### Gaps
- I found no Sound On Sound Serum 2 review, and no Reddit threads came up in search results.
