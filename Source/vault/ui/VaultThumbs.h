#pragma once
#include "../VaultEngine.h"
#include <juce_events/juce_events.h>
#include <deque>
#include <map>

// Waveform / level thumbnails of presets, rendered lazily on a low-priority background thread with VaultEngine::render
// (one short note, preset defaults) and cached for the editor's lifetime. The UI never waits: get() returns false until
// the thumbnail is ready, then `onReady` is called on the message thread.
// getLive(): the same with the current macro settings (one "live" slot, the newest request wins - knob drags never queue up).
class VaultThumbs : private juce::Thread, private juce::AsyncUpdater
{
public:
    static constexpr int numBins = 56;
    struct Thumb { std::array<float, numBins> level {}; float peakDb = -100.0f; };   // level 0..1 per bin (envelope, 30 dB range under the peak)

    explicit VaultThumbs (const std::vector<VaultPreset>& b) : juce::Thread ("CHROME VAULT thumbs"), bank (b)
    {
        startThread (juce::Thread::Priority::low);
    }
    ~VaultThumbs() override
    {
        signalThreadShouldExit();
        wake.signal();
        stopThread (4000);
        cancelPendingUpdate();
    }

    std::function<void (int)> onReady;   // message thread: a thumbnail became available

    // message thread: the thumbnail if ready; otherwise it is queued (first in line) and false is returned
    bool get (int index, Thumb& out)
    {
        if (! juce::isPositiveAndBelow (index, (int) bank.size())) return false;
        {
            const juce::ScopedLock sl (lock);
            if (auto it = done.find (index); it != done.end()) { out = it->second; return true; }
            queue.erase (std::remove (queue.begin(), queue.end(), index), queue.end());
            queue.push_front (index);
        }
        wake.signal();
        return false;
    }
    // message thread: render soon (behind the explicitly requested ones), e.g. the neighbours in the list
    void prefetch (int index)
    {
        if (! juce::isPositiveAndBelow (index, (int) bank.size())) return;
        {
            const juce::ScopedLock sl (lock);
            if (done.count (index) != 0 || std::find (queue.begin(), queue.end(), index) != queue.end()) return;
            queue.push_back (index);
            while (queue.size() > 24) queue.pop_back();
        }
        wake.signal();
    }
    bool isReady (int index) const { const juce::ScopedLock sl (lock); return done.count (index) != 0; }

    // message thread: the thumbnail of `index` played with `macros` (4 values 0..1). At the preset's own macro settings this is
    // get(). Otherwise the live render is requested; until it is ready the last live picture of this preset (or the
    // default one) is returned, so the display never blinks while a knob turns.
    bool getLive (int index, const float* macros, Thumb& out)
    {
        if (! juce::isPositiveAndBelow (index, (int) bank.size()) || macros == nullptr) return false;
        std::array<float, vault::numMacros> m {};
        bool atDefaults = true;
        for (int i = 0; i < vault::numMacros; ++i)
        {
            m[(size_t) i] = std::round (juce::jlimit (0.0f, 1.0f, macros[i]) * 100.0f) / 100.0f;
            atDefaults &= std::abs (m[(size_t) i] - bank[(size_t) index].macroDefaults[(size_t) i]) < 0.006f;
        }
        if (atDefaults) return get (index, out);
        {
            const juce::ScopedLock sl (lock);
            if (live.ready && live.index == index && live.macros == m) { out = live.thumb; return true; }
            if (! (liveReq.index == index && liveReq.macros == m)) { liveReq.index = index; liveReq.macros = m; liveWanted = true; }
        }
        wake.signal();
        {
            const juce::ScopedLock sl (lock);
            if (live.ready && live.index == index) { out = live.thumb; return true; }
        }
        return get (index, out);
    }
    // tests: block until the live picture for exactly these macros is ready
    bool waitForLive (int index, const float* macros, int timeoutMs)
    {
        Thumb t;
        const auto end = juce::Time::getMillisecondCounter() + (juce::uint32) timeoutMs;
        while (juce::Time::getMillisecondCounter() < end)
        {
            getLive (index, macros, t);
            {
                const juce::ScopedLock sl (lock);
                if (! liveWanted && live.ready && live.index == index && live.macros == liveReq.macros) return true;
            }
            juce::Thread::sleep (5);
        }
        return false;
    }
    // tests / snapshots: block until `index` is ready (or the timeout)
    bool waitFor (int index, int timeoutMs)
    {
        Thumb t;
        get (index, t);
        const auto end = juce::Time::getMillisecondCounter() + (juce::uint32) timeoutMs;
        while (! isReady (index) && juce::Time::getMillisecondCounter() < end) juce::Thread::sleep (5);
        return isReady (index);
    }

    static Thumb analyse (const VaultPreset& p, const float* macros = nullptr)
    {
        constexpr double sr = 22050.0, secs = 2.0;
        const int note = p.category == vault::cat808Bass ? 48 : 60;
        const auto buf = macros != nullptr ? VaultEngine::render (p, note, 0.8f, secs, sr, macros) : VaultEngine::render (p, note, 0.8f, secs, sr);
        Thumb t;
        const int n = buf.getNumSamples(), per = std::max (1, n / numBins);
        float pk = 0.0f;
        std::array<float, numBins> mag {};
        for (int b = 0; b < numBins; ++b)
        {
            for (int c = 0; c < buf.getNumChannels(); ++c)
                mag[(size_t) b] = std::max (mag[(size_t) b], buf.getMagnitude (c, b * per, std::min (per, n - b * per)));
            pk = std::max (pk, mag[(size_t) b]);
        }
        t.peakDb = juce::Decibels::gainToDecibels (pk, -100.0f);
        for (int b = 0; b < numBins; ++b)
        {
            const float db = juce::Decibels::gainToDecibels (mag[(size_t) b], -100.0f) - t.peakDb;
            t.level[(size_t) b] = juce::jlimit (0.0f, 1.0f, (db + 30.0f) / 30.0f);
        }
        return t;
    }

private:
    void run() override
    {
        while (! threadShouldExit())
        {
            int idx = -1;
            LiveSlot req;
            bool doLive = false;
            {
                const juce::ScopedLock sl (lock);
                if (liveWanted) { req = liveReq; liveWanted = false; doLive = true; }
                else if (! queue.empty()) { idx = queue.front(); queue.pop_front(); }
            }
            if (doLive)
            {
                req.thumb = analyse (bank[(size_t) req.index], req.macros.data());
                req.ready = true;
                {
                    const juce::ScopedLock sl (lock);
                    live = req;
                    readyList.push_back (req.index);
                }
                triggerAsyncUpdate();
                continue;
            }
            if (idx < 0) { wake.wait (1000); continue; }
            const auto t = analyse (bank[(size_t) idx]);
            {
                const juce::ScopedLock sl (lock);
                done[idx] = t;
                readyList.push_back (idx);
            }
            triggerAsyncUpdate();
        }
    }
    void handleAsyncUpdate() override
    {
        std::vector<int> ready;
        { const juce::ScopedLock sl (lock); ready.swap (readyList); }
        if (onReady) for (int i : ready) onReady (i);
    }

    struct LiveSlot { int index = -1; std::array<float, vault::numMacros> macros {}; Thumb thumb; bool ready = false; };

    const std::vector<VaultPreset>& bank;
    LiveSlot live, liveReq;
    bool liveWanted = false;
    juce::CriticalSection lock;
    std::map<int, Thumb> done;
    std::deque<int> queue;
    std::vector<int> readyList;
    juce::WaitableEvent wake;
};
