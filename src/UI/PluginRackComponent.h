#pragma once

#include <JuceHeader.h>

class PlayerEngine;

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

    int getTargetPathIndex() const { return pathCombo.getSelectedItemIndex(); }

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
        juce::TextButton addButton { "\uFF0B" };
    };

    void rebuildRows();
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void timerCallback() override;
    void openEditor (int pathIndex, int slotIndex);

    PlayerEngine& engine;

    juce::Label title { {}, "效果链" };
    juce::Label countLabel;
    juce::Label latencyLabel;
    juce::TextButton addButton { "＋  添加效果" };
    juce::Label pathLabel { {}, "目标路径" };
    juce::ComboBox pathCombo;

    juce::Viewport viewport;
    juce::Component rowContainer;

    juce::Slider mixSlider { juce::Slider::RotaryVerticalDrag, juce::Slider::TextBoxBelow };
    juce::Label mixLabel { {}, "干湿比" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginRackComponent)
};
