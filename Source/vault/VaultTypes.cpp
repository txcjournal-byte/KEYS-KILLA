#include "VaultTypes.h"
#include "../ParamKeys.h"
#include <map>

namespace vault
{
namespace
{
constexpr char magic[4] { 'C', 'V', 'B', '1' };
constexpr int formatVersion = 1;
constexpr int flagGzip = 1;
constexpr int maxKeys = 255;
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
    // key table: the canonical synth keys first (stable indices), then any other ID the bank uses
    juce::StringArray keys;
    for (int k = 0; k < kk::vk::count; ++k) keys.add (kk::vk::info (k).id);
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
        payload.writeInt ((int) p.styles);
        payload.writeInt ((int) p.chars);
        payload.writeFloat (p.gain);
        for (float m : p.macroDefaults) payload.writeFloat (m);
        const int np = juce::jmin ((int) p.parts.size(), 255);
        payload.writeByte ((char) np);
        for (int pi = 0; pi < np; ++pi)
        {
            const auto& part = p.parts[(size_t) pi];
            payload.writeString (part.label);
            payload.writeFloat (part.level);
            payload.writeFloat (part.pan);
            payload.writeByte ((char) juce::jlimit (-128, 127, part.octave));
            payload.writeByte ((char) juce::jlimit (-128, 127, part.semi));
            int nv = 0;
            for (auto& [id, v] : part.values) if (keyIndex.count (id)) ++nv;
            jassert (nv == (int) part.values.size());   // more than 255 distinct parameter IDs in one bank
            payload.writeShort ((short) juce::jmin (nv, 65535));
            for (auto& [id, v] : part.values)
                if (const auto it = keyIndex.find (id); it != keyIndex.end())
                {
                    payload.writeByte ((char) (uint8_t) it->second);
                    payload.writeFloat (v);
                }
        }
    }

    juce::MemoryOutputStream out;
    out.write (magic, 4);
    out.writeShort ((short) formatVersion);
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

bool deserializeBank (const void* data, size_t size, std::vector<VaultPreset>& out, juce::String* error)
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
    if (version != formatVersion) return fail ("unsupported bank version");
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
    if (nKeys > 65535) return fail ("bad key table");
    juce::StringArray keys;
    for (int i = 0; i < nKeys; ++i) { juce::String k; if (! readString (in, k)) return fail ("bad key table"); keys.add (k); }

    out.reserve ((size_t) count);
    for (int i = 0; i < count; ++i)
    {
        if (in.isExhausted()) return fail ("truncated bank");
        VaultPreset p;
        if (! readString (in, p.id) || ! readString (in, p.name)) return fail ("bad preset text");
        p.category = (uint8_t) in.readByte();
        p.styles = (uint32_t) in.readInt();
        p.chars = (uint32_t) in.readInt();
        p.gain = in.readFloat();
        for (float& m : p.macroDefaults) m = in.readFloat();
        const int np = (uint8_t) in.readByte();
        p.parts.resize ((size_t) np);
        for (auto& part : p.parts)
        {
            if (! readString (in, part.label)) return fail ("bad part label");
            part.level = in.readFloat();
            part.pan = in.readFloat();
            part.octave = (int8_t) in.readByte();
            part.semi = (int8_t) in.readByte();
            const int nv = (uint16_t) in.readShort();
            part.values.reserve ((size_t) nv);
            for (int v = 0; v < nv; ++v)
            {
                const int k = (uint8_t) in.readByte();
                const float x = in.readFloat();
                if (k >= keys.size()) return fail ("bad parameter key");
                part.values.push_back ({ keys[k], x });
            }
        }
        out.push_back (std::move (p));
    }
    if (error) error->clear();
    return true;
}

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
        for (auto& [id, v] : part.values)
        {
            if (! std::isfinite (v)) return no ("non-finite value for " + id);
            if (kk::vk::keyOf (id) < 0) return no ("unknown parameter " + id);
        }
    }
    return true;
}
} // namespace vault
