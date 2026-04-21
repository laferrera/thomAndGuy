#include "Factory.h"
#include "BinaryData.h"

namespace
{
    struct FactoryDef { const char* name; const char* resource; };
    const FactoryDef defs[] = {
        { "HAA Rhythm",      "HAARhythm_xml"     },
        { "Clean Funk",      "CleanFunk_xml"     },
        { "Vowel Sweep",     "VowelSweep_xml"    },
        { "Robot Voice",     "RobotVoice_xml"    },
        { "Sub Synth",       "SubSynth_xml"      },
        { "Subtle Auto-Wah", "SubtleAutoWah_xml" },
    };
}

juce::StringArray FactoryPresets::listNames()
{
    juce::StringArray out;
    for (const auto& d : defs) out.add (d.name);
    return out;
}

bool FactoryPresets::load (int index, juce::AudioProcessorValueTreeState& apvts)
{
    if (index < 0 || index >= (int) juce::numElementsInArray (defs)) return false;
    const auto& def = defs[index];

    int size = 0;
    const char* data = BinaryData::getNamedResource (def.resource, size);
    if (data == nullptr) return false;

    std::unique_ptr<juce::XmlElement> xml (juce::XmlDocument::parse (juce::String (data, (size_t) size)));
    if (xml == nullptr) return false;
    if (! xml->hasTagName (apvts.state.getType())) return false;

    apvts.replaceState (juce::ValueTree::fromXml (*xml));
    return true;
}
