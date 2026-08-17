#pragma once

#include <JuceHeader.h>

#include "CustomLookAndFeel.h"
#include "../Player/PlayerEngine.h"
#include "PlaylistComponent.h"
#include "PluginBrowserDialog.h"
#include "PluginRackComponent.h"

//==============================================================================
/** The main application window content. */
class MainComponent : public juce::Component, public juce::Timer, private juce::KeyListener
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;
    bool keyPressed (const juce::KeyPress&, juce::Component*) override;

private:
    /** A seekable progress bar that renders the audio waveform of the loaded
        track, with the played portion highlighted in the accent colour. */
    class WaveformBar : public juce::Component, private juce::ChangeListener
    {
    public:
        WaveformBar();

        void setFile (const juce::File& file);
        void clear();
        void setPosition01 (double position);

        std::function<void (double)> onSeek;

    private:
        void changeListenerCallback (juce::ChangeBroadcaster*) override;
        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseDrag (const juce::MouseEvent&) override;

        juce::AudioFormatManager formatManager;
        juce::AudioThumbnailCache thumbnailCache { 5 };
        juce::AudioThumbnail thumbnail { 512, formatManager, thumbnailCache };
        double position = 0.0;    };

    /** A small icon-only button that toggles between day and night themes
        (draws a sun when in day mode, a crescent moon when in night mode). */
    class ThemeButton : public juce::Button
    {
    public:
        ThemeButton();

        void setTheme (aur::Theme::Mode m) { mode = m; repaint(); }

    private:
        void paintButton (juce::Graphics&, bool shouldDrawButtonAsHighlighted,
                          bool shouldDrawButtonAsDown) override;
        aur::Theme::Mode mode = aur::Theme::Mode::Night;
    };

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
    void togglePlayPause();
    void openPluginBrowser();

    void refreshDeviceList();
    void refreshBufferList();
    void setDeviceSelection();
    void setBufferSelection();
    void updateNowPlaying();
    void updateTransportUi();
    void applyTheme();

    PlayerEngine engine;

    // --- header ---
    juce::Label appTitle { {}, "Joker Player" };
    ThemeButton themeButton;
    juce::Label deviceLabel { {}, juce::String (juce::CharPointer_UTF8 ("输出设备")) };
    juce::ComboBox deviceCombo;
    juce::Label bufferLabel { {}, juce::String (juce::CharPointer_UTF8 ("缓冲区")) };
    juce::ComboBox bufferCombo;
    juce::Slider volumeSlider { juce::Slider::LinearHorizontal, juce::Slider::NoTextBox };

    // --- now playing card ---
    juce::Label trackName { {}, juce::String (juce::CharPointer_UTF8 ("未加载曲目")) };
    juce::Label trackMeta;
    juce::Label timeLabel { {}, "--:-- / --:--" };
    WaveformBar waveformBar;

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
    float playPulsePhase = 0.0f;
    juce::Rectangle<int> nowPlayingBounds;

    struct DeviceEntry
    {
        juce::String type;
        juce::String name;
    };
    juce::Array<DeviceEntry> deviceEntries;
    juce::Array<int> bufferSizes;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};