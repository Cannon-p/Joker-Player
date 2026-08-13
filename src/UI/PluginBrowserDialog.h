#pragma once

#include <JuceHeader.h>

class PlayerEngine;
class FilteredPluginTableModel;

//==============================================================================
/**
    A window for browsing / scanning VST3 plug-ins.

    Scanning is incremental and re-uses the cached KnownPluginList, so a plug-in
    is only ever scanned once; on later launches the cache is loaded instantly
    and nothing needs to be re-scanned.
*/
class PluginBrowserDialog : public juce::DocumentWindow
{
public:
    PluginBrowserDialog (PlayerEngine& engine,
                         std::function<void (const juce::PluginDescription&)> onAdd,
                         std::function<int()> getTargetPath);
    ~PluginBrowserDialog() override;

    void closeButtonPressed() override;

    std::function<void()> onClose;

private:
    class ContentComponent : public juce::Component
    {
    public:
        ContentComponent (PluginBrowserDialog& owner,
                          PlayerEngine& engine,
                          std::function<void (const juce::PluginDescription&)> onAdd,
                          std::function<int()> getTargetPath);
        ~ContentComponent() override;

        void paint (juce::Graphics&) override;
        void resized() override;

    private:
        void addPlugin (const juce::PluginDescription&);
        static juce::AudioPluginFormat* findVst3Format (const juce::AudioPluginFormatManager&);
        static juce::StringArray getDefaultScanFolders();

        PluginBrowserDialog& owner;
        PlayerEngine& engine;
        std::function<void (const juce::PluginDescription&)> onAdd;
        std::function<int()> getTargetPath;

        juce::Label title { {}, "插件管理器" };
        juce::TextEditor searchBox;
        juce::TextButton doneButton { "完成" };
        juce::Label statusLabel;

        std::unique_ptr<juce::PluginListComponent> pluginList;
        FilteredPluginTableModel* filteredModel = nullptr;
        std::unique_ptr<juce::PropertiesFile> properties;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ContentComponent)
    };

    PlayerEngine& engine;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginBrowserDialog)
};