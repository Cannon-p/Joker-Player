#include "PlaylistComponent.h"
#include "CustomLookAndFeel.h"
#include "../Trace.h"

#include <cctype>
#include <cmath>

namespace
{
constexpr int kRowHeight = 44;
constexpr int kHeaderHeight = 40;
constexpr int kFooterHeight = 44;

juce::String shortName (const juce::String& fullText, int maxChars)
{
    if (fullText.length() <= maxChars)
        return fullText;

    return fullText.substring (0, juce::jmax (0, maxChars - 1)) + "…";
}

juce::String formatTime (double seconds)
{
    if (seconds < 0.0)
        return "--:--";

    const int secs = (int) std::floor (seconds);
    const int m = secs / 60;
    const int s = secs % 60;
    return juce::String (m) + ":" + juce::String (s).paddedLeft ('0', 2);
}
} // namespace

//==============================================================================
PlaylistComponent::PlaylistComponent()
{
    aur::traceStep ("PlaylistComponent ctor start");
    setColour (juce::ListBox::backgroundColourId, juce::Colour (0x00ffffff));

    listBox.setModel (this);
    listBox.setRowHeight (kRowHeight);
    listBox.setOutlineThickness (0);
    listBox.setColour (juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
    listBox.setColour (juce::ListBox::outlineColourId, juce::Colours::transparentBlack);
    listBox.getViewport()->setScrollBarsShown (true, false);
    addAndMakeVisible (listBox);
    aur::traceStep ("PlaylistComponent listbox ready");

    listTitle.setText (juce::String (juce::CharPointer_UTF8 ("播放列表")), juce::dontSendNotification);
    listTitle.setFont (aur::Theme::uiFont (15.0f).boldened());
    listTitle.setJustificationType (juce::Justification::centredLeft);
    listTitle.setColour (juce::Label::textColourId, aur::Theme::text());
    addAndMakeVisible (listTitle);

    countLabel.setJustificationType (juce::Justification::centredRight);
    countLabel.setFont (aur::Theme::uiFont (12.0f));
    countLabel.setColour (juce::Label::textColourId, aur::Theme::textDim());
    addAndMakeVisible (countLabel);

    addButton.onClick = [this]
    {
        juce::FileChooser chooser (juce::String (juce::CharPointer_UTF8 ("选择音频文件")),
                                   juce::File::getSpecialLocation (juce::File::userHomeDirectory),
                                   "*");
        if (chooser.browseForMultipleFilesToOpen())
            addFiles (chooser.getResults());
    };

    clearButton.onClick = [this]
    {
        const bool wasPlaying = playingIndex >= 0;
        clear();
        juce::ignoreUnused (wasPlaying);

        if (onListChanged)
            onListChanged();
    };

    addAndMakeVisible (addButton);
    addAndMakeVisible (clearButton);

    updateCount();
    aur::traceStep ("PlaylistComponent ctor done");
}

PlaylistComponent::~PlaylistComponent() = default;

//==============================================================================
void PlaylistComponent::addFiles (const juce::Array<juce::File>& files)
{
    for (auto& f : files)
        addFile (f);
}

void PlaylistComponent::addFile (const juce::File& file)
{
    if (! isPlausibleAudioFile (file))
        return;

    for (auto& t : tracks)
        if (t.file == file)
            return; // ignore duplicates

    Track newTrack;
    newTrack.file = file;
    newTrack.title = shortName (file.getFileNameWithoutExtension(), 42);
    newTrack.lengthSecs = 0.0;
    tracks.add (std::move (newTrack));

    listBox.updateContent();
    updateCount();
    listBox.scrollToEnsureRowIsOnscreen (tracks.size() - 1);

    if (onListChanged)
        onListChanged();
}

void PlaylistComponent::removeTrack (int index)
{
    if (index < 0 || index >= tracks.size())
        return;

    const bool wasCurrent = (index == playingIndex);
    tracks.remove (index);

    if (wasCurrent)
        playingIndex = -1;
    else if (index < playingIndex)
        --playingIndex;

    listBox.updateContent();
    updateCount();

    if (onListChanged)
        onListChanged();
}

void PlaylistComponent::clear()
{
    tracks.clear();
    playingIndex = -1;
    listBox.updateContent();
    updateCount();
}

//==============================================================================
const PlaylistComponent::Track& PlaylistComponent::getTrack (int index) const
{
    jassert (index >= 0 && index < tracks.size());
    return tracks.getReference (index);
}

void PlaylistComponent::setTrackLength (int index, double seconds)
{
    if (index < 0 || index >= tracks.size())
        return;

    tracks.getReference (index).lengthSecs = seconds;
    listBox.repaint();
}

void PlaylistComponent::setPlayingIndex (int newIndex)
{
    if (playingIndex == newIndex)
        return;

    playingIndex = newIndex;
    listBox.repaint();
    listBox.scrollToEnsureRowIsOnscreen (juce::jmax (0, newIndex));

    if (playingIndex >= 0)
    {
        eqPhase = 0.0f;
        startTimerHz (30);
    }
    else
    {
        stopTimer();
    }
}

//==============================================================================
void PlaylistComponent::timerCallback()
{
    if (playingIndex < 0)
    {
        stopTimer();
        return;
    }

    eqPhase += 0.12f;
    listBox.repaintRow (playingIndex);
}

//==============================================================================
void PlaylistComponent::applyTheme()
{
    listTitle.setColour (juce::Label::textColourId, aur::Theme::text());
    countLabel.setColour (juce::Label::textColourId, aur::Theme::textDim());
    repaint();
    listBox.repaint();
}

//==============================================================================
void PlaylistComponent::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat().reduced (2.0f);

    juce::DropShadow shadow (juce::Colour (0x40000000), 10, { 0, 3 });
    shadow.drawForRectangle (g, b.toNearestInt());

    g.setGradientFill (aur::Theme::panelGradient (b));
    g.fillRoundedRectangle (b, 10.0f);

    g.setColour (aur::Theme::border().withAlpha (0.6f));
    g.drawRoundedRectangle (b, 10.0f, 1.0f);
}

//==============================================================================
void PlaylistComponent::resized()
{
    auto b = getLocalBounds();

    auto footer = b.removeFromBottom (kFooterHeight);
    auto header = b.removeFromTop (kHeaderHeight);

    addButton.setBounds (footer.removeFromRight (96).reduced (6, 7));
    clearButton.setBounds (footer.removeFromRight (80).reduced (6, 7));

    listTitle.setBounds (header.removeFromLeft (header.getWidth() * 0.6).reduced (8, 0));
    countLabel.setBounds (header.reduced (8, 0));

    listBox.setBounds (b.reduced (6, 0));
}

//==============================================================================
int PlaylistComponent::getNumRows()
{
    return tracks.size();
}

void PlaylistComponent::paintListBoxItem (int rowNumber, juce::Graphics& g,
                                          int width, int height, bool rowIsSelected)
{
    const bool rowIsPlaying = (rowNumber == playingIndex);

    if (tracks.size() == 0)
    {
        g.setColour (aur::Theme::textDim());
        g.setFont (aur::Theme::uiFont (14.0f));
        g.drawText (juce::String (juce::CharPointer_UTF8 ("将音频文件拖放到这里，或点击“添加歌曲”")), getLocalBounds(),
                    juce::Justification::centred);
        return;
    }

    if (! isPositiveAndBelow (rowNumber, tracks.size()))
        return;

    const auto& track = tracks.getReference (rowNumber);

    auto area = juce::Rectangle<int> (3, 1, width - 6, height - 2).toFloat();

    // Row background.
    if (rowIsSelected)
    {
        g.setColour (rowIsPlaying ? aur::Theme::accentSoft()
                                  : aur::Theme::panelHover());
        g.fillRoundedRectangle (area, 8.0f);
    }
    else if (rowIsPlaying)
    {
        g.setColour (aur::Theme::accentSoft());
        g.fillRoundedRectangle (area, 8.0f);
    }

    const auto textColour = rowIsPlaying ? aur::Theme::accent() : aur::Theme::text();
    const auto dimColour = rowIsPlaying ? aur::Theme::accent() : aur::Theme::textDim();

    // Index / icon column.
    auto indexBox = area.removeFromLeft (34.0f);

    if (rowIsPlaying)
    {
        // Live animated equalizer bars.
        const float cy = area.getCentreY();
        const float barW = 3.0f;
        const float gap = 2.0f;
        float xs[] = { 6.0f, 6.0f + barW + gap, 6.0f + 2 * (barW + gap) };

        const float heights[] = {
            juce::jmap (std::sin (eqPhase * 1.3f + 0.0f) * 0.5f + 0.5f, 0.0f, 1.0f, 3.0f, 11.0f),
            juce::jmap (std::sin (eqPhase * 1.7f + 2.1f) * 0.5f + 0.5f, 0.0f, 1.0f, 4.0f, 14.0f),
            juce::jmap (std::sin (eqPhase * 1.1f + 4.2f) * 0.5f + 0.5f, 0.0f, 1.0f, 3.0f, 9.0f)
        };

        g.setColour (aur::Theme::accent().withMultipliedAlpha (0.4f));
        g.fillRoundedRectangle (indexBox.getX() + xs[0], cy - heights[0] * 0.5f, barW, heights[0], 1.5f);
        g.setColour (aur::Theme::accent());
        g.fillRoundedRectangle (indexBox.getX() + xs[1], cy - heights[1] * 0.5f, barW, heights[1], 1.5f);
        g.fillRoundedRectangle (indexBox.getX() + xs[2], cy - heights[2] * 0.5f, barW, heights[2], 1.5f);
    }
    else
    {
        g.setColour (dimColour);
        g.setFont (aur::Theme::uiFont (12.0f));
        g.drawText (juce::String (rowNumber + 1), indexBox, juce::Justification::centred);
    }

    // Title.
    auto titleBox = area.removeFromLeft (area.getWidth() * 0.72f);
    g.setColour (textColour);
    g.setFont (aur::Theme::uiFont (13.5f));
    g.drawText (track.title, titleBox.reduced (8, 0), juce::Justification::centredLeft);

    // Duration.
    g.setColour (dimColour);
    g.setFont (aur::Theme::uiFont (12.0f));
    g.drawText (formatTime (track.lengthSecs), area.reduced (8, 0),
                juce::Justification::centredRight);
}

//==============================================================================
void PlaylistComponent::listBoxItemDoubleClicked (int rowNumber, const juce::MouseEvent&)
{
    if (isPositiveAndBelow (rowNumber, tracks.size()) && onTrackDoubleClicked)
        onTrackDoubleClicked (rowNumber);
}

void PlaylistComponent::listBoxItemClicked (int rowNumber, const juce::MouseEvent& e)
{
    if (! e.mods.isRightButtonDown())
        return;

    if (! isPositiveAndBelow (rowNumber, tracks.size()))
        return;

    listBox.selectRow (rowNumber);

    juce::PopupMenu menu;
    menu.addItem (1, juce::String (juce::CharPointer_UTF8 ("删除该歌曲")));
    menu.addItem (2, juce::String (juce::CharPointer_UTF8 ("打开所在文件夹")));

    switch (menu.show())
    {
        case 1:
            removeTrack (rowNumber);
            break;

        case 2:
            tracks.getReference (rowNumber).file.getParentDirectory().startAsProcess();
            break;

        default:
            break;
    }
}

void PlaylistComponent::deleteKeyPressed (int lastRowSelected)
{
    if (isPositiveAndBelow (lastRowSelected, tracks.size()))
        removeTrack (lastRowSelected);
}

void PlaylistComponent::selectedRowsChanged (int lastRowSelected)
{
    lastSelectedRow = lastRowSelected;
}

//==============================================================================
bool PlaylistComponent::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (auto& f : files)
        if (isPlausibleAudioFile (juce::File (f)))
            return true;

    return false;
}

void PlaylistComponent::filesDropped (const juce::StringArray& files, int, int)
{
    juce::Array<juce::File> dropped;
    for (auto& f : files)
        dropped.add (juce::File (f));

    addFiles (dropped);
}

//==============================================================================
bool PlaylistComponent::isPlausibleAudioFile (const juce::File& file)
{
    if (! file.existsAsFile())
        return false;

    const auto ext = file.getFileExtension().toLowerCase();
    static const juce::StringArray okExtensions {
        ".wav", ".aif", ".aiff", ".flac", ".ogg", ".mp3", ".m4a", ".wma", ".aac"
    };
    if (okExtensions.contains (ext))
        return true;

    return false;
}

void PlaylistComponent::updateCount()
{
    countLabel.setText (juce::String (tracks.size()) + " 首", juce::dontSendNotification);
}

//==============================================================================
void PlaylistComponent::saveToFile (const juce::File& destination) const
{
    juce::XmlElement root ("playlist");
    root.setAttribute ("count", tracks.size());

    for (int i = 0; i < tracks.size(); ++i)
        root.createNewChildElement ("track")
            ->setAttribute ("path", tracks.getReference (i).file.getFullPathName());

    root.writeTo (destination);
}

void PlaylistComponent::loadFromFile (const juce::File& source)
{
    tracks.clear();

    if (auto root = juce::XmlDocument::parse (source))
    {
        for (auto* child : root->getChildIterator())
        {
            if (child->hasTagName ("track"))
            {
                juce::File f (child->getStringAttribute ("path"));
                if (f.existsAsFile())
                    addFile (f);
            }
        }
    }
}