#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
    Owns the plug-in format manager + the known plug-in list.

    The list of known plug-ins is persisted to an XML cache file. On startup the
    cache is loaded so previously scanned plug-ins are available instantly; when
    scanning a folder, files that are already described in the cache are skipped,
    so a full re-scan is only ever needed once.
*/
class PluginManager : public juce::ChangeBroadcaster
{
public:
    PluginManager();
    ~PluginManager() override;

    /** Folder that stores the app's cache / settings. */
    juce::File getDataDirectory() const;

    juce::AudioPluginFormatManager& getFormatManager() { return formatManager; }
    const juce::AudioPluginFormatManager& getFormatManager() const { return formatManager; }

    juce::KnownPluginList& getKnownPlugins() { return knownPlugins; }
    const juce::KnownPluginList& getKnownPlugins() const { return knownPlugins; }

    /** Loads the cached list of plug-ins (if present). */
    void loadCache();

    /** Writes the current known-plug-in list to the cache file. */
    void saveCache();

    /** Instantiate a plug-in from a description. */
    std::unique_ptr<juce::AudioPluginInstance> createInstance (const juce::PluginDescription& description,
                                                               double sampleRate,
                                                               int blockSize,
                                                               juce::String& errorMessage) const;

    /** Touch-and-go: helper that builds the list of plug-in descriptions (all formats). */
    juce::Array<juce::PluginDescription> getAllDescriptions() const;

private:
    juce::AudioPluginFormatManager formatManager;
    juce::KnownPluginList knownPlugins;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginManager)
};