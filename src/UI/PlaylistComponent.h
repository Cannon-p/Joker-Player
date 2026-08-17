#pragma once

#include <JuceHeader.h>

//==============================================================================
/** A skinnable playlist with drag & drop support. */
class PlaylistComponent : public juce::Component,
                          public juce::ListBoxModel,
                          public juce::FileDragAndDropTarget,
                          private juce::Timer
{
public:
    PlaylistComponent();
    ~PlaylistComponent() override;

    struct Track
    {
        juce::File file;
        juce::String title;
        double lengthSecs = 0.0;
    };

    //==========================================================================
    void addFiles (const juce::Array<juce::File>& files);
    void addFile (const juce::File& file);
    void removeTrack (int index);
    void clear();

    int getNumTracks() const { return tracks.size(); }
    const Track& getTrack (int index) const;

    /** Metadata hooks used by the main component. */
    void setTrackLength (int index, double seconds);

    int getPlayingIndex() const { return playingIndex; }
    void setPlayingIndex (int newIndex);
    void clearPlayingIndex() { setPlayingIndex (-1); }

    //==========================================================================
    std::function<void (int trackIndex)> onTrackDoubleClicked;
    std::function<void ()> onListChanged;

    //==========================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    /** Re-applies theme colours to labels after a mode switch. */
    void applyTheme();

    //==========================================================================
    // juce::ListBoxModel
    int getNumRows() override;
    void paintListBoxItem (int rowNumber, juce::Graphics& g,
                           int width, int height, bool rowIsSelected) override;
    void listBoxItemDoubleClicked (int rowNumber, const juce::MouseEvent&) override;
    void deleteKeyPressed (int lastRowSelected) override;
    void selectedRowsChanged (int lastRowSelected) override;

    //==========================================================================
    // juce::FileDragAndDropTarget
    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int x, int y) override;

    //==========================================================================
    // Persistence (a simple xml file of absolute paths)
    void saveToFile (const juce::File& destination) const;
    void loadFromFile (const juce::File& source);

private:
    static bool isPlausibleAudioFile (const juce::File& file);
    void updateCount();
    void timerCallback() override;

    juce::Array<Track> tracks;
    int playingIndex = -1;
    int lastSelectedRow = -1;
    float eqPhase = 0.0f;

    juce::ListBox listBox { {}, nullptr };
    juce::Label listTitle { {}, {} };
    juce::Label countLabel { {}, {} };
    juce::TextButton addButton { juce::String (juce::CharPointer_UTF8 ("添加歌曲")) };
    juce::TextButton clearButton { juce::String (juce::CharPointer_UTF8 ("清空")) };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PlaylistComponent)
};