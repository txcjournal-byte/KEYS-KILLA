#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

// The other KILLA plugins, compiled into KEYS KILLA with their own design and every function (v0.17).
// Each one is a complete juce::AudioProcessor + editor, hosted inside KEYS KILLA.
juce::AudioProcessor* kkCreateVoodooKilla();     // HALF
juce::AudioProcessor* kkCreateEffectorKilla();   // EFFECTOR
juce::AudioProcessor* kkCreateDiggaKilla();      // DIGGA
