#include "MainComponent.h"
#include "CustomLookAndFeel.h"

#include "../Player/PlayerEngine.h"
#include "../Trace.h"

namespace
{
juce::String formatClock (double seconds)
{
    if (seconds < 0.0)
        return "--:--";

    const int s = (int) std::floor (seconds);
    const int mins = s / 60;
    const int secs = s % 60;
    return juce::String (mins) + ":" + juce::String (secs).paddedLeft ('0', 2);
}
} // namespace

//==============================================================================
MainComponent::MainComponent()
    : rack (engine)
{
    aur::traceStep ("MainComponent ctor start");
    setSize (1180, 760);

    setLookAndFeel (&aur::CustomLookAndFeel::instance());

    // --- header ---
    appTitle.setFont (aur::Theme::uiFont (20.0f).boldened());
    appTitle.setColour (juce::Label::textColourId, aur::Theme::text());
    appTitle.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (appTitle);

    appSubtitle.setFont (aur::Theme::uiFont (11.5f));
    appSubtitle.setColour (juce::Label::textColourId, aur::Theme::textDim());
    appSubtitle.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (appSubtitle);

    deviceLabel.setFont (aur::Theme::uiFont (12.0f));
    deviceLabel.setColour (juce::Label::textColourId, aur::Theme::textDim());
    deviceLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (deviceLabel);

    deviceCombo.onChange = [this] { setDeviceSelection(); };
    addAndMakeVisible (deviceCombo);

    volumeSlider.setRange (0.0, 1.0, 0.01);
    volumeSlider.setValue (0.9, juce::dontSendNotification);
    volumeSlider.onValueChange = [this]
    {
        engine.setMasterVolume ((float) volumeSlider.getValue());
    };
    addAndMakeVisible (volumeSlider);

    // --- now playing card ---
    trackName.setFont (aur::Theme::uiFont (20.0f).boldened());
    trackName.setColour (juce::Label::textColourId, aur::Theme::text());
    trackName.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (trackName);

    trackMeta.setFont (aur::Theme::uiFont (12.0f));
    trackMeta.setColour (juce::Label::textColourId, aur::Theme::textDim());
    trackMeta.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (trackMeta);

    timeLabel.setFont (aur::Theme::uiFont (13.0f).boldened());
    timeLabel.setColour (juce::Label::textColourId, aur::Theme::textDim());
    timeLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (timeLabel);

    progressSlider.setRange (0.0, 1.0, 0.0001);
    progressSlider.setValue (0.0, juce::dontSendNotification);
    progressSlider.onValueChange = [this]
    {
        if (isSeeking)
            engine.setPosition (progressSlider.getValue() * engine.getLengthInSeconds());
    };
    progressSlider.onDragStart = [this] { isSeeking = true; };
    progressSlider.onDragEnd = [this] { isSeeking = false; };
    addAndMakeVisible (progressSlider);

    // --- playlist ---
    playlist.onTrackDoubleClicked = [this] (int index) { playTrack (index); };
    playlist.onListChanged = [this] { updateNowPlaying(); };
    addAndMakeVisible (playlist);

    // --- effect rack ---
    rack.onAddPluginClicked = [this] (int) { openPluginBrowser(); };
    addAndMakeVisible (rack);

    // --- transport bar ---
    prevButton.onClick = [this] { playPrevious(); };
    addAndMakeVisible (prevButton);

    playButton.onClick = [this]
    {
        if (! engine.hasTrack())
        {
            if (playlist.getNumTracks() > 0)
                playTrack (0);
            return;
        }

        if (engine.isPlaying())
        {
            engine.pause();
        }
        else
        {
            if (engine.hasStreamFinished())
                engine.restartFromStart();

            engine.play();
        }
    };
    addAndMakeVisible (playButton);

    stopButton.onClick = [this]
    {
        engine.stop();
        updateTransportUi();
    };
    addAndMakeVisible (stopButton);

    nextButton.onClick = [this] { playNext(); };
    addAndMakeVisible (nextButton);

    // --- restore playlist from last session ---
    playlist.loadFromFile (engine.getPluginManager().getDataDirectory()
                               .getChildFile ("playlist.xml"));

    refreshDeviceList();
    updateTransportUi();

    startTimer (60);
    aur::traceStep ("MainComponent ctor done");
}

MainComponent::~MainComponent()
{
    stopTimer();

    // Persist the playlist for next launch.
    playlist.saveToFile (engine.getPluginManager().getDataDirectory()
                             .getChildFile ("playlist.xml"));

    engine.getPluginManager().saveCache();
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    static bool traced = false;
    if (! traced) { traced = true; aur::traceStep ("first paint"); }
    const auto area = getLocalBounds().toFloat();

    juce::ColourGradient bg (aur::Theme::bgTop(), 0.0f, 0.0f,
                             aur::Theme::bg(), 0.0f, area.getHeight(), false);
    g.setGradientFill (bg);
    g.fillAll();

    // Subtle header underline.
    g.setColour (aur::Theme::border().withAlpha (0.6f));
    g.fillRect (0.0f, 56.0f, (float) getWidth(), 1.0f);
}

//==============================================================================
void MainComponent::resized()
{
    auto b = getLocalBounds();

    // --- header ---
    auto header = b.removeFromTop (56).reduced (18, 0);

    auto titleArea = header.removeFromLeft (240);
    appTitle.setBounds (titleArea.removeFromTop (24));
    appSubtitle.setBounds (titleArea);

    auto right = header.removeFromRight (300);
    volumeSlider.setBounds (right.removeFromLeft (150).withTrimmedTop (16));
    deviceLabel.setBounds (right.removeFromLeft (46).withTrimmedTop (18));
    deviceCombo.setBounds (right.reduced (0, 12));

    // --- footer / transport ---
    auto footer = b.removeFromBottom (64);

    const int btnSize = 38;
    const int bigSize = 46;

    const int centreX = footer.getCentreX();
    const int centreY = footer.getCentreY();

    prevButton.setBounds (centreX - btnSize * 1.5f - 20, centreY - btnSize / 2,
                          btnSize, btnSize);
    stopButton.setBounds (centreX - btnSize / 2 - 5, centreY - btnSize / 2,
                          btnSize, btnSize);
    playButton.setBounds (centreX + btnSize / 2 + 5 - bigSize * 0.5f,
                          centreY - bigSize / 2, bigSize, bigSize);
    nextButton.setBounds (centreX + btnSize * 1.5f + 10, centreY - btnSize / 2,
                          btnSize, btnSize);

    // --- main content ---
    auto mainArea = b.reduced (18, 12);

    auto rightColumn = mainArea.removeFromRight (juce::jmax (280, mainArea.getWidth() / 3));
    auto leftColumn = mainArea;

    // now playing card
    auto card = leftColumn.removeFromTop (118);
    trackName.setBounds (card.removeFromTop (48).reduced (8, 4));
    trackMeta.setBounds (card.removeFromTop (26).reduced (8, 2));

    auto timeArea = card.removeFromTop (24);
    timeLabel.setBounds (timeArea.removeFromRight (timeArea.getWidth() / 3).reduced (0, 2));
    progressSlider.setBounds (timeArea.reduced (4, 4));

    // playlist
    playlist.setBounds (leftColumn.reduced (0, 6));

    // effect rack
    rack.setBounds (rightColumn.reduced (0, 0));
}

//==============================================================================
void MainComponent::timerCallback()
{
    const bool playing = engine.isPlaying();
    const bool finished = engine.hasStreamFinished();

    // --- auto-advance to the next track -------------------------------------
    if (playing && finished)
    {
        if (playingIndex + 1 < playlist.getNumTracks())
        {
            playTrack (playingIndex + 1);
        }
        else
        {
            playlist.clearPlayingIndex();
            engine.stop();
            updateNowPlaying();
        }
    }

    // --- update progress / time ---------------------------------------------
    if (engine.hasTrack() && ! isSeeking)
    {
        const double len = engine.getLengthInSeconds();
        const double pos = engine.getPositionInSeconds();

        if (len > 0.0)
            progressSlider.setValue (juce::jlimit (0.0, 1.0, pos / len), juce::dontSendNotification);

        timeLabel.setText (formatClock (pos) + " / " + formatClock (len),
                           juce::dontSendNotification);
    }

    updateTransportUi();

    // --- refresh device list if the active device changed externally ---------
    if (auto* device = engine.getDeviceManager().getCurrentAudioDevice())
    {
        const auto activeName = device->getName();
        const int idx = deviceCombo.getSelectedItemIndex();

        if (idx < 0 || idx >= deviceEntries.size() || deviceEntries[idx].name != activeName)
            refreshDeviceList();
    }
}

//==============================================================================
void MainComponent::playTrack (int index)
{
    if (index < 0 || index >= playlist.getNumTracks())
        return;

    if (! engine.loadFile (playlist.getTrack (index).file))
    {
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon,
                                                "\u65E0\u6CD5\u64AD\u653E",
                                                "\u8BE5\u6587\u4EF6\u683C\u5F0F\u4E0D\u53D7\u652F\u6301\uFF0C\u6216\u6587\u4EF6\u5DF2\u635F\u574F\u3002");
        return;
    }

    playingIndex = index;
    playlist.setPlayingIndex (index);
    playlist.setTrackLength (index, engine.getLengthInSeconds());

    engine.play();
    updateNowPlaying();
    updateTransportUi();
}

void MainComponent::playNext()
{
    if (playlist.getNumTracks() == 0)
        return;

    playTrack ((playingIndex + 1) % playlist.getNumTracks());
}

void MainComponent::playPrevious()
{
    if (playlist.getNumTracks() == 0)
        return;

    // If we're more than a few seconds in, restart the current track instead.
    if (playingIndex >= 0 && engine.hasTrack() && engine.getPositionInSeconds() > 3.0)
    {
        engine.restartFromStart();
        return;
    }

    const int target = playingIndex < 0 ? 0 : (playingIndex - 1 + playlist.getNumTracks()) % playlist.getNumTracks();
    playTrack (target);
}

//==============================================================================
void MainComponent::openPluginBrowser()
{
    if (pluginBrowser != nullptr)
    {
        pluginBrowser->toFront (true);
        return;
    }

    pluginBrowser = std::make_unique<PluginBrowserDialog> (
        engine,
        [this] (const juce::PluginDescription&)
        {
            // The engine already added the plug-in; the rack refreshes itself
            // via the chain's change notifications.
        },
        [this] { return rack.getTargetPathIndex(); });

    pluginBrowser->onClose = [this]
    {
        pluginBrowser.reset();
    };
}

//==============================================================================
void MainComponent::refreshDeviceList()
{
    deviceCombo.clear (juce::dontSendNotification);
    deviceEntries.clear();

    auto& dm = engine.getDeviceManager();
    const juce::String currentType = dm.getCurrentAudioDeviceType();
    const juce::String currentDevice = dm.getCurrentAudioDevice()
                                           ? dm.getCurrentAudioDevice()->getName()
                                           : juce::String();

    int selected = -1;

    for (auto* type : dm.getAvailableDeviceTypes())
    {
        if (type == nullptr)
            continue;

        for (auto& name : type->getDeviceNames (false))
        {
            if (name.isEmpty())
                continue;

            deviceEntries.add ({ type->getTypeName(), name });
            deviceCombo.addItem (name, deviceEntries.size());

            if (type->getTypeName() == currentType && name == currentDevice)
                selected = deviceEntries.size() - 1;
        }
    }

    if (selected >= 0)
        deviceCombo.setSelectedItemIndex (selected, juce::dontSendNotification);
}

void MainComponent::setDeviceSelection()
{
    const int idx = deviceCombo.getSelectedItemIndex();

    if (idx < 0 || idx >= deviceEntries.size())
        return;

    const auto& entry = deviceEntries[idx];
    auto& dm = engine.getDeviceManager();

    if (auto* type = dm.getCurrentDeviceTypeObject())
        if (type->getTypeName() != entry.type)
            dm.setCurrentAudioDeviceType (entry.type, true);

    auto setup = dm.getAudioDeviceSetup();
    setup.outputDeviceName = entry.name;
    dm.setAudioDeviceSetup (setup, true);

    refreshDeviceList();
}

//==============================================================================
void MainComponent::updateNowPlaying()
{
    if (! engine.hasTrack())
    {
        trackName.setText ("\u672A\u52A0\u8F7D\u66F2\u76EE", juce::dontSendNotification);
        trackMeta.setText ("\u6253\u5F00\u6587\u4EF6\u3001\u62D6\u5165\u97F3\u9891\uFF0C\u6216\u53CC\u51FB\u64AD\u653E\u5217\u8868\u4E2D\u7684\u66F2\u76EE\u3002", juce::dontSendNotification);
        timeLabel.setText ("--:-- / --:--", juce::dontSendNotification);
        progressSlider.setValue (0.0, juce::dontSendNotification);
        return;
    }

    const auto file = engine.getCurrentFile();
    trackName.setText (file.getFileNameWithoutExtension(), juce::dontSendNotification);

    const auto len = engine.getLengthInSeconds();
    const int mins = (int) (len / 60.0);
    const int secs = (int) std::fmod (len, 60.0);

    trackMeta.setText (juce::String::formatted ("%.1f kHz \u00B7 %d:%02d \u00B7 %s",
                                                engine.getSampleRate() / 1000.0,
                                                mins, secs,
                                                file.getParentDirectory().getFileName()),
                       juce::dontSendNotification);

    timeLabel.setText ("0:00 / " + formatClock (len), juce::dontSendNotification);
}

void MainComponent::updateTransportUi()
{
    const bool playing = engine.isPlaying();

    playButton.setIcon (playing ? TransportButton::Icon::Pause : TransportButton::Icon::Play);
    playButton.setToggleState (playing, juce::dontSendNotification);

    const bool hasAnyTrack = engine.hasTrack() || playlist.getNumTracks() > 0;
    prevButton.setEnabled (hasAnyTrack);
    nextButton.setEnabled (playlist.getNumTracks() > 0);
    stopButton.setEnabled (engine.hasTrack());

    if (playing != wasPlaying)
    {
        wasPlaying = playing;
        updateNowPlaying();
    }
}

//==============================================================================
MainComponent::TransportButton::TransportButton (Icon i)
    : icon (i)
{
    setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
}

void MainComponent::TransportButton::paintButton (juce::Graphics& g,
                                                  bool shouldDrawButtonAsHighlighted,
                                                  bool shouldDrawButtonAsDown)
{
    const bool isMain = (icon == Icon::Play || icon == Icon::Pause);
    const bool isOn = getToggleState();

    auto area = getLocalBounds().toFloat().reduced (isMain ? 1.0f : 3.0f);

    juce::Colour base = juce::Colour (0xff232a39);

    if (isMain)
    {
        if (isOn)
            base = aur::Theme::accent();
        else if (shouldDrawButtonAsHighlighted)
            base = juce::Colour (0xff2d3550);
    }
    else if (shouldDrawButtonAsHighlighted)
    {
        base = juce::Colour (0xff2a3244);
    }

    if (shouldDrawButtonAsDown)
        base = base.darker (0.15f);

    if (! isEnabled())
        base = base.withAlpha (0.35f);

    g.setColour (base);
    g.fillEllipse (area);

    g.setColour (juce::Colours::white.withAlpha (isEnabled() ? 0.95f : 0.4f));

    const float cx = area.getCentreX();
    const float cy = area.getCentreY();
    const float s = area.getWidth();

    juce::Path p;

    auto effectiveIcon = icon;
    if (isMain)
        effectiveIcon = isOn ? Icon::Pause : Icon::Play;

    switch (effectiveIcon)
    {
        case Icon::Play:
            p.addTriangle (cx - s * 0.18f, cy - s * 0.28f,
                           cx - s * 0.18f, cy + s * 0.28f,
                           cx + s * 0.30f, cy);
            break;

        case Icon::Pause:
            p.addRectangle (cx - s * 0.24f, cy - s * 0.26f, s * 0.14f, s * 0.52f);
            p.addRectangle (cx + s * 0.10f, cy - s * 0.26f, s * 0.14f, s * 0.52f);
            break;

        case Icon::Stop:
            p.addRoundedRectangle (cx - s * 0.22f, cy - s * 0.22f, s * 0.44f, s * 0.44f, 2.0f);
            break;

        case Icon::Prev:
            p.addTriangle (cx + s * 0.22f, cy - s * 0.24f, cx + s * 0.22f, cy + s * 0.24f, cx - s * 0.08f, cy);
            p.addRectangle (cx - s * 0.24f, cy - s * 0.24f, s * 0.12f, s * 0.48f);
            break;

        case Icon::Next:
            p.addTriangle (cx - s * 0.22f, cy - s * 0.24f, cx - s * 0.22f, cy + s * 0.24f, cx + s * 0.08f, cy);
            p.addRectangle (cx + s * 0.12f, cy - s * 0.24f, s * 0.12f, s * 0.48f);
            break;

        default:
            break;
    }

    g.fillPath (p);
}
