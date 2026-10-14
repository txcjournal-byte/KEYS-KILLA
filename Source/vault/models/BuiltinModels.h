#pragma once
#include "ModelVoice.h"

// The physical models compiled into CHROME VAULT. Each family file exposes its ids and an info factory; ModelRegistry.cpp
// adds one line per model. Research notes, targets and measurements: docs/VAULT_MODELS.md.
namespace vault::models
{
// StringModels.cpp - digital waveguide plucked / struck strings (commuted body, dispersion, pickups, amp + cabinet)
ModelInfo stringModelInfo (const juce::String& id);
juce::StringArray stringModelIds();
// KeysModels.cpp - tine / reed electric pianos and the felt toy piano (modal tine / reed + nonlinear pickups)
ModelInfo keysModelInfo (const juce::String& id);
juce::StringArray keysModelIds();
// VoiceModels.cpp - source-filter voice: LF glottal pulse, aspiration, cascade formants, choir ensembles, vocal pad
ModelInfo voiceModelInfo (const juce::String& id);
juce::StringArray voiceModelIds();
} // namespace vault::models
