#include "MainComponent.h"
#include "CustomLookAndFeel.h"
#include "UIAnimator.h"

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
MainComponent::WaveformBar::WaveformBar()
{
    formatManager.registerBasicFormats();
    thumbnail.addChangeListener (this);
}
void MainComponent::WaveformBar::setFile (const juce::File& file)
{
    thumbnail.setSource (new juce::FileInputSource (file));
    position = 0.0;
    repaint();
}

void MainComponent::WaveformBar::clear()
{
    thumbnail.clear();
    position = 0.0;
    repaint();
}

void MainComponent::WaveformBar::setPosition01 (double newPosition)
{
    position = juce::jlimit (0.0, 1.0, newPosition);
    repaint();
}

void MainComponent::WaveformBar::changeListenerCallback (juce::ChangeBroadcaster*)
{
    repaint();
}

void MainComponent::WaveformBar::paint (juce::Graphics& g)
{
    const auto area = getLocalBounds().toFloat();
    const float corner = juce::jmin (5.0f, area.getHeight() * 0.5f);

    g.setColour (aur::Theme::inputBg());
    g.fillRoundedRectangle (area, corner);

    const double total = thumbnail.getTotalLength();

    if (total > 0.0)
    {
        const int playedW = (int) std::lround (area.getWidth() * (float) position);

        g.saveState();
        if (playedW > 0)
            g.reduceClipRegion (juce::Rectangle<int> (area.getX(), area.getY(),
                                                      playedW, area.getHeight()));
        g.setColour (aur::Theme::accent());
        thumbnail.drawChannels (g, area.toNearestInt(), 0.0, total, 1.0f);
        g.restoreState();

        g.saveState();
        if (playedW < (int) area.getWidth())
            g.reduceClipRegion (juce::Rectangle<int> (area.getX() + playedW, area.getY(),
                                                      (int) area.getWidth() - playedW,
                                                      (int) area.getHeight()));
        g.setColour (aur::Theme::textDim().withAlpha (0.55f));
        thumbnail.drawChannels (g, area.toNearestInt(), 0.0, total, 1.0f);
        g.restoreState();

        // playhead
        g.setColour (aur::Theme::text());
        g.drawLine (area.getX() + (float) playedW, area.getY(),
                    area.getX() + (float) playedW, area.getBottom(), 1.5f);
    }
    else
    {
        // placeholder baseline
        g.setColour (aur::Theme::border());
        g.drawHorizontalLine ((int) area.getCentreY(),
                              (int) area.getX() + 2, (int) area.getRight() - 2);
    }
}

void MainComponent::WaveformBar::mouseDown (const juce::MouseEvent& e)
{
    if (onSeek != nullptr && getWidth() > 0)
        onSeek (juce::jlimit (0.0, 1.0, (double) e.getPosition().getX() / (double) getWidth()));
}

void MainComponent::WaveformBar::mouseDrag (const juce::MouseEvent& e)
{
    if (onSeek != nullptr && getWidth() > 0)
        onSeek (juce::jlimit (0.0, 1.0, (double) e.getPosition().getX() / (double) getWidth()));
}

//==============================================================================
MainComponent::MainComponent()
    : rack (engine)
{
    aur::traceStep ("MainComponent ctor start");
    setSize (1180, 760);

    setLookAndFeel (&aur::CustomLookAndFeel::instance());
    setWantsKeyboardFocus (true);
    addKeyListener (this);

    // --- header ---
    appTitle.setFont (aur::Theme::uiFont (24.0f).boldened());
    appTitle.setColour (juce::Label::textColourId, aur::Theme::text());
    appTitle.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (appTitle);

    themeButton.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    themeButton.setTooltip (juce::String (juce::CharPointer_UTF8 ("切换日间 / 夜间模式")));
    themeButton.onClick = [this]
    {
        aur::Theme::setMode (aur::Theme::getMode() == aur::Theme::Mode::Night
                                 ? aur::Theme::Mode::Day
                                 : aur::Theme::Mode::Night);
        aur::Theme::saveMode();
        applyTheme();
    };
    addAndMakeVisible (themeButton);

    deviceLabel.setFont (aur::Theme::uiFont (15.0f));
    deviceLabel.setColour (juce::Label::textColourId, aur::Theme::textDim());
    deviceLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (deviceLabel);

    deviceCombo.onChange = [this] { setDeviceSelection(); };
    addAndMakeVisible (deviceCombo);

    bufferLabel.setFont (aur::Theme::uiFont (13.0f));
    bufferLabel.setColour (juce::Label::textColourId, aur::Theme::textDim());
    bufferLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (bufferLabel);

    bufferCombo.onChange = [this] { setBufferSelection(); };
    addAndMakeVisible (bufferCombo);

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

    waveformBar.onSeek = [this] (double fraction)
    {
        if (engine.hasTrack())
            engine.setPosition (fraction * engine.getLengthInSeconds());
    };
    addAndMakeVisible (waveformBar);

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

    playButton.onClick = [this] { togglePlayPause(); };
    addAndMakeVisible (playButton);

    stopButton.onClick = [this]
    {
        engine.stop();
    updateTransportUi();

    // --- breathing glow on the play button while playing ----------------------
    if (engine.isPlaying())
    {
        playPulsePhase += 0.04f;
        if (playPulsePhase >= 1.0f)
            playPulsePhase -= 1.0f;
        playButton.getProperties().set ("pulse", playPulsePhase);
        playButton.repaint();
    }
    else if (playPulsePhase != 0.0f)
    {
        playPulsePhase = 0.0f;
        playButton.getProperties().set ("pulse", 0.0f);
        playButton.repaint();
    }
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
    applyTheme();
    aur::traceStep ("MainComponent ctor done");
}

MainComponent::~MainComponent()
{
    stopTimer();
    removeKeyListener (this);

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

    juce::ColourGradient bg = aur::Theme::windowBackgroundGradient (area);
    g.setGradientFill (bg);
    g.fillAll();

    // Vivid accent glows: a cool blue tint near the top-left, warm violet near the bottom-right.
    juce::ColourGradient topGlow (aur::Theme::accent().withAlpha (0.16f), 0.0f, 0.0f,
                                  aur::Theme::accent().withAlpha (0.0f),
                                  (float) getWidth() * 0.6f, (float) getHeight() * 0.5f, true);
    g.setGradientFill (topGlow);
    g.fillAll();

    juce::ColourGradient bottomGlow (aur::Theme::accent2().withAlpha (0.13f),
                                     (float) getWidth(), (float) getHeight(),
                                     aur::Theme::accent2().withAlpha (0.0f),
                                     (float) getWidth() * 0.5f, (float) getHeight() * 0.5f, true);
    g.setGradientFill (bottomGlow);
    g.fillAll();

    // Subtle header underline.
    g.setColour (aur::Theme::border().withAlpha (0.6f));
    g.fillRect (0.0f, 64.0f, (float) getWidth(), 1.0f);

    // Now-playing card.
    if (! nowPlayingBounds.isEmpty())
    {
        const auto card = nowPlayingBounds.toFloat();

        juce::DropShadow shadow (juce::Colour (0x40000000), 10, { 0, 3 });
        shadow.drawForRectangle (g, nowPlayingBounds);

        g.setGradientFill (aur::Theme::panelGradient (card));
        g.fillRoundedRectangle (card, 10.0f);
        g.setColour (aur::Theme::border().withAlpha (0.6f));
        g.drawRoundedRectangle (card, 10.0f, 1.0f);
    }
}

//==============================================================================
void MainComponent::resized()
{
    auto b = getLocalBounds();

    // --- header ---
    auto header = b.removeFromTop (64).reduced (18, 0);

    auto titleArea = header.removeFromLeft (240);
    appTitle.setBounds (titleArea.removeFromBottom (40));

    themeButton.setBounds (header.removeFromLeft (38).withTrimmedTop (20).withHeight (24));

    auto right = header.removeFromRight (360);
    deviceLabel.setBounds (right.removeFromLeft (72).withTrimmedTop (22).withHeight (20));
    auto bufferArea = right.removeFromRight (150);
    bufferLabel.setBounds (bufferArea.removeFromLeft (50).withTrimmedTop (22).withHeight (20));
    bufferCombo.setBounds (bufferArea.reduced (0, 16));
    deviceCombo.setBounds (right.reduced (0, 16));

    // --- footer / transport ---
    auto footer = b.removeFromBottom (64);

    const int btnSize = 38;
    const int bigSize = 46;
    const int gap = 12;

    const int centreX = footer.getCentreX();
    const int centreY = footer.getCentreY();

    const int groupWidth = btnSize * 3 + bigSize + gap * 3;
    const int startX = centreX - groupWidth / 2;

    prevButton.setBounds (startX, centreY - btnSize / 2, btnSize, btnSize);
    stopButton.setBounds (startX + btnSize + gap, centreY - btnSize / 2,
                          btnSize, btnSize);
    playButton.setBounds (startX + 2 * (btnSize + gap), centreY - bigSize / 2,
                          bigSize, bigSize);
    nextButton.setBounds (startX + 2 * (btnSize + gap) + bigSize + gap,
                          centreY - btnSize / 2, btnSize, btnSize);
    volumeSlider.setBounds (nextButton.getRight() + 20, centreY - 4,
                            footer.getRight() - nextButton.getRight() - 20 - 18, 8);

    // --- main content ---
    auto mainArea = b.reduced (18, 14);
    mainArea.removeFromTop (6);       // breathing room below the header

    auto rightColumn = mainArea.removeFromRight (juce::jmax (280, mainArea.getWidth() / 3));
    rightColumn.removeFromLeft (14);  // gap between the two columns

    auto leftColumn = mainArea;

    // now playing card
    auto card = leftColumn.removeFromTop (118);
    nowPlayingBounds = card;
    trackName.setBounds (card.removeFromTop (48).reduced (8, 4));
    trackMeta.setBounds (card.removeFromTop (26).reduced (8, 2));

    auto timeArea = card.removeFromTop (24);
    timeLabel.setBounds (timeArea.removeFromRight (70).reduced (0, 2));
    waveformBar.setBounds (timeArea.reduced (2, 4));

    // playlist fills the rest of the left column
    playlist.setBounds (leftColumn.reduced (0, 14));

    // effect rack fills the right column, top-aligned with the now-playing card
    rack.setBounds (rightColumn.reduced (0, 14));
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
            waveformBar.setPosition01 (juce::jlimit (0.0, 1.0, pos / len));

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
                                                juce::String (juce::CharPointer_UTF8 ("无法播放")),
                                                juce::String (juce::CharPointer_UTF8 ("该文件格式不受支持，或文件已损坏。")));
        return;
    }

    playingIndex = index;
    playlist.setPlayingIndex (index);
    playlist.setTrackLength (index, engine.getLengthInSeconds());

    waveformBar.setFile (playlist.getTrack (index).file);

    engine.play();
    updateNowPlaying();
    updateTransportUi();
}

void MainComponent::togglePlayPause()
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
bool MainComponent::keyPressed (const juce::KeyPress& key, juce::Component*)
{
    if (key == juce::KeyPress::spaceKey)
    {
        // Don't hijack space while the user is typing (e.g. the plug-in
        // browser's search box has keyboard focus).
        if (auto* focused = juce::Component::getCurrentlyFocusedComponent())
            if (dynamic_cast<juce::TextEditor*> (focused) != nullptr)
                return false;

        togglePlayPause();
        return true;
    }

    return false;
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

    refreshBufferList();
}

void MainComponent::refreshBufferList()
{
    bufferCombo.clear (juce::dontSendNotification);
    bufferSizes.clear();

    auto& dm = engine.getDeviceManager();
    auto* device = dm.getCurrentAudioDevice();

    if (device == nullptr)
        return;

    const int current = dm.getAudioDeviceSetup().bufferSize;

    // ASIO drivers advertise a fixed set of supported buffer sizes; WASAPI /
    // DirectSound report a range, so present a handful of common power-of-two
    // options (the engine coerces the device to a power of two anyway).
    auto sizes = device->getAvailableBufferSizes();

    if (sizes.isEmpty())
    {
        for (int s = 64; s <= 2048; s *= 2)
            sizes.add (s);
    }

    for (auto size : sizes)
    {
        bufferSizes.add (size);
        bufferCombo.addItem (juce::String (size), bufferSizes.size());
    }

    const int index = bufferSizes.indexOf (current);

    if (index >= 0)
        bufferCombo.setSelectedItemIndex (index, juce::dontSendNotification);
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

void MainComponent::setBufferSelection()
{
    const int idx = bufferCombo.getSelectedItemIndex();

    if (idx < 0 || idx >= bufferSizes.size())
        return;

    auto& dm = engine.getDeviceManager();
    auto setup = dm.getAudioDeviceSetup();
    setup.bufferSize = bufferSizes[idx];
    dm.setAudioDeviceSetup (setup, true);

    refreshBufferList();
}

//==============================================================================
void MainComponent::updateNowPlaying()
{
    if (! engine.hasTrack())
    {
        trackName.setText (juce::String (juce::CharPointer_UTF8 ("未加载曲目")), juce::dontSendNotification);
        trackMeta.setText (juce::String (juce::CharPointer_UTF8 ("打开文件、拖入音频，或双击播放列表中的曲目。")), juce::dontSendNotification);
        timeLabel.setText ("--:-- / --:--", juce::dontSendNotification);
        waveformBar.clear();
        return;
    }

    const auto file = engine.getCurrentFile();
    trackName.setText (file.getFileNameWithoutExtension(), juce::dontSendNotification);

    const auto len = engine.getLengthInSeconds();
    const int mins = (int) (len / 60.0);
    const int secs = (int) std::fmod (len, 60.0);

    const auto rateStr = juce::String (engine.getSampleRate() / 1000.0, 1);
        trackMeta.setText (rateStr
                               + juce::String (juce::CharPointer_UTF8 (" kHz · "))
                               + juce::String::formatted ("%d:%02d", mins, secs)
                               + juce::String (juce::CharPointer_UTF8 (" · "))
                               + file.getParentDirectory().getFileName(),
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

void MainComponent::applyTheme()
{
    const bool day = (aur::Theme::getMode() == aur::Theme::Mode::Day);

    themeButton.setTheme (aur::Theme::getMode());

    aur::CustomLookAndFeel::instance().refreshScheme();
    setLookAndFeel (&aur::CustomLookAndFeel::instance());
    sendLookAndFeelChange();

    // Header labels.
    appTitle.setColour (juce::Label::textColourId, aur::Theme::text());
    deviceLabel.setColour (juce::Label::textColourId, aur::Theme::textDim());
    bufferLabel.setColour (juce::Label::textColourId, aur::Theme::textDim());

    // Now-playing card labels.
    trackName.setColour (juce::Label::textColourId, aur::Theme::text());
    trackMeta.setColour (juce::Label::textColourId, aur::Theme::textDim());
    timeLabel.setColour (juce::Label::textColourId, aur::Theme::textDim());

    playlist.applyTheme();
    rack.applyTheme();

    if (pluginBrowser != nullptr)
        pluginBrowser->applyTheme();

    repaint();
}

//==============================================================================
MainComponent::ThemeButton::ThemeButton()
    : juce::Button ({})
{
    setClickingTogglesState (false);
    setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
}

void MainComponent::ThemeButton::paintButton (juce::Graphics& g,
                                              bool shouldDrawButtonAsHighlighted,
                                              bool shouldDrawButtonAsDown)
{
    // Soft rounded backdrop so the icon reads clearly on both themes.
    auto bg = getLocalBounds().toFloat().reduced (2.0f);
    juce::Colour bgCol = shouldDrawButtonAsDown ? aur::Theme::panelActive()
                        : shouldDrawButtonAsHighlighted ? aur::Theme::panelHover()
                        : aur::Theme::panel();
    g.setColour (bgCol);
    g.fillRoundedRectangle (bg, bg.getHeight() * 0.5f);
    g.setColour (aur::Theme::border());
    g.drawRoundedRectangle (bg, bg.getHeight() * 0.5f, 1.0f);

    juce::Colour icon = shouldDrawButtonAsDown ? aur::Theme::accent()
                       : shouldDrawButtonAsHighlighted ? aur::Theme::accent()
                       : aur::Theme::text();
    g.setColour (icon);

    const auto centre = bg.getCentre();
    const float stroke = juce::jmax (1.3f, bg.getHeight() * 0.08f);

    if (mode == aur::Theme::Mode::Day)
    {
        // Sun: small disc + four compact rays.
        const float r = bg.getWidth() * 0.16f;
        g.fillEllipse (centre.x - r, centre.y - r, r * 2.0f, r * 2.0f);

        const float rayLen = bg.getWidth() * 0.10f;
        for (int i = 0; i < 4; ++i)
        {
            const float angle = juce::MathConstants<float>::halfPi * i + juce::MathConstants<float>::halfPi * 0.5f;
            const float cx = std::cos (angle);
            const float sy = std::sin (angle);
            g.drawLine (centre.x + cx * (r + 2.0f), centre.y + sy * (r + 2.0f),
                        centre.x + cx * (r + 2.0f + rayLen), centre.y + sy * (r + 2.0f + rayLen),
                        stroke);
        }
    }
    else
    {
        // Crescent moon: a filled disc with the backdrop colour carved out of
        // its upper-left so only a smooth crescent remains.
        const float r = bg.getHeight() * 0.40f;
        const float xc = centre.x;
        const float yc = centre.y;

        g.fillEllipse (xc - r, yc - r, r * 2.0f, r * 2.0f);

        g.setColour (bgCol);
        g.fillEllipse (xc - r * 0.40f, yc - r * 0.90f, r * 2.0f, r * 2.0f);
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

    juce::Colour base = aur::Theme::panel();

    if (isMain)
    {
        if (isOn)
            base = aur::Theme::accent();
        else if (shouldDrawButtonAsHighlighted)
            base = aur::Theme::panelHover();
    }
    else if (shouldDrawButtonAsHighlighted)
    {
        base = aur::Theme::panelHover();
    }

    if (shouldDrawButtonAsDown)
        base = base.darker (0.15f);

    if (! isEnabled())
        base = base.withAlpha (0.35f);

    // Breathing accent glow behind the play button while playing.
    if (isMain && isOn && isEnabled())
    {
        const float pulse = aur::UIAnimator::value (*this, "pulse", 0.0f);
        const float glowA = 0.28f + 0.22f * std::sin (pulse * juce::MathConstants<float>::twoPi);
        juce::ColourGradient glow (aur::Theme::accent().withAlpha (glowA),
                                   area.getCentreX(), area.getCentreY(),
                                   aur::Theme::accent().withAlpha (0.0f),
                                   area.getCentreX(), area.getCentreY(),
                                   true);
        g.setGradientFill (glow);
        g.fillEllipse (area.expanded (area.getWidth() * 0.35f));
    }

    g.setColour (base);
    g.fillEllipse (area);

    // Icons on an accent (playing) button read best in white on both themes.
    const bool iconOnAccent = isMain && isOn && isEnabled();
    const juce::Colour iconCol = iconOnAccent ? juce::Colours::white
                                              : aur::Theme::text().withAlpha (isEnabled() ? 0.95f : 0.4f);
    g.setColour (iconCol);

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
