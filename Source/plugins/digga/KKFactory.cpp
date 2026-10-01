#include "PluginProcessor.h"
juce::AudioProcessor* kkCreateDiggaKilla() { return new digga::DiggaKillaProcessor(); }

// PAIR YOUR OWN: hand the one-shots Digga cut (and their KILL variations) to KEYS KILLA
std::vector<std::pair<juce::String, std::shared_ptr<const juce::AudioBuffer<float>>>> kkDiggaShots (juce::AudioProcessor* p, double& rate, bool loopsToo)
{
    std::vector<std::pair<juce::String, std::shared_ptr<const juce::AudioBuffer<float>>>> out;
    auto* d = dynamic_cast<digga::DiggaKillaProcessor*> (p);
    if (d == nullptr) return out;
    rate = d->getEngine().getSampleRate();
    for (auto& n : d->getEngine().getTree().all())
        if (n.audio != nullptr && (n.kind == digga::ClipKind::shot || loopsToo))
            out.push_back ({ n.displayName(), n.audio });
    return out;
}
