#include "VaultLibrary.h"
#include "LibraryTuning.h"
#include <unordered_map>
#include <juce_audio_formats/juce_audio_formats.h>
#include <algorithm>
#include <thread>

namespace vault
{
//==============================================================================
int InstrumentDesc::articulationIndex (const juce::String& n) const
{
    for (int i = 0; i < (int) articulations.size(); ++i)
        if (articulations[(size_t) i].name.equalsIgnoreCase (n)) return i;
    return 0;
}

const SampleInstrumentData::Zone* SampleInstrumentData::pick (int artIndex, int key, int velocity, unsigned rrCounter, bool rel) const
{
    if (! juce::isPositiveAndBelow (artIndex, (int) articulations.size()) || ! juce::isPositiveAndBelow (key, 128)) return nullptr;
    const auto& a = articulations[(size_t) artIndex];
    const auto& starts = rel ? a.relKeyStart : a.keyStart;
    const auto& list = rel ? a.relKeyZones : a.keyZones;
    const auto& zones = rel ? a.releaseZones : a.zones;
    const uint32_t b = starts[(size_t) key], e = starts[(size_t) key + 1];
    if (b >= e) return nullptr;
    velocity = juce::jlimit (1, 127, velocity);
    // the velocity cell: zones whose range holds the velocity (nearest layer if none does), then round robin inside it
    int bestDist = 1000;
    for (uint32_t i = b; i < e; ++i)
    {
        const auto& z = zones[list[i]];
        const int d = velocity < z.vlo ? z.vlo - velocity : velocity > z.vhi ? velocity - z.vhi : 0;
        bestDist = std::min (bestDist, d);
    }
    int n = 0;
    for (uint32_t i = b; i < e; ++i)
    {
        const auto& z = zones[list[i]];
        const int d = velocity < z.vlo ? z.vlo - velocity : velocity > z.vhi ? velocity - z.vhi : 0;
        if (d == bestDist) ++n;
    }
    int want = (int) (rrCounter % (unsigned) std::max (1, n));
    for (uint32_t i = b; i < e; ++i)
    {
        const auto& z = zones[list[i]];
        const int d = velocity < z.vlo ? z.vlo - velocity : velocity > z.vhi ? velocity - z.vhi : 0;
        if (d == bestDist && want-- == 0) return &z;
    }
    return nullptr;
}

//==============================================================================
namespace
{
std::mutex sharedLock;
std::weak_ptr<SampleLibrary> sharedLib;

juce::File libraryIn (const juce::File& f) { return f.getChildFile ("library.json"); }
}

juce::Array<juce::File> SampleLibrary::searchLocations()
{
    juce::Array<juce::File> out;
    if (const auto env = juce::SystemStats::getEnvironmentVariable ("CHROME_VAULT_LIBRARY", {}); env.isNotEmpty() && juce::File::isAbsolutePath (env))
        out.add (juce::File (env));
    const juce::String sub = "TrapWin/CHROME VAULT/Library";
   #if JUCE_WINDOWS
    const auto pd = juce::SystemStats::getEnvironmentVariable ("ProgramData", "C:\\ProgramData");
    out.add (juce::File (pd).getChildFile (sub));
    out.add (juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory).getChildFile (sub));   // %APPDATA%
   #elif JUCE_MAC
    out.add (juce::File ("/Library/Application Support").getChildFile (sub));
    out.add (juce::File::getSpecialLocation (juce::File::userHomeDirectory).getChildFile ("Library/Application Support").getChildFile (sub));
   #else
    out.add (juce::File ("/usr/share").getChildFile (sub));
    out.add (juce::File::getSpecialLocation (juce::File::userHomeDirectory).getChildFile (".local/share").getChildFile (sub));
   #endif
    return out;
}

std::shared_ptr<SampleLibrary> SampleLibrary::getShared()
{
    const std::lock_guard<std::mutex> g (sharedLock);
    if (auto l = sharedLib.lock()) return l;
    juce::File use;
    const auto locs = searchLocations();
    for (auto& f : locs) if (libraryIn (f).existsAsFile()) { use = f; break; }
    if (use == juce::File() && ! locs.isEmpty()) use = locs.getFirst();
    auto l = std::make_shared<SampleLibrary> (use);
    sharedLib = l;
    return l;
}

SampleLibrary::SampleLibrary (const juce::File& f) : juce::Thread ("CHROME VAULT library"), folder (f)
{
    if (const auto mb = juce::SystemStats::getEnvironmentVariable ("CHROME_VAULT_RAM_MB", {}).getIntValue(); mb > 0)
        budget = (int64_t) mb * 1024 * 1024;
    if (folder != juce::File() && libraryIn (folder).existsAsFile()) found = parseIndex (libraryIn (folder));
    else error = "library.json not found in " + folder.getFullPathName();
    if (found) startThread (juce::Thread::Priority::normal);
}

SampleLibrary::~SampleLibrary()
{
    stopThread (10000);
    for (auto& h : handles) { h->loaded.store (nullptr); h->owned.reset(); }
}

InstrumentHandle* SampleLibrary::find (const juce::String& id) const
{
    const auto it = byId.find (id);
    return it == byId.end() ? nullptr : it->second;
}

namespace
{
// the built-in tuning patch (LibraryTuning.h), indexed by file once
const std::unordered_map<std::string, const ZoneTuneFix*>& tuneFixes()
{
    static const auto m = []
    {
        std::unordered_map<std::string, const ZoneTuneFix*> r;
        int n = 0;
        const auto* f = libraryTuneFixes (n);
        for (int i = 0; i < n; ++i) r[f[i].file] = f + i;
        return r;
    }();
    return m;
}
const std::unordered_map<std::string, const ZoneTunePin*>& tunePins()
{
    static const auto m = []
    {
        std::unordered_map<std::string, const ZoneTunePin*> r;
        int n = 0;
        const auto* f = libraryTunePins (n);
        for (int i = 0; i < n; ++i) r[f[i].file] = f + i;
        return r;
    }();
    return m;
}
const std::unordered_map<std::string, const ZoneRangeFix*>& rangeFixes()
{
    static const auto m = []
    {
        std::unordered_map<std::string, const ZoneRangeFix*> r;
        int n = 0;
        const auto* f = libraryRangeFixes (n);
        for (int i = 0; i < n; ++i) r[f[i].file] = f + i;
        return r;
    }();
    return m;
}
} // namespace

bool SampleLibrary::parseIndex (const juce::File& jsonFile)
{
    const auto root = juce::JSON::parse (jsonFile);
    if (! root.isObject()) { error = "library.json is not valid JSON"; return false; }
    const auto* insts = root["instruments"].getArray();
    if (insts == nullptr) { error = "library.json has no instruments"; return false; }
    for (auto& iv : *insts)
    {
        InstrumentDesc d;
        d.id = iv["id"].toString();
        d.name = iv["name"].toString();
        d.family = iv["family"].toString();
        d.credit = iv["credit"].toString();
        if (d.id.isEmpty() || byId.count (d.id) != 0) continue;
        if (const auto* arts = iv["articulations"].getArray())
            for (auto& av : *arts)
            {
                SampleArticulationDesc a;
                a.name = av["name"].toString();
                a.oneShot = (bool) av.getProperty ("oneShot", false);
                a.release = (float) (double) av.getProperty ("release", 0.3);
                a.releaseGainDb = (float) (double) av.getProperty ("releaseGainDb", 0.0);
                a.releaseDecayDb = (float) (double) av.getProperty ("releaseDecayDb", 6.0);
                const bool retuned = (bool) av.getProperty ("retuned", false);   // the library tuning pass ran on this one
                if (const auto* zs = av["zones"].getArray())
                    for (auto& zv : *zs)
                    {
                        SampleZone z;
                        z.file = zv["file"].toString();
                        z.root = (int) zv.getProperty ("root", 60);
                        z.fine = (float) (double) zv.getProperty ("fine", 0.0);
                        z.lo = juce::jlimit (0, 127, (int) zv.getProperty ("lo", 0));
                        z.hi = juce::jlimit (0, 127, (int) zv.getProperty ("hi", 127));
                        z.vlo = juce::jlimit (1, 127, (int) zv.getProperty ("vlo", 1));
                        z.vhi = juce::jlimit (1, 127, (int) zv.getProperty ("vhi", 127));
                        z.rr = std::max (0, (int) zv.getProperty ("rr", 0));
                        z.gainDb = (float) (double) zv.getProperty ("gain", 0.0);
                        z.start = std::max (0, (int) zv.getProperty ("start", 0));
                        z.loopStart = std::max (0, (int) zv.getProperty ("loopStart", 0));
                        z.loopEnd = std::max (0, (int) zv.getProperty ("loopEnd", 0));
                        z.xfade = std::max (0, (int) zv.getProperty ("xfade", 0));
                        z.frames = (int) zv.getProperty ("frames", 0);
                        z.channels = (int) zv.getProperty ("channels", 0);
                        z.releaseTrigger = (bool) zv.getProperty ("release", false);
                        if (! retuned && ! z.releaseTrigger)
                            if (const auto it = tuneFixes().find (z.file.toStdString()); it != tuneFixes().end()
                                && it->second->oldRoot == z.root && std::abs (it->second->oldFine - z.fine) < 0.06f)
                            { z.root = it->second->root; z.fine = it->second->fine; }
                        if (! z.releaseTrigger)
                            if (const auto it = tunePins().find (z.file.toStdString()); it != tunePins().end())
                            { z.root = it->second->root; z.fine = it->second->fine; }
                        if (const auto it = rangeFixes().find (z.file.toStdString()); it != rangeFixes().end() && ! z.releaseTrigger)
                        {
                            const auto& f = *it->second;
                            if (z.lo == f.oldLo && z.hi == f.oldHi && z.vlo == f.oldVlo && z.vhi == f.oldVhi)
                            {
                                if (f.lo < 0) continue;   // taken out
                                z.lo = f.lo; z.hi = f.hi; z.vlo = f.vlo; z.vhi = f.vhi;
                            }
                        }
                        if (z.file.isNotEmpty() && z.lo <= z.hi && z.vlo <= z.vhi) a.zones.push_back (z);
                    }
                if (! a.zones.empty()) d.articulations.push_back (std::move (a));
            }
        if (d.articulations.empty()) continue;
        auto h = std::make_unique<InstrumentHandle> (std::move (d));
        byId[h->desc.id] = h.get();
        handles.push_back (std::move (h));
    }
    if (handles.empty()) { error = "library.json lists no usable instrument"; return false; }
    return true;
}

void SampleLibrary::setMemoryBudget (int64_t bytes) { budget = std::max ((int64_t) 0, bytes); notify(); }

void SampleLibrary::pin (InstrumentHandle* h)
{
    if (h == nullptr) return;
    h->pins.fetch_add (1);
    h->lastUse.store (++InstrumentHandle::useClock());
    if (! h->isLoaded()) { h->request(); notify(); }
}

void SampleLibrary::unpin (InstrumentHandle* h)
{
    if (h == nullptr) return;
    if (h->pins.fetch_sub (1) <= 0) h->pins.store (0);
    notify();
}

bool SampleLibrary::loadNow (InstrumentHandle* h)
{
    if (h == nullptr) return false;
    if (h->isLoaded()) return true;
    const std::lock_guard<std::mutex> g (loadLock);
    if (h->isLoaded()) return true;
    const bool ok = loadInstrument (*h);
    evictIfNeeded();
    return ok;
}

bool SampleLibrary::waitUntilIdle (int timeoutMs)
{
    const auto end = juce::Time::getMillisecondCounter() + (juce::uint32) timeoutMs;
    while (juce::Time::getMillisecondCounter() < end)
    {
        bool pending = busy.load();
        for (auto& h : handles) pending |= h->isLoading();
        if (! pending) return true;
        juce::Thread::sleep (10);
    }
    return false;
}

void SampleLibrary::run()
{
    while (! threadShouldExit())
    {
        InstrumentHandle* next = nullptr;
        uint32_t best = 0;
        for (auto& h : handles)   // the most recently wanted instrument first
            if (! h->isLoaded() && ! h->hasFailed() && (h->wanted.load() || h->pins.load() > 0))
                if (next == nullptr || h->lastUse.load() >= best) { next = h.get(); best = h->lastUse.load(); }
        if (next != nullptr)
        {
            busy = true;
            {
                const std::lock_guard<std::mutex> g (loadLock);
                if (! next->isLoaded()) loadInstrument (*next);
                evictIfNeeded();
            }
            busy = false;
            continue;
        }
        {
            const std::lock_guard<std::mutex> g (loadLock);
            evictIfNeeded();
        }
        wait (20);   // the audio thread only sets flags (no signalling from there): poll
    }
}

bool SampleLibrary::loadInstrument (InstrumentHandle& h)
{
    const auto& d = h.desc;
    auto data = std::make_unique<SampleInstrumentData>();
    data->id = d.id;
    const auto audioDir = folder.getChildFile ("audio");

    // unique files, decoded in parallel (each a separate reader)
    std::vector<juce::String> files;
    std::map<juce::String, int> fileIndex;
    for (auto& a : d.articulations)
        for (auto& z : a.zones)
            if (fileIndex.count (z.file) == 0) { fileIndex[z.file] = (int) files.size(); files.push_back (z.file); }
    data->audio.resize (files.size());
    std::atomic<int> nextFile { 0 };
    std::atomic<bool> bad { false };
    auto worker = [&]
    {
        juce::AudioFormatManager fm;
        fm.registerBasicFormats();
        for (;;)
        {
            const int i = nextFile.fetch_add (1);
            if (i >= (int) files.size() || bad.load() || threadShouldExit()) return;
            const auto f = audioDir.getChildFile (files[(size_t) i]);
            std::unique_ptr<juce::AudioFormatReader> r (fm.createReaderFor (f));
            if (r == nullptr || r->lengthInSamples <= 0 || r->lengthInSamples > (int64_t) 44100 * 600) { bad = true; return; }
            auto a = std::make_unique<SampleAudio>();
            a->channels = juce::jlimit (1, 2, (int) r->numChannels);
            a->frames = (int) r->lengthInSamples;
            a->sampleRate = r->sampleRate > 0 ? r->sampleRate : 44100.0;
            juce::AudioBuffer<float> buf (a->channels, a->frames);
            if (! r->read (&buf, 0, a->frames, 0, true, a->channels > 1)) { bad = true; return; }
            const size_t stride = (size_t) (a->frames + 2 * SampleAudio::pad);
            a->data.assign (stride * (size_t) a->channels, 0);
            for (int c = 0; c < a->channels; ++c)
            {
                const float* src = buf.getReadPointer (c);
                int16_t* dst = a->data.data() + (size_t) c * stride + SampleAudio::pad;
                for (int i2 = 0; i2 < a->frames; ++i2)
                    dst[i2] = (int16_t) juce::jlimit (-32767, 32767, (int) std::lrint (src[i2] * 32767.0f));
            }
            data->audio[(size_t) i] = std::move (a);
        }
    };
    {
        const int nThreads = juce::jlimit (1, 4, (int) std::thread::hardware_concurrency() - 1);
        std::vector<std::thread> pool;
        for (int t = 1; t < nThreads; ++t) pool.emplace_back (worker);
        worker();
        for (auto& t : pool) t.join();
    }
    if (bad.load() || threadShouldExit())
    {
        if (! threadShouldExit()) { h.failed = true; h.wanted = false; }
        return false;
    }

    for (auto& ad : d.articulations)
    {
        SampleInstrumentData::Articulation a;
        a.oneShot = ad.oneShot;
        a.release = juce::jlimit (0.005f, 20.0f, ad.release);
        a.releaseGain = juce::Decibels::decibelsToGain (ad.releaseGainDb);
        a.releaseDecayDb = std::max (0.0f, ad.releaseDecayDb);
        std::vector<std::pair<int, int>> keyed[2];   // (zone index, key) for attack / release zones
        for (auto& zd : ad.zones)
        {
            const auto* au = data->audio[(size_t) fileIndex[zd.file]].get();
            SampleInstrumentData::Zone z;
            z.audio = au;
            z.root = zd.root; z.fine = zd.fine; z.gain = juce::Decibels::decibelsToGain (zd.gainDb);
            z.vlo = zd.vlo; z.vhi = zd.vhi; z.rr = zd.rr;
            z.start = juce::jlimit (0, std::max (0, au->frames - 1), zd.start);
            z.looped = zd.looped() && zd.loopEnd <= au->frames;
            if (z.looped)
            {
                z.loopStart = zd.loopStart; z.loopEnd = zd.loopEnd;
                // the crossfade reads `xfade` frames before loopStart: keep it inside the sample
                z.xfade = juce::jlimit (0, std::min (z.loopStart, (z.loopEnd - z.loopStart) / 2), zd.xfade);
            }
            auto& list = zd.releaseTrigger ? a.releaseZones : a.zones;
            list.push_back (z);
            for (int k = zd.lo; k <= zd.hi; ++k) keyed[zd.releaseTrigger ? 1 : 0].push_back ({ (int) list.size() - 1, k });
        }
        for (int t = 0; t < 2; ++t)
        {
            const auto& zones = t == 0 ? a.zones : a.releaseZones;
            auto& starts = t == 0 ? a.keyStart : a.relKeyStart;
            auto& out = t == 0 ? a.keyZones : a.relKeyZones;
            auto& kz = keyed[t];
            std::stable_sort (kz.begin(), kz.end(), [&] (auto& x, auto& y)
            {
                if (x.second != y.second) return x.second < y.second;
                const auto& zx = zones[(size_t) x.first]; const auto& zy = zones[(size_t) y.first];
                return zx.vlo != zy.vlo ? zx.vlo < zy.vlo : zx.rr < zy.rr;
            });
            size_t j = 0;
            for (int k = 0; k <= 128; ++k)
            {
                starts[(size_t) k] = (uint32_t) out.size();
                while (k < 128 && j < kz.size() && kz[j].second == k) out.push_back ((uint16_t) kz[j++].first);
            }
        }
        data->articulations.push_back (std::move (a));
    }
    for (auto& a : data->audio) data->bytes += a->bytes();

    used += (int64_t) data->bytes;
    h.owned = std::move (data);
    h.loaded.store (h.owned.get(), std::memory_order_seq_cst);
    h.wanted = false;
    return true;
}

void SampleLibrary::evictIfNeeded()
{
    // least recently used first; never one in use by an engine or pinned by a plugin instance
    while (used.load() > budget.load())
    {
        InstrumentHandle* victim = nullptr;
        for (auto& h : handles)
            if (h->owned != nullptr && h->users.load() == 0 && h->pins.load() == 0 && ! h->wanted.load())
                if (victim == nullptr || h->lastUse.load() < victim->lastUse.load()) victim = h.get();
        if (victim == nullptr) return;
        // unpublish first, then check that no engine took it meanwhile (an engine acquires before it reads the pointer)
        victim->loaded.store (nullptr, std::memory_order_seq_cst);
        if (victim->users.load (std::memory_order_seq_cst) != 0)
        {
            victim->loaded.store (victim->owned.get(), std::memory_order_seq_cst);
            return;
        }
        used -= (int64_t) victim->owned->bytes;
        victim->owned.reset();
    }
}
} // namespace vault
