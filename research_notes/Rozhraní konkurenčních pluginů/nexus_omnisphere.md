# UI of reFX Nexus (4/5) and Spectrasonics Omnisphere (2.8, with notes on 3.0)

> Method note: refx.com, support.spectrasonics.net, gearnews, synthanatomy and edmprod could NOT be fetched directly from this environment (DNS/proxy 403). All findings below come from search-engine extracts of those pages (official manuals, reviews, KVR threads). Facts are cited to the page the extract came from. Items marked "(background, unverified)" in Inferences are from prior general knowledge and should be checked before relying on them.
>
> Version context (Oct 2026): Nexus current = NEXUS5 (released Black Friday 2024; 5.3.x updates Aug–Oct 2025; version history lists 5.5.19). Omnisphere 3 was released Oct 2025, so Omnisphere 2.8 is now the previous generation; Omnisphere 3 notes are included where relevant.

## 1. Page/tab structure

### Takeaway
Nexus uses a single window with a large preset browser/librarian plus a row of tabs for Modulation, Arpeggiator, Trance Gate, Mixer/routing and FX; Omnisphere uses a header with patch name/browser, a "Main" dashboard page (patch-wide controls + Orb), per-layer "Edit"/Layer pages (A–D), an FX page, an Arp page, and multi-timbral pages (Mixer, Live, Stack) for 8 Parts.

### Cited Findings
**Nexus**
- NEXUS manual "First Steps" lists views including Modulation (vibrato, portamento, mod matrix, LFOs), Arpeggiator ("arp": edit patterns, load arp presets), Trance Gate (sequenced audio gate; edit or load preset patterns) and a Mixer ("mix") view. — [NEXUS2 manual, First Steps](https://refx.com/nexus/manual/2)
- NEXUS5 is no longer just a ROMpler: "full-fledged synthesizer" with VA, sampler, time-stretcher, wavetable, retro, grain, cloud and FM sources and a "fully accessible modular engine". — [Synth Anatomy](https://synthanatomy.com/2024/11/refx-nexus-5-the-synth-rompler-days-will-end-on-black-friday.html)
- reFX describes NEXUS5 as keeping "high-quality playback engine, extensive preset browser and lightning-fast, intuitive sound control", with a "sleek new interface", "fully customizable librarian", and an "N2 Retro Mode" that brings back the NEXUS2 look. — [NEXUS5 manual (refx.com)](https://refx.com/nexus/manual/)
- NEXUS5 has a routing page; changelog 5.1.x "Add icons and activity meter to layers on routing page." — [KVR NEXUS5 thread](https://www.kvraudio.com/forum/viewtopic.php?t=615870&start=105)
- Signal flow (NEXUS4): generator → layer → effects group → main effects → output. — [refx nexus 4 article, EDMProd](https://www.edmprod.com/refx-nexus-4/); [NEXUS4 manual](https://refx.com/nexus/manual/4)
- NEXUS5 manual: up to 16 parallel effects chains plus one global effect chain. — [NEXUS5 manual](https://refx.com/nexus/manual/)

**Omnisphere 2.8**
- Main Page = "dashboard": quick patch changes and performance controls; parameters apply to all four Layers. — [The Main Page – Omnisphere 2.8](https://support.spectrasonics.net/manual/Omnisphere2/25/en/topic/main-index)
- Layer Pages: oscillators, filters, LFOs, envelopes, modulation routing per layer (A–D). — [The Layer Pages – 2.8](https://support.spectrasonics.net/manual/Omnisphere2/25/en/topic/layer-page-index)
- Manual sections include Main Page, The Orb, Interface Conventions, Live and Stack mode indicators, Latch and Trigger modes. — [Omnisphere 2 manual](https://support.spectrasonics.net/manual/Omnisphere2/25/en/topic/supported-hardware-synths)
- Mixer page: overview of all 8 Parts of a Multi in an 8-channel mixer layout; per Part: MIDI channel, output channel, Mute/Solo, patch name, level, pan, 4 Aux sends. — [The Mixer – 2.8](https://support.spectrasonics.net/manual/Omnisphere2/25/en/topic/mixer-page-index)
- Live Mode page has 8 slots, one per Part; Stack Mode builds splits, layers and crossfades across up to 8 Parts, switchable/crossfadable via MIDI CC. — [Andertons product page](https://www.andertons.co.uk/recording/software/virtual-instruments-softsynths/spectrasonic-omnisphere-2-psychoacoustic-virtual-synth)
- Omnisphere 1 Main page layout: Part-wide controls on left and right, layer-specific controls in the centre; Main page has Solo modes, Glide and Arp on/off. — [Omnisphere (v1) Main Controls](https://support.spectrasonics.net/manual/Omnisphere/main/controls/index.html); [Arpeggiator](https://support.spectrasonics.net/manual/Omnisphere/main/controls/page10.html)
- Omnisphere 3 Main page: Global controls for Filter, Envelope, Unison; Part Level and Mute/Solo; Voices/Octave, Scale, Clock Speed; Arpeggiator on its own page. — [Main Page – Omnisphere 3](https://support.spectrasonics.net/manual/Omnisphere3/30/en/topic/main-index)
- Omnisphere 3 adds "Adaptive Global Controls" and "Quadzone" (map up to four layers across the keyboard) on the Main tab's Global page. — [Synth and Software review](https://synthandsoftware.com/2025/11/spectrasonics-omnisphere-3-the-synth-and-software-review/)

### Inferences
- Both plugins separate "play" (browser + a few global controls) from "edit" (layer pages). Nexus keeps the browser dominant; Omnisphere keeps the patch-wide dashboard dominant and the browser is opened on demand (full browser) or used as a mini-browser.
- (background, unverified) Omnisphere's header tab row is approximately: MAIN | A | B | C | D (layer edit) | ARP | FX | plus multi pages (MIXER, LIVE, STACK) and a SYSTEM/utility page; parts 1–8 are selected via a numbered strip at the top.

### Gaps
- Exact current NEXUS5 tab labels and order could not be confirmed from the primary manual (fetch blocked).
- Default window pixel size for either plugin not found.

## 2. Preset browser

### Takeaway
Both are tag/attribute-driven with favourites and search; Nexus5 uses a two-column librarian with colour tags, heart favourites and counters; Omnisphere has a Full Browser (Attributes vs File Directory modes), a Mini-Browser, Sound Match (similar sounds ranked by tag analysis) and Sound Lock (hold arp/FX etc. while browsing).

### Cited Findings
**Nexus**
- NEXUS3 introduced a "Category" browser section; colour-coded tags, bookmarks, favourites and a dedicated location for user presets were added. — [NEXUS3 manual](https://refx.com/nexus/manual/3); [Synth Anatomy on Nexus 3](https://synthanatomy.com/2019/11/refx-intros-nexus-3-a-redesign-with-many-new-features.html)
- NEXUS5 uses a two-column librarian with tags, and counters for folders, categories and presets. Heart icon marks favourites; a heart filter shows only favourites; expansions/folders can be favourited and move to their own section at the top of the folders list. — [NEXUS5 manual](https://refx.com/nexus/manual/)
- NEXUS5 Starter edition contains 5,454 presets; 52 trancegate presets. — [Dubspot Nexus 5 page](https://blog.dubspot.com/plugins/nexus-5)

**Omnisphere 2.8**
- Full Browser filters work in two modes: Attributes and File Directory. — [Browser Filters – 2.8](https://support.spectrasonics.net/manual/Omnisphere2/25/en/topic/browser-operation-page04)
- Search field, Results and Info pane, auditioning sounds, marking sounds (favourites/marks) are separate manual topics. — [Search Field](https://support.spectrasonics.net/manual/Omnisphere2/25/en/topic/browser-operation-page03); [Results and Info](https://support.spectrasonics.net/manual/Omnisphere2/25/en/topic/browser-mini-page05); [Auditioning Sounds](https://support.spectrasonics.net/manual/Omnisphere2/25/en/topic/browser-operation-page05a); [Marking Sounds](https://support.spectrasonics.net/manual/Omnisphere2/25/en/topic/browser-operation-page05d)
- Sound Match: finds similar sounds by analysing every sound's tags; matches ranked with "strength" shown as a solid bar right of the patch name; selecting it hides browser filters. — [Sound Match – 2.8](https://support.spectrasonics.net/manual/Omnisphere2/25/en/topic/browser-sound-match-page01)
- Sound Lock: lock icon top-right of Full Browser and Mini-Browser; holds a section (e.g. arpeggiator or effects) while browsing other patches. — [Sound Lock – 2.8](https://support.spectrasonics.net/manual/Omnisphere2/25/en/topic/browser-sound-lock-page01)
- Soundsource browser opened from Main page: select LAYERS in central Info section and click the image. — [Soundsource Browser – 2.8](https://support.spectrasonics.net/manual/Omnisphere2/25/en/topic/browser-overview-page09)
- Omnisphere 3: redesigned Full Browser with directory tree, sub-categories, "Hide" option, simplified tagging with keywords and moods. — [Synth Anatomy (Omni 3)](https://synthanatomy.com/2025/10/spectrasonics-omnisphere-3-flagship-synthesizer-plugin.html)

### Inferences
- Sound Lock (keep my arp/FX while browsing) and Sound Match (more like this) are high-value, low-complexity features to copy in a trap preset instrument.
- (background, unverified) Both plugins have prev/next arrows beside the patch name in the header for one-click stepping, and Omnisphere's attribute browser has columns like Category / Type / Genre / Mood / Complexity / Timbre plus star ratings.

### Gaps
- Exact column names in NEXUS5 librarian and Omnisphere attribute columns not confirmed from primary sources.

## 3. Main/performance page: macros and master controls

### Takeaway
Nexus5 shows 4 macro knobs at the bottom of the window (assigned via the mod matrix) plus quick global sound controls (historically filter/amp modifiers, delay, reverb, output); Omnisphere's Main page is a patch-wide dashboard (global filter, envelope, unison, glide, level) plus the Orb XY "performance" control with Dice/Depth/Inertia.

### Cited Findings
- NEXUS5: four quick-access macro controls at the bottom of the window, mapped via Modulation Matrix (MOD tab: select Macro 1–4 as source, enable, pick destination, set amount). Multiple parameters per macro (since NEXUS3); macros automatable in DAW. — [NEXUS5 manual](https://refx.com/nexus/manual/); [Sleepfreaks Nexus 3 tutorial](https://sleepfreaks-dtm.com/en/softsynth/refx-nexus3-new-features/)
- NEXUS2 front panel had six sections: Filter Modifier, Amp Modifier, Master Filter, Delay, Reverb, Output. NEXUS3 side-panel sections: FILTER MOD (LowPass), DELAY, FILTER, AMP MOD, REVERB. — [NEXUS2 manual](https://refx.com/nexus/manual/2); [Sleepfreaks Nexus 3](https://sleepfreaks-dtm.com/en/softsynth/refx-nexus3-new-features/)
- Omnisphere Main page parameters affect the entire patch (all 4 layers). — [Main Page – 2.8](https://support.spectrasonics.net/manual/Omnisphere2/25/en/topic/main-index)
- Orb: DICE creates a new group of "intelligent sonic modifications and effects", different each press; DEPTH sets intensity; INERTIA keeps the cursor moving after release (mouse/touch only, not MIDI); Attractor; Orb can be a mod source (manual routing overrides Dice). — [Dice and Depth](https://support.spectrasonics.net/manual/Omnisphere2/25/en/topic/orb-page00); [Inertia and Attractor](https://support.spectrasonics.net/manual/Omnisphere2/25/en/topic/orb-page00b); [Orb as Mod Source](https://support.spectrasonics.net/manual/Omnisphere2/25/en/topic/orb-page07)
- Omnisphere 3 Main page makeover: "immediate addressing of the basics", filter and envelope available right away; previously global control was "sometimes difficult to access". — [MusicRadar Omnisphere 3 review](https://www.musicradar.com/music-tech/soft-synths/spectrasonics-omnisphere-3-review)

### Inferences
- The industry-standard "after load" view = patch name + prev/next + 4–8 macro-type controls + global filter/envelope + FX send knobs. The Omnisphere 3 redesign confirms users wanted global filter/ADSR directly on the first page.
- (background, unverified) Omnisphere 2 Main page shows per-layer level/pan/mute for A–D, global Filter Cutoff/Resonance, ADSR "Attack/Release" offsets, Unison, Glide, and 4 FX Aux send knobs; Nexus macros are user-renameable.

### Gaps
- Whether NEXUS5 macro labels are renamable per preset not confirmed.

## 4. Edit: layers and per-layer controls

### Takeaway
Omnisphere: fixed 4 layers (A–D) per Part, 8 Parts per Multi; per-layer oscillator, filter, amp/filter envelopes, FX rack. Nexus 4/5: generators grouped into up to 16 layers, each with volume, pan, mute, solo, arp mode, own FX group/mod matrix.

### Cited Findings
- Omnisphere: each Part has four layers A–D; numbered items (LFOs 1–8, Mod Envelopes 1–4) are common to all layers; unnumbered items (Amp and Filter envelopes) are per-layer. — [Layers – 2.8](https://support.spectrasonics.net/manual/Omnisphere2/25/en/topic/layers); [Layer Page Basics](https://support.spectrasonics.net/manual/Omnisphere2/25/en/topic/layer-page-concepts-index)
- Layers can be selected, linked and soloed; Layer FX rack is not linked when layers are linked. Initialize Layer resets routings and FX. — [Selecting, Linking, Soloing Layers](https://support.spectrasonics.net/manual/Omnisphere2/25/en/topic/layer-page-concepts-page00a); [Clear, Save, Copy, Paste](https://support.spectrasonics.net/manual/Omnisphere2/25/en/topic/concepts-header-page03)
- Envelope editing controls exist as a dedicated topic (zoom envelope editor). — [Editing Controls – 2.8](https://support.spectrasonics.net/manual/Omnisphere2/25/en/topic/layer-page-envelopes-page12)
- NEXUS4: generators grouped into layers; each layer block shows volume, pan, mute, solo, arp mode; oscillator output routed to one or more layers where it is filtered, processed with layer effects and shaped with envelopes; layers have their own mod matrix. Layer effects moved into effects groups, fully editable. — [NEXUS4 manual](https://refx.com/nexus/manual/4); [EDMProd](https://www.edmprod.com/refx-nexus-4/)
- NEXUS4: "sixteen layer arpeggiators plus the main one" (implies up to 16 layers). — [EDMProd](https://www.edmprod.com/refx-nexus-4/); [Gearnews Nexus 4](https://www.gearnews.com/refx-nexus-4-pop-hip-hop-and-edm-rompler-does-it-a-4th-time/)
- NEXUS5: individual layers can use separate arpeggiators; one instance can combine samplers, oscillators, granular and FM sources. — [Synth Anatomy](https://synthanatomy.com/2024/11/refx-nexus-5-the-synth-rompler-days-will-end-on-black-friday.html)

### Inferences
- A layer strip with Vol/Pan/Mute/Solo + one "edit" click-through is the shared pattern; Omnisphere's fixed A–D tabs is easier to learn than Nexus' variable layer count.

### Gaps
- Exact filter types/count per layer not confirmed for either from fetched sources.

## 5. Modulation

### Takeaway
Omnisphere: 8 LFOs + 4 mod envelopes shared across layers, mod matrix, and "Flex-Mod" right-click assignment (fastest). Nexus: mod matrix (sources incl. Macro 1–4, biLFO1/2, step envelope) with source shape curves; vibrato/portamento on the Mod page.

### Cited Findings
- Omnisphere Flex-Mod: right-click a control, pick a source; if LFO1 is used it auto-assigns LFO2; most controls modulatable. Four ways to route: right-click, Layer page Modulation section, Mod Matrix Zoom (overview of all routings), Modulation column. — [Flex-Mod – 2.8](https://support.spectrasonics.net/manual/Omnisphere2/25/en/topic/layer-page-modulation-page01); [Modulation Sections](https://support.spectrasonics.net/manual/Omnisphere2/25/en/topic/layer-page-modulation-page02)
- Omnisphere LFOs 1–8, Mod Envelopes 1–4 common across layers. — [Layer Page Basics](https://support.spectrasonics.net/manual/Omnisphere2/25/en/topic/layer-page-concepts-index)
- NEXUS4: all mod matrix sources gained shape control (linear, soft, hard, ease, square); main LFOs biLFO1 and biLFO2; step envelope as a source. — [EDMProd](https://www.edmprod.com/refx-nexus-4/)
- NEXUS5 updates: clear button on mod matrix entries (v5.3.6, Aug 2025); mod destinations for each trancegate (v5.3.22, Oct 2025). — [KVR news 5.3.6](https://www.kvraudio.com/news/refx-updates-nexus-to-v5-3-6-64575); [KVR news 5.3.22](https://www.kvraudio.com/news/refx-updates-nexus-to-v5-3-22-64994)

### Inferences
- Right-click "modulate this" (Flex-Mod) is the most beginner-friendly mod UX; a full matrix is better hidden on an advanced page.

### Gaps
- LFO shape lists and sync options not confirmed from primary text.

## 6. Effects

### Takeaway
Omnisphere: per-Part racks of 4 FX slots each — 4 Layer racks, 1 Common rack, 4 Aux racks — with reordering, per-unit presets and rack presets; Nexus4/5: per-layer effects groups (up to 16) + main chain (insert slots, filter, EQ, delay, reverb, trailing inserts, limiter), drag-and-drop reorder, save/load chain.

### Cited Findings
- Omnisphere: rack types LAYER, COMMON, AUX; each rack up to 4 FX units; every layer feeds the Common rack; Aux rack fed by Aux Send faders. — [Architecture and Signal Flow – 2.8](https://support.spectrasonics.net/manual/Omnisphere2/25/en/topic/architecture-and-signal-flow)
- Omnisphere FX units can be re-ordered; FX presets per unit and Rack presets saveable. — [Re-ordering FX Units](https://support.spectrasonics.net/manual/Omnisphere2/25/en/topic/fx-page-fx-unit-basics-page05); [FX Presets](https://support.spectrasonics.net/manual/Omnisphere2/25/en/topic/fx-page-fx-preset-system-index); [Rack Presets](https://support.spectrasonics.net/manual/Omnisphere2/25/en/topic/fx-page-fx-preset-system-page04)
- Omnisphere 3: 93 FX units (count for v3). — [Omnisphere 3 FX Plug-in](https://support.spectrasonics.net/manual/Omnisphere3/30/en/topic/omnisphere-fx-plug-in)
- NEXUS4 main block: insert effects (5 slots), filter, EQ, delay, reverb, impulse, trailing inserts, limiter; Glitch and OTT added; drag-and-drop reorder; whole chain save/load. — [EDMProd](https://www.edmprod.com/refx-nexus-4/); [Gearnews Nexus 4](https://www.gearnews.com/refx-nexus-4-pop-hip-hop-and-edm-rompler-does-it-a-4th-time/)
- NEXUS5: up to 16 parallel effects chains + one global chain. — [NEXUS5 manual](https://refx.com/nexus/manual/)
- Dubspot: built-in effects rack, arpeggiator and trancegate presets "add movement and production value with a single click". — [Dubspot Nexus 5](https://blog.dubspot.com/plugins/nexus-5)

### Inferences
- Fixed-slot racks (4 per rack) with on/off + preset menu per slot is the shared paradigm; Nexus' "fixed delay + reverb always present" in the main chain is what makes quick tweaking fast.

### Gaps
- Omnisphere 2.8 FX unit count (often quoted ~58) not confirmed.

## 7. Arpeggiator / trance gate

### Takeaway
Omnisphere arp: up to 32 steps, per-step length/velocity, 2.8 added pattern modes, step dividers, pitch slides, per-step chord voicing/inversion, MIDI export; Nexus: main arp + per-layer arps (up to 256 steps, zoomable in N4) and a trance gate with Mix/Speed/Contour, 52 trancegate presets.

### Cited Findings
- Omnisphere arp: up to 32 steps, each with own length and velocity; Range slider sets pattern length; Play modes Loop/Chaos/Once; rotate left/right, reverse; one arp per Part (up to 8 in Stack/Live). — [Arpeggiator – 2.8](https://support.spectrasonics.net/manual/Omnisphere2/25/en/topic/arpeggiator); [Pattern Programming](https://support.spectrasonics.net/manual/Omnisphere2/25/en/topic/pattern-programming); [Play Modes – 3.0](https://support.spectrasonics.net/manual/Omnisphere3/30/en/topic/play-modes)
- Omnisphere 2.8 arp: new Pattern Modes, Step Dividers, Pitch Slides, per-step Chord Voicings and Inversions, export arp as MIDI file. — [Andertons](https://www.andertons.co.uk/spectrasonic-omnisphere-2-psychoacoustic-virtual-synth)
- Step-modifier functions work best with ≤16 steps. — [Synth and Software 2019](https://synthandsoftware.com/2019/09/exploring-spectrasonics-omnispheres-updated-arpeggiator/)
- Older Gearspace thread complained the arp was limited (timing at max strength/Groove Lock). — [Gearspace](https://gearspace.com/threads/eric-why-omnisphere-s-arpeggiator-is-so-limited.363853/page-2)
- NEXUS4: 16 layer arps + main arp; patterns up to 256 steps; zoom in/out. — [EDMProd](https://www.edmprod.com/refx-nexus-4/)
- Nexus trance gate: Mix (max = full mute on gated steps), Spd (speed, host-synced), Cont (fade time; lower = less choppy); enabling main trance gate disables layer trance gates; fade-in from 16th notes to 4 bars (N3). 52 trancegate presets in N5. — [NEXUS5 manual](https://refx.com/nexus/manual/); [NEXUS3 manual](https://refx.com/nexus/manual/3); [Dubspot](https://blog.dubspot.com/plugins/nexus-5)

### Inferences
- For trap: a short (16-step) gate/arp with preset menus and a single on/off is enough; Nexus' one-click trancegate presets are its most-cited quick-movement feature.

### Gaps
- No source found quantifying arp popularity among trap producers specifically.

## 8. Window size/scaling

### Takeaway
Both resize: Omnisphere via Utility menu > Magnify Window; NEXUS4 became vector-based and resizable; NEXUS5 removed N4.5's zoom/scroll panels and adds reduce-animations and disable-GPU options.

### Cited Findings
- Omnisphere interface enlargeable for screen resolutions via Utility Menu / Magnify Window sub-menu. — [Magnify Window – 2.8](https://support.spectrasonics.net/manual/Omnisphere2/25/en/topic/concepts-header-page04b)
- NEXUS4 UI updated to vector-based with resizing. — [Hyperbits Nexus 4 review](https://hyperbits.com/blog/refx-nexus/)
- NEXUS5: user: "clunky zooming/scrolling type of interface in some sections in version 4.5 is gone in version 5". Settings to disable hardware graphics acceleration (Windows) and reduce UI animations. — [KVR thread](https://www.kvraudio.com/forum/viewtopic.php?t=615870)
- NEXUS5 5.5.19: dials pause briefly at useful values instead of snapping. — [NEXUS5 version history](https://refx.com/nexus/version/)

### Gaps
- Default pixel dimensions and exact zoom percentages for both not found.

## 9. Praise and complaints

### Takeaway
Nexus: praised for clean UI, instant chart-ready presets, one-click FX/arp/gate; criticised for limited editing historically and, in N5, still convoluted editing. Omnisphere: praised for depth and the Orb/browser tools; criticised for dated look ("Windows XP era"), fragmented tagging, slow search, and (pre-3.0) hard-to-reach global controls.

### Cited Findings
- Dubspot Nexus 5: score 8.1; "polished, chart-ready sounds instantly, at the cost of deep synthesis control"; clean interface. — [Dubspot](https://blog.dubspot.com/plugins/nexus-5)
- KVR user rating for Nexus 3.79/5 (14 reviews). — [KVR product page](https://www.kvraudio.com/product/nexus-by-refx)
- KVR NEXUS5 thread: "still too convoluted ... to edit and is not much fun as Avenger 2"; others call it one of the most powerful synths; early sampler bugs. — [KVR thread](https://www.kvraudio.com/forum/viewtopic.php?t=615870&start=345)
- Omnisphere 3 KVR comments: interface "looks like from Windows XP era" vs Pigments/Serum; tagging fragmented; search slow; new browser learning curve. — [KVR Omnisphere 3 review](https://www.kvraudio.com/omnisphere-3-boldly-going-everywhere)
- MusicRadar Omnisphere 3: "one of the most unique, stylish and feature-packed workhorses"; global control previously hard to access, fixed with main page makeover. — [MusicRadar](https://www.musicradar.com/music-tech/soft-synths/spectrasonics-omnisphere-3-review)

### Inferences
- Design lesson: copy Nexus' preset-first speed + Omnisphere 3's "basics on page one" + Sound Lock/Sound Match; avoid deep nested editors and fragmented tag systems.

### Gaps
- No Reddit r/trapproduction or Attack Magazine / Sound on Sound UI-specific quotes could be retrieved (fetch blocked; search returned none).
