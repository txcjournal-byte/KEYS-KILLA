#pragma once
#include "../VaultTypes.h"
#include <algorithm>

// CHROME VAULT browser logic (no GUI): filters, search, favorites, recent, stepping through the filtered list.
// Message thread only. The bank is immutable for the plugin's lifetime, so indices into it are stable.
struct VaultBrowserFilter
{
    int category = -1;        // -1 = All Sounds
    int layers = 0;           // 0 = all, else 1..4
    juce::String query;       // search text: every word must match name / category / layer labels / instrument ids
    bool favoritesOnly = false;
    bool recentOnly = false;  // list = the recently played sounds, newest first

    bool anyNarrowing() const { return category >= 0 || layers > 0 || query.trim().isNotEmpty() || favoritesOnly || recentOnly; }
    bool anyChip() const      { return layers > 0; }
};

class VaultBrowserModel
{
public:
    static constexpr int maxRecent = 30;

    explicit VaultBrowserModel (const std::vector<VaultPreset>& b) : bank (b)
    {
        const int n = (int) bank.size();
        haystack.reserve ((size_t) n);
        favorite.assign ((size_t) n, false);
        for (int i = 0; i < n; ++i)
        {
            const auto& p = bank[(size_t) i];
            juce::String h;
            h << p.name << " | " << vault::categoryNames()[juce::jlimit (0, (int) vault::numCategories - 1, p.category)];
            for (auto& part : p.parts)
            {
                h << " | " << part.label;
                if (part.kind == VaultPartKind::sample) h << " | sampled | " << part.source.replaceCharacter ('.', ' ') << " " << part.articulation;
                if (part.kind == VaultPartKind::model) h << " | modelled";
            }
            h << " | " << (int) p.parts.size() << (p.parts.size() == 1 ? " layer" : " layers");
            haystack.push_back (h.toLowerCase());
            idToIndex.set (p.id, i);
            if (juce::isPositiveAndBelow (p.category, (int) vault::numCategories)) ++catCount[p.category];
        }
        refilter();
    }

    int size() const { return (int) bank.size(); }
    int categoryCount (int c) const { return juce::isPositiveAndBelow (c, (int) vault::numCategories) ? catCount[c] : size(); }
    int indexOfId (const juce::String& id) const { return idToIndex.contains (id) ? idToIndex[id] : -1; }

    //==========================================================================
    VaultBrowserFilter filter;
    const std::vector<int>& visible() const { return rows; }    // bank indices shown in the list, in list order

    void refilter()
    {
        rows.clear();
        auto words = juce::StringArray::fromTokens (filter.query.toLowerCase(), " ,;/+", "\"");
        words.removeEmptyStrings();
        auto ok = [&] (int i)
        {
            const auto& p = bank[(size_t) i];
            if (filter.category >= 0 && p.category != filter.category) return false;
            if (filter.layers > 0 && (int) p.parts.size() != filter.layers) return false;
            if (filter.favoritesOnly && ! favorite[(size_t) i]) return false;
            for (auto& w : words) if (! haystack[(size_t) i].contains (w)) return false;
            return true;
        };
        if (filter.recentOnly) { for (int i : recent) if (ok (i)) rows.push_back (i); }
        else { rows.reserve (bank.size()); for (int i = 0; i < size(); ++i) if (ok (i)) rows.push_back (i); }
    }

    int rowOf (int bankIndex) const
    {
        const auto it = std::find (rows.begin(), rows.end(), bankIndex);
        return it == rows.end() ? -1 : (int) (it - rows.begin());
    }

    // the sound `delta` rows away from `current` in the filtered list (wrap = endless ◀ ▶; otherwise clamp like ↑ ↓).
    // current not in the list: +1 -> first row, -1 -> last row. -1 if the list is empty.
    int step (int current, int delta, bool wrap) const
    {
        const int n = (int) rows.size();
        if (n == 0) return -1;
        int r = rowOf (current);
        if (r < 0) return rows[(size_t) (delta >= 0 ? 0 : n - 1)];
        r += delta;
        r = wrap ? ((r % n) + n) % n : juce::jlimit (0, n - 1, r);
        return rows[(size_t) r];
    }

    //==========================================================================
    bool isFavorite (int i) const { return juce::isPositiveAndBelow (i, size()) && favorite[(size_t) i]; }
    void setFavorite (int i, bool on) { if (juce::isPositiveAndBelow (i, size())) favorite[(size_t) i] = on; }
    int numFavorites() const { return (int) std::count (favorite.begin(), favorite.end(), true); }
    juce::StringArray favoriteIds() const
    {
        juce::StringArray s;
        for (int i = 0; i < size(); ++i) if (favorite[(size_t) i]) s.add (bank[(size_t) i].id);
        return s;
    }
    void setFavoriteIds (const juce::StringArray& ids)
    {
        std::fill (favorite.begin(), favorite.end(), false);
        for (auto& id : ids) if (const int i = indexOfId (id); i >= 0) favorite[(size_t) i] = true;
    }

    const std::vector<int>& recentList() const { return recent; }   // newest first
    void pushRecent (int i)
    {
        if (! juce::isPositiveAndBelow (i, size())) return;
        recent.erase (std::remove (recent.begin(), recent.end(), i), recent.end());
        recent.insert (recent.begin(), i);
        if ((int) recent.size() > maxRecent) recent.resize ((size_t) maxRecent);
    }
    juce::StringArray recentIds() const
    {
        juce::StringArray s;
        for (int i : recent) s.add (bank[(size_t) i].id);
        return s;
    }
    void setRecentIds (const juce::StringArray& ids)
    {
        recent.clear();
        for (auto& id : ids)
            if (const int i = indexOfId (id); i >= 0 && std::find (recent.begin(), recent.end(), i) == recent.end() && (int) recent.size() < maxRecent)
                recent.push_back (i);
    }

private:
    const std::vector<VaultPreset>& bank;
    std::vector<juce::String> haystack;
    std::vector<bool> favorite;
    std::vector<int> recent, rows;
    juce::HashMap<juce::String, int> idToIndex;
    int catCount[vault::numCategories] {};
};
