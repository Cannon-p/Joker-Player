#pragma once

#include <JuceHeader.h>

class PlayerEngine;

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
        void addSelectedPlugin();
        void scanDefaultFolders();
        void chooseAndScanFolder();

        static juce::AudioPluginFormat* findVst3Format (const juce::AudioPluginFormatManager&);
        static juce::StringArray getDefaultScanFolders();

        PluginBrowserDialog& owner;
        PlayerEngine& engine;
        std::function<void (const juce::PluginDescription&)> onAdd;
        std::function<int()> getTargetPath;

        juce::Label title { {}, "插件管理器" };
        juce::TextButton scanDefaultButton { "扫描默认目录" };
        juce::TextButton browseButton { "选择目录…" };
        juce::TextButton addSelectedButton { "＋ 添加所选插件" };
        juce::TextButton doneButton { "完成" };
        juce::Label statusLabel;

        std::unique_ptr<juce::PluginListComponent> pluginList;
        std::unique_ptr<juce::PropertiesFile> properties;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ContentComponent)
    };

    PlayerEngine& engine;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginBrowserDialog)
};