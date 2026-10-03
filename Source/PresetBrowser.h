#pragma once
// SOUND LIBRARY (preset browser): colourful category chips, search, filters, NEW sounds, favourites, your presets.

// every category has its own colour - you see at a glance what you pick
inline Colour categoryColour (int cat)
{
    static const uint32 c[] { 0xffff5d8f, 0xffff8a3d, 0xff7cdcff, 0xffffd23f, 0xffffb86b, 0xffe0a060, 0xffc77dff, 0xffffc23d, 0xfff0a6ff, 0xff6ee7b7,
                              0xffff3b5c, 0xff8b8bff, 0xff4dd2ff, 0xff3d8bff, 0xff3d5bff, 0xffa78bfa, 0xff34d399, 0xffff3fd2,
                              0xfff59e0b, 0xff22e07a, 0xff2dd4bf, 0xffff6b4a, 0xffa3e635, 0xff9b4dff };
    return isPositiveAndBelow (cat, (int) (sizeof (c) / sizeof (c[0]))) ? Colour (c[cat]).withMultipliedSaturation (0.55f).withMultipliedBrightness (kk::theme().night ? 0.95f : 0.8f) : TC (0xffff2f6d);   // v0.34: calm category colours
}

class PresetBrowser : public Component, public FileDragAndDropTarget, private ListBoxModel
{
public:
    std::function<void()> focusKeys;   // after a click: let the keys play, not type
    std::function<void()> onChanged;
    std::function<StringArray()> getFavourites;
    std::function<void (const String&)> toggleFavourite;
    std::function<void (int)> pickHandler;          // BREED LAB: choose a parent instead of loading
    std::function<void()> onBred;
    String title { "PRESETS" };

    PresetBrowser (KeysKillaProcessor& p, KKLookAndFeel& l) : proc (p), lnf (l)
    {
        search.setTextToShowWhenEmpty ("Search presets...", lnf.skin->textDim);
        search.setFont (serif (24.0f, false, 0.05f));
        search.onTextChange = [this] { refresh(); };
        search.setTooltip ("Type part of a preset name, category, era or sub-category.");
        addAndMakeVisible (search);

        category.addItem ("All categories", 1);
        for (int c = 0; c < numCategories; ++c) if (c != c808) category.addItem (categoryNames()[c], c + 2);   // 808 sounds are in BASS
        category.onChange = [this] { fillSubs(); refresh(); };
        subcat.onChange = [this] { if (packsOnly) packSel = subcat.getSelectedId() - 2; refresh(); };
        fillSubs();
        era.addItem ("All eras", 1);
        for (int e = 0; e < numEras; ++e) era.addItem (eraNames()[e], e + 2);
        era.onChange = [this] { refresh(); };
        auto fill = [this] (ComboBox& cb, const String& all, const StringArray& items)
        {
            cb.addItem (all, 1);
            for (int i = 0; i < items.size(); ++i) cb.addItem (items[i], i + 2);
            cb.setSelectedId (1, dontSendNotification);
            cb.onChange = [this] { refresh(); };
            addAndMakeVisible (cb);
        };
        fill (mood, "Mood", moodNames());
        fill (character, "Character", characterNames());
        fill (artic, "Articulation", articulationNames());
        fill (voicing, "Mono/Poly", { "Mono", "Poly" });
        fill (bright, "Brightness", { "Dark (1-2)", "Medium (3)", "Bright (4-5)" });
        fill (motion, "Movement", { "Static (0-1)", "Moving (2-3)", "Animated (4-5)" });
        fill (cpuSel, "CPU", { "Light", "Medium", "Heavy" });
        category.setTooltip ("Category"); subcat.setTooltip ("Subcategory"); era.setTooltip ("Era 2010 -> FUTURE");
        mood.setTooltip ("Mood"); character.setTooltip ("Character"); artic.setTooltip ("Articulation");
        voicing.setTooltip ("Mono / poly"); bright.setTooltip ("Brightness"); motion.setTooltip ("Movement"); cpuSel.setTooltip ("CPU class");
        for (auto* b : { &exclusiveOnly, &favOnly, &userOnly })
        {
            b->setClickingTogglesState (true);
            b->onClick = [this] { refresh(); };
            addAndMakeVisible (*b);
        }
        exclusiveOnly.setButtonText ("EXCLUSIVE"); favOnly.setButtonText ("FAVOURITES"); userOnly.setButtonText ("USER");
        addChildComponent (category); addAndMakeVisible (subcat); addChildComponent (era);   // eras are not shown any more (v0.7); chips pick the category
        for (auto* b : { &exclusiveOnly, &favOnly, &userOnly }) b->setVisible (false);
        chips.push_back ({ "ALL SOUNDS", -1, 0 }); chips.push_back ({ "NEW", -1, 3 }); chips.push_back ({ "FAVOURITES", -1, 2 }); chips.push_back ({ "MY PRESETS", -1, 1 });
        chips.push_back ({ "PACKS", -1, 4 });
        for (int c : { (int) cPiano, (int) cKeys, (int) cOrgan, (int) cBells, (int) cMallets, (int) cPlucks, (int) cGuitar, (int) cStrings, (int) cBrass,
                       (int) cWoodwind, (int) cWorld, (int) cChoir, (int) cLead, (int) cSynth, (int) cPads, (int) cBass, (int) cChip, (int) cArp,
                       (int) cTexture, (int) cDrums, (int) cFX, (int) cGameFx, (int) cCinematic })
            chips.push_back ({ categoryNames()[c], c, 0 });
        for (auto& ch : chips)
            for (auto& pr : factoryPresets()) ch.count += (ch.cat >= 0 && pr.cat == ch.cat) || (ch.cat < 0 && ch.special == 0) || (ch.special == 3 && pr.version == "0.30");
        list.setModel (this);
        list.setRowHeight (40);
        list.setColour (ListBox::backgroundColourId, Colours::transparentBlack);
        addAndMakeVisible (list);
        installBtn.setButtonText ("INSTALL PACK");
        installBtn.setTooltip ("Install a sound pack (.kkpack) - or just drag the file into this window");
        installBtn.onClick = [this] { chooseAndInstall(); };
        addAndMakeVisible (installBtn);
        closeBtn.setButtonText ("CLOSE");
        closeBtn.onClick = [this] { setVisible (false); };
        addAndMakeVisible (closeBtn);
        // v0.37 choosing (a seed / a parent): click = hear it, USE IT (or a double-click) = take it
        useBtn.setButtonText ("USE IT");
        useBtn.setColour (TextButton::buttonColourId, kk::theme().accent);
        useBtn.setColour (TextButton::textColourOffId, Colours::white);
        useBtn.onClick = [this] { if (pickRow >= 0) activate (pickRow, true); };
        addChildComponent (useBtn);
        count.setFont (serif (18.0f, false, 0.1f));
        addAndMakeVisible (count);
    }

    void open (int cat, int eraIdx, bool exclusive, std::function<void (int)> pick = nullptr, const String& heading = "PRESETS")
    {
        pickHandler = std::move (pick); title = heading;
        pickRow = -1; useBtn.setVisible (false);
        proc.rescanPacks(); updatePackCount(); packsOnly = false; packSel = -1;
        category.setSelectedId (cat >= 0 ? cat + 2 : 1, dontSendNotification);
        fillSubs();
        if (cat >= 0 && proc.uiSub >= 0 && ! pick) subcat.setSelectedId (proc.uiSub + 2, dontSendNotification);
        era.setSelectedId (1, dontSendNotification); ignoreUnused (eraIdx);
        exclusiveOnly.setToggleState (exclusive, dontSendNotification);
        newOnly = false; favOnly.setToggleState (false, dontSendNotification); userOnly.setToggleState (false, dontSendNotification);
        refresh();
        setVisible (true); toFront (true);
        if (focusKeys) focusKeys();   // the computer keys play notes; click the search box to type
    }

    void paint (Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        {
            Graphics::ScopedSaveState ss (g);
            Path clip; clip.addRoundedRectangle (r.reduced (4), 12); g.reduceClipRegion (clip);
            pageBackdrop (g, *this, 0.8f);
        }
        auto glow = [] (Point<float>, float, Colour) {};   // v0.34: calm - no coloured light
        glow ({ r.getWidth() * 0.15f, 30 }, 380, TC (0xffff2f6d)); glow ({ r.getWidth() * 0.85f, 60 }, 380, TC (0xff9b4dff)); glow ({ r.getWidth() * 0.5f, r.getBottom() }, 420, TC (0xff22d3ee));
        g.setGradientFill (ColourGradient (TC (0xffff2f6d), 0, 0, TC (0xff9b4dff), r.getRight(), r.getBottom(), false));
        g.drawRoundedRectangle (r.reduced (4), 12, 1.6f);
        // title
        g.setColour (TC (0xffffffff)); g.setFont (Font (FontOptions (26.0f, Font::bold)).withExtraKerningFactor (0.06f));
        const bool lib = title == "PRESETS";
        g.drawText (lib ? "SOUND" : title, 24, 12, 400, 34, Justification::centredLeft);
        if (lib)
        {
            g.setGradientFill (ColourGradient (TC (0xffff2f6d), 140, 0, TC (0xffff8a3d), 300, 0, false));
            g.drawText ("LIBRARY", 134, 12, 300, 34, Justification::centredLeft);
        }
        // category chips
        for (int i = 0; i < (int) chips.size(); ++i)
        {
            const auto& ch = chips[(size_t) i];
            auto cr = chipRect (i).reduced (3, 3);
            const bool sel = i == chipSel, hot = i == chipHot;
            const Colour col = ch.cat >= 0 ? categoryColour (ch.cat) : ch.special == 3 ? TC (0xff36ff6a) : ch.special == 2 ? TC (0xffffd23f)
                             : ch.special == 1 ? TC (0xff22d3ee) : ch.special == 4 ? TC (0xffff8a3d) : TC (0xffff2f6d);
            if (sel)
            {
                g.setColour (col.withAlpha (0.3f)); g.fillRoundedRectangle (cr.expanded (3), 10);
                g.setGradientFill (ColourGradient (col, cr.getX(), cr.getY(), col.darker (0.45f), cr.getRight(), cr.getBottom(), false)); g.fillRoundedRectangle (cr, 8);
            }
            else
            {
                g.setGradientFill (ColourGradient (col.withAlpha (hot ? 0.32f : 0.2f), cr.getX(), cr.getY(), col.withAlpha (0.05f), cr.getRight(), cr.getBottom(), false));
                g.fillRoundedRectangle (cr, 8);
                g.setColour (col.withAlpha (hot ? 0.95f : 0.6f)); g.drawRoundedRectangle (cr, 8, 1.2f);
            }
            g.setColour (sel ? TC (0xffffffff) : col); g.fillEllipse (cr.getX() + 9, cr.getCentreY() - 4, 8, 8);
            g.setColour (sel ? TC (0xffffffff) : TC (0xffeeeaff)); g.setFont (Font (FontOptions (13.0f, Font::bold)).withExtraKerningFactor (0.05f));
            g.drawFittedText (ch.name, cr.withTrimmedLeft (22).withTrimmedRight (30).toNearestInt(), Justification::centredLeft, 1, 0.65f);
            if (ch.count > 0)
            {
                g.setColour (sel ? TC (0xffffffff).withAlpha (0.85f) : col.withAlpha (0.85f)); g.setFont (Font (FontOptions (11.0f)));
                g.drawText (String (ch.count), cr.withTrimmedRight (8).toNearestInt(), Justification::centredRight);
            }
        }
    }
    void mouseMove (const MouseEvent& e) override { int h = -1; for (int i = 0; i < (int) chips.size(); ++i) if (chipRect (i).contains (e.position)) h = i; if (h != chipHot) { chipHot = h; repaint(); } }
    void mouseExit (const MouseEvent&) override { chipHot = -1; repaint(); }
    void mouseDown (const MouseEvent& e) override
    {
        for (int i = 0; i < (int) chips.size(); ++i)
            if (chipRect (i).contains (e.position)) { selectChip (i); return; }
    }

    void resized() override
    {
        closeBtn.setBounds (getWidth() - 140, 14, 116, 36);
        installBtn.setBounds (getWidth() - 300, 14, 150, 36);
        useBtn.setBounds (getWidth() - 560, 12, 250, 40);
        search.setBounds (300, 14, getWidth() - 300 - 320, 36);
        auto r = getLocalBounds().reduced (20).withTrimmedTop (chipsBottom() - 10);
        auto row2 = r.removeFromTop (34);
        subcat.setBounds (row2.removeFromLeft (230)); row2.removeFromLeft (6);
        const int w = row2.getWidth() / 7;
        for (auto* cb : { &mood, &character, &artic, &voicing, &bright, &motion, &cpuSel })
            cb->setBounds (row2.removeFromLeft (w).reduced (3, 0));
        r.removeFromTop (8);
        count.setBounds (r.removeFromBottom (24));
        list.setBounds (r);
    }

    void refresh()
    {
        entries.clear();
        const auto q = search.getText().trim().toLowerCase();
        const auto favs = getFavourites ? getFavourites() : StringArray();
        const int cat = category.getSelectedId() - 2, er = era.getSelectedId() - 2, sb = packsOnly ? -1 : subcat.getSelectedId() - 2;
        const int md = mood.getSelectedId() - 2, chx = character.getSelectedId() - 2, ar = artic.getSelectedId() - 2;
        const int vo = voicing.getSelectedId() - 2, br = bright.getSelectedId() - 2, mv = motion.getSelectedId() - 2, cp = cpuSel.getSelectedId() - 2;
        const bool tagFilter = sb >= 0 || md >= 0 || chx >= 0 || ar >= 0 || vo >= 0 || br >= 0 || mv >= 0 || cp >= 0;
        auto matches = [&] (const String& hay) { return q.isEmpty() || hay.toLowerCase().contains (q); };
        if (! exclusiveOnly.getToggleState() && ! newOnly)
            for (auto& f : proc.userPresets())
            {
                const auto name = f.getFileNameWithoutExtension();
                if (cat >= 0 || er >= 0 || tagFilter) continue;
                if (favOnly.getToggleState() && ! favs.contains (name)) continue;
                if (packsOnly) continue;
                if (matches (name + " user")) entries.push_back ({ name, "MY PRESET", -1, f });
            }
        if (! exclusiveOnly.getToggleState() && ! newOnly && ! userOnly.getToggleState() && ! (packsOnly && packSel == 0) && er < 0 && sb < 0 && ! tagFilter)
            for (auto& ps : proc.packSounds())   // sounds from installed packs
            {
                if (packsOnly && packSel > 0 && ps.pack != proc.packs()[(size_t) (packSel - 1)].name) continue;
                if (cat >= 0 && ps.cat != cat) continue;
                if (favOnly.getToggleState() && ! favs.contains (ps.name)) continue;
                if (matches (ps.name + " " + ps.pack + " pack")) entries.push_back ({ ps.name, ps.pack, -1, ps.file, ps.pack, ps.cat });
            }
        if (! userOnly.getToggleState() && ! (packsOnly && packSel != 0))
        {
            const auto& ps = factoryPresets();
            for (int i = 0; i < (int) ps.size(); ++i)
            {
                const auto& pr = ps[(size_t) i];
                if (cat >= 0 && pr.cat != cat) continue;
                if (sb >= 0 && pr.sub != subcategoryNames (cat)[sb]) continue;
                if (er >= 0 && pr.era != er) continue;
                if (md >= 0 && pr.mood != moodNames()[md]) continue;
                if (chx >= 0 && ! pr.character.contains (characterNames()[chx])) continue;
                if (ar >= 0 && pr.articulation != articulationNames()[ar]) continue;
                if (vo >= 0 && pr.mono != (vo == 0)) continue;
                if (br >= 0 && (br == 0 ? pr.brightness > 2 : br == 1 ? pr.brightness != 3 : pr.brightness < 4)) continue;
                if (mv >= 0 && (mv == 0 ? pr.movement > 1 : mv == 1 ? (pr.movement < 2 || pr.movement > 3) : pr.movement < 4)) continue;
                if (cp >= 0 && pr.cpu != cp + 1) continue;
                if (exclusiveOnly.getToggleState() && ! pr.exclusive) continue;
                if (newOnly && pr.version != "0.30") continue;
                if (favOnly.getToggleState() && ! favs.contains (pr.name)) continue;
                const auto info = pr.info();
                if (q.isEmpty() || pr.searchText().contains (q) || pr.legacyName.toLowerCase().contains (q)) entries.push_back ({ pr.name, info, i, {}, {}, pr.cat });
            }
        }
        list.updateContent();
        list.repaint();
        count.setText (String ((int) entries.size()) + " sounds   -   click = hear it and play it on the keys,  double-click = load and close,  right-click = more", dontSendNotification);
        int want = 0;
        if (userOnly.getToggleState()) want = 3; else if (favOnly.getToggleState()) want = 2; else if (newOnly) want = 1; else if (packsOnly) want = 4;
        else for (int i = 5; i < (int) chips.size(); ++i) if (chips[(size_t) i].cat == cat) want = i;
        if (want != chipSel) { chipSel = want; repaint(); }
    }

private:
    struct Entry { String name, info; int factoryIndex; File file; String pack; int cat = -1; };

    int getNumRows() override { return (int) entries.size(); }

    void paintListBoxItem (int row, Graphics& g, int w, int h, bool selected) override
    {
        if (! isPositiveAndBelow (row, (int) entries.size())) return;
        const auto& e = entries[(size_t) row];
        const bool current = (e.factoryIndex >= 0 && e.factoryIndex == proc.currentPresetIndex())
                          || (e.factoryIndex < 0 && e.file == proc.currentUserFile());
        const auto* pr = e.factoryIndex >= 0 ? &factoryPresets()[(size_t) e.factoryIndex] : nullptr;
        const bool packSound = pr == nullptr && e.pack.isNotEmpty();
        const Colour col = pr ? categoryColour (pr->cat) : packSound && e.cat >= 0 ? categoryColour (e.cat) : TC (0xff22d3ee);
        auto r = Rectangle<float> (2, 2, (float) w - 4, (float) h - 4);
        g.setGradientFill (ColourGradient (col.withAlpha (current ? 0.34f : selected ? 0.24f : (row % 2 ? 0.07f : 0.11f)), r.getX(), 0,
                                           TC (0x0015123a), r.getRight() * 0.7f, 0, false));
        g.fillRoundedRectangle (r, 7);
        if (current) { g.setColour (col); g.drawRoundedRectangle (r, 7, 1.6f); }
        g.setColour (col); g.fillRoundedRectangle (r.getX(), r.getY() + 4, 4, r.getHeight() - 8, 2);
        const auto favs = getFavourites ? getFavourites() : StringArray();
        const bool fav = favs.contains (e.name);
        g.setColour (fav ? TC (0xffffd23f) : TC (0xff6a6290));
        g.setFont (Font (FontOptions (22.0f))); g.drawText (fav ? String (CharPointer_UTF8 ("\xe2\x98\x85")) : String (CharPointer_UTF8 ("\xe2\x98\x86")), 8, 0, 30, h, Justification::centred);
        // category pill
        const String tag = pr ? categoryNames()[pr->cat] : packSound ? (e.cat >= 0 ? categoryNames()[e.cat] : String ("PACK")) : String ("MY PRESET");
        auto pill = Rectangle<float> (44, (float) h * 0.5f - 11, 150, 22);
        g.setColour (col.withAlpha (0.22f)); g.fillRoundedRectangle (pill, 11);
        g.setColour (col); g.drawRoundedRectangle (pill, 11, 1.0f);
        if (! kk::theme().night) g.setColour (col.darker (0.6f));   // day: dark text on the light pill
        g.setFont (Font (FontOptions (11.0f, Font::bold)).withExtraKerningFactor (0.06f));
        g.drawFittedText (tag, pill.reduced (8, 0).toNearestInt(), Justification::centred, 1, 0.6f);
        // name, NEW badge, tags
        g.setColour (TC (0xffffffff)); g.setFont (Font (FontOptions (19.0f, Font::bold)));
        g.drawFittedText (e.name, 206, 0, w * 2 / 5, h, Justification::centredLeft, 1, 0.8f);
        if (pr && pr->version == "0.30")
        {
            GlyphArrangement ga; ga.addLineOfText (Font (FontOptions (19.0f, Font::bold)), e.name, 0, 0);
            const float nx = 206.0f + jmin ((float) w * 0.4f, ga.getBoundingBox (0, -1, true).getWidth()) + 10.0f;
            auto badge = Rectangle<float> (nx, (float) h * 0.5f - 9, 40, 18);
            g.setGradientFill (ColourGradient (TC (0xff36ff6a), badge.getX(), 0, TC (0xff22d3ee), badge.getRight(), 0, false)); g.fillRoundedRectangle (badge, 9);
            g.setColour (TC (0xff0a0920)); g.setFont (Font (FontOptions (10.5f, Font::bold))); g.drawText ("NEW", badge, Justification::centred);
        }
        g.setColour (TC (0xffaaa4cf)); g.setFont (Font (FontOptions (13.0f)));
        String info = pr ? pr->sub.toUpperCase() + "   " + pr->mood + " . " + pr->articulation + (pr->mono ? " . MONO" : "") : packSound ? String() : String ("your sound");
        g.drawFittedText (info, w * 2 / 5 + 260, 0, w - (w * 2 / 5 + 260) - 150, h, Justification::centredRight, 1, 0.8f);
        // pack tag: which sound pack the sound comes from (factory sounds = FACTORY)
        const String packTag = pr ? String ("FACTORY") : packSound ? e.pack.toUpperCase() : String ("MY SOUNDS");
        auto pt = Rectangle<float> ((float) w - 136, (float) h * 0.5f - 10, 124, 20);
        g.setColour (TC (0xffff8a3d).withAlpha (pr ? 0.35f : 0.8f)); g.drawRoundedRectangle (pt, 10, 1.0f);
        g.setColour ((kk::theme().night ? kk::theme().accent : kk::theme().accentDeep.darker (0.2f)).withAlpha (pr ? 0.7f : 1.0f)); g.setFont (Font (FontOptions (10.5f, Font::bold)).withExtraKerningFactor (0.08f));
        g.drawFittedText (packTag, pt.reduced (8, 0).toNearestInt(), Justification::centred, 1, 0.6f);
    }

    void activate (int row, bool close)
    {
        if (! isPositiveAndBelow (row, (int) entries.size())) return;
        const auto en = entries[(size_t) row];
        if (pickHandler)
        {
            if (en.factoryIndex < 0) proc.loadUserPreset (en.file);   // user sound: load it, then it becomes the parent
            auto h = pickHandler; pickHandler = nullptr; title = "PRESETS";
            pickRow = -1; useBtn.setVisible (false);
            h (en.factoryIndex);
            setVisible (false);
            return;
        }
        // the header arrows keep browsing inside the category / subcategory chosen here
        proc.uiCat = category.getSelectedId() - 2;
        proc.uiSub = proc.uiCat >= 0 ? subcat.getSelectedId() - 2 : -1;
        if (en.factoryIndex >= 0) proc.loadPreset (en.factoryIndex); else proc.loadUserPreset (en.file);
        list.repaint();
        if (onChanged) onChanged();
        if (close) setVisible (false);
    }

    void listBoxItemClicked (int row, const MouseEvent& e) override
    {
        if (! isPositiveAndBelow (row, (int) entries.size())) return;
        if (e.x < 40) { if (toggleFavourite) toggleFavourite (entries[(size_t) row].name); list.repaint(); return; }
        if (e.mods.isPopupMenu())
        {
            const auto en = entries[(size_t) row];
            PopupMenu m;
            m.addItem (1, "Load");
            m.addItem (2, "BREED: current sound x this preset", en.factoryIndex >= 0);
            m.addItem (4, "Use as PARENT A", en.factoryIndex >= 0);
            m.addItem (5, "Use as PARENT B", en.factoryIndex >= 0);
            m.addItem (3, "Favourite on / off");
            if (en.factoryIndex < 0 && en.pack.isEmpty()) { m.addSeparator(); m.addItem (6, "Delete this preset (Del)"); }
            m.showMenuAsync (PopupMenu::Options(), [this, row, en, safe = Component::SafePointer<Component> (this)] (int r)
            {
                if (safe == nullptr || r == 0) return;
                if (r == 1) activate (row, false);
                else if (r == 2) { proc.setParentCurrent (0); proc.setParentPreset (1, en.factoryIndex); proc.breed(); setVisible (false); if (onBred) onBred(); }
                else if (r == 4 || r == 5) { proc.setParentPreset (r - 4, en.factoryIndex); if (onBred) onBred(); }
                else if (r == 3 && toggleFavourite) { toggleFavourite (en.name); list.repaint(); }
                else if (r == 6) askDelete (row);
            });
            return;
        }
        if (pickHandler)   // choosing: hear it first - USE IT (or double-click) takes it
        {
            const auto en = entries[(size_t) row];
            if (en.factoryIndex >= 0) proc.loadPreset (en.factoryIndex); else proc.loadUserPreset (en.file);
            proc.previewNote = proc.apvts.getRawParameterValue (ID::bassMode)->load() > 0.5f ? 36 : 60;
            pickRow = row;
            useBtn.setButtonText ("USE  " + en.name.toUpperCase().substring (0, 22));
            useBtn.setVisible (true);
            if (focusKeys) focusKeys();
            return;
        }
        activate (row, false);
        // hear it straight away, and keep the computer keys / MIDI keyboard playing it (not typing)
        proc.previewNote = proc.apvts.getRawParameterValue (ID::bassMode)->load() > 0.5f ? 36 : 60;
        if (focusKeys) focusKeys();
    }

    void listBoxItemDoubleClicked (int row, const MouseEvent&) override { activate (row, true); }
    void deleteKeyPressed (int row) override { askDelete (row); }
    // v0.35: your own presets can be deleted (to the recycle bin) - factory sounds stay
    void askDelete (int row)
    {
        if (! isPositiveAndBelow (row, (int) entries.size())) return;
        const auto en = entries[(size_t) row];
        if (en.factoryIndex >= 0 || en.pack.isNotEmpty() || ! en.file.existsAsFile()) return;
        AlertWindow::showOkCancelBox (MessageBoxIconType::QuestionIcon, "DELETE PRESET", "Delete \"" + en.name + "\"?  (it goes to the recycle bin)", "DELETE", "CANCEL", this,
            ModalCallbackFunction::create ([this, en, safe = Component::SafePointer<Component> (this)] (int res)
            {
                if (safe == nullptr || res == 0) return;
                if (en.file == proc.currentUserFile()) proc.deleteUserPreset(); else en.file.moveToTrash();
                refresh();
                if (onChanged) onChanged();
            }));
    }
    void returnKeyPressed (int row) override { activate (row, true); }

    KeysKillaProcessor& proc;
    KKLookAndFeel& lnf;
    TextEditor search;
    ComboBox category, subcat, era, mood, character, artic, voicing, bright, motion, cpuSel;
    void fillSubs()
    {
        subcat.clear (dontSendNotification);
        if (packsOnly)
        {
            subcat.addItem ("All packs", 1); subcat.addItem ("FACTORY", 2);
            for (int k = 0; k < (int) proc.packs().size(); ++k) subcat.addItem (proc.packs()[(size_t) k].name, k + 3);
            subcat.setSelectedId (packSel + 2, dontSendNotification); subcat.setEnabled (true);
            return;
        }
        subcat.addItem ("All subcategories", 1);
        const int c = category.getSelectedId() - 2;
        if (c >= 0) { const auto& subs = subcategoryNames (c); for (int i = 0; i < subs.size(); ++i) subcat.addItem (subs[i], i + 2); }
        subcat.setSelectedId (1, dontSendNotification);
        subcat.setEnabled (c >= 0);
    }
    TextButton exclusiveOnly, favOnly, userOnly, closeBtn, installBtn, useBtn;
    int pickRow = -1;
    struct Chip { String name; int cat; int special; int count = 0; };   // special: 1 my presets, 2 favourites, 3 NEW, 4 PACKS
    std::vector<Chip> chips;
    int chipSel = 0, chipHot = -1;
    bool newOnly = false, packsOnly = false, dropHot = false;
    int packSel = -1;                       // PACKS: -1 all packs, 0 FACTORY, 1.. installed packs
    std::unique_ptr<FileChooser> chooser;
    static constexpr int chipCols = 10;
    int chipsBottom() const { return 60 + ((int) chips.size() + chipCols - 1) / chipCols * 38 + 14; }
    Rectangle<float> chipRect (int i) const
    {
        const float x0 = 20, w = ((float) getWidth() - 40) / (float) chipCols;
        return { x0 + (float) (i % chipCols) * w, 60.0f + (float) (i / chipCols) * 38.0f, w, 38.0f };
    }
    void selectChip (int i)
    {
        const auto& ch = chips[(size_t) i];
        userOnly.setToggleState (ch.special == 1, dontSendNotification);
        favOnly.setToggleState (ch.special == 2, dontSendNotification);
        newOnly = ch.special == 3;
        packsOnly = ch.special == 4; packSel = -1;
        category.setSelectedId (ch.cat >= 0 ? ch.cat + 2 : 1, dontSendNotification);
        fillSubs(); chipSel = i; refresh(); repaint();
    }
    void updatePackCount()
    {
        for (auto& ch : chips) if (ch.special == 4) ch.count = 1 + (int) proc.packs().size();   // FACTORY + installed packs
    }
public:
    bool isInterestedInFileDrag (const StringArray& files) override
    {
        for (auto& f : files) if (f.endsWithIgnoreCase (".kkpack") || f.endsWithIgnoreCase (".zip")) return true;
        return false;
    }
    void fileDragEnter (const StringArray&, int, int) override { dropHot = true; repaint(); }
    void fileDragExit (const StringArray&) override { dropHot = false; repaint(); }
    void filesDropped (const StringArray& files, int, int) override
    {
        dropHot = false; repaint();
        for (auto& f : files) install (File (f));
    }
    void paintOverChildren (Graphics& g) override
    {
        if (! dropHot) return;
        g.setColour (TC (0xcc0a0920)); g.fillRoundedRectangle (getLocalBounds().toFloat().reduced (4), 12);
        g.setColour (TC (0xffff8a3d)); g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (14), 12, 2.0f);
        g.setFont (Font (FontOptions (30.0f, Font::bold)).withExtraKerningFactor (0.1f));
        g.drawText ("DROP TO INSTALL THE SOUND PACK", getLocalBounds(), Justification::centred);
    }
    void install (const File& f)
    {
        String err;
        const auto name = proc.installPack (f, &err);
        if (name.isEmpty())
        {
            AlertWindow::showMessageBoxAsync (MessageBoxIconType::WarningIcon, "INSTALL PACK", f.getFileName() + ": " + err, "OK", this);
            return;
        }
        updatePackCount();
        for (int i = 0; i < (int) chips.size(); ++i) if (chips[(size_t) i].special == 4) selectChip (i);
        for (int k = 0; k < (int) proc.packs().size(); ++k) if (proc.packs()[(size_t) k].name == name) { packSel = k + 1; subcat.setSelectedId (k + 3, dontSendNotification); }
        refresh(); repaint();
    }
    void chooseAndInstall()
    {
        chooser = std::make_unique<FileChooser> ("Install a sound pack", File::getSpecialLocation (File::userDocumentsDirectory), "*.kkpack;*.zip");
        chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles,
                              [this, safe = Component::SafePointer<Component> (this)] (const FileChooser& fc)
                              { if (safe != nullptr && fc.getResult() != File()) install (fc.getResult()); });
    }
private:
    Label count;
    ListBox list;
    std::vector<Entry> entries;
};
