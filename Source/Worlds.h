#pragma once
#include <juce_core/juce_core.h>
#include <array>
#include <vector>
#include <cmath>

// v0.45 WORLDS: three playful sound worlds - BIOSPHERE (animals, nature, the human body), GARAGE (FLEX: a ladder of items from the
// worst to the most expensive) and PARTY (a club: crowd, lights, disco ball -> energy). Everything here is pure mapping logic (no UI,
// no audio): a choice + a roll seed -> 10 ALCHEMY recipes (exciter x body x matter x size + a SCULPT). Costs no memory: the sounds are
// only recipes until you hear / drag / keep one.
namespace kk::worlds
{
using juce::uint32;

// the ALCHEMY parts (Alchemy.h): exciters 0 METAL STRIKE 1 BIO FRICTION 2 ELECTRIC SHORT 3 BREATH 4 PRESSURE WAVE,
// bodies 0 HOLLOW BONE 1 COMPRESSED GAS 2 MAGNETIC LIQUID 3 CRYSTAL LATTICE 4 MOLTEN CORE
enum { eMetal, eFriction, eShort, eBreath, ePressure };
enum { bBone, bGas, bLiquid, bCrystal, bCore };

// one sound as a recipe: proc.sculpt (proc.alchemy (exc, body, matter, size, seed), stretch, bright, heat, cool, split)
struct Recipe
{
    int exc = 0, body = 3; float matter = 0.3f, size = 0.5f; uint32 seed = 1;
    float stretch = 0, bright = 0, heat = 0, cool = 0, split = 0;
    juce::String name;
    int kind = 0;                                                       // which flavour of its world (garage / party)
    float richness() const { return size + 0.5f * (stretch + 1.0f) + split; }   // bigger, longer, more layers
    bool operator== (const Recipe& o) const { return exc == o.exc && body == o.body && matter == o.matter && size == o.size && seed == o.seed && stretch == o.stretch && bright == o.bright && heat == o.heat && cool == o.cool && split == o.split; }
};

// a character: which exciters / bodies it likes, where its matter / size sit, how it is sculpted
struct Trait
{
    std::array<float, 5> exc {}, body {};
    float matter = 0.4f, size = 0.5f, stretch = 0, bright = 0, heat = 0, cool = 0, split = 0;
};

inline uint32 mix (uint32 a, uint32 b)
{
    uint32 x = a * 0x9E3779B1u ^ (b + 0x7F4A7C15u + (a << 6) + (a >> 2));
    x ^= x >> 16; x *= 0x7feb352du; x ^= x >> 15; x *= 0x846ca68bu; x ^= x >> 16;
    return x;
}
struct Rng
{
    uint32 s;
    explicit Rng (uint32 seed) : s (mix (seed, 0xA5A5u) | 1u) {}
    uint32 next() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
    float uni() { return (float) (next() >> 8) * (1.0f / 16777216.0f); }
    float gauss() { return (uni() + uni() + uni() - 1.5f) * 0.8f; }       // ~ -1.2 .. 1.2
    int pick (const std::array<float, 5>& w)
    {
        float sum = 0; for (auto x : w) sum += std::max (0.0f, x);
        if (sum <= 0) return (int) (next() % 5u);
        float u = uni() * sum;
        for (int i = 0; i < 5; ++i) { u -= std::max (0.0f, w[(size_t) i]); if (u <= 0) return i; }
        return 4;
    }
};

// a trait: the first exciter / body listed is the strongest
inline Trait T (std::initializer_list<int> ex, std::initializer_list<int> bo, float matter, float size, float stretch, float bright, float heat, float cool, float split)
{
    Trait t; float w = 2.0f;
    for (int e : ex) { t.exc[(size_t) e] += w; w = 1.0f; }
    w = 2.0f;
    for (int b : bo) { t.body[(size_t) b] += w; w = 1.0f; }
    t.matter = matter; t.size = size; t.stretch = stretch; t.bright = bright; t.heat = heat; t.cool = cool; t.split = split;
    return t;
}
inline Trait blend (const Trait& a, const Trait& b, float wb)
{
    Trait t; const float wa = 1.0f - wb;
    for (size_t i = 0; i < 5; ++i) { t.exc[i] = a.exc[i] * wa + b.exc[i] * wb; t.body[i] = a.body[i] * wa + b.body[i] * wb; }
    t.matter = a.matter * wa + b.matter * wb; t.size = a.size * wa + b.size * wb; t.stretch = a.stretch * wa + b.stretch * wb;
    t.bright = a.bright * wa + b.bright * wb; t.heat = a.heat * wa + b.heat * wb; t.cool = a.cool * wa + b.cool * wb; t.split = a.split * wa + b.split * wb;
    return t;
}
inline void clampRecipe (Recipe& r)
{
    r.exc = juce::jlimit (0, 4, r.exc); r.body = juce::jlimit (0, 4, r.body);
    r.matter = juce::jlimit (0.0f, 1.0f, r.matter); r.size = juce::jlimit (0.0f, 1.0f, r.size);
    r.stretch = juce::jlimit (-1.0f, 1.0f, r.stretch); r.bright = juce::jlimit (-1.0f, 1.0f, r.bright);
    r.heat = juce::jlimit (0.0f, 1.0f, r.heat); r.cool = juce::jlimit (0.0f, 1.0f, r.cool); r.split = juce::jlimit (0.0f, 1.0f, r.split);
}
// one recipe of a trait (the same trait + seed + slot = the same recipe; the spread is small so the character stays)
inline Recipe draw (const Trait& t, uint32 seed, int slot, float spread = 1.0f)
{
    Rng r (mix (seed, (uint32) slot * 7919u + 13u));
    Recipe o;
    o.exc = r.pick (t.exc); o.body = r.pick (t.body);
    o.seed = mix (seed, (uint32) slot + 101u) | 1u;
    o.matter = t.matter + 0.16f * spread * r.gauss(); o.size = t.size + 0.12f * spread * r.gauss();
    o.stretch = t.stretch + 0.14f * spread * r.gauss(); o.bright = t.bright + 0.12f * spread * r.gauss();
    o.heat = t.heat + 0.06f * spread * r.gauss(); o.cool = t.cool; o.split = t.split;
    clampRecipe (o);
    return o;
}

// any recipe -> the genome (P = KeysKillaProcessor; a template so this header stays free of the processor)
template <class P> auto genome (P& proc, const Recipe& r)
{
    auto g = proc.sculpt (proc.alchemy (r.exc, r.body, r.matter, r.size, r.seed), r.stretch, r.bright, r.heat, r.cool, r.split);
    if (g.valid() && r.name.isNotEmpty()) g.name = r.name;
    return g;
}

//==============================================================================
// the traits a part of a living thing gives its sound
namespace trait
{
    inline Trait breath()   { return T ({ eBreath }, { bGas, bLiquid }, 0.30f, 0.55f, 0.30f, 0.0f, 0.0f, 0.30f, 0.0f); }      // lungs: breath / air
    inline Trait bone()     { return T ({ eMetal, eFriction }, { bBone }, 0.40f, 0.45f, -0.40f, 0.1f, 0.05f, 0.0f, 0.0f); }   // bones: hollow bone
    inline Trait pulse()    { return T ({ ePressure }, { bCore, bLiquid }, 0.60f, 0.70f, -0.30f, -0.2f, 0.20f, 0.0f, 0.0f); } // heart: pulse / bass
    inline Trait vowel()    { return T ({ eFriction }, { bGas, bLiquid }, 0.40f, 0.50f, 0.20f, 0.0f, 0.05f, 0.10f, 0.15f); } // throat: vowel-ish
    inline Trait airy()     { return T ({ eBreath }, { bGas, bCrystal }, 0.15f, 0.55f, 0.20f, 0.40f, 0.0f, 0.20f, 0.30f); } // wings
    inline Trait metallic() { return T ({ eMetal }, { bCrystal, bCore }, 0.20f, 0.45f, 0.0f, 0.30f, 0.10f, 0.0f, 0.10f); }   // scales
    inline Trait fur()      { return T ({ eFriction }, { bLiquid, bGas }, 0.62f, 0.50f, 0.10f, -0.30f, 0.0f, 0.10f, 0.0f); } // soft fur
    inline Trait claws()    { return T ({ eMetal, eShort }, { bBone, bCrystal }, 0.25f, 0.40f, -0.60f, 0.40f, 0.30f, 0.0f, 0.0f); }
    inline Trait blood()    { return T ({ eFriction, eBreath }, { bLiquid }, 0.55f, 0.55f, 0.20f, -0.1f, 0.10f, 0.10f, 0.0f); }
    inline Trait neural()   { return T ({ eShort }, { bCrystal, bLiquid }, 0.20f, 0.45f, 0.0f, 0.20f, 0.05f, 0.0f, 0.40f); } // brain: sparks
    inline Trait buzz()     { return T ({ eShort }, { bGas, bCore }, 0.35f, 0.30f, -0.10f, 0.20f, 0.50f, 0.0f, 0.50f); }    // insect wings
    inline Trait deep()     { return T ({ ePressure, eBreath }, { bLiquid, bGas }, 0.60f, 0.92f, 0.60f, -0.50f, 0.0f, 0.30f, 0.10f); }
    inline Trait leaf()     { return T ({ eBreath, eFriction }, { bCrystal, bGas }, 0.20f, 0.45f, 0.0f, 0.20f, 0.0f, 0.20f, 0.10f); }
    inline Trait wood()     { return T ({ eMetal, eFriction }, { bBone }, 0.45f, 0.55f, -0.20f, -0.1f, 0.05f, 0.0f, 0.0f); }
    inline Trait roots()    { return T ({ ePressure }, { bCore, bBone }, 0.70f, 0.85f, 0.30f, -0.40f, 0.10f, 0.0f, 0.0f); }
    inline Trait water()    { return T ({ eBreath, eFriction }, { bLiquid }, 0.50f, 0.60f, 0.40f, 0.0f, 0.0f, 0.30f, 0.20f); }
    inline Trait thunder()  { return T ({ ePressure }, { bCore }, 0.70f, 0.95f, 0.50f, -0.30f, 0.60f, 0.0f, 0.20f); }
    inline Trait spark()    { return T ({ eShort }, { bCrystal, bCore }, 0.10f, 0.40f, -0.50f, 0.60f, 0.40f, 0.0f, 0.20f); }
    inline Trait rain()     { return T ({ eMetal, eBreath }, { bCrystal }, 0.10f, 0.30f, -0.40f, 0.30f, 0.0f, 0.0f, 0.10f); }
    inline Trait sand()     { return T ({ eFriction, eShort }, { bGas }, 0.75f, 0.50f, -0.10f, -0.10f, 0.50f, 0.0f, 0.0f); }
    inline Trait sun()      { return T ({ eBreath, eShort }, { bCrystal, bCore }, 0.15f, 0.70f, 0.40f, 0.60f, 0.10f, 0.0f, 0.30f); }
    inline Trait shell()    { return T ({ eMetal }, { bBone, bCrystal }, 0.35f, 0.50f, -0.20f, 0.10f, 0.0f, 0.0f, 0.0f); }
}

//==============================================================================
// BIOSPHERE: ANIMALS / NATURE / HUMAN BODY.  Every being has parts (a hit ellipse in the being's 0..1 box, a drawn shape)
enum Kingdom { kAnimals, kNature, kBody, numKingdoms };
enum Shape { shEllipse, shWing, shSpikes, shWave, shBolt, shDrop, shHeart, shLungs, shBrain, shBones, shLeaf, shRing };
struct BioPart { const char* name; const char* trait; uint32 colour; float cx, cy, rx, ry; int shape; Trait t; };
struct Being { const char* name; const char* hint; uint32 colour; std::vector<BioPart> parts; };

inline const std::vector<Being>& beings (int kingdom)
{
    using namespace trait;
    static const std::vector<Being> animals {
        { "WOLF", "howls, breathes, scratches", 0xff9fb3c8, {
            { "HOWL", "throat - a vowel", 0xffffd23f, 0.80f, 0.30f, 0.11f, 0.13f, shRing, vowel() },
            { "LUNGS", "breath / air", 0xff7fe0ff, 0.60f, 0.50f, 0.085f, 0.11f, shLungs, breath() },
            { "FUR", "soft, rubbed", 0xffc9a27a, 0.32f, 0.42f, 0.15f, 0.10f, shWave, fur() },
            { "CLAWS", "sharp clicks", 0xffff5a6e, 0.62f, 0.88f, 0.16f, 0.07f, shSpikes, claws() } } },
        { "WHALE", "sings in the deep", 0xff4d7dff, {
            { "SONG", "throat - a long vowel", 0xffb388ff, 0.76f, 0.52f, 0.10f, 0.12f, shRing, blend (vowel(), deep(), 0.5f) },
            { "BLOWHOLE", "breath / spray", 0xff7fe0ff, 0.62f, 0.25f, 0.08f, 0.12f, shDrop, breath() },
            { "FINS", "water", 0xff3dd6c6, 0.48f, 0.72f, 0.11f, 0.09f, shWing, water() },
            { "BELLY", "deep, huge", 0xff2a4fd6, 0.38f, 0.56f, 0.16f, 0.10f, shEllipse, deep() } } },
        { "BIRD", "wings and a tiny fast heart", 0xffffb04d, {
            { "BEAK", "sharp, short", 0xffffd23f, 0.88f, 0.36f, 0.08f, 0.06f, shSpikes, claws() },
            { "WINGS", "airy", 0xff9be7ff, 0.42f, 0.32f, 0.17f, 0.16f, shWing, airy() },
            { "FEATHERS", "soft shimmer", 0xffff8fd0, 0.20f, 0.62f, 0.12f, 0.10f, shLeaf, blend (fur(), airy(), 0.5f) },
            { "HEART", "pulse", 0xffff3b5c, 0.58f, 0.60f, 0.07f, 0.08f, shHeart, pulse() } } },
        { "SWARM", "a buzzing cloud of insects", 0xffc6ff3d, {
            { "WINGS", "buzz", 0xffeaff7a, 0.30f, 0.30f, 0.14f, 0.13f, shWing, buzz() },
            { "STINGERS", "sparks", 0xffff8a3d, 0.75f, 0.70f, 0.13f, 0.12f, shSpikes, spark() },
            { "HIVE MIND", "brain - sparks", 0xffb388ff, 0.52f, 0.50f, 0.11f, 0.13f, shBrain, neural() },
            { "SHELLS", "metallic", 0xff9fb3c8, 0.22f, 0.74f, 0.12f, 0.11f, shEllipse, metallic() } } },
        { "CAT", "purrs, scratches, sees in the dark", 0xffff9f6b, {
            { "PURR", "throat - low pulse", 0xffffd23f, 0.50f, 0.52f, 0.12f, 0.10f, shRing, blend (vowel(), pulse(), 0.5f) },
            { "WHISKERS", "thin metal", 0xffd8dde6, 0.74f, 0.30f, 0.12f, 0.06f, shWave, metallic() },
            { "CLAWS", "sharp clicks", 0xffff5a6e, 0.42f, 0.90f, 0.14f, 0.06f, shSpikes, claws() },
            { "EYES", "glints", 0xff36ff9a, 0.66f, 0.20f, 0.09f, 0.06f, shEllipse, neural() } } },
        { "SNAKE", "scales, rattle and fangs", 0xff5bd16b, {
            { "SCALES", "metallic", 0xff3dd6c6, 0.45f, 0.55f, 0.16f, 0.12f, shSpikes, metallic() },
            { "RATTLE", "dry, grainy", 0xffffd23f, 0.10f, 0.75f, 0.08f, 0.10f, shWave, sand() },
            { "FANGS", "sharp", 0xffff3b5c, 0.86f, 0.28f, 0.08f, 0.09f, shSpikes, claws() },
            { "COILS", "slow, liquid", 0xff8a5bff, 0.70f, 0.72f, 0.13f, 0.11f, shRing, blend (fur(), water(), 0.5f) } } },
    };
    static const std::vector<Being> nature {
        { "TREE", "leaves, wood and roots", 0xff6bd16b, {
            { "LEAVES", "rustling air", 0xff6bff8a, 0.50f, 0.25f, 0.30f, 0.18f, shLeaf, leaf() },
            { "BRANCHES", "knocking wood", 0xffc9a27a, 0.30f, 0.46f, 0.12f, 0.08f, shBones, wood() },
            { "TRUNK", "hollow wood", 0xff9a6b43, 0.50f, 0.66f, 0.07f, 0.16f, shEllipse, blend (wood(), bone(), 0.4f) },
            { "ROOTS", "deep under the ground", 0xffff8a3d, 0.50f, 0.92f, 0.26f, 0.06f, shWave, roots() } } },
        { "FOREST", "a canopy, moss, mushrooms, a creek", 0xff2ee6a6, {
            { "CANOPY", "wide air", 0xff6bff8a, 0.50f, 0.22f, 0.40f, 0.14f, shLeaf, blend (leaf(), airy(), 0.5f) },
            { "MOSS", "soft, wet", 0xff9fd16b, 0.22f, 0.80f, 0.14f, 0.07f, shEllipse, fur() },
            { "MUSHROOMS", "strange sparks", 0xffff8fd0, 0.78f, 0.74f, 0.10f, 0.11f, shBrain, neural() },
            { "CREEK", "running water", 0xff7fe0ff, 0.50f, 0.92f, 0.36f, 0.06f, shWave, water() } } },
        { "OCEAN", "waves, the deep, foam, coral", 0xff3d8bff, {
            { "WAVES", "water", 0xff7fe0ff, 0.50f, 0.30f, 0.40f, 0.10f, shWave, water() },
            { "THE DEEP", "huge and dark", 0xff2a4fd6, 0.50f, 0.66f, 0.30f, 0.12f, shEllipse, deep() },
            { "FOAM", "airy hiss", 0xffeaf6ff, 0.22f, 0.20f, 0.14f, 0.08f, shDrop, airy() },
            { "CORAL", "crystal shells", 0xffff7a9a, 0.78f, 0.88f, 0.12f, 0.10f, shBones, shell() } } },
        { "STORM", "clouds, lightning, rain, thunder", 0xffa78bfa, {
            { "CLOUDS", "a big breath", 0xffc8d2e6, 0.50f, 0.20f, 0.38f, 0.14f, shEllipse, blend (breath(), deep(), 0.4f) },
            { "LIGHTNING", "sparks", 0xffffe94d, 0.55f, 0.52f, 0.08f, 0.18f, shBolt, spark() },
            { "RAIN", "glassy drops", 0xff7fe0ff, 0.22f, 0.66f, 0.14f, 0.16f, shDrop, rain() },
            { "THUNDER", "a pressure wave", 0xffff8a3d, 0.80f, 0.70f, 0.14f, 0.14f, shRing, thunder() } } },
        { "DESERT", "dunes, sun, cactus, wind", 0xffffb04d, {
            { "DUNES", "dry sand", 0xffffc46b, 0.40f, 0.86f, 0.38f, 0.09f, shWave, sand() },
            { "SUN", "hot shine", 0xffffe94d, 0.78f, 0.22f, 0.12f, 0.15f, shRing, sun() },
            { "CACTUS", "spikes", 0xff5bd16b, 0.30f, 0.56f, 0.08f, 0.20f, shSpikes, blend (claws(), wood(), 0.5f) },
            { "WIND", "airy", 0xffeaf6ff, 0.52f, 0.40f, 0.16f, 0.08f, shWave, airy() } } },
    };
    static const std::vector<Being> body {
        { "HUMAN BODY", "an anatomy model like at school", 0xffffc9a8, {
            { "BRAIN", "neurons - sparks", 0xffff8fd0, 0.50f, 0.075f, 0.075f, 0.045f, shBrain, neural() },
            { "THROAT", "a vowel", 0xffffd23f, 0.50f, 0.185f, 0.035f, 0.035f, shRing, vowel() },
            { "LUNGS", "breath / air", 0xff7fe0ff, 0.50f, 0.30f, 0.17f, 0.075f, shLungs, breath() },
            { "HEART", "pulse / bass", 0xffff3b5c, 0.535f, 0.335f, 0.04f, 0.035f, shHeart, pulse() },
            { "BLOOD", "flowing liquid", 0xffff5a6e, 0.22f, 0.42f, 0.06f, 0.13f, shWave, blood() },
            { "BONES", "hollow bone", 0xfff1e3c6, 0.42f, 0.78f, 0.09f, 0.17f, shBones, bone() } } },
    };
    return kingdom == kAnimals ? animals : kingdom == kNature ? nature : body;
}
inline const char* kingdomName (int k) { static const char* n[] { "ANIMALS", "NATURE", "HUMAN BODY" }; return n[juce::jlimit (0, 2, k)]; }

// the part under a point of the being's 0..1 box (-1 = none); later parts sit on top
inline int partAt (int kingdom, int being, float x, float y)
{
    const auto& bs = beings (kingdom);
    if (! juce::isPositiveAndBelow (being, (int) bs.size())) return -1;
    const auto& ps = bs[(size_t) being].parts;
    int best = -1; float bd = 1.0f;
    for (int i = 0; i < (int) ps.size(); ++i)
    {
        const float dx = (x - ps[(size_t) i].cx) / ps[(size_t) i].rx, dy = (y - ps[(size_t) i].cy) / ps[(size_t) i].ry, d = dx * dx + dy * dy;
        if (d <= 1.0f && d < bd) { bd = d; best = i; }
    }
    return best;
}
inline Trait beingTrait (int kingdom, int being)
{
    const auto& ps = beings (kingdom)[(size_t) being].parts;
    Trait t = ps.front().t;
    for (int i = 1; i < (int) ps.size(); ++i) t = blend (t, ps[(size_t) i].t, 1.0f / (float) (i + 1));
    return t;
}

// living reactions: fast movement near it = PANIC, a slow approach = SHY, lingering long (or a double-click) = it MUTATES
struct Mood { float panic = 0, shy = 0, linger = 0; int mutations = 0; };
enum { reactNone, reactPanic, reactShy, reactMutate };
// near: 0 (far) .. 1 (on it); speed: mouse px / s; dt: seconds.  Returns what just started (an edge, not a state)
inline int react (Mood& m, float near, float speed, float dt)
{
    const float p0 = m.panic, s0 = m.shy;
    near = juce::jlimit (0.0f, 1.0f, near);
    if (near > 0.05f && speed > 900.0f)       { m.panic = std::min (1.0f, m.panic + dt * 3.5f * near * std::min (2.0f, speed / 1200.0f)); m.shy = std::max (0.0f, m.shy - dt * 2.0f); m.linger = 0; }
    else if (near > 0.05f && speed < 260.0f)  { m.shy = std::min (1.0f, m.shy + dt * 0.9f * near); m.panic = std::max (0.0f, m.panic - dt * 0.6f); m.linger += speed < 60.0f && near > 0.45f ? dt : 0.0f; }
    else                                      { m.panic = std::max (0.0f, m.panic - dt * 0.45f); m.shy = std::max (0.0f, m.shy - dt * 0.35f); m.linger = std::max (0.0f, m.linger - dt); }
    if (near <= 0.05f) m.linger = 0;
    if (m.linger > 3.0f) { m.linger = 0; m.mutations = std::min (6, m.mutations + 1); m.shy = 0; return reactMutate; }
    if (p0 < 0.5f && m.panic >= 0.5f) return reactPanic;
    if (s0 < 0.5f && m.shy >= 0.5f) return reactShy;
    return reactNone;
}
// the mood bends the recipe: panic = wilder (hotter, brighter, shorter, more movement), shy = softer / darker / hidden,
// a mutation = a new seed and a torn second layer
inline Recipe applyMood (Recipe r, const Mood& m)
{
    r.heat += 0.6f * m.panic; r.bright += 0.45f * m.panic; r.stretch -= 0.35f * m.panic; r.split += 0.3f * m.panic; r.matter += 0.15f * m.panic;
    r.bright -= 0.7f * m.shy; r.heat *= 1.0f - 0.9f * m.shy; r.stretch += 0.3f * m.shy; r.cool += 0.4f * m.shy; r.size -= 0.15f * m.shy;
    if (m.mutations > 0)
    {
        r.seed = mix (r.seed, 0x51ED27u * (uint32) m.mutations) | 1u;
        r.split = std::max (r.split, std::min (1.0f, 0.45f + 0.12f * (float) m.mutations));
        r.matter += (m.mutations % 2 ? 0.2f : -0.2f);
        r.exc = (r.exc + m.mutations) % 5;
    }
    clampRecipe (r);
    return r;
}
inline juce::String moodWord (const Mood& m) { return m.mutations > 0 ? "Mutant " : m.panic > 0.5f ? "Wild " : m.shy > 0.5f ? "Shy " : ""; }

// a roll of 10: the coloured (lit) parts lead (the last one first); nothing coloured = the whole being
inline std::vector<Recipe> bioRoll (int kingdom, int being, uint32 litMask, int lastPart, const Mood& mood, uint32 seed, int count = 10)
{
    const auto& B = beings (kingdom)[(size_t) juce::jlimit (0, (int) beings (kingdom).size() - 1, being)];
    const int np = (int) B.parts.size();
    std::vector<int> order;
    if (litMask != 0)
    {
        if (juce::isPositiveAndBelow (lastPart, np) && (litMask >> lastPart) & 1u) order.push_back (lastPart);
        for (int i = 0; i < np; ++i) if (((litMask >> i) & 1u) && i != lastPart) order.push_back (i);
    }
    else for (int i = 0; i < np; ++i) order.push_back (i);
    const auto base = beingTrait (kingdom, being);
    static const char* variant[] { "", "Echo", "Ghost", "Pulse", "Dust", "Bloom", "Shadow", "Glow", "Drift", "Spark" };
    std::vector<Recipe> out;
    const uint32 s0 = mix (seed, (uint32) kingdom * 131u + (uint32) being * 17u + 5u);
    for (int i = 0; i < count; ++i)
    {
        const int pi = order[(size_t) (i % (int) order.size())];
        const auto& P = B.parts[(size_t) pi];
        auto r = draw (blend (P.t, base, litMask != 0 ? 0.2f : 0.45f), mix (s0, (uint32) pi * 977u + 3u), i);
        r = applyMood (r, mood);
        r.kind = pi;
        const juce::String bn = juce::String (B.name).toLowerCase(), pn = juce::String (P.name).toLowerCase();
        r.name = (moodWord (mood) + (kingdom == kBody ? juce::String() : bn.substring (0, 1).toUpperCase() + bn.substring (1) + " ") + pn.substring (0, 1).toUpperCase() + pn.substring (1)
                  + (i < (int) order.size() ? juce::String() : " " + juce::String (variant[i % 10]))).trim();
        out.push_back (r);
    }
    return out;
}

//==============================================================================
// GARAGE (FLEX): ARSENAL / CARS / HARBOUR / HANGAR - 6 items on a ladder from the worst to the most expensive (invented names)
enum GarageTab { gArsenal, gCars, gHarbour, gHangar, numGarage };
struct Item { const char* name; const char* price; const char* hint; };
struct GaragePart { const char* name; const char* hint; float cx, cy, rx, ry; float stretch, bright, heat, cool, split; };
inline const char* garageName (int tab) { static const char* n[] { "ARSENAL", "CARS", "HARBOUR", "HANGAR" }; return n[juce::jlimit (0, 3, tab)]; }
inline const std::vector<Item>& items (int tab)
{
    static const std::vector<Item> arsenal {
        { "PEBBLE SLINGER", "40 CR", "a forked stick and a rubber band" },
        { "CORK POPPER", "900 CR", "pops a cork on a string" },
        { "BUBBLE BLASTER", "6 500 CR", "a tank of soap and a big nozzle" },
        { "THUNDER TUBE", "48 000 CR", "a cartoon tube that goes BOOM" },
        { "PLASMA SPLATTER", "390 000 CR", "glowing coils, splats of light" },
        { "NOVA LASER CANNON", "9 900 000 CR", "the biggest beam in the galaxy" } };
    static const std::vector<Item> cars {
        { "RUST BUCKET", "60 CR", "a rusty hatchback held together by tape" },
        { "DUSTY HATCH", "1 200 CR", "it starts on the second try" },
        { "NEON CRUISER", "14 000 CR", "lowered, glowing, loud stereo" },
        { "TURBO WEDGE", "120 000 CR", "a turbo and a big spoiler" },
        { "GOLDEN BULLION GT", "980 000 CR", "gold paint, gold wheels" },
        { "OBSIDIAN HYPER", "12 000 000 CR", "the fastest thing on four wheels" } };
    static const std::vector<Item> harbour {
        { "LEAKY ROWBOAT", "80 CR", "two oars and a bucket" },
        { "FISHING TUB", "2 500 CR", "smells like the sea" },
        { "SUNNY SLOOP", "30 000 CR", "one sail, one sunset" },
        { "SPEED DART", "260 000 CR", "spray in your face" },
        { "PEARL CATAMARAN", "2 400 000 CR", "two hulls, a white deck" },
        { "LEVIATHAN MEGA YACHT", "95 000 000 CR", "decks, a pool, a helipad" } };
    static const std::vector<Item> hangar {
        { "PUDDLE JUMPER", "300 CR", "an old prop plane" },
        { "CROP DUSTER", "6 000 CR", "low, loud, yellow" },
        { "TWIN PROP", "70 000 CR", "two engines, eight seats" },
        { "SKY COURIER", "900 000 CR", "a small private jet" },
        { "AURUM JET", "8 800 000 CR", "gold trim, leather cabin" },
        { "CLOUD PALACE", "150 000 000 CR", "a flying palace" } };
    return tab == gArsenal ? arsenal : tab == gCars ? cars : tab == gHarbour ? harbour : hangar;
}
inline const char* rarity (int tier) { static const char* r[] { "COMMON", "COMMON", "RARE", "RARE", "EPIC", "LEGENDARY" }; return r[juce::jlimit (0, 5, tier)]; }
inline uint32 rarityColour (int tier) { static const uint32 c[] { 0xffa8afb7, 0xffa8afb7, 0xff4d9dff, 0xff4d9dff, 0xffb15cff, 0xffffc83d }; return c[juce::jlimit (0, 5, tier)]; }

// the parts of the drawn item (in its 0..1 box) - each re-shapes the selected sound in its own way (a SCULPT step)
inline const std::vector<GaragePart>& garageParts (int tab)
{
    static const std::vector<GaragePart> arsenal {
        { "BARREL", "longer, deeper boom", 0.84f, 0.40f, 0.13f, 0.11f, 0.40f, -0.25f, 0.10f, 0.0f, 0.0f },
        { "TRIGGER", "snappier hit", 0.47f, 0.66f, 0.07f, 0.10f, -0.45f, 0.20f, 0.0f, 0.0f, 0.0f },
        { "MAGAZINE", "rapid layers", 0.30f, 0.72f, 0.08f, 0.14f, 0.0f, 0.0f, 0.10f, 0.0f, 0.40f },
        { "SCOPE", "focused shine", 0.50f, 0.20f, 0.13f, 0.08f, 0.0f, 0.45f, 0.0f, 0.15f, 0.0f } };
    static const std::vector<GaragePart> cars {
        { "ENGINE", "a hotter, deeper rumble", 0.80f, 0.62f, 0.09f, 0.08f, 0.0f, -0.20f, 0.40f, 0.0f, 0.0f },
        { "EXHAUST", "growl and tail", 0.04f, 0.70f, 0.05f, 0.07f, 0.30f, 0.0f, 0.30f, 0.0f, 0.0f },
        { "TYRES", "a short screech", 0.25f, 0.78f, 0.08f, 0.15f, -0.40f, 0.30f, 0.0f, 0.0f, 0.0f },
        { "HORN", "brighter, doubled", 0.955f, 0.66f, 0.035f, 0.07f, 0.0f, 0.40f, 0.0f, 0.0f, 0.30f },
        { "TURBO", "spooled up", 0.80f, 0.45f, 0.07f, 0.06f, 0.0f, 0.20f, 0.20f, 0.0f, 0.35f } };
    static const std::vector<GaragePart> harbour {
        { "HULL", "deeper, longer", 0.52f, 0.76f, 0.30f, 0.09f, 0.30f, -0.40f, 0.0f, 0.0f, 0.0f },
        { "SAIL", "airy, floating", 0.44f, 0.30f, 0.12f, 0.20f, 0.40f, 0.10f, 0.0f, 0.40f, 0.0f },
        { "DECK", "a short knock", 0.66f, 0.56f, 0.12f, 0.06f, -0.40f, 0.10f, 0.0f, 0.0f, 0.0f },
        { "ENGINE", "rumble", 0.08f, 0.70f, 0.06f, 0.09f, 0.0f, -0.10f, 0.40f, 0.0f, 0.0f } };
    static const std::vector<GaragePart> hangar {
        { "ENGINE", "a rising roar", 0.30f, 0.33f, 0.09f, 0.08f, 0.40f, 0.30f, 0.30f, 0.0f, 0.0f },
        { "WINGS", "wide, airy", 0.56f, 0.70f, 0.13f, 0.12f, 0.0f, 0.0f, 0.0f, 0.20f, 0.40f },
        { "CABIN", "warm, soft luxury", 0.72f, 0.44f, 0.12f, 0.06f, 0.0f, -0.35f, 0.0f, 0.30f, 0.0f } };
    return tab == gArsenal ? arsenal : tab == gCars ? cars : tab == gHarbour ? harbour : hangar;
}
// a prop plane has its engine on the nose (a propeller), a jet at the back
inline GaragePart garagePart (int tab, int part, int tier)
{
    auto p = garageParts (tab)[(size_t) juce::jlimit (0, (int) garageParts (tab).size() - 1, part)];
    if (tab == gHangar && part == 0 && tier < 3) { p.name = "PROPELLER"; p.cx = 0.94f; p.cy = 0.48f; p.rx = 0.05f; p.ry = 0.13f; }
    return p;
}
inline int garagePartAt (int tab, int tier, float x, float y)
{
    int best = -1; float bd = 1.0f;
    for (int i = 0; i < (int) garageParts (tab).size(); ++i)
    {
        const auto p = garagePart (tab, i, tier);
        const float dx = (x - p.cx) / p.rx, dy = (y - p.cy) / p.ry, d = dx * dx + dy * dy;
        if (d <= 1.0f && d < bd) { bd = d; best = i; }
    }
    return best;
}

// the character of a tab: 4 flavours (cards cycle through them)
struct Flavour { const char* word; Trait t; };
inline const std::vector<Flavour>& flavours (int tab)
{
    static const std::vector<Flavour> arsenal {
        { "Hit", T ({ eMetal, ePressure }, { bBone, bCore }, 0.35f, 0.45f, -0.60f, 0.10f, 0.30f, 0.0f, 0.0f) },
        { "Snap", T ({ eShort, eMetal }, { bBone, bCrystal }, 0.20f, 0.30f, -0.80f, 0.40f, 0.20f, 0.0f, 0.0f) },
        { "Clank", T ({ eMetal }, { bCrystal, bCore }, 0.15f, 0.45f, -0.30f, 0.30f, 0.10f, 0.0f, 0.10f) },
        { "Boom", T ({ ePressure }, { bCore, bBone }, 0.60f, 0.80f, -0.10f, -0.30f, 0.50f, 0.0f, 0.0f) } };
    static const std::vector<Flavour> cars {
        { "Rumble", T ({ ePressure }, { bCore, bLiquid }, 0.70f, 0.75f, 0.0f, -0.30f, 0.50f, 0.0f, 0.0f) },
        { "808", T ({ ePressure }, { bBone, bLiquid }, 0.50f, 0.60f, 0.20f, -0.10f, 0.20f, 0.0f, 0.0f) },
        { "Rev", T ({ eShort, ePressure }, { bCore }, 0.40f, 0.50f, -0.20f, 0.20f, 0.60f, 0.0f, 0.20f) },
        { "Screech", T ({ eFriction, eShort }, { bGas, bCrystal }, 0.30f, 0.40f, -0.40f, 0.30f, 0.30f, 0.0f, 0.0f) } };
    static const std::vector<Flavour> harbour {
        { "Pad", T ({ eBreath, eFriction }, { bGas, bLiquid }, 0.40f, 0.75f, 0.50f, 0.0f, 0.0f, 0.50f, 0.30f) },
        { "Bell", T ({ eMetal }, { bCrystal, bBone }, 0.15f, 0.60f, 0.20f, 0.20f, 0.0f, 0.0f, 0.0f) },
        { "Water", T ({ eBreath, eFriction }, { bLiquid }, 0.50f, 0.60f, 0.30f, 0.0f, 0.0f, 0.30f, 0.10f) },
        { "Horn", T ({ ePressure, eFriction }, { bCore, bGas }, 0.50f, 0.85f, 0.30f, -0.20f, 0.20f, 0.0f, 0.0f) } };
    static const std::vector<Flavour> hangar {
        { "Riser", T ({ eBreath, eShort }, { bGas }, 0.30f, 0.60f, 0.70f, 0.40f, 0.20f, 0.10f, 0.10f) },
        { "Swoosh", T ({ eBreath }, { bGas, bCrystal }, 0.20f, 0.50f, 0.10f, 0.30f, 0.0f, 0.10f, 0.0f) },
        { "Air Lead", T ({ eShort, eBreath }, { bCrystal, bLiquid }, 0.25f, 0.45f, 0.10f, 0.40f, 0.10f, 0.0f, 0.30f) },
        { "Turbine", T ({ ePressure, eBreath }, { bCore, bGas }, 0.50f, 0.80f, 0.30f, 0.0f, 0.40f, 0.0f, 0.0f) } };
    return tab == gArsenal ? arsenal : tab == gCars ? cars : tab == gHarbour ? harbour : hangar;
}
// a higher tier = a richer recipe: bigger, longer, more layers, shinier matter
inline Trait tierTrait (const Trait& t0, int tier)
{
    Trait t = t0; const float k = (float) juce::jlimit (0, 5, tier) / 5.0f;
    t.size += 0.25f * k; t.stretch += 0.45f * k; t.split += 0.15f + 0.55f * k; t.matter -= 0.15f * k;
    if (tier == 0) { t.split = 0; t.heat += 0.15f; t.matter += 0.1f; }   // the worst one is dirty and thin
    return t;
}
inline std::vector<Recipe> garageRoll (int tab, int tier, uint32 seed, int count = 10)
{
    tab = juce::jlimit (0, 3, tab); tier = juce::jlimit (0, 5, tier);
    const auto& fl = flavours (tab);
    const auto& it = items (tab)[(size_t) tier];
    const uint32 s0 = mix (seed, (uint32) tab * 1009u + 7u);   // the same spread on every rung: only the tier makes it richer
    static const char* tierWord[] { "Cheap", "Basic", "Fresh", "Prime", "Royal", "Legend" };
    std::vector<Recipe> out;
    for (int i = 0; i < count; ++i)
    {
        const int f = (i + (int) (s0 % 4u)) % 4;
        auto r = draw (tierTrait (fl[(size_t) f].t, tier), s0, i, 0.8f);
        // the item itself is its own mutation (two items never sound alike)
        r.seed = mix (r.seed, (uint32) tier * 113u + 1u) | 1u;
        Rng ir (mix (s0, 77u + (uint32) i + (uint32) tier * 5u));
        if (ir.uni() < 0.3f) r.body = (r.body + 1 + tier) % 5;
        r.kind = f;
        const juce::String n (it.name);
        r.name = juce::String (tierWord[tier]) + " " + n.substring (0, 1) + n.substring (1).toLowerCase().upToFirstOccurrenceOf (" ", false, false) + " " + fl[(size_t) f].word;
        if (i >= 4) r.name << " " << juce::String::charToString ((juce::juce_wchar) ('A' + (i - 4)));
        clampRecipe (r);
        out.push_back (r);
    }
    return out;
}
// a click on a part re-shapes the sound (each click goes a step further)
inline Recipe applyPart (Recipe r, int tab, int part, int tier)
{
    const auto p = garagePart (tab, part, tier);
    r.stretch += p.stretch; r.bright += p.bright; r.heat += p.heat; r.cool += p.cool; r.split += p.split;
    clampRecipe (r);
    const juce::String pn (p.name);
    if (! r.name.containsIgnoreCase (pn)) r.name = pn.substring (0, 1) + pn.substring (1).toLowerCase() + " " + r.name;
    return r;
}

//==============================================================================
// PARTY: the crowd, the lights and the disco ball make the ENERGY; it maps monotonically to the sounds and the groove
struct Party { float crowd = 0.35f, lights = 0.35f, spin = 0.3f; float energy() const { return juce::jlimit (0.0f, 1.0f, 0.36f * crowd + 0.30f * lights + 0.34f * spin); } };
enum PartyKind { pChords, pStrings, pOrgan, pPluck, pBass, numPartyKinds };
inline const char* partyKindName (int k) { static const char* n[] { "House Chords", "Disco Strings", "Organ Stab", "Pluck", "Bassline" }; return n[juce::jlimit (0, 4, k)]; }
inline juce::String energyWord (float e) { return e < 0.25f ? "DEEP" : e < 0.5f ? "WARM" : e < 0.75f ? "BRIGHT" : "EUPHORIC"; }
inline Trait partyKindTrait (int k)
{
    switch (k)
    {
        case pChords:  return T ({ eFriction, eBreath }, { bLiquid, bGas }, 0.45f, 0.55f, 0.10f, 0.0f, 0.0f, 0.0f, 0.10f);
        case pStrings: return T ({ eFriction }, { bGas, bLiquid }, 0.40f, 0.60f, 0.30f, 0.0f, 0.0f, 0.0f, 0.20f);
        case pOrgan:   return T ({ eShort, eBreath }, { bLiquid }, 0.35f, 0.50f, -0.30f, 0.0f, 0.10f, 0.0f, 0.0f);
        case pPluck:   return T ({ eMetal, eShort }, { bBone, bCrystal }, 0.30f, 0.45f, -0.50f, 0.0f, 0.0f, 0.0f, 0.0f);
        default:       return T ({ ePressure }, { bBone, bLiquid }, 0.50f, 0.55f, -0.10f, 0.0f, 0.10f, 0.0f, 0.0f);
    }
}
// energy: calm = deep, warm, long, sparse ... high = bright, glassy, hot, layered (monotonic in every field it touches)
inline Recipe partyEnergy (Recipe r, float e)
{
    e = juce::jlimit (0.0f, 1.0f, e);
    r.bright += -0.55f + 1.1f * e; r.matter += 0.3f - 0.6f * e; r.heat += 0.4f * e; r.split += 0.5f * e;
    r.cool += 0.35f * (1.0f - e); r.stretch += 0.35f - 0.6f * e;
    clampRecipe (r);
    return r;
}
inline std::vector<Recipe> partyRoll (float energy, uint32 seed, int count = 10)
{
    std::vector<Recipe> out;
    for (int i = 0; i < count; ++i)
    {
        const int k = i % numPartyKinds;
        auto r = draw (partyKindTrait (k), mix (seed, (uint32) k * 31u + 1u), i, 0.7f);
        r = partyEnergy (r, energy);
        r.kind = k;
        const juce::String w = energyWord (energy);
        r.name = w.substring (0, 1) + w.substring (1).toLowerCase() + " " + partyKindName (k) + (i >= numPartyKinds ? " II" : "");
        out.push_back (r);
    }
    return out;
}
// the 4-on-the-floor phrase (4 bars = 16 beats): bass on the beat, chords on top.  More energy = busier and higher
struct PNote { float start, len; int note; bool low; };
inline std::vector<PNote> partyPhrase (float e, uint32 seed, int kind = pChords)
{
    e = juce::jlimit (0.0f, 1.0f, e);
    static const int prog[2][4] { { 45, 41, 48, 43 }, { 45, 43, 41, 40 } };                   // Am F C G  |  Am G F E
    static const int qual[2][4][4] { { { 0, 3, 7, 10 }, { 0, 4, 7, 11 }, { 0, 4, 7, 11 }, { 0, 4, 7, 10 } },
                                     { { 0, 3, 7, 10 }, { 0, 4, 7, 10 }, { 0, 4, 7, 11 }, { 0, 4, 7, 10 } } };
    const int pi = (int) (mix (seed, 3u) % 2u);
    const bool chords = kind != pBass, bassBusy = kind == pBass;
    const int up = e >= 0.75f ? 12 : 0;
    std::vector<PNote> n;
    for (int bar = 0; bar < 4; ++bar)
    {
        const float b0 = (float) bar * 4.0f; const int root = prog[pi][bar];
        // bass: calm = beats 1 and 3, then every beat (4 on the floor), then house offbeat octaves, then 16th ghosts
        for (int beat = 0; beat < 4; ++beat)
        {
            if (e >= 0.25f || beat % 2 == 0 || bassBusy) n.push_back ({ b0 + (float) beat, e >= 0.25f ? 0.45f : 1.8f, root, true });
            if (e >= 0.6f || (bassBusy && e >= 0.3f)) n.push_back ({ b0 + (float) beat + 0.5f, 0.3f, root + 12, true });
            if (e >= 0.85f) n.push_back ({ b0 + (float) beat + 0.75f, 0.15f, root + (beat % 2 ? 7 : 12), true });
        }
        if (! chords) continue;
        const int nv = e >= 0.6f ? 4 : 3;
        auto chord = [&] (float at, float len) { for (int v = 0; v < nv; ++v) n.push_back ({ at, len, root + 12 + up + qual[pi][bar][v] + (root < 43 ? 12 : 0), false }); };
        if (e < 0.35f) chord (b0, 3.6f);                                          // one long warm chord a bar
        else for (int beat = 0; beat < 4; ++beat) chord (b0 + (float) beat + 0.5f, 0.4f);   // offbeat stabs
        if (e >= 0.8f) { chord (b0 + 2.75f, 0.2f); chord (b0 + 3.75f, 0.2f); }    // euphoric pushes
    }
    return n;
}
} // namespace kk::worlds
