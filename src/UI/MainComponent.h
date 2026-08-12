#pragma once

#include <JuceHeader.h>

#include "../Player/PlayerEngine.h"
#include "PlaylistComponent.h"
#include "PluginBrowserDialog.h"
#include "PluginRackComponent.h"

//==============================================================================
/** The main application window content. */
class MainComponent : public juce::Component, public juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    class TransportButton : public juce::TextButton
    {
    public:
        enum class Icon { Play, Pause, Stop, Prev, Next };

        explicit TransportButton (Icon i);
        void setIcon (Icon i) { icon = i; repaint(); }

    private:
        void paintButton (juce::Graphics&, bool shouldDrawButtonAsHighlighted,
                          bool shouldDrawButtonAsDown) override;
        Icon icon;
    };

    void playTrack (int index);
    void playNext();
    void playPrevious();
    void openPluginBrowser();

    void refreshDeviceList();
    void setDeviceSelection();
    void updateNowPlaying();
    void updateTransportUi();

    PlayerEngine engine;

    // --- header ---
    juce::Label appTitle { {}, "Joker Player" };
    juce::Label appSubtitle { {}, "多格式播放器 · 实时 VST 效果" };
    juce::Label deviceLabel { {}, "输出设备" };
    juce::ComboBox deviceCombo;
    juce::Slider volumeSlider { juce::Slider::LinearHorizontal, juce::Slider::NoTextBox };

    // --- now playing card ---
    juce::Label trackName { {}, "未加载曲目" };
    juce::Label trackMeta;
    juce::Label timeLabel { {}, "--:-- / --:--" };
    juce::Slider progressSlider { juce::Slider::LinearHorizontal, juce::Slider::NoTextBox };

    // --- playlist ---
    PlaylistComponent playlist;

    // --- effect rack ---
    PluginRackComponent rack;

    // --- transport bar ---
    TransportButton prevButton { TransportButton::Icon::Prev };
    TransportButton playButton { TransportButton::Icon::Play };
    TransportButton stopButton { TransportButton::Icon::Stop };
    TransportButton nextButton { TransportButton::Icon::Next };

    std::unique_ptr<PluginBrowserDialog> pluginBrowser;

    bool isSeeking = false;
    int playingIndex = -1;
    bool wasPlaying = false;

    struct DeviceEntry
    {
        juce::String type;
        juce::String name;
    };
    juce::Array<DeviceEntry> deviceEntries;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};