#include "PluginManager.h"

//==============================================================================
PluginManager::PluginManager()
{
    // JUCE_PLUGINHOST_VST3 must be enabled (see CMakeLists.txt).
#if JUCE_PLUGINHOST_VST3
    formatManager.addFormat (new juce::VST3PluginFormat());
#endif

#if JUCE_PLUGINHOST_VST
    formatManager.addFormat (new juce::VSTPluginFormat());
#endif

#if JUCE_PLUGINHOST_AU
    formatManager.addFormat (new juce::AudioUnitPluginFormat());
#endif

    // Restore the previously scanned plug-in list (if any) so that the user
    // doesn't have to re-scan every time the app starts.
    loadCache();
}

PluginManager::~PluginManager() = default;

//==============================================================================
juce::File PluginManager::getDataDirectory() const
{
    auto dir = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                   .getChildFile ("JokerPlayer");

    if (! dir.isDirectory())
        dir.createDirectory();

    return dir;
}

//==============================================================================
void PluginManager::loadCache()
{
    auto cacheFile = getDataDirectory().getChildFile ("plugin_cache.xml");

    if (auto xml = juce::XmlDocument::parse (cacheFile))
        knownPlugins.recreateFromXml (*xml);
}

void PluginManager::saveCache()
{
    auto cacheFile = getDataDirectory().getChildFile ("plugin_cache.xml");

    if (auto xml = knownPlugins.createXml())
        xml->writeTo (cacheFile);
}

//==============================================================================
std::unique_ptr<juce::AudioPluginInstance> PluginManager::createInstance (
    const juce::PluginDescription& description,
    double sampleRate,
    int blockSize,
    juce::String& errorMessage) const
{
    // Use createInstanceFromDescription (NOT formatManager.createPluginInstance)
    // so that the instance comes back *unprepared*. The caller then calls
    // setRateAndBufferSizeDetails() + prepareToPlay() once, in the correct order.
    // (Auto-preparing inside the format and then calling enableAllBuses()
    // afterwards crashes strict plug-ins, and enabling extra buses makes
    // plug-ins expect more channels than this stereo app can provide.)
    for (auto* format : formatManager.getFormats())
    {
        if (format->getName() == description.pluginFormatName)
            return format->createInstanceFromDescription (description, sampleRate, blockSize, errorMessage);
    }

    errorMessage = "Plug-in format not registered: " + description.pluginFormatName;
    return {};
}

//==============================================================================
juce::Array<juce::PluginDescription> PluginManager::getAllDescriptions() const
{
    auto types = knownPlugins.getTypes();
    return types;
}