#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

// The other KILLA plugins, compiled into KEYS KILLA with their own design and every function (v0.17).
// Each one is a complete juce::AudioProcessor + editor, hosted inside KEYS KILLA.
juce::AudioProcessor* kkCreateVoodooKilla();     // HALF
juce::AudioProcessor* kkCreateEffectorKilla();   // EFFECTOR
juce::AudioProcessor* kkCreateDiggaKilla();      // DIGGA
#include <memory>
#include <vector>
// PAIR YOUR OWN <- DIGGA: the one-shots (and KILL variations) Digga Killa cut from the sample
std::vector<std::pair<juce::String, std::shared_ptr<const juce::AudioBuffer<float>>>> kkDiggaShots (juce::AudioProcessor* digga, double& rate, bool loopsToo);
// CHOP: Digga Killa's decoded source sample and its file name
std::shared_ptr<const juce::AudioBuffer<float>> kkDiggaSource (juce::AudioProcessor* digga, double& rate);
juce::String kkDiggaSourceName (juce::AudioProcessor* digga);
