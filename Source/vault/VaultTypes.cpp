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

juce::MemoryBlock serializeBank (const std::vector<VaultPreset>& bank, bool compress)
{
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

bool deserializeBank (const void* data, size_t size, std::vector<VaultPreset>& out, juce::String* error, int* versionOut)
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
    return true;
}
} // namespace vault
