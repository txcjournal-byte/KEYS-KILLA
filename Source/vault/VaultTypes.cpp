#include "VaultTypes.h"
#include "../ParamKeys.h"
#include <map>

namespace vault
{
namespace
{
constexpr char magic[4] { 'C', 'V', 'B', '1' };
constexpr int flagGzip = 1;
constexpr int maxKeys = 65535;
constexpr int maxStringBytes = 4096;

bool readString (juce::MemoryInputStream& in, juce::String& s)
{
    juce::MemoryOutputStream bytes;
    for (int i = 0; i < maxStringBytes; ++i)
    {
        if (in.isExhausted()) return false;
        const char c = in.readByte();
        if (c == 0) { s = juce::String::fromUTF8 ((const char*) bytes.getData(), (int) bytes.getDataSize()); return true; }
        bytes.writeByte (c);
    }
    return false;
}
} // namespace

namespace
{
void writeDests (juce::MemoryOutputStream& o, const std::vector<VaultModDest>& d)
{
    const int n = juce::jmin ((int) d.size(), 255);
    o.writeByte ((char) n);
    for (int i = 0; i < n; ++i) { o.writeString (d[(size_t) i].target); o.writeFloat (d[(size_t) i].amount); }
}
bool readDests (juce::MemoryInputStream& in, std::vector<VaultModDest>& d)
{
    const int n = (uint8_t) in.readByte();
    d.resize ((size_t) n);
    for (auto& x : d) { if (! readString (in, x.target)) return false; x.amount = in.readFloat(); }
    return true;
}
// the v3 section of one preset (after the parts)
void writeV3 (juce::MemoryOutputStream& o, const VaultPreset& p)
{
    o.writeFloat (p.finish);
    const int nfx = juce::jmin ((int) p.fx.size(), 255);
    o.writeByte ((char) nfx);
    for (int i = 0; i < nfx; ++i)
    {
        const auto& f = p.fx[(size_t) i];
        o.writeByte ((char) (uint8_t) f.type); o.writeByte (f.on ? 1 : 0);
        o.writeByte ((char) maxFxParams);
        for (float v : f.p) o.writeFloat (v);
    }
    for (const auto& l : p.mod.lfo)
    {
        o.writeByte ((char) l.shape); o.writeFloat (l.rate); o.writeByte (l.sync ? 1 : 0); o.writeByte ((char) l.div);
        o.writeFloat (l.delay); o.writeFloat (l.fade); o.writeFloat (l.phase); o.writeByte (l.retrig ? 1 : 0); o.writeFloat (l.depth);
        writeDests (o, l.dests);
    }
    writeDests (o, p.mod.modWheel); writeDests (o, p.mod.velocity); writeDests (o, p.mod.aftertouch);
    for (const auto& m : p.macros)
    {
        o.writeString (m.name);
        const int n = juce::jmin ((int) m.targets.size(), 255);
        o.writeByte ((char) n);
        for (int i = 0; i < n; ++i)
        {
            const auto& t = m.targets[(size_t) i];
            o.writeString (t.target); o.writeFloat (t.min); o.writeFloat (t.max); o.writeFloat (t.curve);
        }
    }
    const auto& a = p.arp;
    o.writeByte (a.on ? 1 : 0); o.writeByte ((char) a.mode); o.writeByte ((char) a.rate); o.writeByte ((char) a.octaves);
    o.writeFloat (a.gate); o.writeFloat (a.swing); o.writeByte (a.latch ? 1 : 0); o.writeByte ((char) a.steps);
    for (float v : a.vel) o.writeFloat (v);
    for (auto v : a.type) o.writeByte ((char) v);
    for (auto v : a.oct) o.writeByte ((char) v);
    const auto& g = p.gate;
    o.writeByte (g.on ? 1 : 0); o.writeByte ((char) g.rate); o.writeByte ((char) g.steps);
    for (float v : g.level) o.writeFloat (v);
    o.writeFloat (g.depth); o.writeFloat (g.smooth); o.writeFloat (g.swing); o.writeFloat (g.mix);
}
bool readV3 (juce::MemoryInputStream& in, VaultPreset& p)
{
    p.v3 = true;
    p.finish = in.readFloat();
    const int nfx = (uint8_t) in.readByte();
    p.fx.resize ((size_t) nfx);
    for (auto& f : p.fx)
    {
        const int t = (uint8_t) in.readByte();
        if (t >= numFxTypes) return false;
        f = VaultFxSlot::make ((FxType) t);
        f.on = in.readByte() != 0;
        const int np = (uint8_t) in.readByte();
        for (int i = 0; i < np; ++i) { const float v = in.readFloat(); if (i < maxFxParams) f.p[(size_t) i] = v; }
    }
    for (auto& l : p.mod.lfo)
    {
        l.shape = (uint8_t) in.readByte(); l.rate = in.readFloat(); l.sync = in.readByte() != 0; l.div = (uint8_t) in.readByte();
        l.delay = in.readFloat(); l.fade = in.readFloat(); l.phase = in.readFloat(); l.retrig = in.readByte() != 0; l.depth = in.readFloat();
        if (! readDests (in, l.dests)) return false;
    }
    if (! readDests (in, p.mod.modWheel) || ! readDests (in, p.mod.velocity) || ! readDests (in, p.mod.aftertouch)) return false;
    for (auto& m : p.macros)
    {
        if (! readString (in, m.name)) return false;
        m.targets.resize ((size_t) (uint8_t) in.readByte());
        for (auto& t : m.targets) { if (! readString (in, t.target)) return false; t.min = in.readFloat(); t.max = in.readFloat(); t.curve = in.readFloat(); }
    }
    auto& a = p.arp;
    a.on = in.readByte() != 0; a.mode = (uint8_t) in.readByte(); a.rate = (uint8_t) in.readByte(); a.octaves = (uint8_t) in.readByte();
    a.gate = in.readFloat(); a.swing = in.readFloat(); a.latch = in.readByte() != 0; a.steps = (uint8_t) in.readByte();
    for (auto& v : a.vel) v = in.readFloat();
    for (auto& v : a.type) v = (uint8_t) in.readByte();
    for (auto& v : a.oct) v = (int8_t) in.readByte();
    auto& g = p.gate;
    g.on = in.readByte() != 0; g.rate = (uint8_t) in.readByte(); g.steps = (uint8_t) in.readByte();
    for (auto& v : g.level) v = in.readFloat();
    g.depth = in.readFloat(); g.smooth = in.readFloat(); g.swing = in.readFloat(); g.mix = in.readFloat();
    return ! in.isExhausted() || in.getPosition() == in.getTotalLength();
}
} // namespace

juce::MemoryBlock serializeBank (const std::vector<VaultPreset>& bankIn, bool compress)
{
    // v1 / v2 style presets are written as v3 (their FX become the chain)
    std::vector<VaultPreset> upgraded;
    bool anyLegacy = false;
    for (auto& p : bankIn) anyLegacy |= ! p.v3;
    if (anyLegacy) { upgraded = bankIn; for (auto& p : upgraded) upgradeToV3 (p); }
    const auto& bank = anyLegacy ? upgraded : bankIn;

    // key table: the canonical synth keys first (stable indices), then the sample keys, then any other ID the bank uses
    juce::StringArray keys;
    for (int k = 0; k < kk::vk::count; ++k) keys.add (kk::vk::info (k).id);
    for (auto& sp : sampleParams()) keys.add (sp.id);
    std::map<juce::String, int> keyIndex;
    for (int i = 0; i < keys.size(); ++i) keyIndex[keys[i]] = i;
    for (auto& p : bank)
        for (auto& part : p.parts)
            for (auto& [id, v] : part.values)
                if (keyIndex.find (id) == keyIndex.end() && keys.size() < maxKeys) { keyIndex[id] = keys.size(); keys.add (id); }

    juce::MemoryOutputStream payload;
    payload.writeShort ((short) keys.size());
    for (auto& k : keys) payload.writeString (k);
    for (auto& p : bank)
    {
        payload.writeString (p.id);
        payload.writeString (p.name);
        payload.writeByte ((char) juce::jlimit (0, 255, p.category));
        payload.writeFloat (p.gain);
        for (float m : p.macroDefaults) payload.writeFloat (m);
        const int np = juce::jmin ((int) p.parts.size(), 255);
        payload.writeByte ((char) np);
        for (int pi = 0; pi < np; ++pi)
        {
            const auto& part = p.parts[(size_t) pi];
            payload.writeByte ((char) (uint8_t) part.kind);
            payload.writeString (part.source);
            payload.writeString (part.articulation);
            payload.writeString (part.label);
            payload.writeFloat (part.level);
            payload.writeFloat (part.pan);
            payload.writeByte ((char) juce::jlimit (-128, 127, part.octave));
            payload.writeByte ((char) juce::jlimit (-128, 127, part.semi));
            int nv = 0;
            for (auto& [id, v] : part.values) if (keyIndex.count (id)) ++nv;
            jassert (nv == (int) part.values.size());   // more than maxKeys distinct parameter IDs in one bank
            payload.writeShort ((short) juce::jmin (nv, 65535));
            for (auto& [id, v] : part.values)
                if (const auto it = keyIndex.find (id); it != keyIndex.end())
                {
                    payload.writeShort ((short) (uint16_t) it->second);
                    payload.writeFloat (v);
                }
        }
        writeV3 (payload, p);
    }

    juce::MemoryOutputStream out;
    out.write (magic, 4);
    out.writeShort ((short) bankFormatVersion);
    out.writeShort ((short) (compress ? flagGzip : 0));
    out.writeInt ((int) bank.size());
    out.writeInt ((int) payload.getDataSize());
    if (compress)
    {
        juce::GZIPCompressorOutputStream gz (out, 9);
        gz.write (payload.getData(), payload.getDataSize());
        gz.flush();
    }
    else out.write (payload.getData(), payload.getDataSize());
    return out.getMemoryBlock();
}

bool deserializeBank (const void* data, size_t size, std::vector<VaultPreset>& out, juce::String* error, int* versionOut, bool keepLegacy)
{
    out.clear();
    auto fail = [&] (const char* why) { if (error) *error = why; out.clear(); return false; };
    if (data == nullptr || size < 16 || std::memcmp (data, magic, 4) != 0) return fail ("not a CHROME VAULT bank (CVB1)");
    juce::MemoryInputStream head (data, size, false);
    head.skipNextBytes (4);
    const int version = (uint16_t) head.readShort();
    const int flags = (uint16_t) head.readShort();
    const int count = head.readInt();
    const int rawSize = head.readInt();
    if (version < 1 || version > bankFormatVersion) return fail ("unsupported bank version");
    if (versionOut) *versionOut = version;
    if (count < 0 || count > 1000000 || rawSize < 2 || rawSize > 512 * 1024 * 1024) return fail ("bad bank header");

    juce::MemoryBlock payload;
    if (flags & flagGzip)
    {
        juce::MemoryInputStream zin ((const char*) data + 16, size - 16, false);
        juce::GZIPDecompressorInputStream gz (zin);
        payload.setSize ((size_t) rawSize);
        if (gz.read (payload.getData(), rawSize) != rawSize) return fail ("damaged bank payload");
    }
    else
    {
        if (size - 16 < (size_t) rawSize) return fail ("truncated bank payload");
        payload.append ((const char*) data + 16, (size_t) rawSize);
    }

    juce::MemoryInputStream in (payload, false);
    const int nKeys = (uint16_t) in.readShort();
    juce::StringArray keys;
    for (int i = 0; i < nKeys; ++i) { juce::String k; if (! readString (in, k)) return fail ("bad key table"); keys.add (k); }

    out.reserve ((size_t) count);
    for (int i = 0; i < count; ++i)
    {
        if (in.isExhausted()) return fail ("truncated bank");
        VaultPreset p;
        if (! readString (in, p.id) || ! readString (in, p.name)) return fail ("bad preset text");
        p.category = (uint8_t) in.readByte();
        if (version == 1) { in.readInt(); in.readInt(); }   // v1 style / character tags: dropped
        p.gain = in.readFloat();
        for (float& m : p.macroDefaults) m = in.readFloat();
        const int np = (uint8_t) in.readByte();
        p.parts.resize ((size_t) np);
        for (auto& part : p.parts)
        {
            if (version >= 2)
            {
                const int kind = (uint8_t) in.readByte();
                if (kind > (int) VaultPartKind::model) return fail ("unknown part kind");
                part.kind = (VaultPartKind) kind;
                if (! readString (in, part.source) || ! readString (in, part.articulation)) return fail ("bad part source");
            }
            if (! readString (in, part.label)) return fail ("bad part label");
            part.level = in.readFloat();
            part.pan = in.readFloat();
            part.octave = (int8_t) in.readByte();
            part.semi = (int8_t) in.readByte();
            const int nv = (uint16_t) in.readShort();
            part.values.reserve ((size_t) nv);
            for (int v = 0; v < nv; ++v)
            {
                const int k = version >= 2 ? (int) (uint16_t) in.readShort() : (int) (uint8_t) in.readByte();
                const float x = in.readFloat();
                if (k >= keys.size()) return fail ("bad parameter key");
                part.values.push_back ({ keys[k], x });
            }
        }
        if (version == 1)
        {
            p.category = v1::toV2Category (p.category, p);
            p.name = v1::cleanText (p.name);
            for (auto& part : p.parts) part.label = v1::cleanText (part.label);
        }
        if (version >= 3) { if (! readV3 (in, p)) return fail ("bad v3 preset section"); }
        else if (! keepLegacy) upgradeToV3 (p);   // v1 / v2: the FX of part 0 become the chain, the classic macros the sound macros
        out.push_back (std::move (p));
    }
    if (error) error->clear();
    return true;
}

namespace v1
{
juce::String cleanText (const juce::String& s)
{
    if (! s.contains ("808")) return s;
    auto t = s.replace ("808s", "Subs").replace ("808", "Sub");
    while (t.contains ("Sub Sub")) t = t.replace ("Sub Sub", "Sub");
    return t.trim();
}

int toV2Category (int c, const VaultPreset& p)
{
    switch (c)
    {
        case v1::cat808Bass:       return vault::catBass;
        case catBellsMallets:  return vault::catBellsMallets;
        case catPianoKeys:
        {
            juce::String words = p.name;
            for (auto& part : p.parts) words << " " << part.label;
            words = words.toLowerCase();
            return words.contains ("piano") || words.contains ("grand") || words.contains ("upright") || words.contains ("felt")
                       ? vault::catPiano : vault::catKeysOrgan;
        }
        case catPlucks:        return vault::catPlucks;
        case catLeads:         return vault::catLeads;
        case catPads:          return vault::catPads;
        case catStrings:       return vault::catStrings;
        case catBrassHorns:    return vault::catBrass;
        case catFlutesWinds:   return vault::catFlutesWoodwinds;
        case catGuitars:       return vault::catGuitars;
        case catVocalsChoir:   return vault::catVocalsChoir;
        case catSynth:         return vault::catSynth;
        case catArpsSequences: return vault::catSynth;
        case catOrchestral:    return vault::catOrchestral;
        case catWorld:         return vault::catWorld;
        default:               return vault::catFxAtmos;
    }
}
} // namespace v1

bool saveBankFile (const juce::File& file, const std::vector<VaultPreset>& bank)
{
    const auto data = serializeBank (bank, true);
    file.getParentDirectory().createDirectory();
    return file.replaceWithData (data.getData(), data.getSize());
}

bool loadBankFile (const juce::File& file, std::vector<VaultPreset>& out, juce::String* error)
{
    juce::MemoryBlock mb;
    if (! file.loadFileAsData (mb)) { if (error) *error = "cannot read " + file.getFullPathName(); out.clear(); return false; }
    return deserializeBank (mb.getData(), mb.getSize(), out, error);
}

bool validatePreset (const VaultPreset& p, juce::String* why)
{
    auto no = [&] (const juce::String& w) { if (why) *why = p.id + ": " + w; return false; };
    if (p.id.isEmpty()) return no ("empty id");
    if (p.name.trim().isEmpty()) return no ("empty name");
    if (p.category < 0 || p.category >= numCategories) return no ("category out of range");
    if (p.parts.empty() || (int) p.parts.size() > maxParts) return no ("needs 1..4 parts");
    if (! std::isfinite (p.gain) || p.gain <= 0.0f || p.gain > 16.0f) return no ("bad gain");
    for (float m : p.macroDefaults) if (! std::isfinite (m) || m < 0.0f || m > 1.0f) return no ("macro default out of 0..1");
    for (auto& part : p.parts)
    {
        if (! std::isfinite (part.level) || part.level < 0.0f || part.level > 2.0f) return no ("part level out of 0..2");
        if (! std::isfinite (part.pan) || std::abs (part.pan) > 1.0f) return no ("part pan out of -1..1");
        if (std::abs (part.octave) > 3 || std::abs (part.semi) > 12) return no ("part transpose out of range");
        if (part.kind == VaultPartKind::sample && part.source.isEmpty()) return no ("sample part without instrument id");
        if (part.kind == VaultPartKind::model && part.source.isEmpty()) return no ("model part without model id");
        if (part.kind == VaultPartKind::synth && (part.source.isNotEmpty() || part.articulation.isNotEmpty())) return no ("synth part with a source");
        for (auto& [id, v] : part.values)
        {
            if (! std::isfinite (v)) return no ("non-finite value for " + id);
            if (kk::vk::keyOf (id) >= 0) continue;   // EVOLVE synth / FX keys: valid in every kind (FX read from part 0)
            if (id == peakKey)                        // preset peak control (read from part 0)
            {
                if (v < -60.0f || v > 0.0f) return no ("peak control out of -60..0 dB");
                continue;
            }
            if (id == peakVelKey)
            {
                if (v < 0.0f || v > 4.0f) return no ("peak control velocity exponent out of 0..4");
                continue;
            }
            if (part.kind == VaultPartKind::sample)
            {
                const int si = sampleParamIndex (id);
                if (si < 0) return no ("unknown sample parameter " + id);
                const auto& in = sampleParams()[(size_t) si];
                if (v < in.lo || v > in.hi) return no ("sample parameter out of range: " + id);
                continue;
            }
            if (part.kind == VaultPartKind::model && id.startsWith ("m.")) continue;
            return no ("unknown parameter " + id);
        }
    }
    if (! p.v3) return true;
    // ---- v3 sections
    if (! std::isfinite (p.finish) || p.finish < 0.0f || p.finish > 1.0f) return no ("finish out of 0..1");
    if ((int) p.fx.size() > maxFxSlots) return no ("more than 8 FX slots");
    int perType[numFxTypes] {};
    for (auto& f : p.fx)
    {
        if ((int) f.type >= numFxTypes) return no ("unknown FX type");
        if (++perType[(int) f.type] > maxSameFxType) return no (juce::String ("more than 2 ") + fxTypeInfo (f.type).name + " slots");
        const auto& ti = fxTypeInfo (f.type);
        for (size_t i = 0; i < ti.params.size(); ++i)
        {
            const auto& in = ti.params[i];
            const float v = f.p[i];
            if (! std::isfinite (v) || v < in.lo - 1.0e-4f * (1.0f + std::abs (in.lo)) || v > in.hi + 1.0e-4f * (1.0f + std::abs (in.hi)))
                return no (juce::String ("FX parameter out of range: ") + ti.id + "." + in.id);
        }
    }
    auto targetOk = [&] (const juce::String& t)
    {
        const auto r = parseTarget (t);
        if (! r.valid()) return false;
        if (r.kind == TargetRef::fxSlot)
        {
            if (r.a >= p.fx.size()) return false;
            return fxTypeInfo (p.fx[r.a].type).indexOf (t.fromFirstOccurrenceOf (".", false, false).toRawUTF8()) >= 0;
        }
        if (r.kind == TargetRef::layer) return r.a < p.parts.size();
        return true;
    };
    auto destsOk = [&] (const std::vector<VaultModDest>& d, int maxN, const char* what) -> bool
    {
        if ((int) d.size() > maxN) { no (juce::String ("too many ") + what); return false; }
        for (auto& x : d)
            if (! targetOk (x.target) || ! std::isfinite (x.amount) || std::abs (x.amount) > 48.0f) { no (juce::String ("bad ") + what + " target " + x.target); return false; }
        return true;
    };
    for (auto& l : p.mod.lfo)
    {
        if (l.shape >= VaultLfo::numShapes || ! std::isfinite (l.rate) || l.rate < 0.01f || l.rate > 50.0f || l.div >= divisions().size()
            || ! std::isfinite (l.delay) || l.delay < 0 || l.delay > 10 || ! std::isfinite (l.fade) || l.fade < 0 || l.fade > 10
            || ! std::isfinite (l.phase) || l.phase < 0 || l.phase > 1 || ! std::isfinite (l.depth) || l.depth < 0 || l.depth > 1)
            return no ("LFO setting out of range");
        if (! destsOk (l.dests, maxLfoDests, "LFO destinations")) return false;
    }
    if (! destsOk (p.mod.modWheel, maxRoutings, "mod wheel routings") || ! destsOk (p.mod.velocity, maxRoutings, "velocity routings")
        || ! destsOk (p.mod.aftertouch, maxRoutings, "aftertouch routings")) return false;
    for (auto& m : p.macros)
    {
        if ((int) m.targets.size() > maxMacroTargets) return no ("more than 8 macro targets");
        if (m.targets.size() > 0 && m.name.trim().isEmpty()) return no ("macro with targets but no name");
        for (auto& t : m.targets)
            if (! targetOk (t.target) || ! std::isfinite (t.min) || ! std::isfinite (t.max) || ! std::isfinite (t.curve) || std::abs (t.curve) > 1.0f)
                return no ("bad macro target " + t.target);
    }
    const auto& a = p.arp;
    if (a.mode >= VaultArp::numModes || a.rate >= divisions().size() || a.octaves < 1 || a.octaves > 4 || a.steps < 1 || a.steps > numSteps
        || ! std::isfinite (a.gate) || a.gate < 0.01f || a.gate > 1.0f || ! std::isfinite (a.swing) || a.swing < 0 || a.swing > 1)
        return no ("arpeggiator setting out of range");
    for (int i = 0; i < numSteps; ++i)
        if (! std::isfinite (a.vel[(size_t) i]) || a.vel[(size_t) i] < 0 || a.vel[(size_t) i] > 1 || a.type[(size_t) i] > VaultArp::tie || std::abs (a.oct[(size_t) i]) > 2)
            return no ("arpeggiator step out of range");
    const auto& g = p.gate;
    if (g.rate >= divisions().size() || g.steps < 1 || g.steps > numSteps)
        return no ("gate setting out of range");
    for (float v : { g.depth, g.smooth, g.swing, g.mix }) if (! std::isfinite (v) || v < 0 || v > 1) return no ("gate setting out of range");
    for (float v : g.level) if (! std::isfinite (v) || v < 0 || v > 1) return no ("gate step out of range");
    return true;
}
} // namespace vault

//==============================================================================
// v3 tables, structs, targets
namespace vault
{
const std::vector<Division>& divisions()
{
    static const std::vector<Division> d {
        { "8 bars", 32.0 }, { "4 bars", 16.0 }, { "2 bars", 8.0 }, { "1/1", 4.0 }, { "1/2D", 3.0 }, { "1/2", 2.0 }, { "1/2T", 4.0 / 3.0 },
        { "1/4D", 1.5 }, { "1/4", 1.0 }, { "1/4T", 2.0 / 3.0 }, { "1/8D", 0.75 }, { "1/8", 0.5 }, { "1/8T", 1.0 / 3.0 },
        { "1/16D", 0.375 }, { "1/16", 0.25 }, { "1/16T", 1.0 / 6.0 }, { "1/32D", 0.1875 }, { "1/32", 0.125 }, { "1/32T", 1.0 / 12.0 },
        { "1/64", 0.0625 } };
    return d;
}
int divisionIndex (const char* name)
{
    const auto& d = divisions();
    for (int i = 0; i < (int) d.size(); ++i) if (juce::String (d[(size_t) i].name) == name) return i;
    return -1;
}

float FxParamInfo::toNorm (float v) const
{
    if (hi <= lo) return 0.0f;
    v = juce::jlimit (lo, hi, v);
    if ((flags & logScale) && lo > 0.0f) return (float) (std::log (v / lo) / std::log (hi / lo));
    return (v - lo) / (hi - lo);
}
float FxParamInfo::fromNorm (float n) const
{
    n = juce::jlimit (0.0f, 1.0f, n);
    float v = (flags & logScale) && lo > 0.0f ? lo * std::pow (hi / lo, n) : lo + (hi - lo) * n;
    if (flags & (integer | toggle)) v = std::round (v);
    return juce::jlimit (lo, hi, v);
}
int FxTypeInfo::indexOf (const char* paramId) const
{
    for (int i = 0; i < (int) params.size(); ++i) if (juce::String (params[(size_t) i].id) == paramId) return i;
    return -1;
}

namespace
{
using F = FxParamInfo;
constexpr uint8_t LOG = F::logScale, INT = F::integer, TOG = F::toggle;
std::vector<FxTypeInfo> makeFxTypes()
{
    const float d4 = (float) divisionIndex ("1/4"), d8d = (float) divisionIndex ("1/8D"), d8 = (float) divisionIndex ("1/8");
    const float dMax = (float) divisions().size() - 1;
    static const juce::String divNames = []
    {
        juce::StringArray a;
        for (auto& d : divisions()) a.add (d.name);
        return a.joinIntoString ("|");
    }();
    const char* dn = divNames.toRawUTF8();
    std::vector<FxTypeInfo> t;
    t.push_back ({ FxType::eq, "eq", "EQ", 11, {
        { "tilt", "Tilt", -12, 12, 0, 0, nullptr, "dB" }, { "low", "Low", -18, 18, 0, 0, nullptr, "dB" },
        { "lowFreq", "Low Freq", 30, 1000, 160, LOG, nullptr, "Hz" }, { "mid", "Mid", -18, 18, 0, 0, nullptr, "dB" },
        { "midFreq", "Mid Freq", 150, 12000, 1000, LOG, nullptr, "Hz" }, { "midQ", "Mid Q", 0.2f, 8, 0.7f, LOG, nullptr, "" },
        { "high", "High", -18, 18, 0, 0, nullptr, "dB" }, { "highFreq", "High Freq", 1000, 18000, 6000, LOG, nullptr, "Hz" } } });
    t.push_back ({ FxType::compressor, "comp", "Compressor", 1, {
        { "threshold", "Threshold", -48, 0, -18, 0, nullptr, "dB" }, { "ratio", "Ratio", 1, 20, 3, LOG, nullptr, ":1" },
        { "attack", "Attack", 0.1f, 200, 10, LOG, nullptr, "ms" }, { "release", "Release", 10, 2000, 150, LOG, nullptr, "ms" },
        { "makeup", "Makeup", 0, 24, 0, 0, nullptr, "dB" }, { "mix", "Mix", 0, 1, 1, 0, nullptr, "" },
        { "knee", "Knee", 0, 12, 6, 0, nullptr, "dB" } } });
    t.push_back ({ FxType::drive, "drive", "Drive", 2, {
        { "amount", "Amount", 0, 1, 0.3f, 0, nullptr, "" }, { "type", "Type", 0, 5, 0, INT, "Tube|Tape|Hard|Blown|Fold|Bit", "" },
        { "tone", "Tone", 500, 20000, 20000, LOG, nullptr, "Hz" }, { "mix", "Mix", 0, 1, 1, 0, nullptr, "" },
        { "cleanLow", "Clean Lows", 0, 1, 0, TOG, nullptr, "" }, { "output", "Output", -12, 12, 0, 0, nullptr, "dB" } } });
    t.push_back ({ FxType::lofi, "lofi", "Lo-Fi", 4, {
        { "crush", "Crush", 0, 1, 0.3f, 0, nullptr, "" }, { "wow", "Wow", 0, 1, 0.2f, 0, nullptr, "" },
        { "mix", "Mix", 0, 1, 1, 0, nullptr, "" } } });
    t.push_back ({ FxType::chorus, "chorus", "Chorus", 8, {
        { "mix", "Mix", 0, 1, 0.4f, 0, nullptr, "" }, { "rate", "Rate", 0.05f, 5, 0.33f, LOG, nullptr, "Hz" },
        { "depth", "Depth", 0, 1, 0.5f, 0, nullptr, "" }, { "mode", "Mode", 0, 1, 0, INT, "Chorus|Ensemble", "" } } });
    t.push_back ({ FxType::phaser, "phaser", "Phaser", 9, {
        { "mix", "Mix", 0, 1, 0.4f, 0, nullptr, "" }, { "rate", "Rate", 0.02f, 5, 0.35f, LOG, nullptr, "Hz" },
        { "depth", "Depth", 0, 1, 1, 0, nullptr, "" }, { "feedback", "Feedback", 0, 0.9f, 0.5f, 0, nullptr, "" },
        { "center", "Center", 50, 2000, 250, LOG, nullptr, "Hz" } } });
    t.push_back ({ FxType::flanger, "flanger", "Flanger", 10, {
        { "mix", "Mix", 0, 1, 0.4f, 0, nullptr, "" }, { "rate", "Rate", 0.02f, 5, 0.22f, LOG, nullptr, "Hz" },
        { "depth", "Depth", 0, 1, 1, 0, nullptr, "" }, { "feedback", "Feedback", -0.95f, 0.95f, 0.65f, 0, nullptr, "" } } });
    t.push_back ({ FxType::filter, "filter", "Filter", 7, {
        { "cutoff", "Cutoff", 20, 20000, 2000, LOG, nullptr, "Hz" }, { "reso", "Resonance", 0, 1, 0.2f, 0, nullptr, "" },
        { "mode", "Mode", 0, 2, 0, INT, "Low-pass|High-pass|Band-pass", "" }, { "env", "Env Amount", -1, 1, 0, 0, nullptr, "" },
        { "lfo", "LFO Amount", 0, 1, 0, 0, nullptr, "" }, { "lfoRate", "LFO Rate", 0.05f, 20, 1, LOG, nullptr, "Hz" },
        { "lfoSync", "LFO Sync", 0, 1, 0, TOG, nullptr, "" }, { "lfoDiv", "LFO Division", 0, dMax, d4, INT, dn, "" },
        { "mix", "Mix", 0, 1, 1, 0, nullptr, "" } } });
    t.push_back ({ FxType::delay, "delay", "Delay", 12, {
        { "mix", "Mix", 0, 1, 0.25f, 0, nullptr, "" }, { "time", "Time", 0, dMax, d8d, INT, dn, "" },
        { "feedback", "Feedback", 0, 0.95f, 0.35f, 0, nullptr, "" }, { "mode", "Mode", 0, 2, 0, INT, "Ping-pong|Straight|Tape", "" },
        { "sync", "Sync", 0, 1, 1, TOG, nullptr, "" }, { "ms", "Time (free)", 10, 2000, 375, LOG, nullptr, "ms" },
        { "hiCut", "High Cut", 1000, 20000, 20000, LOG, nullptr, "Hz" }, { "loCut", "Low Cut", 20, 1000, 20, LOG, nullptr, "Hz" },
        { "flutter", "Flutter", 0, 1, 0, 0, nullptr, "" } } });
    t.push_back ({ FxType::reverb, "reverb", "Reverb", 13, {
        { "mix", "Mix", 0, 1, 0.25f, 0, nullptr, "" }, { "size", "Size", 0, 1, 0.6f, 0, nullptr, "" },
        { "type", "Type", 0, 4, 0, INT, "Hall|Plate|Cloud|Room|Shimmer", "" }, { "decay", "Decay", 0.25f, 4, 1, LOG, nullptr, "x" },
        { "predelay", "Pre-delay", 0, 250, 0, 0, nullptr, "ms" }, { "damping", "Damping", -1, 1, 0, 0, nullptr, "" },
        { "lowCut", "Low Cut", 20, 1000, 160, LOG, nullptr, "Hz" }, { "shimmer", "Shimmer", 0, 1, 0.5f, 0, nullptr, "" },
        { "freeze", "Freeze", 0, 1, 0, TOG, nullptr, "" } } });
    t.push_back ({ FxType::width, "width", "Width", 16, {
        { "width", "Width", 0, 1, 0.5f, 0, nullptr, "" }, { "monoBelow", "Mono Below", 0, 400, 0, 0, nullptr, "Hz" } } });
    t.push_back ({ FxType::reverse, "reverse", "Reverse", 14, {
        { "mix", "Mix", 0, 1, 0.3f, 0, nullptr, "" }, { "length", "Length", 0, dMax, d8, INT, dn, "" } } });
    t.push_back ({ FxType::transient, "transient", "Transient", 0, {
        { "attack", "Attack", 0, 1, 0.3f, 0, nullptr, "" } } });
    t.push_back ({ FxType::body, "body", "Body", 3, {
        { "model", "Model", 1, 6, 1, INT, "Kalimba|Bell|Glass|Metal Pipe|Wood Box|String", "" }, { "mix", "Mix", 0, 1, 0.5f, 0, nullptr, "" } } });
    t.push_back ({ FxType::ghost, "ghost", "Ghost", 15, {
        { "mix", "Mix", 0, 1, 0.3f, 0, nullptr, "" }, { "blur", "Blur", 0, 1, 0.6f, 0, nullptr, "" },
        { "reverse", "Reverse", 0, 1, 1, TOG, nullptr, "" } } });
    t.push_back ({ FxType::stutter, "stutter", "Stutter", 5, {
        { "amount", "Amount", 0, 1, 0.3f, 0, nullptr, "" }, { "rate", "Rate", 0, dMax, (float) divisionIndex ("1/16"), INT, dn, "" },
        { "seed", "Seed", 0, 9999, 1234, INT, nullptr, "" } } });
    t.push_back ({ FxType::halftime, "halftime", "Half-Time", 6, {
        { "mix", "Mix", 0, 1, 0.5f, 0, nullptr, "" } } });
    for (auto& ti : t) jassert ((int) ti.params.size() <= maxFxParams);
    return t;
}
} // namespace

const FxTypeInfo& fxTypeInfo (FxType type)
{
    static const auto t = makeFxTypes();
    return t[(size_t) juce::jlimit (0, numFxTypes - 1, (int) type)];
}
int fxTypeOf (const juce::String& id)
{
    for (int i = 0; i < numFxTypes; ++i) if (id == fxTypeInfo ((FxType) i).id) return i;
    return -1;
}
int fxMainParam (FxType t)
{
    switch (t)
    {
        case FxType::reverb: case FxType::delay: case FxType::chorus: return 0;
        case FxType::drive: return 0;
        case FxType::width: return 0;
        case FxType::eq: return 0;
        default: return -1;
    }
}

const std::array<QuickInfo, numQuick>& quickInfo()
{
    static const std::array<QuickInfo, numQuick> q { {
        { "cutoff", "Cutoff", 0.5f, "filter cutoff of every layer, +-4 octaves" },
        { "reso", "Resonance", 0.5f, "filter resonance, +-1" },
        { "attack", "Attack", 0.5f, "amp attack x1/16 .. x16 (+ up to 2 s)" },
        { "release", "Release", 0.5f, "amp release x1/16 .. x16 (+ up to 4 s)" },
        { "envAmt", "Env Amount", 0.5f, "filter envelope amount, +-1" },
        { "glide", "Glide", 0.5f, "portamento of the synth layers, +-1" },
        { "vibrato", "Vibrato", 0.0f, "built-in vibrato 5.5 Hz, up to +-60 cents" },
        { "tremolo", "Tremolo", 0.0f, "built-in tremolo 4.5 Hz, depth" },
        { "reverb", "Reverb", 0.5f, "the chain's reverb mix (a default hall when the preset has none)" },
        { "delay", "Delay", 0.5f, "the chain's delay mix" },
        { "chorus", "Chorus", 0.5f, "the chain's chorus mix" },
        { "drive", "Drive", 0.5f, "the chain's drive amount" },
        { "width", "Width", 0.5f, "the chain's stereo width" },
        { "eqTilt", "EQ Tilt", 0.5f, "dark .. bright, +-9 dB tilt on the chain's EQ" },
        { "pan", "Pan", 0.5f, "output balance" } } };
    return q;
}

bool TargetRef::absolute() const
{
    switch (kind)
    {
        case fxType: case fxSlot: case lfo: case gate: case arpGate: case finish: case classic: return true;
        case layer: return b == lLevel || b == lPan;
        default: return false;
    }
}

TargetRef parseTarget (const juce::String& idIn)
{
    TargetRef r;
    const auto id = idIn.trim();
    auto simple = [&] (const char* s, TargetRef::Kind k) { if (id == s) { r.kind = k; return true; } return false; };
    if (simple ("pitch", TargetRef::pitch) || simple ("cutoff", TargetRef::cutoff) || simple ("reso", TargetRef::reso)
        || simple ("volume", TargetRef::volume) || simple ("pan", TargetRef::pan) || simple ("finish", TargetRef::finish)
        || simple ("arp.gate", TargetRef::arpGate))
        return r;
    const auto head = id.upToFirstOccurrenceOf (".", false, false), tail = id.fromFirstOccurrenceOf (".", false, false);
    if (head == "quick")
    {
        for (int i = 0; i < numQuick; ++i) if (tail == quickInfo()[(size_t) i].id) { r.kind = TargetRef::quick; r.a = (uint8_t) i; }
        return r;
    }
    if (head == "classic")
    {
        const int i = juce::StringArray { "tone", "space", "drive", "motion" }.indexOf (tail);
        if (i >= 0) { r.kind = TargetRef::classic; r.a = (uint8_t) i; }
        return r;
    }
    if (head == "gate")
    {
        const int i = juce::StringArray { "depth", "mix", "smooth" }.indexOf (tail);
        if (i >= 0) { r.kind = TargetRef::gate; r.a = (uint8_t) i; }
        return r;
    }
    if (head.startsWith ("layer") && head.length() == 6)
    {
        const int n = head.getLastCharacter() - '1';
        const int i = juce::StringArray { "level", "pan", "cutoff", "reso", "attack", "release", "tune" }.indexOf (tail);
        if (juce::isPositiveAndBelow (n, (int) maxParts) && i >= 0) { r.kind = TargetRef::layer; r.a = (uint8_t) n; r.b = (uint8_t) i; }
        return r;
    }
    if (head.startsWith ("lfo") && head.length() == 4)
    {
        const int n = head.getLastCharacter() - '1';
        const int i = juce::StringArray { "depth", "rate" }.indexOf (tail);
        if (juce::isPositiveAndBelow (n, (int) numLfos) && i >= 0) { r.kind = TargetRef::lfo; r.a = (uint8_t) n; r.b = (uint8_t) i; }
        return r;
    }
    if (head == "fx")   // fx.<type>.<param>
    {
        const int t = fxTypeOf (tail.upToFirstOccurrenceOf (".", false, false));
        if (t < 0) return r;
        const int pi = fxTypeInfo ((FxType) t).indexOf (tail.fromFirstOccurrenceOf (".", false, false).toRawUTF8());
        if (pi >= 0) { r.kind = TargetRef::fxType; r.a = (uint8_t) t; r.b = (uint8_t) pi; }
        return r;
    }
    if (head.startsWith ("fx") && head.length() == 3)   // fx<1-8>.<param>: the parameter name is resolved against the slot's type
    {
        const int n = head.getLastCharacter() - '1';
        if (juce::isPositiveAndBelow (n, (int) maxFxSlots) && tail.isNotEmpty() && tail.containsOnly ("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"))
        { r.kind = TargetRef::fxSlot; r.a = (uint8_t) n; r.b = 255; }
        return r;
    }
    return r;
}

juce::String describeTarget (const juce::String& id)
{
    const auto t = parseTarget (id);
    const auto tail = id.fromFirstOccurrenceOf (".", false, false);
    switch (t.kind)
    {
        case TargetRef::pitch:  return "Pitch";
        case TargetRef::cutoff: return "Cutoff";
        case TargetRef::reso:   return "Resonance";
        case TargetRef::volume: return "Volume";
        case TargetRef::pan:    return "Pan";
        case TargetRef::finish: return "Finish";
        case TargetRef::arpGate: return "Arp Gate";
        case TargetRef::quick:  return quickInfo()[t.a].name;
        case TargetRef::classic: return classicMacroNames()[t.a];
        case TargetRef::gate:   return juce::String ("Gate ") + juce::StringArray { "Depth", "Mix", "Smooth" }[t.a];
        case TargetRef::layer:  return "Layer " + juce::String (t.a + 1) + " " + juce::StringArray { "Level", "Pan", "Cutoff", "Resonance", "Attack", "Release", "Tune" }[t.b];
        case TargetRef::lfo:    return "LFO " + juce::String (t.a + 1) + (t.b == 0 ? " Depth" : " Rate");
        case TargetRef::fxType: { const auto& ti = fxTypeInfo ((FxType) t.a); return juce::String (ti.name) + " " + ti.params[t.b].name; }
        case TargetRef::fxSlot: return "FX " + juce::String (t.a + 1) + " " + tail.substring (0, 1).toUpperCase() + tail.substring (1);
        case TargetRef::none: default: return {};
    }
}
} // namespace vault

VaultFxSlot VaultFxSlot::make (vault::FxType t)
{
    VaultFxSlot s;
    s.type = t;
    const auto& ti = vault::fxTypeInfo (t);
    s.p.fill (0.0f);
    for (size_t i = 0; i < ti.params.size(); ++i) s.p[i] = ti.params[i].def;
    return s;
}
float VaultFxSlot::get (const char* id) const
{
    const auto& ti = vault::fxTypeInfo (type);
    const int i = ti.indexOf (id);
    return i >= 0 ? p[(size_t) i] : 0.0f;
}
VaultFxSlot& VaultFxSlot::set (const char* id, float v)
{
    const auto& ti = vault::fxTypeInfo (type);
    const int i = ti.indexOf (id);
    jassert (i >= 0);
    if (i >= 0) p[(size_t) i] = v;
    return *this;
}
bool VaultFxSlot::operator== (const VaultFxSlot& o) const
{
    return type == o.type && on == o.on && std::equal (p.begin(), p.end(), o.p.begin(), [] (float a, float b) { return juce::exactlyEqual (a, b); });
}

VaultLfo::VaultLfo() { div = (uint8_t) vault::divisionIndex ("1/4"); }
bool VaultLfo::operator== (const VaultLfo& o) const
{
    return shape == o.shape && juce::exactlyEqual (rate, o.rate) && sync == o.sync && div == o.div && juce::exactlyEqual (delay, o.delay)
        && juce::exactlyEqual (fade, o.fade) && juce::exactlyEqual (phase, o.phase) && retrig == o.retrig && juce::exactlyEqual (depth, o.depth) && dests == o.dests;
}
bool VaultMod::operator== (const VaultMod& o) const
{
    return lfo[0] == o.lfo[0] && lfo[1] == o.lfo[1] && modWheel == o.modWheel && velocity == o.velocity && aftertouch == o.aftertouch;
}
VaultArp::VaultArp()
{
    rate = (uint8_t) vault::divisionIndex ("1/16");
    vel.fill (1.0f); type.fill (play); oct.fill (0);
}
bool VaultArp::operator== (const VaultArp& o) const
{
    return on == o.on && mode == o.mode && rate == o.rate && octaves == o.octaves && juce::exactlyEqual (gate, o.gate) && juce::exactlyEqual (swing, o.swing)
        && latch == o.latch && steps == o.steps && vel == o.vel && type == o.type && oct == o.oct;
}
VaultGate::VaultGate()
{
    rate = (uint8_t) vault::divisionIndex ("1/16");
    for (int i = 0; i < vault::numSteps; ++i) level[(size_t) i] = (i % 2) == 0 ? 1.0f : 0.0f;
}
bool VaultGate::operator== (const VaultGate& o) const
{
    return on == o.on && rate == o.rate && steps == o.steps && level == o.level && juce::exactlyEqual (depth, o.depth)
        && juce::exactlyEqual (smooth, o.smooth) && juce::exactlyEqual (swing, o.swing) && juce::exactlyEqual (mix, o.mix);
}
