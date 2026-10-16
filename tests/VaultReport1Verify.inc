// CHROME VAULT: an independent re-test of the owner's FL Studio test report #1 (docs/qa/FL_TEST_REPORT_1.md, items 1-16
// and the MUST NOT BREAK list) - `VaultTests -report1` (run under xvfb-run; item 0 is `VaultTests -pitch`).
//
// Unlike the -uifix checks (written with the fixes), these drive the editor the way the owner does: the editor is put on
// the desktop as a real window and every click / key goes in through its ComponentPeer (the path the OS events take:
// focus changes, mouse-down handlers of the parents, key routing to the focused component), then the processor, the
// audio and the on-screen state are checked. Included by tests/VaultTests.cpp (its helpers: check, Rig, findChildOfType,
// countButtons, findKnob).

struct PeerWindow
{
    Rig rig;
    std::unique_ptr<VaultEditor> ed;
    juce::ComponentPeer* peer = nullptr;
    std::set<int> down;                           // the keys "physically" down (no real keyboard under xvfb)
    juce::int64 t = 1000000;

    static void pump (int ms) { juce::MessageManager::getInstance()->runDispatchLoopUntil (ms); }
    void open()
    {
        ed = std::make_unique<VaultEditor> (rig.p);
        ed->computerKeys().isKeyDown = [this] (int k) { return down.count ((int) juce::CharacterFunctions::toLowerCase ((juce::juce_wchar) k)) > 0; };
        ed->setTopLeftPosition (10, 10);
        ed->addToDesktop (0);
        ed->setVisible (true);
        peer = ed->getPeer();
        pump (100);
        if (peer != nullptr) peer->grabFocus();   // the host / window manager activates the plugin window it shows
        pump (150);
    }
    void close()
    {
        down.clear();
        if (ed != nullptr) { ed->removeFromDesktop(); ed.reset(); }
        peer = nullptr;
        pump (50);
    }
    VaultMainView& v() { return ed->view(); }
    void mouse (juce::Point<float> pos, juce::ModifierKeys m)
    {
        t += 700;   // far enough apart: never a double-click
        peer->handleMouseEvent (juce::MouseInputSource::InputSourceType::mouse, pos, m, juce::MouseInputSource::defaultPressure,
                                juce::MouseInputSource::defaultOrientation, t);
    }
    juce::Point<float> where (juce::Component& c, juce::Point<int> local) { return ed->getLocalPoint (&c, local).toFloat(); }
    void clickAt (juce::Component& c, juce::Point<int> local)
    {
        const auto pos = where (c, local);
        mouse (pos, {});
        mouse (pos, juce::ModifierKeys::leftButtonModifier);
        mouse (pos, {});
        pump (30);
    }
    void click (juce::Component& c) { clickAt (c, c.getLocalBounds().getCentre()); }
    void drag (juce::Component& c, int dy)   // a vertical knob drag (up = negative dy = more)
    {
        auto pos = where (c, c.getLocalBounds().getCentre());
        mouse (pos, {});
        mouse (pos, juce::ModifierKeys::leftButtonModifier);
        for (int i = 1; i <= 8; ++i)
        {
            peer->handleMouseEvent (juce::MouseInputSource::InputSourceType::mouse, pos.translated (0.0f, (float) dy * (float) i / 8.0f),
                                    juce::ModifierKeys::leftButtonModifier, juce::MouseInputSource::defaultPressure,
                                    juce::MouseInputSource::defaultOrientation, t += 20);
        }
        mouse (pos.translated (0.0f, (float) dy), {});
        pump (30);
    }
    // a key going down (and staying down until up()): what the OS sends - a key-state change, then the key press
    bool key (int ch)
    {
        down.insert (ch);
        peer->handleKeyUpOrDown (true);
        const bool used = peer->handleKeyPress (juce::KeyPress (ch, 0, (juce::juce_wchar) ch));
        pump (5);
        return used;
    }
    bool special (int code) { peer->handleKeyUpOrDown (true); const bool u = peer->handleKeyPress (juce::KeyPress (code)); peer->handleKeyUpOrDown (false); pump (5); return u; }
    void up (int ch) { down.erase (ch); peer->handleKeyUpOrDown (false); pump (5); }
    bool noteOn (int n) { return rig.p.getKeyboardState().isNoteOn (1, n); }
    bool anyNoteOn() { for (int n = 0; n < 128; ++n) if (noteOn (n)) return true; return false; }
    juce::String searchText() { return v().mainPage().searchBox().getText(); }
    // the audio thread's view: blocks processed while the UI timers run
    void run (int blocks) { for (int i = 0; i < blocks; ++i) { rig.blocks (1); if (i % 8 == 0) pump (1); } pump (60); }
};

int report1Verify()
{
    using namespace vault;
    const int before = fails;
    uiSetup ("report1");
    VaultEditor::simulatedDisplay() = { 0, 0, 1920, 1040 };
    std::printf ("== owner report #1: independent re-test through the window's peer ==\n");

    // ---------------------------------------------------------------- 1: computer keyboard after clicks anywhere
    {
        PeerWindow w;
        w.open();
        check (w.peer != nullptr, "report1 #1: the editor opens as a desktop window (xvfb)");
        if (w.peer == nullptr) return fails - before;
        auto& v = w.v();
        auto& mp = v.mainPage();
        juce::StringArray bad;
        int tried = 0;
        auto plays = [&] (const juce::String& after)
        {
            ++tried;
            const auto s0 = w.searchText();
            w.key ('z');
            const bool z = w.noteOn (48);
            w.up ('z');
            w.key ('q');
            const bool q = w.noteOn (60);
            w.up ('q');
            const bool clean = ! w.anyNoteOn() && w.searchText() == s0;
            if (! (z && q && clean)) bad.add (after + (z ? "" : " [Z silent]") + (q ? "" : " [Q silent]") + (clean ? "" : " [typed / hanging: '" + w.searchText() + "']"));
        };
        w.clickAt (mp.presetList(), { 60, 40 });                            plays ("a sound in the list");
        w.click (mp.quickKnob (qCutoff));                                    plays ("the CUTOFF knob");
        w.click (mp.quickKnob (qReverb));                                    plays ("the REVERB knob");
        w.click (mp.macroKnob (0));                                          plays ("sound macro 1");
        w.click (mp.volumeKnob());                                           plays ("the VOLUME knob");
        w.clickAt (mp.categoryList(), mp.categoryList().rowRect (3).getCentre()); plays ("a category");
        for (const char* t : { "FX", "ARP", "GATE" })
            if (auto* b = findChildOfType<vui::LedToggle> (v, t))
            {
                w.click (*b); plays (juce::String (t) + " toggle (on)");
                w.click (*b); plays (juce::String (t) + " toggle (back)");
            }
        w.clickAt (v, { 5, v.getHeight() / 2 });                             plays ("the window's empty edge");
        w.clickAt (mp, { mp.getWidth() / 2, 3 });                            plays ("the MAIN page background");
        w.clickAt (v.keys(), { 30, v.keys().getHeight() - 8 });              plays ("the on-screen keyboard");
        for (int pg : { (int) VaultMainView::pageLayers, (int) VaultMainView::pageFx, (int) VaultMainView::pageMod, (int) VaultMainView::pageArp })
        {
            w.click (v.tab (pg));
            plays ("the " + v.tab (pg).getButtonText() + " tab");
            auto& page = v.pageComponent (pg);
            int n = 0;
            for (auto* k : page.getChildren())   // the first two knobs / buttons of the page
                if (n < 2 && (dynamic_cast<vui::Knob*> (k) != nullptr || dynamic_cast<vui::PowerButton*> (k) != nullptr) && k->isShowing())
                { w.click (*k); plays ("a control on " + v.tab (pg).getButtonText()); ++n; }
        }
        w.click (v.advancedButton()); plays ("ADVANCED");
        w.click (v.tab (VaultMainView::pageMain)); plays ("the MAIN tab");
        check (bad.isEmpty(), "report1 #1: after a click on any of " + juce::String (tried) + " places, Z / Q play C4 / C5 and nothing is typed"
               + (bad.isEmpty() ? juce::String() : " - FAILED after: " + bad.joinIntoString (", ")));

        // the search box: a click in, typing types (no notes); Return / a click outside hand the keys back
        auto& search = mp.searchBox();
        w.click (search);
        for (auto ch : juce::String ("warm")) { w.key ((int) ch); w.up ((int) ch); }
        const bool typed = w.searchText() == "warm" && ! w.anyNoteOn();
        const auto shownTyping = mp.visibleCount();
        w.special (juce::KeyPress::returnKey);
        bad.clear(); plays ("Return in the search box");
        const bool afterReturn = bad.isEmpty();
        w.click (search);
        w.key ('s'); w.up ('s');
        const bool typed2 = w.searchText() == "warms" && ! w.anyNoteOn();
        w.click (mp.quickKnob (qAttack));
        bad.clear(); plays ("a click outside the search box");
        const bool afterOutside = bad.isEmpty() && w.searchText() == "warms";
        w.click (search);
        w.special (juce::KeyPress::escapeKey);
        bad.clear(); plays ("Escape in the search box");
        const bool afterEsc = bad.isEmpty();
        check (typed && afterReturn && typed2 && afterOutside && afterEsc,
               "report1 #1: a click into search types \"warm\" (no notes, " + juce::String (shownTyping) + " results); Return, a click outside, Escape "
               "- the keys play again (typed " + juce::String ((int) typed) + " return " + juce::String ((int) afterReturn) + " typed2 "
               + juce::String ((int) typed2) + " outside " + juce::String ((int) afterOutside) + " esc " + juce::String ((int) afterEsc) + ")");
        // the computer keys reach the audio
        {
            w.v().setQuery ({});
            int synth = 0;
            for (int i = 0; i < (int) w.rig.p.bank().size(); ++i) if (! w.rig.p.bank()[(size_t) i].usesSamples() && ! w.rig.p.bank()[(size_t) i].arp.on) { synth = i; break; }
            w.v().loadPreset (synth, false);
            w.run (20);
            w.click (mp.quickKnob (qRelease));
            w.key ('q');
            float pk = 0;
            juce::AudioBuffer<float> b (2, Rig::bs);
            juce::MidiBuffer m;
            for (int i = 0; i < 40; ++i) { b.clear(); m.clear(); w.rig.p.processBlock (b, m); pk = std::max (pk, b.getMagnitude (0, Rig::bs)); }
            w.up ('q');
            check (pk > 0.01f, "report1 #1: Q after a knob click is heard (peak " + juce::String (pk, 3) + ")");
        }
        w.close();
    }

    // ---------------------------------------------------------------- 2 + 14: same sound keeps edits; the badge after any edit
    {
        PeerWindow w;
        w.open();
        auto& v = w.v();
        auto& mp = v.mainPage();
        v.setCategory (catPads);
        w.run (4);
        w.clickAt (mp.presetList(), { 60, 3 * vui::PresetList::rowH + 12 });
        w.run (10);
        const int idx = w.rig.p.currentPreset();
        const bool clean = ! w.rig.p.hasEdits() && mp.editedBadge().isEmpty();
        w.drag (mp.quickKnob (qCutoff), -40);
        const bool badgeAfterMain = w.rig.p.hasEdits() && ! mp.editedBadge().isEmpty();
        w.drag (mp.quickKnob (qReverb), -30);
        w.drag (mp.quickKnob (qAttack), -25);
        w.run (8);
        const float c0 = w.rig.p.getQuick (qCutoff), r0 = w.rig.p.getQuick (qReverb), a0 = w.rig.p.getQuick (qAttack);
        const float kc = mp.quickKnob (qCutoff).getValue(), kr = mp.quickKnob (qReverb).getValue(), ka = mp.quickKnob (qAttack).getValue();
        w.clickAt (mp.presetList(), { 60, 3 * vui::PresetList::rowH + 12 });   // the same row again
        w.run (10);
        const bool kept = w.rig.p.currentPreset() == idx && std::abs (w.rig.p.getQuick (qCutoff) - c0) < 1e-6f && std::abs (w.rig.p.getQuick (qReverb) - r0) < 1e-6f
                       && std::abs (w.rig.p.getQuick (qAttack) - a0) < 1e-6f && std::abs (mp.quickKnob (qCutoff).getValue() - kc) < 1e-4f
                       && std::abs (mp.quickKnob (qReverb).getValue() - kr) < 1e-4f && std::abs (mp.quickKnob (qAttack).getValue() - ka) < 1e-4f
                       && w.rig.p.hasEdits() && ! mp.editedBadge().isEmpty();
        check (clean && badgeAfterMain && kept && std::abs (c0 - quickInfo()[qCutoff].neutral) > 0.02f,
               "report1 #2/#14: 3 MAIN knobs dragged - the \"edited\" badge shows after the first; the same sound clicked again keeps the 3 values, "
               "the knobs and the badge (clean " + juce::String ((int) clean) + " badge " + juce::String ((int) badgeAfterMain) + " kept " + juce::String ((int) kept) + ")");
        // the badge after a macro, a switch, a layer edit; the explicit revert clears everything
        w.rig.p.revertAll(); w.run (6);
        const bool cleared = ! w.rig.p.hasEdits() && mp.editedBadge().isEmpty() && std::abs (w.rig.p.getQuick (qCutoff) - quickInfo()[qCutoff].neutral) < 1e-6f;
        w.drag (mp.macroKnob (0), -30); w.run (4);
        const bool macroBadge = ! mp.editedBadge().isEmpty();
        w.rig.p.revertAll(); w.run (6);
        if (auto* gt = findChildOfType<vui::LedToggle> (v, "GATE")) w.click (*gt);
        w.run (4);
        const bool switchBadge = ! mp.editedBadge().isEmpty();
        w.rig.p.revertAll(); w.run (6);
        w.rig.p.editSound ([] (VaultPreset& x) { x.parts[0].level *= 0.5f; });
        v.refreshAll(); w.run (4);
        const bool layerBadge = ! mp.editedBadge().isEmpty();
        // another sound may drop the edits
        w.clickAt (mp.presetList(), { 60, 5 * vui::PresetList::rowH + 12 });
        w.run (10);
        const bool other = w.rig.p.currentPreset() != idx && ! w.rig.p.hasEdits();
        check (cleared && macroBadge && switchBadge && layerBadge && other,
               "report1 #14: the badge after a macro / a switch / a layer edit; Revert clears it; another sound starts clean ("
               + juce::String ((int) cleared) + juce::String ((int) macroBadge) + juce::String ((int) switchBadge) + juce::String ((int) layerBadge) + juce::String ((int) other) + ")");
        w.close();
    }

    // ---------------------------------------------------------------- 3: knobs match values (load / reopen / project / automation)
    {
        PeerWindow w;
        w.open();
        auto& v = w.v();
        auto& mp = v.mainPage();
        const auto& bank = w.rig.p.bank();
        const int fxKnobs[] { qReverb, qDelay, qChorus, qDrive, qWidth };
        // the first preset of every category with an FX chain + 4 more at random: knob text "off" / "0%" = minimum; for each
        // FX knob the positions sort like the percentages they print (no knob stuck at 12 o'clock); a fresh editor shows the same
        std::vector<int> picks;
        for (int c = 0; c < vault::numCategories; ++c)
            for (int i = 0; i < (int) bank.size(); ++i) if (bank[(size_t) i].category == c) { picks.push_back (i); break; }
        juce::Random rnd (17);
        for (int k = 0; k < 4; ++k) picks.push_back (rnd.nextInt ((int) bank.size()));
        juce::StringArray bad;
        std::map<int, std::vector<std::pair<float, float>>> pctPos;   // knob -> (printed %, position)
        int atNoon = 0, checkedKnobs = 0;
        for (int idx : picks)
        {
            v.loadPreset (idx, false);
            w.run (12);
            for (int q : fxKnobs)
            {
                auto& k = mp.quickKnob (q);
                const auto txt = k.currentValueText();
                const float pos = k.getValue();
                ++checkedKnobs;
                if ((txt == "off" || txt == "0%") && pos > 0.001f) bad.add (bank[(size_t) idx].name + " " + k.label + " '" + txt + "' at " + juce::String (pos, 2));
                if (txt.endsWithChar ('%')) pctPos[q].push_back ({ txt.getFloatValue(), pos });
                atNoon += std::abs (pos - 0.5f) < 0.004f && txt != "50%" ? 1 : 0;
            }
        }
        for (auto& [q, pts] : pctPos)
        {
            std::sort (pts.begin(), pts.end());
            for (size_t i = 1; i < pts.size(); ++i)
                if (pts[i].first > pts[i - 1].first + 0.5f && pts[i].second + 0.002f < pts[i - 1].second)
                    bad.add (juce::String (quickInfo()[(size_t) q].name) + ": " + juce::String (pts[i].first, 0) + "% sits below " + juce::String (pts[i - 1].first, 0) + "%");
        }
        check (bad.isEmpty() && atNoon <= 1, "report1 #3: " + juce::String ((int) picks.size()) + " sounds (every category): REVERB / DELAY / CHORUS / DRIVE / WIDTH - "
               "'off' / 0% at the minimum, positions ordered like their texts, " + juce::String (atNoon) + " of " + juce::String (checkedKnobs) + " at 12 o'clock without saying 50%"
               + (bad.isEmpty() ? juce::String() : " - " + bad.joinIntoString ("; ")));

        // every MAIN knob: the open window == a freshly opened window after automation, a project load and a reopen
        auto snapshot = [] (vui::MainPage& m)
        {
            std::vector<std::pair<float, juce::String>> s;
            for (int q = 0; q < numQuick; ++q) s.push_back ({ m.quickKnob (q).getValue(), m.quickKnob (q).currentValueText() });
            for (int i = 0; i < numMacros; ++i) s.push_back ({ m.macroKnob (i).getValue(), m.macroKnob (i).currentValueText() });
            return s;
        };
        auto same = [] (const std::vector<std::pair<float, juce::String>>& a, const std::vector<std::pair<float, juce::String>>& b, juce::String& why)
        {
            for (size_t i = 0; i < a.size() && i < b.size(); ++i)
                if (std::abs (a[i].first - b[i].first) > 0.002f || a[i].second != b[i].second)
                { why << "knob " << (int) i << ": " << juce::String (a[i].first, 3) << " '" << a[i].second << "' vs fresh " << juce::String (b[i].first, 3) << " '" << b[i].second << "'"; return false; }
            return a.size() == b.size();
        };
        v.loadPreset (picks[3], false);
        w.run (10);
        // host automation (the host thread writes the parameters; the editor is open)
        auto& params = w.rig.p.getParameters();
        int automated = 0;
        for (auto* prm : params)
            if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (prm))
                for (auto* id : { VaultProcessor::idQuick[qReverb], VaultProcessor::idQuick[qCutoff], VaultProcessor::idQuick[qDrive], VaultProcessor::idMacro[1] })
                    if (rp->getParameterID() == id)
                    {
                        const float to = id == VaultProcessor::idQuick[qCutoff] ? 0.2f : 0.85f;
                        std::thread ([rp, to] { rp->beginChangeGesture(); rp->setValueNotifyingHost (to); rp->endChangeGesture(); }).join();
                        ++automated;
                    }
        w.run (30);
        const auto live = snapshot (v.mainPage());
        juce::String why;
        {
            VaultEditor fresh (w.rig.p);
            fresh.view().refreshAll();
            PeerWindow::pump (40);
            check (automated == 4 && same (live, snapshot (fresh.view().mainPage()), why),
                   "report1 #3: after host automation of REVERB / CUTOFF / DRIVE / macro 2 the open window shows what a fresh window shows " + why);
        }
        // a project load into the open window
        juce::MemoryBlock state;
        w.rig.p.getStateInformation (state);
        v.loadPreset (picks[7], false);
        w.run (10);
        w.rig.p.setStateInformation (state.getData(), (int) state.getSize());
        w.run (30);
        why.clear();
        {
            VaultEditor fresh (w.rig.p);
            fresh.view().refreshAll();
            PeerWindow::pump (40);
            check (same (snapshot (v.mainPage()), snapshot (fresh.view().mainPage()), why) && std::abs (w.rig.p.getQuick (qReverb) - 0.85f) < 0.01f,
                   "report1 #3: a project loaded into the open window: every MAIN knob / macro redrawn (= a fresh window) " + why);
        }
        // ARP RATE and GATE RATE: the same rate, the same angle and text
        w.click (v.tab (VaultMainView::pageArp));
        int sameAngle = 0, rates = 0;
        juce::String rateBad;
        for (int div : vui::ArpGatePage::rateKnobDivisions())
        {
            w.rig.p.editSound ([div] (VaultPreset& x) { x.arp.rate = (uint8_t) div; x.gate.rate = (uint8_t) div; });
            v.refreshAll();
            PeerWindow::pump (5);
            std::vector<vui::Knob*> rk;
            std::function<void (juce::Component&)> find = [&] (juce::Component& c)
            { for (auto* ch : c.getChildren()) { if (auto* k = dynamic_cast<vui::Knob*> (ch); k != nullptr && k->label == "RATE") rk.push_back (k); find (*ch); } };
            find (v.arpPage());
            ++rates;
            if (rk.size() == 2 && std::abs (rk[0]->getValue() - rk[1]->getValue()) < 1e-4f && rk[0]->currentValueText() == rk[1]->currentValueText()) ++sameAngle;
            else if (rateBad.isEmpty() && rk.size() == 2) rateBad = rk[0]->currentValueText() + " " + juce::String (rk[0]->getValue(), 3) + " vs " + rk[1]->currentValueText() + " " + juce::String (rk[1]->getValue(), 3);
        }
        check (rates > 10 && sameAngle == rates, "report1 #3/#5: ARP RATE and GATE RATE at the same rate sit at the same angle with the same text ("
               + juce::String (sameAngle) + " / " + juce::String (rates) + " rates) " + rateBad);
        // #5: the labels
        juce::StringArray texts;
        for (auto [base, feel] : { std::pair<int, int> { 4, 1 }, { 3, 2 }, { 4, 0 } })
        {
            const int div = vui::ArpGatePage::rateIndex (base, feel);
            w.rig.p.editSound ([div] (VaultPreset& x) { x.arp.rate = (uint8_t) div; });
            v.refreshAll();
            PeerWindow::pump (5);
            if (auto* k = findKnob (v.arpPage(), "RATE")) texts.add (k->currentValueText());
        }
        check (texts.joinIntoString (",") == "1/16T,1/8D,1/16", "report1 #5: ARP RATE reads \"1/16T\" (triplet), \"1/8D\" (dotted), \"1/16\": " + texts.joinIntoString (","));
        w.rig.p.revertAll();
        w.close();
    }

    // ---------------------------------------------------------------- 4: ADVANCED
    {
        PeerWindow w;
        w.open();
        auto& v = w.v();
        w.click (v.advancedButton());
        const bool opened = v.currentPage() != VaultMainView::pageMain && v.pageComponent (v.currentPage()).isShowing();
        check (opened && v.advancedButton().isEnabled(), "report1 #4: a real click on ADVANCED opens an ADVANCED page (page " + juce::String (v.currentPage()) + ")");
        w.close();
    }

    // ---------------------------------------------------------------- 6: the list after reopening (and the very first open)
    {
        PeerWindow w;
        w.open();
        auto& v = w.v();
        v.setCategory (catBass);
        w.run (4);
        const auto& vis = v.browser().visible();
        const int deep = vis.size() > 120 ? vis[110] : vis.back();
        v.loadPreset (deep, false);
        w.run (6);
        v.mainPage().presetList().scrollToTop();
        w.close();
        w.open();
        auto& v2 = w.v();
        w.run (8);
        const int row = v2.browser().rowOf (deep);
        const bool reopened = v2.mainPage().categoryList().getSelected() == catBass && v2.browser().filter.category == catBass
                           && v2.mainPage().presetList().isRowVisible (row) && v2.shownPreset() == deep;
        // the project reopened in a new host session (new processor), the window opened the first time
        juce::MemoryBlock st;
        w.rig.p.getStateInformation (st);
        w.close();
        PeerWindow w2;
        w2.rig.p.setStateInformation (st.getData(), (int) st.getSize());
        w2.run (10);
        w2.open();
        w2.run (8);
        const int row2 = w2.v().browser().rowOf (deep);
        const bool firstOpen = w2.v().browser().filter.category == catBass && w2.v().mainPage().presetList().isRowVisible (row2) && w2.v().shownPreset() == deep;
        check (reopened && firstOpen, "report1 #6: Bass, the 111th sound: reopening the window and reopening the project (first open of a new window) "
               "keep the category and show the sound highlighted in view (" + juce::String ((int) reopened) + juce::String ((int) firstOpen) + ")");
        w2.close();
    }

    // ---------------------------------------------------------------- 7: modulation ring at once
    {
        PeerWindow w;
        w.open();
        auto& v = w.v();
        int idx = 0;
        for (int i = 0; i < (int) w.rig.p.bank().size(); ++i) if (w.rig.p.bank()[(size_t) i].mod.modWheel.empty()) { idx = i; break; }
        v.loadPreset (idx, false);
        w.run (6);
        auto& cut = v.mainPage().quickKnob (qCutoff);
        const bool before = cut.isModulated();
        vui::mod::apply (v.context(), vui::MainPage::quickTarget (qCutoff), 2);   // the menu's "Mod Wheel"
        PeerWindow::pump (40);
        const bool now = cut.isModulated();
        // on the FX page too
        w.click (v.tab (VaultMainView::pageFx));
        auto knobs = v.fxPage().slotKnobs();
        bool fxRing = knobs.empty();
        if (! knobs.empty())
        {
            const auto& slot = w.rig.p.currentSound().fx[(size_t) knobs[0].slot];
            const auto target = "fx" + juce::String (knobs[0].slot + 1) + "." + fxTypeInfo (slot.type).params[(size_t) knobs[0].param].id;
            vui::mod::apply (v.context(), target, 0);   // LFO 1
            PeerWindow::pump (40);
            for (auto& sk : v.fxPage().slotKnobs()) if (sk.slot == knobs[0].slot && sk.param == knobs[0].param) fxRing = sk.knob->isModulated();
        }
        check (! before && now && fxRing, "report1 #7: Modulate with... -> the green ring appears at once (MAIN CUTOFF " + juce::String ((int) now) + ", FX page " + juce::String ((int) fxRing) + ")");
        w.close();
    }

    // ---------------------------------------------------------------- 8: search
    {
        PeerWindow w;
        w.open();
        auto& v = w.v();
        const auto& bank = w.rig.p.bank();
        w.click (v.searchBox());
        for (auto ch : juce::String ("warm")) { w.key ((int) ch); w.up ((int) ch); }
        w.run (12);
        std::printf ("   search text '%s', query '%s', category %d, %d shown\n", w.searchText().toRawUTF8(), v.browser().filter.query.toRawUTF8(),
                     v.browser().filter.category, v.visibleCount());
        const auto vis = v.browser().visible();
        int warmSub = -1;
        for (size_t i = 0; i < vis.size(); ++i) if (bank[(size_t) vis[i]].name == "Warm Sub") warmSub = (int) i;
        int sum = 0;
        for (int c = 0; c < vault::numCategories; ++c) sum += v.browser().categoryCount (c);
        // the exact names: every one of 30 random names typed in full is the first result
        int exactFirst = 0;
        juce::Random rnd (5);
        for (int k = 0; k < 30; ++k)
        {
            const auto& nm = bank[(size_t) rnd.nextInt ((int) bank.size())].name;
            v.setQuery (nm.toLowerCase());
            exactFirst += ! v.browser().visible().empty() && bank[(size_t) v.browser().visible().front()].name.equalsIgnoreCase (nm) ? 1 : 0;
        }
        v.setQuery ("zzqqxx");
        const bool none = v.visibleCount() == 0;
        check (warmSub >= 0 && warmSub < 3 && sum == (int) vis.size() && exactFirst == 30 && none,
               "report1 #8: \"warm\" typed: Warm Sub at row " + juce::String (warmSub + 1) + ", category counts add up to the " + juce::String ((int) vis.size())
               + " results, 30/30 full names first (" + juce::String (exactFirst) + "), nonsense -> none");
        w.special (juce::KeyPress::escapeKey);
        w.close();
    }

    // ---------------------------------------------------------------- 9: window size on small screens
    {
        juce::StringArray bad;
        for (auto area : { juce::Rectangle<int> (0, 0, 1366, 728), juce::Rectangle<int> (0, 0, 1280, 680), juce::Rectangle<int> (0, 0, 1920, 1040), juce::Rectangle<int> (0, 0, 2560, 1400) })
        {
            VaultEditor::simulatedDisplay() = area;
            PeerWindow w;
            w.open();
            for (float s : w.v().scaleChoices()) if (! VaultEditor::fits (s, area)) bad.add (juce::String (s) + " offered on " + area.toString());
            w.ed->setUiScale (2.0f);
            PeerWindow::pump (20);
            if (w.ed->getWidth() > area.getWidth() || w.ed->getHeight() > area.getHeight())
                bad.add ("200% on " + area.toString() + " -> " + juce::String (w.ed->getWidth()) + "x" + juce::String (w.ed->getHeight()));
            if (auto* c = w.ed->getConstrainer(); c != nullptr && (c->getMaximumWidth() > area.getWidth() || c->getMaximumHeight() > area.getHeight()))
                bad.add ("corner drag allows " + juce::String (c->getMaximumWidth()) + "x" + juce::String (c->getMaximumHeight()) + " on " + area.toString());
            w.close();
        }
        VaultEditor::simulatedDisplay() = { 0, 0, 1920, 1040 };
        VaultSettings::get()->setScale (1.0f);
        check (bad.isEmpty(), "report1 #9: on 1366x728 / 1280x680 / 1920x1040 / 2560x1400 the size menu offers only fitting sizes, 200% and the corner "
               "drag stay inside the screen " + bad.joinIntoString ("; "));
    }

    // ---------------------------------------------------------------- 11 + 12 + 15 + 16
    {
        PeerWindow w;
        w.open();
        auto& v = w.v();
        const auto& bank = w.rig.p.bank();
        // 11: a sound with a short chain: every empty slot reads "+ Add effect"
        int shortChain = -1;
        for (int i = 0; i < (int) bank.size() && shortChain < 0; ++i) if (bank[(size_t) i].v3 && bank[(size_t) i].fx.size() <= 3) shortChain = i;
        if (shortChain >= 0)
        {
            v.loadPreset (shortChain, false);
            w.run (6);
            w.click (v.tab (VaultMainView::pageFx));
            const int empties = (int) maxFxSlots - (int) w.rig.p.currentSound().fx.size();
            const int adds = countButtons (v.fxPage(), "Add effect");   // a Pill with the "+" icon
            const int slotWords = countButtons (v.fxPage(), "Slot 7") + countButtons (v.fxPage(), "Slot 8");
            check (adds == empties && slotWords == 0, "report1 #11: " + bank[(size_t) shortChain].name + ": " + juce::String (empties) + " empty slots, "
                   + juce::String (adds) + " \"+ Add effect\", no \"Slot 7/8\"");
        }
        // 12: names / values
        {
            juce::String mono;
            for (auto& prm : fxTypeInfo (FxType::width).params)
                if (juce::String (prm.id) == "monoBelow") mono = vui::fxValueText (prm, 0.0f);
            int rev = -1;
            for (int i = 0; i < (int) bank.size() && rev < 0; ++i)
                for (size_t s = 0; s < bank[(size_t) i].fx.size(); ++s)
                    if (bank[(size_t) i].v3 && bank[(size_t) i].fx[s].type == FxType::reverb && s > 0) { rev = i; break; }
            juce::String name;
            if (rev >= 0)
            {
                const auto& p = bank[(size_t) rev];
                for (size_t s = 0; s < p.fx.size(); ++s) if (p.fx[s].type == FxType::reverb) { name = vui::mod::longName ("fx" + juce::String ((int) s + 1) + ".mix", &p); break; }
            }
            check (mono == "off" && name == "Reverb Mix", "report1 #12: Mono Below at 0 reads '" + mono + "', a reverb slot's mix is named '" + name + "'");
        }
        // 15: a one-layer sound fills LAYERS (the card + the "+ Add layer" column)
        {
            int one = -1;
            for (int i = 0; i < (int) bank.size() && one < 0; ++i) if (bank[(size_t) i].parts.size() == 1) one = i;
            v.loadPreset (one, false);
            w.run (6);
            w.click (v.tab (VaultMainView::pageLayers));
            auto& lp = v.layersPage();
            juce::Rectangle<int> used;
            for (int i = 0; i < lp.numCards(); ++i) if (auto* c = lp.cardComponent (i)) used = used.getUnion (c->getBounds());
            int addW = 0;
            for (auto* ch : lp.getChildren()) if (ch->isVisible() && ch->getWidth() > 100 && ch->getHeight() > 100 && ch != lp.cardComponent (0)) addW = std::max (addW, ch->getWidth());
            const float share = (float) used.getWidth() / (float) lp.getWidth();
            check (lp.numCards() == 1 && lp.numAddCards() >= 1 && share > 0.35f && share < 0.75f && addW > 0,
                   "report1 #15: a one-layer sound: its card takes " + juce::String (share * 100.0f, 0) + "% of LAYERS next to \"+ Add layer\"");
            const int before = (int) w.rig.p.currentSound().parts.size();
            lp.addLayer (1);
            PeerWindow::pump (20);
            const bool added = (int) w.rig.p.currentSound().parts.size() == before + 1 && w.rig.p.currentSound().parts.back().octave == w.rig.p.currentSound().parts[0].octave + 1;
            lp.removeLastLayer();
            PeerWindow::pump (20);
            check (added && (int) w.rig.p.currentSound().parts.size() == before, "report1 #15: + Add layer (an octave up) adds it, removing it brings the one layer back");
        }
        // 16: what the host shows
        {
            juce::RangedAudioParameter* cut = nullptr;
            for (auto* prm : w.rig.p.getParameters())
                if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (prm); rp != nullptr && rp->getParameterID() == VaultProcessor::idQuick[qCutoff]) cut = rp;
            juce::String msgText, hostText;
            float back = -1;
            if (cut != nullptr)
            {
                msgText = cut->getText (0.3f, 64);
                std::thread ([&] { hostText = cut->getText (0.3f, 64); back = cut->getValueForText (hostText); }).join();
            }
            check (cut != nullptr && hostText.containsIgnoreCase ("oct") && ! hostText.contains ("%") && msgText.isNotEmpty() && ! msgText.endsWith ("%")
                       && std::abs (back - 0.3f) < 0.02f,
                   "report1 #16: Cutoff at 0.3 reads '" + hostText + "' to the host ('" + msgText + "' on the message thread), typed back -> " + juce::String (back, 3));
        }
        w.close();
    }

    // ---------------------------------------------------------------- 13: the on-screen keyboard shows what sounds
    {
        PeerWindow w;
        w.open();
        auto& v = w.v();
        auto& kb = v.keys();
        int synth = 0;
        for (int i = 0; i < (int) w.rig.p.bank().size(); ++i) if (! w.rig.p.bank()[(size_t) i].usesSamples() && ! w.rig.p.bank()[(size_t) i].arp.on) { synth = i; break; }
        v.loadPreset (synth, false);
        w.run (40);
        int litAtRest = 0;
        for (int n = 0; n < 128; ++n) litAtRest += kb.isLit (n) ? 1 : 0;
        w.click (v.mainPage().quickKnob (qCutoff));
        w.key ('e');                    // E5 = 64
        w.run (12);
        const bool lit64 = kb.isLit (64) && ! kb.isLit (60);
        w.up ('e');
        w.run (40);
        bool off = true;
        for (int n = 0; n < 128; ++n) off &= ! kb.isLit (n);
        // the arp, latched: C-E-G pressed and let go, the arp keeps playing - the lit keys are the arp's notes
        w.rig.p.editSound ([] (VaultPreset& x) { x.arp.on = true; x.arp.latch = true; x.arp.mode = VaultArp::up; x.arp.octaves = 2; });
        w.rig.p.setSwitch (VaultProcessor::idArpOn, true);
        w.run (6);
        for (int c : { 'q', 'e', 't' }) w.key (c);
        w.run (6);
        for (int c : { 'q', 'e', 't' }) w.up (c);
        std::set<int> seen;
        bool onlyChord = true;
        for (int i = 0; i < 60; ++i)
        {
            w.run (3);
            for (int n = 0; n < 128; ++n)
                if (kb.isLit (n)) { seen.insert (n); onlyChord &= n == 60 || n == 64 || n == 67 || n == 72 || n == 76 || n == 79; }
        }
        w.rig.p.setSwitch (VaultProcessor::idArpOn, false);
        w.run (60);
        bool offAfter = true;
        for (int n = 0; n < 128; ++n) offAfter &= ! kb.isLit (n);
        check (litAtRest == 0 && lit64 && off && seen.size() >= 3 && onlyChord && offAfter,
               "report1 #13: nothing lit at rest (" + juce::String (litAtRest) + "), E5 lit while held, dark after; a latched arp lights its notes ("
               + juce::String ((int) seen.size()) + " seen, only the chord + octave " + juce::String ((int) onlyChord) + "), dark once the arp is off");
        w.close();
    }

    // ---------------------------------------------------------------- MUST NOT BREAK
    {
        PeerWindow w;
        w.open();
        auto& v = w.v();
        const auto& bank = w.rig.p.bank();
        // FX chain: on/off heard, reorder by drag, MAIN REVERB <-> FX page reverb
        int idx = -1;
        for (int i = 0; i < (int) bank.size() && idx < 0; ++i)
        {
            const auto& p = bank[(size_t) i];
            bool hasRev = false;
            for (auto& s : p.fx) hasRev |= s.type == FxType::reverb && s.on;
            if (p.v3 && p.fx.size() >= 3 && hasRev && ! p.usesSamples() && ! p.arp.on && ! p.gate.on) idx = i;
        }
        v.loadPreset (idx, false);
        w.run (20);
        const auto a = w.rig.note (60, 0.5, 0.5);
        int revSlot = -1;
        for (size_t s = 0; s < w.rig.p.currentSound().fx.size(); ++s) if (revSlot < 0 && w.rig.p.currentSound().fx[s].type == FxType::reverb) revSlot = (int) s;
        w.rig.p.editSound ([revSlot] (VaultPreset& x) { x.fx[(size_t) revSlot].on = false; });
        w.run (10);
        const auto b = w.rig.note (60, 0.5, 0.5);
        const float offDiff = relDiff (a, b);
        w.rig.p.editSound ([revSlot] (VaultPreset& x) { x.fx[(size_t) revSlot].on = true; });
        w.run (10);
        const auto c = w.rig.note (60, 0.5, 0.5);
        const float backDiff = relDiff (a, c);
        w.click (v.tab (VaultMainView::pageFx));
        auto& fp = v.fxPage();
        const auto t0 = w.rig.p.currentSound().fx[0].type, t1 = w.rig.p.currentSound().fx[1].type;
        fp.beginDrag (0);
        fp.dragTo (fp.cardRect (1).getCentre());
        fp.endDrag();
        PeerWindow::pump (20);
        const bool reordered = w.rig.p.currentSound().fx[0].type == t1 && w.rig.p.currentSound().fx[1].type == t0;
        w.run (10);
        bool engineOrder = w.rig.p.audioEngine().chainSize() >= 2;
        // (the engine's chain follows the sound's order)
        if (engineOrder) engineOrder = w.rig.p.audioEngine().chainType (0) == t1 || w.rig.p.audioEngine().chainSlot (0) == 0;
        fp.beginDrag (1); fp.dragTo (fp.cardRect (0).getCentre()); fp.endDrag();
        PeerWindow::pump (20);
        // MAIN REVERB moves the FX page's reverb mix knob
        float fxMixBefore = -1, fxMixAfter = -1;
        auto mixKnob = [&]
        {
            for (auto& sk : fp.slotKnobs())
                if (sk.slot == revSlot && juce::String (fxTypeInfo (FxType::reverb).params[(size_t) sk.param].id) == "mix") return sk.knob->getValue();
            return -1.0f;
        };
        fxMixBefore = mixKnob();
        w.rig.p.setQuick (qReverb, 0.95f);
        w.run (20);
        fxMixAfter = mixKnob();
        w.rig.p.setQuick (qReverb, quickInfo()[qReverb].neutral);
        w.run (10);
        check (offDiff > 0.05f && backDiff < 0.02f && reordered && engineOrder && fxMixBefore >= 0 && fxMixAfter > fxMixBefore + 0.02f,
               "report1 MUST: FX chain - reverb off is heard (" + juce::String (offDiff, 3) + "), on again restores (" + juce::String (backDiff, 3)
               + "), drag reorders the chain (" + juce::String ((int) reordered) + "), MAIN REVERB moves the FX page's mix " + juce::String (fxMixBefore, 2) + " -> " + juce::String (fxMixAfter, 2));
        // MOD: an LFO on the cutoff changes the sound; the route shows on MOD
        {
            const auto dry = w.rig.note (60, 1.0, 0.2);
            const auto dry2 = w.rig.note (60, 1.0, 0.2);
            const float base = relDiff (dry, dry2);
            vui::mod::apply (v.context(), "cutoff", 0);
            w.rig.p.editSound ([] (VaultPreset& x) { x.mod.lfo[0].rate = 3.0f; x.mod.lfo[0].depth = 1.0f; for (auto& d : x.mod.lfo[0].dests) if (d.target == "cutoff") d.amount = 2.0f; });
            w.run (10);
            const auto wet = w.rig.note (60, 1.0, 0.2);
            bool routed = false;
            for (auto& d : w.rig.p.currentSound().mod.lfo[0].dests) routed |= d.target == "cutoff";
            w.click (v.tab (VaultMainView::pageMod));
            check (routed && relDiff (dry, wet) > std::max (0.02f, 2.0f * base), "report1 MUST: MOD - Modulate with LFO 1 on the cutoff is routed and heard ("
                   + juce::String (relDiff (dry, wet), 3) + ", the same note twice without it " + juce::String (base, 3) + ")");
        }
        // ARP: every order plays notes; 1/16 at 140 BPM from the host: steps 60/140/4 s apart; latch keeps playing
        {
            struct Head : juce::AudioPlayHead
            {
                double bpm = 140, ppq = 0; bool playing = true; juce::int64 samples = 0;
                juce::Optional<PositionInfo> getPosition() const override
                {
                    PositionInfo i; i.setBpm (bpm); i.setIsPlaying (playing); i.setPpqPosition (ppq); i.setTimeInSamples (samples);
                    i.setTimeSignature (juce::AudioPlayHead::TimeSignature { 4, 4 });
                    return i;
                }
            } head;
            w.rig.p.setPlayHead (&head);
            int ordersOk = 0;
            double worstMs = 0;
            for (int mode = 0; mode < VaultArp::numModes; ++mode)
            {
                w.rig.p.editSound ([mode] (VaultPreset& x)
                {
                    x.arp = VaultArp(); x.arp.on = true; x.arp.mode = (uint8_t) mode; x.arp.octaves = 1; x.arp.latch = false;
                    x.arp.rate = (uint8_t) vui::ArpGatePage::rateIndex (4, 0);
                });
                w.rig.p.setSwitch (VaultProcessor::idArpOn, true);
                w.run (4);
                w.rig.p.audioEngine();   // (const)
                const_cast<VaultEngine&> (w.rig.p.audioEngine()).clearNoteLog();
                juce::AudioBuffer<float> buf (2, Rig::bs);
                std::vector<juce::int64> ons;
                for (int i = 0; i < 260; ++i)
                {
                    juce::MidiBuffer m;
                    if (i == 0) { m.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100), 0); m.addEvent (juce::MidiMessage::noteOn (1, 64, (juce::uint8) 100), 0); m.addEvent (juce::MidiMessage::noteOn (1, 67, (juce::uint8) 100), 0); }
                    if (i == 250) for (int n : { 60, 64, 67 }) m.addEvent (juce::MidiMessage::noteOff (1, n), 0);
                    buf.clear();
                    w.rig.p.processBlock (buf, m);
                    head.samples += Rig::bs; head.ppq += Rig::bs / 44100.0 * head.bpm / 60.0;
                }
                VaultEngine::NoteLog log[512];
                const int n = w.rig.p.audioEngine().readNoteLog (log, 512);
                for (int i = 0; i < n; ++i) if (log[i].on) ons.push_back (log[i].sample);
                const double stepS = 60.0 / 140.0 / 4.0 * 44100.0;
                std::set<juce::int64> uniq (ons.begin(), ons.end());
                std::vector<juce::int64> u (uniq.begin(), uniq.end());
                bool even = u.size() >= 8;
                // (the first note may come up to a quarter step after its grid line: the chord was pressed just after the line
                // and the arp's "late chord" catch plays that step at once - VaultMod.cpp Arp::realign; every later note is on the grid)
                for (size_t i = 2; i < u.size(); ++i)
                {
                    const double d = (double) (u[i] - u[i - 1]);
                    const double err = std::abs (d - stepS * std::round (d / stepS));
                    worstMs = std::max (worstMs, err / 44.1);
                    even &= err <= 1.0;
                }
                ordersOk += even ? 1 : 0;
                if (! even)
                {
                    juce::String l;
                    for (int i = 0; i < n && i < 40; ++i) l << (log[i].on ? "+" : "-") << log[i].note << "@" << juce::String ((double) log[i].sample / stepS, 2) << " ";
                    std::printf ("   arp order %d (%s): %d unique onsets: %s\n", mode, vui::ArpGatePage::orderNames()[mode].toRawUTF8(), (int) u.size(), l.toRawUTF8());
                }
                w.run (20);
            }
            w.rig.p.setPlayHead (nullptr);
            check (ordersOk == VaultArp::numModes, "report1 MUST: ARP - all " + juce::String ((int) VaultArp::numModes) + " orders play 1/16 steps locked to the host's 140 BPM ("
                   + juce::String (ordersOk) + " ok, worst " + juce::String (worstMs, 3) + " ms)");
            w.rig.p.setSwitch (VaultProcessor::idArpOn, false);
            w.rig.p.revertAll();
            w.run (20);
        }
        // GATE: the level is chopped at the step rate
        {
            w.rig.p.editSound ([] (VaultPreset& x) { x.gate = VaultGate(); x.gate.on = true; x.gate.depth = 1.0f; x.gate.mix = 1.0f; x.gate.rate = (uint8_t) vui::ArpGatePage::rateIndex (3, 0); });
            w.rig.p.setSwitch (VaultProcessor::idGateOn, true);
            w.run (10);
            const auto g = w.rig.note (60, 1.5, 0.1);
            float lo = 1e9f, hi = 0;
            for (int s = (int) (0.4 * 44100); s + 512 < (int) (1.4 * 44100); s += 256) { const float r = g.getRMSLevel (0, s, 256); lo = std::min (lo, r); hi = std::max (hi, r); }
            w.rig.p.setSwitch (VaultProcessor::idGateOn, false);
            w.rig.p.revertAll();
            check (hi > 0.01f && lo < hi * 0.2f, "report1 MUST: GATE chops the held note (" + juce::String (juce::Decibels::gainToDecibels (lo / std::max (1e-9f, hi)), 1) + " dB between steps)");
        }
        // Favorites, Recent, arrows, Drag WAV
        {
            const int a0 = w.rig.p.currentPreset();
            v.toggleFavorite (a0);
            v.setFavoritesOnly (true);
            const bool favShown = v.visibleCount() >= 1 && v.browser().isFavorite (a0);
            v.setFavoritesOnly (false);
            v.toggleFavorite (a0);
            v.setCategory (-1); v.setQuery ({});
            w.click (v.tab (VaultMainView::pageMain));
            v.step (1, false);
            const int a1 = w.rig.p.currentPreset();
            v.step (1, false);
            const int a2 = w.rig.p.currentPreset();
            v.step (-1, false);
            const bool arrows = a1 != a0 && a2 != a1 && w.rig.p.currentPreset() == a1;
            v.setRecentOnly (true);
            const bool recent = v.visibleCount() >= 2;
            v.setRecentOnly (false);
            const auto wav = v.renderWavForDrag();
            juce::AudioFormatManager fm; fm.registerBasicFormats();
            std::unique_ptr<juce::AudioFormatReader> rd (fm.createReaderFor (wav));
            float pk = 0;
            if (rd != nullptr) { juce::AudioBuffer<float> wb ((int) rd->numChannels, (int) rd->lengthInSamples); rd->read (&wb, 0, (int) rd->lengthInSamples, 0, true, true); pk = wb.getMagnitude (0, wb.getNumSamples()); }
            check (favShown && arrows && recent && rd != nullptr && pk > 0.01f && wav.getFileName().contains ("C5"),
                   "report1 MUST: Favorites (" + juce::String ((int) favShown) + "), arrows (" + juce::String ((int) arrows) + "), Recent (" + juce::String ((int) recent)
                   + "), Drag WAV '" + wav.getFileName() + "' peak " + juce::String (pk, 3));
        }
        w.close();
    }

    // ---------------------------------------------------------------- settings kept: window closed, project saved / reopened
    {
        PeerWindow w;
        w.open();
        auto& v = w.v();
        v.setCategory (catKeysOrgan);
        w.run (4);
        const int idx = v.browser().visible()[(size_t) std::min (7, (int) v.browser().visible().size() - 1)];
        v.loadPreset (idx, false);
        w.run (10);
        w.rig.p.setQuick (qReverb, 0.9f);
        w.rig.p.setQuick (qCutoff, 0.3f);
        w.rig.p.setMacro (2, 0.15f);
        w.rig.p.setSwitch (VaultProcessor::idGateOn, true);
        w.rig.p.setLock (VaultProcessor::Section::fx, true);
        w.rig.p.editSound ([] (VaultPreset& x) { x.parts[0].level = 0.42f; });
        v.toggleFavorite (idx);
        w.ed->setUiScale (1.15f);
        w.run (6);
        auto summary = [] (VaultProcessor& p)
        {
            juce::String s;
            s << p.currentSound().id << " q " << juce::String (p.getQuick (qReverb), 3) << "/" << juce::String (p.getQuick (qCutoff), 3) << " m " << juce::String (p.getMacro (2), 3)
              << " gate " << (int) p.getSwitch (VaultProcessor::idGateOn) << " lock " << (int) p.getLock (VaultProcessor::Section::fx)
              << " lvl " << juce::String (p.currentSound().parts[0].level, 3) << " edited " << (int) p.hasEdits();
            return s;
        };
        const auto want = summary (w.rig.p);
        w.close();
        w.open();
        const bool reopenWin = summary (w.rig.p) == want && w.v().browser().filter.category == catKeysOrgan && std::abs (w.ed->getUiScale() - 1.15f) < 0.01f
                            && w.v().browser().isFavorite (idx) && w.v().mainPage().quickKnob (qReverb).getValue() > 0.0f;
        juce::MemoryBlock st;
        w.rig.p.getStateInformation (st);
        w.close();
        PeerWindow w2;
        w2.rig.p.setStateInformation (st.getData(), (int) st.getSize());
        w2.run (10);
        const auto got = summary (w2.rig.p);
        w2.open();
        const bool project = got == want && w2.v().browser().filter.category == catKeysOrgan;
        check (reopenWin && project, "report1 MUST: settings kept after closing the window (" + juce::String ((int) reopenWin) + ") and a saved / reopened project ("
               + juce::String ((int) project) + "): " + want + (got == want ? juce::String() : " vs " + got));
        w2.rig.p.setLock (VaultProcessor::Section::fx, false);
        w2.v().toggleFavorite (idx);
        w2.close();
        VaultSettings::get()->setScale (1.0f);
    }

    const int f = fails - before;
    std::printf ("vault report1 failures: %d\n", f);
    return f;
}
