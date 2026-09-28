#pragma once
// Preset browser overlay: search, category / era / exclusive / favourites / user filters.

class PresetBrowser : public Component, private ListBoxModel
{
public:
    std::function<void()> onChanged;
    std::function<StringArray()> getFavourites;
    std::function<void (const String&)> toggleFavourite;

    PresetBrowser (KeysKillaProcessor& p, KKLookAndFeel& l) : proc (p), lnf (l)
    {
        search.setTextToShowWhenEmpty ("Search presets...", lnf.skin->textDim);
        search.setFont (serif (24.0f, false, 0.05f));
        search.onTextChange = [this] { refresh(); };
        search.setTooltip ("Type part of a preset name, category, era or sub-category.");
        addAndMakeVisible (search);

        category.addItem ("All categories", 1);
        for (int c = 0; c < numCategories; ++c) category.addItem (categoryNames()[c], c + 2);
        category.onChange = [this] { fillSubs(); refresh(); };
        subcat.onChange = [this] { refresh(); };
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
        addAndMakeVisible (category); addAndMakeVisible (subcat); addAndMakeVisible (era);
        list.setModel (this);
        list.setRowHeight (40);
        addAndMakeVisible (list);
        closeBtn.setButtonText ("CLOSE");
        closeBtn.onClick = [this] { setVisible (false); };
        addAndMakeVisible (closeBtn);
        count.setFont (serif (18.0f, false, 0.1f));
        addAndMakeVisible (count);
    }

    void open (int cat, int eraIdx, bool exclusive)
    {
        category.setSelectedId (cat >= 0 ? cat + 2 : 1, dontSendNotification);
        fillSubs();
        era.setSelectedId (eraIdx >= 0 ? eraIdx + 2 : 1, dontSendNotification);
        exclusiveOnly.setToggleState (exclusive, dontSendNotification);
        refresh();
        setVisible (true); toFront (true);
        search.grabKeyboardFocus();
    }

    void paint (Graphics& g) override
    {
        g.fillAll (lnf.skin->dark ? Colour (0xf2080606) : Colour (0xf2dfe3e8));
        drawPanel (g, getLocalBounds().toFloat().reduced (10), *lnf.skin, "PRESETS");
    }

    void resized() override
    {
        closeBtn.setBounds (getWidth() - 140, 14, 116, 36);
        auto r = getLocalBounds().reduced (24).withTrimmedTop (34);
        auto top = r.removeFromTop (44);
        search.setBounds (top.removeFromLeft (350)); top.removeFromLeft (8);
        category.setBounds (top.removeFromLeft (250)); top.removeFromLeft (6);
        subcat.setBounds (top.removeFromLeft (250)); top.removeFromLeft (6);
        era.setBounds (top.removeFromLeft (170)); top.removeFromLeft (6);
        exclusiveOnly.setBounds (top.removeFromLeft (200)); top.removeFromLeft (6);
        favOnly.setBounds (top);
        r.removeFromTop (6);
        auto row2 = r.removeFromTop (38);
        userOnly.setBounds (row2.removeFromRight (110));
        const int w = row2.getWidth() / 7;
        for (auto* cb : { &mood, &character, &artic, &voicing, &bright, &motion, &cpuSel })
            cb->setBounds (row2.removeFromLeft (w).reduced (3, 0));
        r.removeFromTop (8);
        count.setBounds (r.removeFromBottom (28));
        list.setBounds (r);
    }

    void refresh()
    {
        entries.clear();
        const auto q = search.getText().trim().toLowerCase();
        const auto favs = getFavourites ? getFavourites() : StringArray();
        const int cat = category.getSelectedId() - 2, er = era.getSelectedId() - 2, sb = subcat.getSelectedId() - 2;
        const int md = mood.getSelectedId() - 2, chx = character.getSelectedId() - 2, ar = artic.getSelectedId() - 2;
        const int vo = voicing.getSelectedId() - 2, br = bright.getSelectedId() - 2, mv = motion.getSelectedId() - 2, cp = cpuSel.getSelectedId() - 2;
        const bool tagFilter = sb >= 0 || md >= 0 || chx >= 0 || ar >= 0 || vo >= 0 || br >= 0 || mv >= 0 || cp >= 0;
        auto matches = [&] (const String& hay) { return q.isEmpty() || hay.toLowerCase().contains (q); };
        if (! exclusiveOnly.getToggleState())
            for (auto& f : proc.userPresets())
            {
                const auto name = f.getFileNameWithoutExtension();
                if (cat >= 0 || er >= 0 || tagFilter) continue;
                if (favOnly.getToggleState() && ! favs.contains (name)) continue;
                if (matches (name + " user")) entries.push_back ({ name, "USER", -1, f });
            }
        if (! userOnly.getToggleState())
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
                if (favOnly.getToggleState() && ! favs.contains (pr.name)) continue;
                const auto info = pr.info();
                if (q.isEmpty() || pr.searchText().contains (q)) entries.push_back ({ pr.name, info, i, {} });
            }
        }
        list.updateContent();
        list.repaint();
        count.setText (String ((int) entries.size()) + " presets", dontSendNotification);
    }

private:
    struct Entry { String name, info; int factoryIndex; File file; };

    int getNumRows() override { return (int) entries.size(); }

    void paintListBoxItem (int row, Graphics& g, int w, int h, bool selected) override
    {
        if (! isPositiveAndBelow (row, (int) entries.size())) return;
        const auto& s = *lnf.skin;
        const auto& e = entries[(size_t) row];
        const bool current = (e.factoryIndex >= 0 && e.factoryIndex == proc.currentPresetIndex())
                          || (e.factoryIndex < 0 && e.file == proc.currentUserFile());
        if (selected || current) { g.setColour (s.accent.withAlpha (current ? 0.28f : 0.14f)); g.fillRect (0, 0, w, h); }
        const auto favs = getFavourites ? getFavourites() : StringArray();
        g.setColour (favs.contains (e.name) ? s.accent : s.textDim.withAlpha (0.5f));
        g.setFont (serif (26.0f)); g.drawText (favs.contains (e.name) ? "*" : "+", 6, 0, 30, h, Justification::centred);
        g.setColour (s.text); g.setFont (serif (23.0f, false, 0.05f));
        g.drawText (e.name, 44, 0, w * 2 / 5, h, Justification::centredLeft);
        g.setColour (s.textDim); g.setFont (serif (16.0f, false, 0.06f));
        g.drawFittedText (e.info, w * 2 / 5 + 50, 0, w * 3 / 5 - 60, h, Justification::centredRight, 1, 0.8f);
    }

    void activate (int row, bool close)
    {
        if (! isPositiveAndBelow (row, (int) entries.size())) return;
        const auto en = entries[(size_t) row];
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
            m.addItem (2, "Breed with current sound", en.factoryIndex >= 0);
            m.addItem (3, "Favourite on / off");
            m.showMenuAsync (PopupMenu::Options(), [this, row, en, safe = Component::SafePointer<Component> (this)] (int r)
            {
                if (safe == nullptr || r == 0) return;
                if (r == 1) activate (row, false);
                else if (r == 2) { proc.breedWith (en.factoryIndex); proc.captureUndo(); list.repaint(); if (onChanged) onChanged(); }
                else if (r == 3 && toggleFavourite) { toggleFavourite (en.name); list.repaint(); }
            });
            return;
        }
        activate (row, false);
    }

    void listBoxItemDoubleClicked (int row, const MouseEvent&) override { activate (row, true); }
    void returnKeyPressed (int row) override { activate (row, true); }

    KeysKillaProcessor& proc;
    KKLookAndFeel& lnf;
    TextEditor search;
    ComboBox category, subcat, era, mood, character, artic, voicing, bright, motion, cpuSel;
    void fillSubs()
    {
        subcat.clear (dontSendNotification);
        subcat.addItem ("All subcategories", 1);
        const int c = category.getSelectedId() - 2;
        if (c >= 0) { const auto& subs = subcategoryNames (c); for (int i = 0; i < subs.size(); ++i) subcat.addItem (subs[i], i + 2); }
        subcat.setSelectedId (1, dontSendNotification);
        subcat.setEnabled (c >= 0);
    }
    TextButton exclusiveOnly, favOnly, userOnly, closeBtn;
    Label count;
    ListBox list;
    std::vector<Entry> entries;
};
