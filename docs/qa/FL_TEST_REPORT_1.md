# CHROME VAULT v3 – FL Studio test report #1 (owner's test, Windows)

PRIORITY 1 – blocking
0. WRONG PITCH (most important): playing notes from FL's piano roll (F5), some neighbouring keys play the SAME pitch, e.g. B4 sounds exactly like C5. Every key must play its own correct pitch for every preset and every layer kind (sample zones must be re-pitched from their root to the played key; models and synth layers too). Add an automatic test: for EVERY preset in the bank, render each key from C2 to C7 and check with pitch detection that consecutive keys differ by one semitone (±10 cents; bells/FX: check relative pitch with partial tracking), and that no two different keys give the same pitch. Fix the sampler / library maps / recipes until it passes for all presets.
1. Computer keyboard: clicking a sound, knob, FX/ARP/GATE button or anywhere in the window and playing Z X C V / Q W E R must ALWAYS play and never type into search. Clicking into search and typing types; Enter or clicking outside → keyboard plays again.
2. Clicking the already-loaded sound must not reset edits. Reset only via an explicit action ("Reset to preset"/"Revert"). Switching to another sound may drop edits. Check: edit 3 knobs, click the sound again → values and the "edited" badge stay.
3. Knob positions must match values: after loading a sound the REVERB/DELAY/CHORUS/DRIVE/WIDTH knobs always point at 12 o'clock whatever the text says ("off", 6 %, 58 %); they redraw only after being turned. Redraw every knob from the real parameter value after preset load, window open, project load and automation. "off"/0 % = knob at minimum. Applies to every page (ARP RATE and GATE RATE with the same value must sit at the same angle). Check 10+ sounds from different categories.
4. ADVANCED tab: clicking does nothing. Either finish it or remove it; an inactive item must not look clickable.

PRIORITY 2 – important
5. ARP RATE label: Triplet must show "1/16T", Dotted "1/8D", like GATE.
6. Sound list after opening the window: keep the selected category; scroll so the selected sound is visible and highlighted (also on first open).
7. Modulation ring: after right-click → Modulate with…, the green ring must appear immediately, not only after reopening the window.
8. Search: exact name matches first ("warm" → "Warm Sub" near the top); category counts follow the search results; show the number of results.
9. Window size: offer only sizes that fit the screen; the window must never overflow the screen edge; the size menu must open fully visible.

PRIORITY 3 – look and polish
10. Clipped texts: "COMPRESS…", "Flutes & Woodwin…", "Wheel +1.20 oc", the clipped hint on MOD. Shorten or make room.
11. FX slots: all empty slots shown uniformly as "+ Add effect", not "Slot 7"/"Slot 8".
12. Values and names: "MONO < 0.00 Hz" → "off"; modulation target named "Reverb Mix", not "FX 1 Mix".
13. On-screen keyboard: don't light C5 permanently; show the notes actually playing, incl. arp and latch notes.
14. "edited" badge: show after ANY edit, also from MAIN.
15. LAYERS: a one-layer sound must not leave half the window empty ("+ Add layer" button or adaptive layout).
16. Host parameter text: report values in the same units as the plugin ("Cutoff −1.2 oct", not "−30 %").

MUST NOT BREAK (re-verify after fixes)
- FX chain: on/off, types, drag reorder, sync with MAIN.
- MOD: LFOs, targets, right-click → Modulate.
- Arpeggiator: all modes, rate, Latch, steps, DAW tempo sync.
- GATE, Favorites, Recent, arrows, Drag WAV.
- Settings kept after closing the window and after saving/reopening the project.
