// VaultModelLab: renders, benchmarks and measures the CHROME VAULT physical models (Source/vault/models).
//
//   VaultModelLab -demos <dir>             2 s demos at C5 (MIDI 60, velocity 0.8) of every model variant through the
//                                          engine (neutral FX), <dir>/<variant>.wav
//   VaultModelLab -measure <dir>           direct voice renders for tools/vault/models_measure.py: every variant at C5
//                                          (velocity 1.0 and 0.5, 3 s, held 2.5 s) + every model at C2..C7 (velocity 1.0)
//   VaultModelLab -cpu [seconds]           CPU per sounding voice at 48 kHz (percent of one core), every model
//   VaultModelLab -sweep                   peak level per model across C2..C7 (gain calibration)
//   VaultModelLab -list                    variants and model ids
// Files are 24-bit WAV, 48 kHz. Notes use the FL Studio names of CHROME VAULT: MIDI 60 = C5.
#include "../../Source/vault/VaultEngine.h"
#include "../../Source/vault/models/ModelVoice.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <chrono>
#include <cstdio>

namespace
{
using vault::models::ModelRegistry;
using vault::models::ModelInfo;

struct Variant { const char* name; const char* model; std::vector<std::pair<const char*, float>> params; float vel = 0.8f; };

const std::vector<Variant>& variants()
{
    static const std::vector<Variant> v = {
        { "guitar_steel_pluck", "guitar.acoustic", {} },
        { "guitar_steel_mute", "guitar.acoustic", { { "m.artic", 1 } } },
        { "guitar_steel_harmonic", "guitar.acoustic", { { "m.artic", 2 } } },
        { "guitar_nylon_pluck", "guitar.nylon", {} },
        { "guitar_nylon_mute", "guitar.nylon", { { "m.artic", 1 } } },
        { "guitar_nylon_harmonic", "guitar.nylon", { { "m.artic", 2 } } },
        { "guitar_banjo_pluck", "guitar.banjo", {} },
        { "guitar_banjo_mute", "guitar.banjo", { { "m.artic", 1 } } },
        { "guitar_banjo_harmonic", "guitar.banjo", { { "m.artic", 2 } } },
        { "guitar_12string_pluck", "guitar.twelve", {} },
        { "guitar_12string_mute", "guitar.twelve", { { "m.artic", 1 } } },
        { "guitar_12string_harmonic", "guitar.twelve", { { "m.artic", 2 } } },
        { "guitar_electric_neck", "guitar.electric", { { "m.pickup", 0 } } },
        { "guitar_electric_bridge", "guitar.electric", { { "m.pickup", 1 } } },
        { "guitar_electric_mute", "guitar.electric", { { "m.artic", 1 } } },
        { "guitar_electric_harmonic", "guitar.electric", { { "m.artic", 2 } } },
        { "guitar_crunch_pluck", "guitar.crunch", {} },
        { "guitar_crunch_mute", "guitar.crunch", { { "m.artic", 1 } } },
        { "guitar_crunch_harmonic", "guitar.crunch", { { "m.artic", 2 } } },
        { "guitar_jazz_pluck", "guitar.jazz", {} },
        { "guitar_jazz_mute", "guitar.jazz", { { "m.artic", 1 } } },
        { "guitar_jazz_harmonic", "guitar.jazz", { { "m.artic", 2 } } },
        { "bass_finger_pluck", "bass.finger", {} },
        { "bass_finger_mute", "bass.finger", { { "m.artic", 1 } } },
        { "bass_pick_pluck", "bass.pick", {} },
        { "bass_pick_mute", "bass.pick", { { "m.artic", 1 } } },
        { "bass_slap", "bass.slap", {} },
        { "bass_slap_mute", "bass.slap", { { "m.artic", 1 } } },
        { "bass_fretless", "bass.fretless", {} },
        { "bass_fretless_harmonic", "bass.fretless", { { "m.artic", 2 } } },
        { "ep_tine_mellow", "ep.tine.mellow", {} },
        { "ep_tine_mellow_soft", "ep.tine.mellow", {}, 0.4f },
        { "ep_tine_bell", "ep.tine.bell", {} },
        { "ep_tine_bell_trem", "ep.tine.bell", { { "m.trem", 0.6f } } },
        { "ep_reed", "ep.reed", {} },
        { "ep_reed_soft", "ep.reed", {}, 0.4f },
        { "ep_reed_bark", "ep.reed", { { "m.drive", 0.8f } }, 1.0f },
        { "ep_clav", "ep.clav", {} },
        { "ep_clav_mute", "ep.clav", { { "m.artic", 1 } } },
        { "keys_toy_felt", "keys.toy", {} },
        { "voice_female_ah", "voice.female", { { "m.vowel", 0 } } },
        { "voice_female_oh", "voice.female", { { "m.vowel", 1 } } },
        { "voice_female_oo", "voice.female", { { "m.vowel", 2 } } },
        { "voice_female_ee", "voice.female", { { "m.vowel", 3 } } },
        { "voice_female_ha", "voice.female", { { "m.artic", 1 } } },
        { "voice_female_breathy", "voice.female", { { "m.artic", 2 } } },
        { "voice_female_whisper", "voice.female", { { "m.artic", 3 } } },
        { "voice_male_ah", "voice.male", { { "m.vowel", 0 } } },
        { "voice_male_oh", "voice.male", { { "m.vowel", 1 } } },
        { "voice_male_oo", "voice.male", { { "m.vowel", 2 } } },
        { "voice_male_ee", "voice.male", { { "m.vowel", 3 } } },
        { "voice_male_ha", "voice.male", { { "m.artic", 1 } } },
        { "voice_male_breathy", "voice.male", { { "m.artic", 2 } } },
        { "voice_male_whisper", "voice.male", { { "m.artic", 3 } } },
        { "choir_female8_ah", "voice.choir", { { "m.voices", 8 }, { "m.gender", 1 } } },
        { "choir_mixed16_ah", "voice.choir", { { "m.voices", 16 }, { "m.gender", 0.5f } } },
        { "choir_mixed16_oo", "voice.choir", { { "m.voices", 16 }, { "m.gender", 0.5f }, { "m.vowel", 2 } } },
        { "choir_male24_oh", "voice.choir", { { "m.voices", 24 }, { "m.gender", 0 }, { "m.vowel", 1 } } },
        { "choir_full24_ah", "voice.choir", { { "m.voices", 24 }, { "m.gender", 0.6f } } },
        { "vocalpad_oo", "voice.pad", {} },
        { "vocalpad_ah", "voice.pad", { { "m.vowel", 0 }, { "m.motion", 0.8f } } },
    };
    return v;
}

float noteHz (int n) { return 440.0f * std::exp2 (((float) n - 69.0f) / 12.0f); }

std::vector<float> paramValues (const ModelInfo& mi, const std::vector<std::pair<const char*, float>>& over)
{
    std::vector<float> p;
    for (auto& in : mi.params)
    {
        float v = in.def;
        for (auto& [k, x] : over) if (juce::String (k) == in.id) v = juce::jlimit (in.lo, in.hi, x);
        p.push_back (v);
    }
    return p;
}

// one voice, rendered the way the engine drives it (setPitch every 128 samples)
juce::AudioBuffer<float> renderVoice (const ModelInfo& mi, const std::vector<float>& params, int note, float vel, double secs,
                                      double hold, double sr)
{
    const int total = (int) (secs * sr), holdN = (int) (hold * sr);
    juce::AudioBuffer<float> b (2, total);
    b.clear();
    auto v = mi.create();
    v->prepare (sr, 512);
    v->setParams (params.data(), (int) params.size());
    v->noteOn (note, vel, noteHz (note));
    for (int pos = 0; pos < total; pos += 128)
    {
        const int n = std::min (128, total - pos);
        if (pos <= holdN && pos + n > holdN) { v->render (b.getWritePointer (0, pos), b.getWritePointer (1, pos), holdN - pos); v->noteOff (0.5f);
                                               v->render (b.getWritePointer (0, holdN), b.getWritePointer (1, holdN), pos + n - holdN); }
        else { v->setPitch (noteHz (note)); v->render (b.getWritePointer (0, pos), b.getWritePointer (1, pos), n); }
    }
    return b;
}

bool writeWav (const juce::AudioBuffer<float>& b, double sr, const juce::File& f)
{
    f.deleteFile();
    std::unique_ptr<juce::FileOutputStream> os (f.createOutputStream());
    if (os == nullptr) return false;
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (os.get(), sr, (unsigned) b.getNumChannels(), 24, {}, 0));
    if (w == nullptr) return false;
    os.release();
    return w->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
}

VaultPreset presetFor (const Variant& v)
{
    VaultPreset p;
    p.id = juce::String ("lab.") + v.name; p.name = v.name; p.category = vault::catSynth; p.gain = 1.0f;
    VaultPart part; part.kind = VaultPartKind::model; part.source = v.model; part.label = v.name;
    for (auto& [k, x] : v.params) part.values.push_back ({ k, x });
    p.parts.push_back (part);
    return p;
}

int demos (const juce::File& dir)
{
    dir.createDirectory();
    int bad = 0;
    for (auto& v : variants())
    {
        const auto b = VaultEngine::render (presetFor (v), vault::homeNote, v.vel, 2.0, 48000.0);
        const float pk = b.getMagnitude (0, b.getNumSamples());
        const bool ok = writeWav (b, 48000.0, dir.getChildFile (juce::String (v.name) + ".wav")) && std::isfinite (pk) && pk > 1.0e-3f;
        if (! ok) ++bad;
        std::printf ("%-28s %-16s peak %6.1f dBFS %s\n", v.name, v.model, juce::Decibels::gainToDecibels (pk), ok ? "" : "FAILED");
    }
    return bad;
}

int measure (const juce::File& dir)
{
    dir.createDirectory();
    auto& reg = ModelRegistry::get();
    for (auto& v : variants())
    {
        const auto* mi = reg.find (v.model);
        if (mi == nullptr) continue;
        const auto p = paramValues (*mi, v.params);
        for (float vel : { 1.0f, 0.5f })
            writeWav (renderVoice (*mi, p, 60, vel, 3.0, 2.5, 48000.0), 48000.0,
                      dir.getChildFile (juce::String (v.name) + (vel > 0.9f ? "_v100" : "_v050") + ".wav"));
    }
    // vowels at C2 (dense harmonics) for the formant check, no vibrato, pressed phonation (a lax source slope hides F1)
    for (auto& v : variants())
        if (const juce::String nm (v.name); nm.startsWith ("voice_") && (nm.endsWith ("_ah") || nm.endsWith ("_oh") || nm.endsWith ("_oo") || nm.endsWith ("_ee")))
        {
            const auto* mi = reg.find (v.model);
            auto over = v.params; over.push_back ({ "m.vibrato", 0.0f }); over.push_back ({ "m.bright", 1.0f });   // pressed: flattest source
            writeWav (renderVoice (*mi, paramValues (*mi, over), 36, 1.0f, 1.2, 1.1, 48000.0), 48000.0,
                      dir.getChildFile (juce::String ("formant_") + v.name + ".wav"));
        }
    juce::String list;
    for (auto& v : variants()) list << v.name << " " << v.model << "\n";
    dir.getChildFile ("variants.txt").replaceWithText (list);
    for (int i = 0; i < reg.size(); ++i)
    {
        const auto& mi = reg.at (i);
        if (mi.id == "sine") continue;
        const auto p = paramValues (mi, {});
        for (int n = 24; n <= 84; n += 12)
            writeWav (renderVoice (mi, p, n, 1.0f, 2.0, 1.6, 48000.0), 48000.0,
                      dir.getChildFile ("sweep_" + mi.id.replaceCharacter ('.', '_') + "_" + juce::String (n) + ".wav"));
    }
    std::printf ("measurement renders -> %s\n", dir.getFullPathName().toRawUTF8());
    return 0;
}

int sweep()
{
    auto& reg = ModelRegistry::get();
    for (int i = 0; i < reg.size(); ++i)
    {
        const auto& mi = reg.at (i);
        const auto p = paramValues (mi, {});
        std::printf ("%-18s", mi.id.toRawUTF8());
        for (int n = 24; n <= 96; n += 6)
        {
            const auto b = renderVoice (mi, p, n, 1.0f, 1.2, 1.0, 48000.0);
            std::printf (" %5.1f", juce::Decibels::gainToDecibels (b.getMagnitude (0, b.getNumSamples()), -99.0f));
        }
        std::printf ("\n");
    }
    return 0;
}

int cpu (double seconds)
{
    auto& reg = ModelRegistry::get();
    const double sr = 48000.0;
    std::printf ("CPU per sounding voice at 48 kHz (128-sample blocks, setPitch every block), %% of one core\n");
    int over = 0;
    for (int i = 0; i <= reg.size() + 1; ++i)
    {
        // the last two rows: the choir with 8 and with 24 singers (the default is 16)
        const auto& mi = i < reg.size() ? reg.at (i) : *reg.find ("voice.choir");
        const float singers = i == reg.size() ? 8.0f : 24.0f;
        const auto p = paramValues (mi, i < reg.size() ? std::vector<std::pair<const char*, float>> {}
                                                      : std::vector<std::pair<const char*, float>> { { "m.voices", singers } });
        const juce::String label = i < reg.size() ? mi.id : mi.id + " x" + juce::String ((int) singers);
        constexpr int nv = 8;
        std::vector<std::unique_ptr<vault::models::ModelVoice>> voices;
        for (int k = 0; k < nv; ++k) { voices.push_back (mi.create()); voices.back()->prepare (sr, 512); }
        std::vector<float> l (128), r (128);
        const int blocks = (int) (seconds * sr / 128);
        long long activeSamples = 0;
        double ns = 0.0;
        std::vector<double> chunks;   // ns / voice-sample of each 1.5 s chunk: the median ignores moments the machine was busy
        const int chunkBlocks = (int) (1.5 * sr / 128);
        for (int b = 0; b < blocks; ++b)
        {
            if (b % chunkBlocks == 0)   // re-strike every 1.5 s: voices keep sounding
            {
                if (activeSamples > 0) chunks.push_back (ns / (double) activeSamples);
                ns = 0.0; activeSamples = 0;
                for (int k = 0; k < nv; ++k) { voices[(size_t) k]->kill(); voices[(size_t) k]->setParams (p.data(), (int) p.size());
                                               voices[(size_t) k]->noteOn (48 + k * 3, 0.8f, noteHz (48 + k * 3)); }
            }
            std::fill (l.begin(), l.end(), 0.0f); std::fill (r.begin(), r.end(), 0.0f);
            const auto t0 = std::chrono::high_resolution_clock::now();
            for (int k = 0; k < nv; ++k)
                if (voices[(size_t) k]->isActive()) { voices[(size_t) k]->setPitch (noteHz (48 + k * 3)); voices[(size_t) k]->render (l.data(), r.data(), 128); activeSamples += 128; }
            ns += (double) std::chrono::duration_cast<std::chrono::nanoseconds> (std::chrono::high_resolution_clock::now() - t0).count();
        }
        if (activeSamples > 0) chunks.push_back (ns / (double) activeSamples);
        std::sort (chunks.begin(), chunks.end());
        const double nsPerSample = chunks.empty() ? 0.0 : chunks[chunks.size() / 2];
        const double pct = nsPerSample * sr / 1.0e9 * 100.0;
        if (pct > 0.3) ++over;
        std::printf ("  %-18s %6.1f ns/sample  %5.3f %%  %s\n", label.toRawUTF8(), nsPerSample, pct, pct > 0.3 ? "OVER 0.3 %" : "");
    }
    return over;
}
} // namespace

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const juce::String mode = argc > 1 ? juce::String (argv[1]) : juce::String ("-list");
    auto arg = [&] (int i, const char* def) { return argc > i ? juce::String (argv[i]) : juce::String (def); };
    const auto cwd = juce::File::getCurrentWorkingDirectory();
    if (mode == "-demos") return demos (cwd.getChildFile (arg (2, "model-demo")));
    if (mode == "-measure") return measure (cwd.getChildFile (arg (2, "model-measure")));
    if (mode == "-cpu") return cpu (arg (2, "12").getDoubleValue());
    if (mode == "-sweep") return sweep();
    if (mode == "-bend" && argc > 3)   // -bend <model> <out.wav>: C5 with a +-2 semitone 5 Hz bend (click inspection)
    {
        const auto* mi = ModelRegistry::get().find (argv[2]);
        if (mi == nullptr) return 1;
        const auto p = paramValues (*mi, {});
        const double sr = 48000.0;
        juce::AudioBuffer<float> b (2, (int) (1.5 * sr));
        b.clear();
        auto v = mi->create();
        v->prepare (sr, 512); v->setParams (p.data(), (int) p.size()); v->noteOn (60, 0.9f, noteHz (60));
        for (int pos = 0; pos < b.getNumSamples(); pos += 128)
        {
            v->setPitch (noteHz (60) * std::exp2 (2.0f * std::sin (6.2831853f * 5.0f * (float) pos / (float) sr) / 12.0f));
            v->render (b.getWritePointer (0, pos), b.getWritePointer (1, pos), std::min (128, b.getNumSamples() - pos));
        }
        writeWav (b, sr, cwd.getChildFile (argv[3]));
        return 0;
    }
    if (mode == "-one" && argc > 3)   // -one <model> <out.wav> [note vel secs hold k=v ...]
    {
        const auto* mi = ModelRegistry::get().find (argv[2]);
        if (mi == nullptr) { std::printf ("unknown model\n"); return 1; }
        std::vector<std::pair<juce::String, float>> kv;
        for (int i = 8; i < argc; ++i) { const juce::String s (argv[i]); kv.push_back ({ s.upToFirstOccurrenceOf ("=", false, false), s.fromFirstOccurrenceOf ("=", false, false).getFloatValue() }); }
        std::vector<float> p;
        for (auto& in : mi->params) { float v = in.def; for (auto& [k, x] : kv) if (k == in.id) v = x; p.push_back (v); }
        const auto b = renderVoice (*mi, p, arg (4, "60").getIntValue(), arg (5, "1").getFloatValue(), arg (6, "3").getDoubleValue(), arg (7, "2.5").getDoubleValue(), 48000.0);
        writeWav (b, 48000.0, cwd.getChildFile (argv[3]));
        std::printf ("peak %.1f dBFS\n", juce::Decibels::gainToDecibels (b.getMagnitude (0, b.getNumSamples())));
        return 0;
    }
    for (auto& v : variants()) std::printf ("%-28s %s\n", v.name, v.model);
    std::printf ("models: %s\n", ModelRegistry::get().ids().joinIntoString (" ").toRawUTF8());
    return 0;
}
