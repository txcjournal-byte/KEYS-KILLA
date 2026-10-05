#pragma once
#include <juce_core/juce_core.h>
#include <array>
#include <cstring>
#include <map>
#include <vector>
#include "DspUtil.h"

// v0.45 WORDS: type anything, get 10 sounds. Offline - no internet, no AI service.
// A built-in dictionary (English + Czech, with or without diacritics) maps MEANING words to ALCHEMY traits: what excites the
// matter, the body that resonates, glass ... mud, tiny ... giant, heat, freeze, short ... long, dark ... bright.
// Every other word (names, nonsense) is hashed: the same word always gives the same 10 sounds - every word in the world has
// its sounds.  A sentence blends all its words.  Pure and deterministic (tested headlessly).
namespace kk::words
{
// the scalar traits: matter (0 glass .. 1 mud), size (0 tiny .. 1 giant), heat, cool (freeze), stretch (-1 short .. 1 long),
// bright (-1 dark .. 1 bright), split (a second layer)
enum Trait { tMatter, tSize, tHeat, tCool, tStretch, tBright, tSplit, numTraits };

struct Traits
{
    std::array<float, 5> e {}, b {};                 // exciter / body weights (kk::alc::exciters() / bodies() order)
    std::array<float, numTraits> v {};
    std::array<bool, numTraits> has {};
    float get (int t) const { static const float def[] { 0.4f, 0.5f, 0, 0, 0, 0, 0 }; return has[(size_t) t] ? v[(size_t) t] : def[t]; }
};

// one recipe = one sound: alchemy (exciter, body, matter, size, seed) + sculpt (stretch, bright, heat, cool, split)
struct Recipe
{
    int exc = 0, body = 3; float matter = 0.4f, size = 0.5f; uint32_t seed = 1;
    float stretch = 0, bright = 0, heat = 0, cool = 0, split = 0;
    juce::String name;
    bool operator== (const Recipe& o) const
    {
        return exc == o.exc && body == o.body && seed == o.seed && matter == o.matter && size == o.size && stretch == o.stretch && bright == o.bright
               && heat == o.heat && cool == o.cool && split == o.split;
    }
};

struct Word { juce::String text, key; bool known = false, stop = false; float weight = 0; };
struct Result { Traits traits; std::vector<Word> words; uint32_t seed = 0; std::vector<Recipe> sounds; juce::String title; };

// ---- the dictionary: "forms" (EN | CZ, plain ascii - diacritics are stripped from what you type) -> traits
//      e<i>:w exciter weight, b<i>:w body weight, m matter, s size, h heat, c cool, t stretch, r bright, x split
//      exciters: 0 METAL STRIKE 1 BIO FRICTION 2 ELECTRIC SHORT 3 BREATH 4 PRESSURE WAVE
//      bodies:   0 HOLLOW BONE 1 COMPRESSED GAS 2 MAGNETIC LIQUID 3 CRYSTAL LATTICE 4 MOLTEN CORE
struct Concept { const char* forms; const char* traits; };
inline const std::vector<Concept>& concepts()
{
    static const std::vector<Concept> c {
        { "dark darkness black shadow shadows gloomy temny temna temne tma temno cerny cerna cerne stin", "r-.8 m.7 b4:.3" },
        { "bright light shiny white shine svetly svetla svetle jasny jasna jasne bily bila bile svetlo zari", "r.8 m.15 b3:.5" },
        { "glass sklo skleneny sklenena glassy", "m0 b3:1 e0:.5 r.5" },
        { "rain raining drops drop dest deste prsi kapky kapka", "b2:1 e3:.3 m.35 t-.2" },
        { "metal metallic steel iron chrome kov kovy kovovy kovova ocel zelezo", "e0:1.2 b4:.3" },
        { "soft gentle tender jemny jemna jemne mekky mekka hebky nezny", "e3:.8 b2:.5 m.55 r-.3 t.2" },
        { "angry rage anger furious mad vztek vzteklo nastvany nastvana zlost zurivy", "e2:.8 b4:1 h.9 r.3" },
        { "space cosmos cosmic universe galaxy stars star vesmir vesmirny kosmos hvezdy hvezda galaxie", "b1:1 e3:.4 s.95 c.5 t.8" },
        { "water ocean sea river lake voda vodni ocean more reka jezero", "b2:1.2 e3:.3 m.4 s.6" },
        { "fire flame flames burning ohen ohne plamen plameny hori horici", "b4:1.2 e2:.4 h.8 r.2" },
        { "night midnight noc nocni pulnoc", "r-.5 b1:.5 s.7 t.5 c.2" },
        { "dream dreams dreamy sen sny snit snovy zdani", "b1:1 e3:.6 c.6 t.8 s.75 m.45" },
        { "ice frozen freeze frost cold winter led ledovy zmrzly mraz studeny studena zima", "b3:.8 c.9 m.1 r.3" },
        { "gold golden zlato zlaty zlata", "e0:.6 b3:.6 r.5 m.2 h.2" },
        { "silver chrome stribro stribrny stribrna", "e0:1 b3:.8 r.6 m.1" },
        { "wood wooden tree trees drevo dreveny drevena strom stromy", "b0:1.2 e0:.4 m.5 t-.3" },
        { "bone bones skeleton kost kosti kostra", "b0:1.4 t-.5 m.45" },
        { "air wind gas vzduch vitr plyn", "e3:1 b1:1 m.3" },
        { "breath breathe whisper whispers dech dychat septat sepot", "e3:1.4 b1:.5 r-.1 m.4" },
        { "bass sub deep low basy basa hluboky hluboka nizky", "e4:1.4 s.85 r-.4" },
        { "808 boom explosion blast bum rana vybuch", "e4:1.4 b4:.4 h.4 s.8 t-.2" },
        { "electric electricity spark sparks zap volt elektrina elektricky proud jiskra jiskry", "e2:1.4 r.4" },
        { "digital computer robot cyber pixel 8bit chip pocitac kyber digitalni", "e2:1.2 b3:.3 m.1 h.3" },
        { "heart love warm warmth srdce laska teply tepla teplo", "e1:.8 b2:.8 m.6 h.25 r-.2" },
        { "sad sadness tears melancholy lonely smutny smutna smutek slzy melancholie samota", "r-.5 e1:1 b2:.5 t.6 m.6" },
        { "happy joy smile fun stastny stastna radost vesely vesela usmev", "r.6 e0:.5 b3:.5 t-.2 m.2" },
        { "string strings violin bow cello struna struny housle smycec", "e1:1.5 b0:.4" },
        { "voice choir sing singing vocal hlas sbor zpev spivat", "e1:1 b1:.6 m.45" },
        { "crystal crystals diamond krystal krystaly diamant", "b3:1.5 m.05 r.6" },
        { "mud swamp dirt bahno blato bazina spina", "m1 b2:.5 r-.5" },
        { "silk hedvabi", "m.25 e3:.5 b2:.5" },
        { "velvet samet sametovy", "m.5 e3:.4 b2:.5 r-.2" },
        { "tiny small little mini maly mala male drobny malinky", "s.05 r.3 t-.3" },
        { "giant huge big large massive obrovsky obrovska velky velka obr masivni", "s.95 t.3 r-.2" },
        { "short quick fast pluck kratky kratka rychly rychla", "t-.8" },
        { "long endless slow forever dlouhy dlouha nekonecny pomaly pomala", "t.8 c.2" },
        { "storm thunder lightning bourka hrom blesk", "e4:1 e2:.6 h.6 s.85 x.3" },
        { "ghost ghosts haunted spirit duch duchove strasidelny", "b1:1 e3:.6 c.4 r-.2 x.5 t.7" },
        { "evil demon hell devil doom zly zla zle peklo dabel cert", "b4:1.2 e4:.6 r-.6 h.7 s.8" },
        { "angel heaven holy divine andel nebe svaty svata nebesky", "b1:.8 b3:.6 e3:.6 r.6 c.3 t.7" },
        { "bell bells chime zvon zvony zvonek", "e0:1.3 b3:1 r.5" },
        { "toy cute kawaii hracka roztomily roztomila", "b3:1 e0:.6 s.08 r.6" },
        { "city urban street mesto ulice", "e2:.6 b4:.5 h.4 m.5" },
        { "forest nature jungle les lesy priroda dzungle", "e3:.6 b0:.8 b2:.3 m.5" },
        { "sun summer sunny slunce leto slunecny", "r.7 h.3 b4:.4 m.3" },
        { "moon luna mesic", "r-.3 b3:.6 c.4 t.5" },
        { "fog mist cloud clouds mlha mrak oblak mraky", "b1:1.2 m.65 r-.4 t.6" },
        { "smoke dust kour prach", "b1:.8 m.75 h.3 r-.3" },
        { "honey sugar sweet med cukr sladky sladka", "b2:.8 e1:.4 m.55 r.1" },
        { "rock stone skala kamen", "e0:.6 b0:1 m.6 s.7" },
        { "wave waves surf vlna vlny", "b2:1 e3:.5 t.5" },
        { "underwater ponoreny hlubina", "b2:1.4 r-.7 m.7" },
        { "glitch bug error broken chyba rozbity rozbita", "e2:1.3 h.5 x.4 t-.5" },
        { "war battle fight valka bitva boj", "e4:1 b4:1 h.8 s.8" },
        { "calm peace relax chill klid klidny klidna mir", "e3:.8 b2:.6 c.3 r-.2 t.5" },
        { "energy power hype energie sila", "e2:.8 b4:.6 h.6 r.4" },
        { "old vintage retro antique stary stara stare", "m.65 r-.4 h.3 e1:.4" },
        { "future futuristic neon budoucnost futuristicky", "e2:1 b3:.6 r.5 m.15" },
        { "alien ufo weird strange mimozemstan divny divna podivny", "e2:.8 b2:.8 x.6 c.3" },
        { "pain scream screaming bolest krik kricet", "e1:1 b4:.8 h.8 r.4" },
        { "blood red krev cerveny cervena", "b4:1 m.7 h.5" },
        { "blue modry modra", "b2:.8 c.4 r-.1" },
        { "green zeleny zelena", "b0:.6 e3:.5 m.45" },
        { "purple violet fialovy fialova", "b1:.8 b2:.4 c.3" },
        { "bubble bubbles bublina bubliny", "b2:1 e3:.4 s.2 t-.4" },
        { "drum drums beat knock hit buben bubny uder klepat", "e0:.5 b0:1.2 t-.8" },
        { "piano keys klavir klavesy", "e0:.6 b2:.8 m.3" },
        { "flute pipe fletna pistala", "e3:1.5 b0:.4 r.2" },
        { "organ church varhany kostel", "e3:.8 b1:.6 s.8 t.6" },
        { "trap drill hard tvrdy tvrda", "e4:.8 e0:.5 h.5 t-.3" },
        { "sleep tired sleepy spanek spat unaveny unavena", "e3:.6 c.4 t.8 r-.5" },
        { "dance party club disco tanec parba klub", "e2:.8 b3:.5 r.5 t-.3" },
        { "rust rusty corroded rez rezavy rezava", "e0:.8 m.7 h.4 r-.3" },
        { "machine engine factory motor stroj tovarna", "e2:.8 b4:.8 h.5 m.6" },
        { "insect bee fly buzz hmyz vcela moucha bzukot", "e2:1 e1:.6 s.1 r.4" },
        { "animal beast monster roar zvire bestie monstrum rev", "e1:1 b4:1 s.85 h.5" },
        { "bird birds ptak ptaci", "e3:1 b3:.5 s.15 r.6" },
        { "cat dog kocka pes", "e1:.8 b0:.5 s.35" },
        { "snow snowflake snih vlocka", "c.7 b3:.8 m.2 r.3 s.3" },
        { "desert sand dry poust pisek suchy sucha", "m.6 b0:.8 h.3 r.1" },
        { "fear horror scary tension strach napeti desivy", "r-.4 e1:.8 x.4 c.2 t.6" },
        { "heavy thick fat tezky tezka husty husta tlusty", "m.8 s.8 h.3" },
        { "thin airy tenky tenka vzdusny", "m.15 s.25 e3:.6" },
        { "echo cave hall ozvena jeskyne sal", "s.9 t.7 b1:.6" },
        { "lava magma volcano sopka", "b4:1.5 m.85 h.7 s.85" },
        { "wet mokry mokra vlhky", "m.45 b2:.6" },
    };
    return c;
}

inline bool isStopWord (const juce::String& w)
{
    static const juce::StringArray s { "the", "a", "an", "and", "of", "in", "on", "at", "to", "with", "is", "are", "my", "your", "i", "you", "it", "this", "that",
                                       "for", "from", "by", "or", "be", "je", "v", "ve", "na", "se", "s", "z", "ze", "do", "o", "u", "k", "pro", "to", "ten", "ta", "mi", "ty", "jsem" };
    return s.contains (w);
}

// lower case ascii: Czech (and other Latin) diacritics are stripped, so "temný" = "temny", "déšť" = "dest"
inline juce::String normalise (const juce::String& in)
{
    static const juce::String from = juce::String::fromUTF8 ("áäàâãåčćçďéěëèêíïìîľĺňñóöòôõřŕšśťúůüùûýÿžźżÁÄÀÂÃÅČĆÇĎÉĚËÈÊÍÏÌÎĽĹŇÑÓÖÒÔÕŘŔŠŚŤÚŮÜÙÛÝŸŽŹŻ"),
                              to   = "aaaaaaccc" "deeeee" "iiii" "lln" "nooooo" "rrss" "tuuuuu" "yyzzz" "aaaaaaccc" "deeeee" "iiii" "lln" "nooooo" "rrss" "tuuuuu" "yyzzz";
    juce::String out;
    for (auto p = in.getCharPointer(); ! p.isEmpty(); ++p)
    {
        const auto ch = *p;
        const int k = from.indexOfChar (ch);
        out += k >= 0 && k < to.length() ? to[k] : (juce::juce_wchar) juce::CharacterFunctions::toLowerCase (ch);
    }
    return out.toLowerCase();
}

inline juce::StringArray tokens (const juce::String& text)
{
    juce::StringArray out; juce::String cur;
    const auto n = normalise (text);
    for (auto p = n.getCharPointer(); ! p.isEmpty(); ++p)
    {
        const auto ch = *p;
        if (juce::CharacterFunctions::isLetterOrDigit (ch)) cur += ch;
        else if (cur.isNotEmpty()) { out.add (cur); cur.clear(); }
    }
    if (cur.isNotEmpty()) out.add (cur);
    return out;
}

inline Traits parseTraits (const char* s)
{
    Traits t;
    juce::StringArray parts; parts.addTokens (s, " ", "");
    for (auto& p : parts)
    {
        if (p.isEmpty()) continue;
        const auto k = p[0];
        if (k == 'e' || k == 'b') { const int i = juce::jlimit (0, 4, p.substring (1, 2).getIntValue()); (k == 'e' ? t.e : t.b)[(size_t) i] += p.fromFirstOccurrenceOf (":", false, false).getFloatValue(); continue; }
        const int idx = k == 'm' ? tMatter : k == 's' ? tSize : k == 'h' ? tHeat : k == 'c' ? tCool : k == 't' ? tStretch : k == 'r' ? tBright : k == 'x' ? tSplit : -1;
        if (idx >= 0) { t.v[(size_t) idx] = p.substring (1).getFloatValue(); t.has[(size_t) idx] = true; }
    }
    return t;
}

// word form -> concept index
inline const std::map<juce::String, int>& lexicon()
{
    static const std::map<juce::String, int> lex = []
    {
        std::map<juce::String, int> m;
        const auto& cs = concepts();
        for (int i = 0; i < (int) cs.size(); ++i) { juce::StringArray f; f.addTokens (cs[(size_t) i].forms, " ", ""); for (auto& w : f) if (w.isNotEmpty()) m.emplace (w, i); }
        return m;
    }();
    return lex;
}
inline int dictionarySize() { return (int) lexicon().size(); }

// a word -> its concept (-1 = not a meaning word). Plurals / Czech endings are tried too (kovy -> kov, drops -> drop)
inline int lookup (const juce::String& w)
{
    const auto& lex = lexicon();
    if (auto it = lex.find (w); it != lex.end()) return it->second;
    static const char* suffixes[] { "ing", "es", "s", "ed", "ly", "y", "ove", "ovy", "ova", "ech", "ich", "ou", "em", "ho", "mu", "e", "a", "i", "u" };
    for (auto* sfx : suffixes)
        if (w.endsWith (sfx) && w.length() - (int) strlen (sfx) >= 3)
            if (auto it = lex.find (w.dropLastCharacters ((int) strlen (sfx))); it != lex.end()) return it->second;
    return -1;
}

inline uint32_t hashString (const juce::String& s)
{
    uint32_t h = 2166136261u;
    for (auto p = s.toUTF8(); *p != 0; ++p) { h ^= (uint8_t) *p; h *= 16777619u; }
    return kk::hash32 (h);
}

// an unknown word (a name, nonsense): its own, always the same traits
inline Traits hashedTraits (uint32_t h)
{
    Rng r; r.seed (h | 1u);
    Traits t;
    t.e[(size_t) (r.next() % 5)] += 1.0f; t.e[(size_t) (r.next() % 5)] += 0.4f;
    t.b[(size_t) (r.next() % 5)] += 1.0f; t.b[(size_t) (r.next() % 5)] += 0.4f;
    const float u0 = r.uni(), u1 = r.uni(), u2 = r.uni(), u3 = r.uni(), u4 = r.uni(), u5 = r.uni(), u6 = r.uni();
    t.v = { u0, 0.15f + 0.7f * u1, 0.5f * u2 * u2, 0.5f * u3 * u3, u4 * 1.6f - 0.8f, u5 * 1.4f - 0.7f, u6 > 0.7f ? 0.4f * u6 : 0.0f };
    t.has.fill (true);
    return t;
}

inline int argmax (const std::array<float, 5>& w) { int k = 0; for (int i = 1; i < 5; ++i) if (w[(size_t) i] > w[(size_t) k]) k = i; return k; }
inline int pickWeighted (const std::array<float, 5>& w, Rng& r)
{
    float sum = 0; for (auto x : w) sum += std::pow (x + 0.08f, 1.5f);
    float u = r.uni() * sum;
    for (int i = 0; i < 5; ++i) { u -= std::pow (w[(size_t) i] + 0.08f, 1.5f); if (u <= 0) return i; }
    return 4;
}

// 10 sounds from blended traits (the first is the most faithful, the others wander around it)
inline std::vector<Recipe> recipes (const Traits& t, uint32_t seed, int count, const juce::String& title)
{
    std::vector<Recipe> out;
    for (int i = 0; i < count; ++i)
    {
        Rng r; r.seed (kk::hash32 (seed * 2654435761u + (uint32_t) i * 40503u + 7u));
        auto j = [&] (float amt) { return i == 0 ? 0.0f : (r.uni() - 0.5f) * amt; };
        Recipe c;
        c.exc = i == 0 ? argmax (t.e) : pickWeighted (t.e, r);
        c.body = i == 0 ? argmax (t.b) : pickWeighted (t.b, r);
        c.matter = juce::jlimit (0.0f, 1.0f, t.get (tMatter) + j (0.3f));
        c.size = juce::jlimit (0.0f, 1.0f, t.get (tSize) + j (0.3f));
        c.stretch = juce::jlimit (-1.0f, 1.0f, t.get (tStretch) + j (0.35f));
        c.bright = juce::jlimit (-1.0f, 1.0f, t.get (tBright) + j (0.35f));
        c.heat = juce::jlimit (0.0f, 1.0f, t.get (tHeat) + j (0.3f));
        c.cool = juce::jlimit (0.0f, 1.0f, t.get (tCool) + j (0.3f));
        c.split = juce::jlimit (0.0f, 1.0f, t.get (tSplit) + (i % 4 == 3 ? 0.3f : 0.0f));
        c.seed = r.next() | 1u;
        c.name = title + " " + juce::String (i + 1);
        out.push_back (c);
    }
    return out;
}

// type anything -> the words (which shaped the sound) + 10 recipes.  Order of the words does not matter.
inline Result type (const juce::String& text, int count = 10)
{
    Result res;
    const auto toks = tokens (text);
    const auto& cs = concepts();
    std::array<float, numTraits> sum {}, wsum {};
    uint32_t mix = 0; int used = 0;
    juce::StringArray titleWords;
    for (auto& w : toks)
    {
        Word wd; wd.text = w;
        if (isStopWord (w)) { wd.stop = true; res.words.push_back (wd); continue; }
        const int ci = lookup (w);
        Traits t;
        if (ci >= 0) { wd.known = true; wd.key = juce::String (cs[(size_t) ci].forms).upToFirstOccurrenceOf (" ", false, false); t = parseTraits (cs[(size_t) ci].traits); wd.weight = 1.0f; }
        else { wd.key = w; t = hashedTraits (hashString (w)); wd.weight = 0.6f; }
        for (int i = 0; i < 5; ++i) { res.traits.e[(size_t) i] += t.e[(size_t) i] * wd.weight; res.traits.b[(size_t) i] += t.b[(size_t) i] * wd.weight; }
        for (int k = 0; k < numTraits; ++k) if (t.has[(size_t) k]) { sum[(size_t) k] += t.v[(size_t) k] * wd.weight; wsum[(size_t) k] += wd.weight; }
        mix += hashString (wd.key);   // a sum: the order of the words does not matter
        ++used;
        if (titleWords.size() < 3) titleWords.add (w.substring (0, 1).toUpperCase() + w.substring (1));
        res.words.push_back (wd);
    }
    if (used == 0) return res;
    for (int k = 0; k < numTraits; ++k) if (wsum[(size_t) k] > 0) { res.traits.v[(size_t) k] = sum[(size_t) k] / wsum[(size_t) k]; res.traits.has[(size_t) k] = true; }
    res.seed = kk::hash32 (mix + (uint32_t) used * 977u);
    res.title = titleWords.joinIntoString (" ").substring (0, 22).trim();
    res.sounds = recipes (res.traits, res.seed, count, res.title);
    return res;
}
} // namespace kk::words
