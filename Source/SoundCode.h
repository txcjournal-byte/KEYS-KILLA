#pragma once
#include <juce_core/juce_core.h>
#include <array>
#include <vector>
#include <algorithm>

// v0.45 SOUND CODE: every COOK recipe (the ingredients in the pot, every cooking gesture, the seasoning and the mutation seed)
// is written as a short human code like EV-7K3Q-MUD4-X - type it in anywhere and you get the exact same dish (= the same sound).
// Crockford base32 (no I L O U, so it reads aloud), dashes every 4 characters, the last character is a checksum: any one typo is caught.
// Only what is in the recipe takes space (an ingredient / gesture that is not used costs one bit), so simple dishes have short codes.
namespace kk::code
{
constexpr int numIngredients = 10, maxSpoons = 4, levels = 16;
enum Act { stir, fry, bake, boil, chop, blend, freeze, salt, pepper, chilli, sugar, numActs };
inline const char* actName (int a) { static const char* n[] { "STIR", "FRY", "BAKE", "BOIL", "CHOP", "BLEND", "FREEZE", "SALT", "PEPPER", "CHILLI", "SUGAR" }; return juce::isPositiveAndBelow (a, (int) numActs) ? n[a] : ""; }

// an ingredient = what it brings to the ALCHEMY: what EXCITES the matter, the BODY that resonates, glass ... mud, tiny ... giant
struct Ingredient { const char* name; const char* word; juce::uint32 colour; int exc, body; float matter, size; const char* hint; };
inline const std::array<Ingredient, numIngredients>& ingredients()
{
    static const std::array<Ingredient, numIngredients> i { {
        { "METAL SPOON",   "Spoon",    0xffd8dde6, 0, 3, 0.12f, 0.45f, "rings like a bell" },
        { "BONE BROTH",    "Broth",    0xfff1d7a8, 1, 0, 0.55f, 0.50f, "woody, warm, knocking" },
        { "SPARKLING GAS", "Fizz",     0xffa78bfa, 2, 1, 0.20f, 0.40f, "buzzing air that swells" },
        { "HONEY",         "Honey",    0xffffb627, 1, 2, 0.78f, 0.50f, "slow, soft, sticky keys" },
        { "CRYSTAL SUGAR", "Sugar",    0xff9be7ff, 0, 3, 0.04f, 0.25f, "glassy chimes" },
        { "HOT COALS",     "Coals",    0xffff4b3a, 2, 4, 0.60f, 0.60f, "roaring, brassy dirt" },
        { "BREATH MINT",   "Mint",     0xffb4f0c8, 3, 1, 0.30f, 0.55f, "airy, flute-like" },
        { "BASS FLOUR",    "Flour",    0xfff4ead5, 4, 0, 0.70f, 0.85f, "subs, 808s, basses" },
        { "MOON DUST",     "Moondust", 0xffc9b8ff, 2, 3, 0.15f, 0.15f, "tiny digital sparkles" },
        { "SQUID INK",     "Ink",      0xff3dd6c6, 3, 2, 0.92f, 0.75f, "deep, dark, wobbling" },
    } };
    return i;
}

struct Recipe
{
    std::array<juce::uint8, numIngredients> ing {};   // spoons in the pot, 0 ... maxSpoons
    std::array<juce::uint8, numActs> act {};          // how much of each gesture, 0 ... levels
    juce::uint16 seed = 1;                            // the mutation
    bool operator== (const Recipe& o) const { return ing == o.ing && act == o.act && seed == o.seed; }
    bool operator!= (const Recipe& o) const { return ! (*this == o); }
    int spoons() const { int n = 0; for (auto x : ing) n += x; return n; }
    float amount (int a) const { return (float) act[(size_t) a] / (float) levels; }
    void setAmount (int a, float v) { act[(size_t) a] = (juce::uint8) juce::jlimit (0, levels, (int) std::round (v * (float) levels)); }
};

inline const char* alphabet() { return "0123456789ABCDEFGHJKMNPQRSTVWXYZ"; }
inline int charValue (juce::juce_wchar c)
{
    c = juce::CharacterFunctions::toUpperCase (c);
    if (c == 'O') c = '0'; if (c == 'I' || c == 'L') c = '1';   // read-aloud mistakes are forgiven
    for (int i = 0; i < 32; ++i) if (alphabet()[i] == c) return i;
    return -1;
}
// the checksum: odd weights mod 32, so changing any single character always changes it
inline int checksum (const std::vector<int>& v) { int s = 7 * (int) v.size(); for (size_t i = 0; i < v.size(); ++i) s += (int) (2 * i + 1) * v[i]; return s & 31; }

inline juce::String encode (const Recipe& r)
{
    std::vector<bool> bits;
    auto put = [&] (int value, int n) { for (int b = n - 1; b >= 0; --b) bits.push_back (((value >> b) & 1) != 0); };
    for (auto x : r.ing) put (x > 0 ? 1 : 0, 1);
    for (auto x : r.ing) if (x > 0) put (juce::jmin ((int) x, maxSpoons) - 1, 2);
    for (auto x : r.act) put (x > 0 ? 1 : 0, 1);
    for (auto x : r.act) if (x > 0) put (juce::jmin ((int) x, levels) - 1, 4);
    put (r.seed, 16);
    while (bits.size() % 5 != 0) bits.push_back (false);
    std::vector<int> v;
    for (size_t i = 0; i < bits.size(); i += 5) { int c = 0; for (int k = 0; k < 5; ++k) c = (c << 1) | (bits[i + (size_t) k] ? 1 : 0); v.push_back (c); }
    v.push_back (checksum (v));
    juce::String s ("EV");
    for (size_t i = 0; i < v.size(); ++i) { if (i % 4 == 0) s << "-"; s << juce::String::charToString ((juce::juce_wchar) alphabet()[v[i]]); }
    return s;
}

// false = not a code (a typo, a missing character, rubbish) - out is left untouched
inline bool decode (const juce::String& text, Recipe& out)
{
    auto t = text.toUpperCase().removeCharacters (" -_.\t\r\n");
    if (t.startsWith ("EV")) t = t.substring (2);
    if (t.length() < 3) return false;
    std::vector<int> v;
    for (auto c : t) { const int x = charValue (c); if (x < 0) return false; v.push_back (x); }
    const int check = v.back(); v.pop_back();
    if (checksum (v) != check) return false;
    size_t pos = 0; const size_t nbits = v.size() * 5;
    bool ok = true;
    auto get = [&] (int n) { int x = 0; for (int k = 0; k < n; ++k) { if (pos >= nbits) { ok = false; return 0; } x = (x << 1) | ((v[pos / 5] >> (4 - (int) (pos % 5))) & 1); ++pos; } return x; };
    Recipe r;
    std::array<bool, numIngredients> hasI {}; std::array<bool, numActs> hasA {};
    for (auto& h : hasI) h = get (1) != 0;
    for (int i = 0; i < numIngredients; ++i) if (hasI[(size_t) i]) r.ing[(size_t) i] = (juce::uint8) (get (2) + 1);
    for (auto& h : hasA) h = get (1) != 0;
    for (int a = 0; a < numActs; ++a) if (hasA[(size_t) a]) r.act[(size_t) a] = (juce::uint8) (get (4) + 1);
    r.seed = (juce::uint16) get (16);
    if (! ok) return false;
    if ((nbits - pos) >= 5) return false;                       // too long
    while (pos < nbits) if (get (1) != 0) return false;          // the padding must be clean
    out = r;
    return true;
}

inline juce::uint32 mix (juce::uint32 x) { x ^= x >> 16; x *= 0x7feb352dU; x ^= x >> 15; x *= 0x846ca68bU; x ^= x >> 16; return x; }

// what the cooking does to the sound: the dominant ingredient is what EXCITES, the second the BODY, the rest nudge the
// matter / size / mutation; the gestures become the SCULPT moves (stretch, bright, heat, cool, split)
struct Cooked
{
    int exc = 3, body = 2; float matter = 0.4f, size = 0.5f; juce::uint32 seed = 1;
    float stretch = 0, bright = 0, heat = 0, cool = 0, split = 0;
    int main = -1, second = -1;
    juce::String dish;
};
inline std::vector<int> potOrder (const Recipe& r)
{
    std::vector<int> o;
    for (int i = 0; i < numIngredients; ++i) if (r.ing[(size_t) i] > 0) o.push_back (i);
    std::stable_sort (o.begin(), o.end(), [&] (int a, int b) { return r.ing[(size_t) a] > r.ing[(size_t) b]; });
    return o;
}
inline Cooked cook (const Recipe& r)
{
    Cooked c;
    const auto& I = ingredients();
    const auto o = potOrder (r);
    const float st = r.amount (stir), fr = r.amount (fry), ba = r.amount (bake), bo = r.amount (boil), ch = r.amount (chop), bl = r.amount (blend), fz = r.amount (freeze);
    const float sa = r.amount (salt), pe = r.amount (pepper), ci = r.amount (chilli), su = r.amount (sugar);
    juce::uint32 h = mix ((juce::uint32) r.seed * 2654435761u + 0x9e37u);
    if (! o.empty())
    {
        c.main = o[0]; c.second = o.size() > 1 ? o[1] : o[0];
        c.exc = I[(size_t) c.main].exc; c.body = I[(size_t) c.second].body;
        // the dominant ingredient leads the matter; stirring blends everything evenly
        float wm = 0, ws = 0, w = 0;
        for (size_t k = 0; k < o.size(); ++k)
        {
            const float wk = (float) r.ing[(size_t) o[k]] * (k == 0 ? 2.5f - 1.5f * st : 1.0f);
            wm += wk * I[(size_t) o[k]].matter; ws += wk * I[(size_t) o[k]].size; w += wk;
            h = mix (h + (juce::uint32) (o[k] + 1) * 7919u + (juce::uint32) r.ing[(size_t) o[k]] * 104729u);
        }
        c.matter = wm / w; c.size = ws / w;
    }
    h = mix (h + (juce::uint32) r.act[stir] * 31337u);       // stirring moves the sound: another mix of the same pot
    c.matter = juce::jlimit (0.0f, 1.0f, c.matter + 0.45f * bo + 0.08f * st);   // boiling: thicker, wobbling
    c.size = juce::jlimit (0.0f, 1.0f, c.size + 0.15f * ba);
    c.seed = h | 1u;
    c.stretch = juce::jlimit (-1.0f, 1.0f, 0.6f * ba - 0.9f * ch + 0.35f * bl - 0.3f * ci);
    c.bright = juce::jlimit (-1.0f, 1.0f, 0.6f * sa + 0.25f * su - 0.5f * ba);
    c.heat = juce::jlimit (0.0f, 1.0f, 0.7f * fr + 0.3f * pe + 0.5f * ci);
    c.cool = juce::jlimit (0.0f, 1.0f, 0.9f * fz + 0.35f * bl);
    c.split = juce::jlimit (0.0f, 1.0f, 0.4f * bl + 0.3f * su);
    // the dish gets a name
    static const char* nouns[] { "Stew", "Fry-Up", "Pie", "Soup", "Tartare", "Smoothie", "Sorbet" };
    int tech = -1; float best = 0.05f;
    for (int a = stir; a <= freeze; ++a) if (r.amount (a) > best) { best = r.amount (a); tech = a; }
    static const char* adj[] { "Salty", "Peppered", "Fiery", "Sweet" };
    int sea = -1; best = 0.05f;
    for (int a = salt; a <= sugar; ++a) if (r.amount (a) > best) { best = r.amount (a); sea = a - salt; }
    juce::String n;
    if (sea >= 0) n << adj[sea] << " ";
    if (o.empty()) n << "Water";
    else { n << I[(size_t) c.main].word; if (c.second != c.main) n << " & " << I[(size_t) c.second].word; }
    n << " " << (tech >= 0 ? nouns[tech] : "Bowl");
    c.dish = n;
    return c;
}

// the sound of a recipe (Proc = KeysKillaProcessor: alchemy + sculpt, no memory - the recipe is the sound)
template <class Proc> typename Proc::Genome dish (Proc& p, const Recipe& r)
{
    const auto c = cook (r);
    auto g = p.sculpt (p.alchemy (c.exc, c.body, c.matter, c.size, c.seed), c.stretch, c.bright, c.heat, c.cool, c.split);
    if (g.valid()) g.name = c.dish;
    return g;
}

inline Recipe random (juce::Random& rnd)
{
    Recipe r;
    const int n = 1 + rnd.nextInt (4);
    for (int k = 0; k < n; ++k) r.ing[(size_t) rnd.nextInt (numIngredients)] = (juce::uint8) (1 + rnd.nextInt (maxSpoons));
    for (auto& a : r.act) a = rnd.nextInt (3) == 0 ? (juce::uint8) rnd.nextInt (levels + 1) : (juce::uint8) 0;
    r.seed = (juce::uint16) (1 + rnd.nextInt (65535));
    return r;
}
} // namespace kk::code
