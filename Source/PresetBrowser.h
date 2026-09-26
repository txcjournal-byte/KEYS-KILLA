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
        search.setFont (serif (16.0f, false, 0.05f));
        search.onTextChange = [this] { refresh(); };
        search.setTooltip ("Type part of a preset name, category, era or sub-category.");
        addAndMakeVisible (search);

        category.addItem ("All categories", 1);
        for (int t = 0; t < numTiles; ++t) category.addItem (tileNames()[t], t + 2);
        category.onChange = [this] { refresh(); };
        era.addItem ("All eras", 1);
        const char* eraLabels[] { "2010-12", "2013-15", "2016-18", "2019-21", "2022-24", "2025-26" };
        for (int e = 0; e < 6; ++e) era.addItem (eraLabels[e], e + 2);
        era.onChange = [this] { refresh(); };
        for (auto* b : { &exclusiveOnly, &favOnly, &userOnly })
        {
            b->setClickingTogglesState (true);
            b->onClick = [this] { refresh(); };
            addAndMakeVisible (*b);
        }
        exclusiveOnly.setButtonText ("EXCLUSIVE"); favOnly.setButtonText ("FAVOURITES"); userOnly.setButtonText ("USER");
        addAndMakeVisible (category); addAndMakeVisible (era);
        list.setModel (this);
        list.setRowHeight (26);
        addAndMakeVisible (list);
        closeBtn.setButtonText ("CLOSE");
        closeBtn.onClick = [this] { setVisible (false); };
        addAndMakeVisible (closeBtn);
        count.setFont (serif (12.0f, false, 0.1f));
        addAndMakeVisible (count);
    }

    void open (int tile, int eraIdx, bool exclusive)
    {
        category.setSelectedId (tile >= 0 ? tile + 2 : 1, dontSendNotification);
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
        closeBtn.setBounds (getWidth() - 116, 18, 90, 26);
        auto r = getLocalBounds().reduced (24).withTrimmedTop (34);
        auto top = r.removeFromTop (32);
        search.setBounds (top.removeFromLeft (360)); top.removeFromLeft (10);
        category.setBounds (top.removeFromLeft (170)); top.removeFromLeft (8);
        era.setBounds (top.removeFromLeft (130)); top.removeFromLeft (8);
        exclusiveOnly.setBounds (top.removeFromLeft (120)); top.removeFromLeft (6);
        favOnly.setBounds (top.removeFromLeft (120)); top.removeFromLeft (6);
        userOnly.setBounds (top.removeFromLeft (80));
        r.removeFromTop (8);
        count.setBounds (r.removeFromBottom (20));
        list.setBounds (r);
    }

    void refresh()
    {
        entries.clear();
        const auto q = search.getText().trim().toLowerCase();
        const auto favs = getFavourites ? getFavourites() : StringArray();
        const int cat = category.getSelectedId() - 2, er = era.getSelectedId() - 2;
        auto matches = [&] (const String& hay) { return q.isEmpty() || hay.toLowerCase().contains (q); };
        if (! exclusiveOnly.getToggleState())
            for (auto& f : proc.userPresets())
            {
                const auto name = f.getFileNameWithoutExtension();
                if (cat >= 0 || er >= 0) continue;
                if (favOnly.getToggleState() && ! favs.contains (name)) continue;
                if (matches (name + " user")) entries.push_back ({ name, "USER", -1, f });
            }
        if (! userOnly.getToggleState())
        {
            const auto& ps = factoryPresets();
            for (int i = 0; i < (int) ps.size(); ++i)
            {
                const auto& pr = ps[(size_t) i];
                if (cat >= 0 && pr.tile != cat) continue;
                if (er >= 0 && pr.era != er) continue;
                if (exclusiveOnly.getToggleState() && ! pr.exclusive) continue;
                if (favOnly.getToggleState() && ! favs.contains (pr.name)) continue;
                const String eraTxt = pr.era >= 0 ? eraNames()[pr.era] : String ("EXPERIMENTAL");
                String info = tileNames()[pr.tile] + (pr.sub.isNotEmpty() ? " / " + pr.sub.toUpperCase() : String()) + "  -  " + eraTxt
                            + (pr.exclusive ? "  -  EXCLUSIVE" : "");
                if (matches (pr.name + " " + info)) entries.push_back ({ pr.name, info, i, {} });
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
        g.setFont (serif (16.0f)); g.drawText (favs.contains (e.name) ? "*" : "+", 6, 0, 20, h, Justification::centred);
        g.setColour (s.text); g.setFont (serif (15.0f, false, 0.05f));
        g.drawText (e.name, 32, 0, w / 2, h, Justification::centredLeft);
        g.setColour (s.textDim); g.setFont (serif (12.0f, false, 0.12f));
        g.drawText (e.info, w / 2, 0, w / 2 - 10, h, Justification::centredRight);
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
        if (e.x < 28) { if (toggleFavourite) toggleFavourite (entries[(size_t) row].name); list.repaint(); return; }
        activate (row, false);
    }

    void listBoxItemDoubleClicked (int row, const MouseEvent&) override { activate (row, true); }
    void returnKeyPressed (int row) override { activate (row, true); }

    KeysKillaProcessor& proc;
    KKLookAndFeel& lnf;
    TextEditor search;
    ComboBox category, era;
    TextButton exclusiveOnly, favOnly, userOnly, closeBtn;
    Label count;
    ListBox list;
    std::vector<Entry> entries;
};
