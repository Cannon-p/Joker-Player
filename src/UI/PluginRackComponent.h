#pragma once

#include <JuceHeader.h>

class PlayerEngine;
class PluginChain;

//==============================================================================
/** The right-hand panel showing the three effect paths.

    Routing (see PlayerEngine): path 1 and path 2 run in parallel on the dry
    input; path 3 is fed by path 1's output. The three path outputs are
    latency-aligned and summed.

    The panel shows one collapsible section per path with an enable toggle and
    a volume slider, the plug-in rows inside each section, a target-path
    selector for the "add" button, and a dry/wet mix knob at the bottom.
*/
class PluginRackComponent : public juce::Component,
                            private juce::ChangeListener,
                            private juce::Timer
{
public:
    explicit PluginRackComponent (PlayerEngine& engine);
    ~PluginRackComponent() override;

    /** Invoked with the currently selected target path (0..2) when the user
        clicks "add". */
    std::function<void (int path)> onAddPluginClicked;

    int getTargetPathIndex() const { return targetPath; }

    /** Sets the target-path combo to `path` (0..2) and opens the add dialog. */
    void requestAddPlugin (int path);

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    class RackRow : public juce::Component
    {
    public:
        RackRow (PluginRackComponent& owner, int pathIndex, int slotIndex);

        void paint (juce::Graphics&) override;
        void resized() override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseDrag (const juce::MouseEvent&) override;
        void mouseUp (const juce::MouseEvent&) override;

        int getPathIndex() const { return pathIndex; }
        int getSlotIndex() const { return slotIndex; }

    private:
        PluginRackComponent& owner;
        int pathIndex;
        int slotIndex;
        juce::ToggleButton bypass { "旁路" };
        juce::TextButton guiButton { "界面" };
        juce::TextButton removeButton { "×" };
    };

    class PathHeader : public juce::Component
    {
    public:
        PathHeader (PluginRackComponent& owner, int pathIndex);

        void paint (juce::Graphics&) override;
        void resized() override;

        int getPathIndex() const { return pathIndex; }

    private:
        PluginRackComponent& owner;
        int pathIndex;
        juce::Label title;
        juce::ToggleButton enable { "启用" };
        juce::Slider volume { juce::Slider::LinearHorizontal, juce::Slider::NoTextBox };
        juce::TextButton addButton { "添加插件" };
    };

    /** A thin accent line shown between rows during a drag, marking where the
        dragged plug-in row would land. */
    class DropIndicator : public juce::Component
    {
    public:
        DropIndicator();
        void paint (juce::Graphics&) override;
    };

    void rebuildRows();
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void timerCallback() override;
    void openEditor (int pathIndex, int slotIndex);

    /** Returns the chain for a path index; PlayerEngine::busPath -> master bus. */
    PluginChain& chainFor (int pathIndex) const;

    // Drag & drop reordering of plug-in rows.
    void beginRowDrag (int pathIndex, int slotIndex, int mouseY);
    void updateRowDrag (int mouseY);
    void endRowDrag();
    int getDropSlotForY (int mouseY) const;
    void updateDropIndicator();

    PlayerEngine& engine;

    // Drag state: -1 when inactive.
    int dragPath = -1;
    int dragSlot = -1;
    int dropSlot = -1;

    // Path the next "add plug-in" action targets (set by requestAddPlugin).
    int targetPath = 0;

    juce::Label title { {}, "效果链" };
    juce::Label countLabel;
    juce::Label latencyLabel;
    juce::TextButton addButton { "插件管理" };

    juce::Viewport viewport;
    juce::Component rowContainer;
    std::unique_ptr<DropIndicator> dropIndicator;

    juce::Slider mixSlider { juce::Slider::RotaryVerticalDrag, juce::Slider::TextBoxBelow };
    juce::Label mixLabel { {}, "干湿比" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginRackComponent)
};
