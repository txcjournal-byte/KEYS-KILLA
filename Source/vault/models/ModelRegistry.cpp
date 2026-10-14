#include "BuiltinModels.h"
#include <cmath>

namespace vault::models
{
namespace
{
// "sine": the trivial example model (tests / template). A sine with a soft attack and an exponential decay.
// m.decay = time to fall 60 dB while held (s), m.release = time to fall 60 dB after note-off (s), m.level (linear).
class SineVoice final : public ModelVoice
{
public:
    void prepare (double sampleRate, int) override { sr = sampleRate > 0 ? sampleRate : 44100.0; kill(); }
    void setParams (const float* v, int n) override
    {
        decay = n > 0 ? v[0] : 4.0f;
        release = n > 1 ? v[1] : 0.3f;
        level = n > 2 ? v[2] : 0.25f;
    }
    void noteOn (int, float velocity, float freqHz) override
    {
        amp = 0.0f; target = level * (0.3f + 0.7f * velocity); phase = 0.0; setPitch (freqHz);
        held = true; active = true;
        atkStep = target / (float) (0.004 * sr);
        mulHeld = (float) std::pow (0.001, 1.0 / (std::max (0.05f, decay) * sr));
        mulRel = (float) std::pow (0.001, 1.0 / (std::max (0.01f, release) * sr));
        attacking = true;
    }
    void noteOff (float) override { held = false; attacking = false; }
    void setPitch (float freqHz) override { inc = 2.0 * 3.14159265358979323846 * std::max (1.0f, freqHz) / sr; }
    void render (float* l, float* r, int n) override
    {
        if (! active) return;
        for (int i = 0; i < n; ++i)
        {
            if (attacking) { amp += atkStep; if (amp >= target) { amp = target; attacking = false; } }
            else { amp *= held ? mulHeld : mulRel; target = amp; }
            const float s = (float) std::sin (phase) * amp;
            phase += inc; if (phase > 6.283185307179586) phase -= 6.283185307179586;
            l[i] += s; r[i] += s;
        }
        if (! attacking && amp < 1.0e-5f) kill();
    }
    void kill() override { active = false; amp = 0.0f; attacking = false; held = false; }
    bool isActive() const override { return active; }

private:
    double sr = 44100.0, phase = 0.0, inc = 0.0;
    float decay = 4.0f, release = 0.3f, level = 0.25f;
    float amp = 0.0f, target = 0.0f, atkStep = 0.0f, mulHeld = 1.0f, mulRel = 1.0f;
    bool held = false, active = false, attacking = false;
};
} // namespace

void registerBuiltinModels (ModelRegistry& r)
{
    r.add ({ "sine", "Sine",
             { { "m.decay", 0.05f, 30.0f, 4.0f, "time to fall 60 dB while held, s" },
               { "m.release", 0.01f, 10.0f, 0.3f, "time to fall 60 dB after note-off, s" },
               { "m.level", 0.0f, 1.0f, 0.25f, "peak level, linear" } },
             16, [] { return std::make_unique<SineVoice>(); } });
    // physical models (docs/VAULT_MODELS.md). Guitars: waveguide strings + commuted bodies / pickups + amp
    r.add (stringModelInfo ("guitar.acoustic"));
    r.add (stringModelInfo ("guitar.nylon"));
    r.add (stringModelInfo ("guitar.banjo"));
    r.add (stringModelInfo ("guitar.twelve"));
    r.add (stringModelInfo ("guitar.electric"));
    r.add (stringModelInfo ("guitar.crunch"));
    r.add (stringModelInfo ("guitar.jazz"));
    // electric basses
    r.add (stringModelInfo ("bass.finger"));
    r.add (stringModelInfo ("bass.pick"));
    r.add (stringModelInfo ("bass.slap"));
    r.add (stringModelInfo ("bass.fretless"));
    // electric pianos / keys
    r.add (keysModelInfo ("ep.tine.mellow"));
    r.add (keysModelInfo ("ep.tine.bell"));
    r.add (keysModelInfo ("ep.reed"));
    r.add (stringModelInfo ("ep.clav"));
    r.add (keysModelInfo ("keys.toy"));
    // voices
    r.add (voiceModelInfo ("voice.female"));
    r.add (voiceModelInfo ("voice.male"));
    r.add (voiceModelInfo ("voice.choir"));
    r.add (voiceModelInfo ("voice.pad"));
    // further models: r.add ({ "id", "Name", { params... }, voices, [] { return std::make_unique<MyVoice>(); } });
}

ModelRegistry::ModelRegistry() { registerBuiltinModels (*this); }

ModelRegistry& ModelRegistry::get()
{
    static ModelRegistry r;
    return r;
}

void ModelRegistry::add (ModelInfo info)
{
    jassert (info.params.size() <= (size_t) maxModelParams && info.create != nullptr && indexOf (info.id) < 0);
    if (info.params.size() > (size_t) maxModelParams) info.params.resize ((size_t) maxModelParams);
    info.maxVoices = juce::jlimit (1, 64, info.maxVoices);
    if (indexOf (info.id) < 0 && info.create != nullptr) models.push_back (std::move (info));
}

int ModelRegistry::indexOf (const juce::String& id) const
{
    for (int i = 0; i < (int) models.size(); ++i) if (models[(size_t) i].id == id) return i;
    return -1;
}

const ModelInfo* ModelRegistry::find (const juce::String& id) const
{
    const int i = indexOf (id);
    return i >= 0 ? &models[(size_t) i] : nullptr;
}

juce::StringArray ModelRegistry::ids() const
{
    juce::StringArray s;
    for (auto& m : models) s.add (m.id);
    return s;
}
} // namespace vault::models
