#pragma once
#include "../VaultTypes.h"
#include "../VaultLibrary.h"
#include "../models/ModelVoice.h"
#include <algorithm>

// CHROME VAULT browser logic (no GUI): filters, search, favorites, recent, stepping through the filtered list.
// Message thread only. The bank is immutable for the plugin's lifetime, so indices into it are stable.
struct VaultBrowserFilter
{
    int category = -1;        // -1 = All Sounds
    int layers = 0;           // 0 = all, else 1..4
    juce::String query;       // search text: every word must match name / category / instruments / layer labels / ids
    bool favoritesOnly = false;
    bool recentOnly = false;  // list = the recently played sounds, newest first

    bool anyNarrowing() const { return category >= 0 || layers > 0 || query.trim().isNotEmpty() || favoritesOnly || recentOnly; }
    bool anyChip() const      { return layers > 0; }
};

class VaultBrowserModel
{
public:
    static constexpr int maxRecent = 30;

    // `library` names the sampled instruments ("Concert Grand"); without it (library not installed) they come from the id
    explicit VaultBrowserModel (const std::vector<VaultPreset>& b, const vault::SampleLibrary* library = nullptr) : bank (b)
    {
        const int n = (int) bank.size();
        haystack.reserve ((size_t) n);
        lowerNames.reserve ((size_t) n);
        instr.reserve ((size_t) n);
        favorite.assign ((size_t) n, false);
        for (int i = 0; i < n; ++i)
        {
            const auto& p = bank[(size_t) i];
            instr.push_back (mainInstruments (p, library));
            juce::String h;
            h << p.name << " | " << vault::categoryNames()[juce::jlimit (0, (int) vault::numCategories - 1, p.category)];
            for (auto& s : instr.back()) h << " | " << s;
            for (auto& part : p.parts)
            {
                h << " | " << part.label;
                if (part.kind == VaultPartKind::sample) h << " | sampled | " << part.source.replaceCharacter ('.', ' ') << " " << part.articulation;
                if (part.kind == VaultPartKind::model) h << " | modelled";
            }
            h << " | " << (int) p.parts.size() << (p.parts.size() == 1 ? " layer" : " layers");
            haystack.push_back (h.toLowerCase());
            lowerNames.push_back (p.name.toLowerCase().trim());
            idToIndex.set (p.id, i);
            if (juce::isPositiveAndBelow (p.category, (int) vault::numCategories)) ++catCount[p.category];
        }
        refilter();
    }

    int size() const { return (int) bank.size(); }
    // what plays the preset, most important first (layer A, then the other layers by level), no duplicates
    const juce::StringArray& instruments (int i) const
    {
        static const juce::StringArray none;
        return juce::isPositiveAndBelow (i, size()) ? instr[(size_t) i] : none;
    }

    //==========================================================================
    // display name of what plays a part: the library instrument ("Concert Grand"), the model ("Sine"), or the synth layer's label
    static juce::String instrumentName (const VaultPart& part, const vault::SampleLibrary* library)
    {
        if (part.kind == VaultPartKind::sample)
        {
            if (library != nullptr)
                if (const auto* h = library->find (part.source); h != nullptr && h->desc.name.isNotEmpty()) return h->desc.name;
            const auto fromId = nameFromId (part.source);
            return fromId.isNotEmpty() ? fromId : part.label;
        }
        if (part.kind == VaultPartKind::model)
        {
            if (const auto* m = vault::models::ModelRegistry::get().find (part.source); m != nullptr && m->name.isNotEmpty()) return m->name;
            return part.label.isNotEmpty() ? part.label : nameFromId (part.source);
        }
        return part.label.isNotEmpty() ? part.label : juce::String ("Synth");
    }
    // "piano.grand.concert" -> "Concert Grand", "strings.violins.ensemble" -> "Ensemble Violins", "mallets.marimba" -> "Marimba"
    // (the family word in front is dropped). Only used when the library is not installed.
    static juce::String nameFromId (const juce::String& id)
    {
        auto seg = juce::StringArray::fromTokens (id, ".", "");
        seg.removeEmptyStrings();
        if (seg.size() > 1) seg.remove (0);
        juce::StringArray words;
        for (int k = seg.size(); --k >= 0;)
            for (auto w : juce::StringArray::fromTokens (seg[k].replaceCharacters ("_-", "  "), " ", ""))
                if (w.isNotEmpty()) words.add (w.substring (0, 1).toUpperCase() + w.substring (1));
        return words.joinIntoString (" ");
    }
    static juce::StringArray mainInstruments (const VaultPreset& p, const vault::SampleLibrary* library)
    {
        std::vector<int> order;
        for (int k = 0; k < (int) p.parts.size(); ++k) order.push_back (k);
        if (order.size() > 2)
            std::stable_sort (order.begin() + 1, order.end(),
                              [&] (int a, int b) { return p.parts[(size_t) a].level > p.parts[(size_t) b].level; });
        juce::StringArray out;
        for (int k : order)
        {
            const auto s = instrumentName (p.parts[(size_t) k], library).trim();
            if (s.isNotEmpty() && ! out.contains (s, true)) out.add (s);
        }
        return out;
    }
    // the counts follow the search (and the favorites / recent / layers filters): what each category holds of the current
    // results; -1 = All Sounds = every result over all categories. Nothing narrowing: the whole bank per category.
    int categoryCount (int c) const { return juce::isPositiveAndBelow (c, (int) vault::numCategories) ? liveCount[c] : liveAll; }
    int bankCategoryCount (int c) const { return juce::isPositiveAndBelow (c, (int) vault::numCategories) ? catCount[c] : size(); }
    int indexOfId (const juce::String& id) const { return idToIndex.contains (id) ? idToIndex[id] : -1; }

    //==========================================================================
    VaultBrowserFilter filter;
    const std::vector<int>& visible() const { return rows; }    // bank indices shown in the list, in list order

    // the rows of the current filter. With a search, the best name matches come first: the exact name, then names that
    // start with the search, then names with a word starting with it, then names containing it, then everything else
    // that matches (instrument, category, layer labels) - each group in bank order.
    void refilter()
    {
        rows.clear();
        const auto q = filter.query.toLowerCase().trim();
        auto words = juce::StringArray::fromTokens (q, " ,;/+", "\"");
        words.removeEmptyStrings();
        std::fill (std::begin (liveCount), std::end (liveCount), 0);
        liveAll = 0;
        auto okButCategory = [&] (int i)
        {
            const auto& p = bank[(size_t) i];
            if (filter.layers > 0 && (int) p.parts.size() != filter.layers) return false;
            if (filter.favoritesOnly && ! favorite[(size_t) i]) return false;
            for (auto& w : words) if (! haystack[(size_t) i].contains (w)) return false;
            return true;
        };
        auto take = [&] (int i)
        {
            if (! okButCategory (i)) return;
            const int c = bank[(size_t) i].category;
            if (juce::isPositiveAndBelow (c, (int) vault::numCategories)) ++liveCount[c];
            ++liveAll;
            if (filter.category >= 0 && c != filter.category) return;
            rows.push_back (i);
        };
        if (filter.recentOnly) { for (int i : recent) take (i); }
        else { rows.reserve (bank.size()); for (int i = 0; i < size(); ++i) take (i); }
        if (words.isEmpty() || filter.recentOnly || rows.size() < 2) return;
        std::vector<std::pair<int, int>> scored;   // (score, bank index), stable in bank order
        scored.reserve (rows.size());
        for (int i : rows) scored.push_back ({ nameScore (i, q, words), i });
        std::stable_sort (scored.begin(), scored.end(), [] (const auto& a, const auto& b) { return a.first < b.first; });
        for (size_t k = 0; k < scored.size(); ++k) rows[k] = scored[k].second;
    }
    // 0 exact name, 1 the name starts with the search, 2 a name word starts with the first search word (and the name holds
    // every word), 3 the name contains every word, 4 the match is elsewhere (instrument, category, label)
    int nameScore (int i, const juce::String& q, const juce::StringArray& words) const
    {
        const auto& n = lowerNames[(size_t) i];
        if (n == q) return 0;
        if (n.startsWith (q)) return 1;
        bool all = true;
        for (auto& w : words) all &= n.contains (w);
        if (! all) return 4;
        const auto& w0 = words[0];
        for (int pos = n.indexOf (w0); pos >= 0; pos = n.indexOf (pos + 1, w0))
            if (pos == 0 || ! juce::CharacterFunctions::isLetterOrDigit (n[pos - 1])) return 2;
        return 3;
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
    std::vector<juce::String> haystack, lowerNames;
    std::vector<juce::StringArray> instr;
    std::vector<bool> favorite;
    std::vector<int> recent, rows;
    juce::HashMap<juce::String, int> idToIndex;
    int catCount[vault::numCategories] {};
    int liveCount[vault::numCategories] {}, liveAll = 0;   // the current results per category / in all
};
