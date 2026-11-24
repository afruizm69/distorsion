#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace dex
{
inline juce::String U (const char* s) { return juce::String::fromUTF8 (s); }

inline juce::String eqId (int i, const char* name) { return "eq" + juce::String (i + 1) + "_" + name; }
inline juce::String pdId (int i, const char* name) { return "pd" + juce::String (i + 1) + "_" + name; }

constexpr int numEqs = 3;
constexpr int numPreStages = 4;
constexpr int numSlots = 4;

// Duraciones (en pulsos) de cada division del LFO sincronizado
constexpr double lfoDivBeats[] = { 16.0, 8.0, 4.0, 2.0, 1.0, 0.5, 0.25 };

juce::StringArray eqTypeNames();
juce::StringArray eqModeNames();
juce::StringArray preStageNames();

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
} // namespace dex
