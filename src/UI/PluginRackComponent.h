#pragma once

#include <JuceHeader.h>
#include "CustomLookAndFeel.h"

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

    /** Opens the editor window of the most recently added plug-in on `path`
        (the last slot of that chain). Used to auto-show a GUI on add. */
    void openEditorForLastSlot (int path);

    void paint (juce::Graphics&) override;
    void resized() override;

    /** Re-applies theme colours to labels after a mode switch. */
    void applyTheme();

private:
    /** A pill-shaped toggle switch whose knob slides left/right with a short
        ease-out animation when the state is changed by the user. */
    class AnimatedToggle : public juce::ToggleButton, public juce::Timer
    {
    public:
        explicit AnimatedToggle (const juce::String& text);
        ~AnimatedToggle() override;

        void paintButton (juce::Graphics&, bool, bool) override;

        /** Snaps the knob to the current toggle state (used on creation). */
        void snapKnob();

        /** Animates the knob toward the current toggle state. */
        void startAnimation();

    private:
        void timerCallback() override;

        float knob = 0.0f;
        float target = 0.0f;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AnimatedToggle)
    };

    class VolumeSlider : public juce::Slider
    {
    public:
        VolumeSlider() : juce::Slider (juce::Slider::LinearHorizontal, juce::Slider::NoTextBox) {}

        void mouseDown (const juce::MouseEvent& e) override
        {
            if (e.mods.isCtrlDown() && e.mods.isLeftButtonDown())
            {
                setValue (1.0);
                return;
            }

            juce::Slider::mouseDown (e);
        }
    };

    /** A small M / S solo-mute button. Painted entirely by this class (red for
        M, yellow for S) so the colour is independent of the global look-and-feel.
        Left-click calls onPress(false); alt+left-click calls onPress(true). */
    class MsButton : public juce::Component, public juce::SettableTooltipClient
    {
    public:
        MsButton (const juce::String& letter, std::function<juce::Colour()> activeColourProvider)
            : letter (letter), colourProvider (std::move (activeColourProvider))
        {
            setMouseCursor (juce::MouseCursor::PointingHandCursor);
        }

        std::function<void (bool altDown)> onPress;

        void setActive (bool active_)
        {
            if (active != active_)
            {
                active = active_;
                repaint();
            }
        }

        void paint (juce::Graphics& g) override
        {
            const auto b = getLocalBounds().toFloat().reduced (1.0f);
            const float corner = juce::jmin (8.0f, b.getHeight() * 0.5f);

            if (active)
            {
                g.setColour (colourProvider());
                g.fillRoundedRectangle (b, corner);
                g.setColour (juce::Colours::black);
            }
            else
            {
                g.setColour (isMouseOver() ? aur::Theme::panelHover() : aur::Theme::panel());
                g.fillRoundedRectangle (b, corner);
                g.setColour (aur::Theme::border());
                g.drawRoundedRectangle (b, corner, 1.0f);
                g.setColour (aur::Theme::textDim());
            }

            g.setFont (aur::Theme::uiFont (b.getHeight() * 0.55f).boldened());
            g.drawText (letter, b, juce::Justification::centred, false);
        }

        void mouseEnter (const juce::MouseEvent&) override { repaint(); }
        void mouseExit  (const juce::MouseEvent&) override { repaint(); }

        void mouseDown (const juce::MouseEvent& e) override
        {
            if (e.mods.isLeftButtonDown())
                mouseDownInside = contains (e.getPosition());
        }

        void mouseUp (const juce::MouseEvent& e) override
        {
            if (mouseDownInside && contains (e.getPosition()) && onPress != nullptr)
                onPress (e.mods.isAltDown());

            mouseDownInside = false;
        }

    private:
        juce::String letter;
        std::function<juce::Colour()> colourProvider;
        bool active = false;
        bool mouseDownInside = false;
    };

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
        AnimatedToggle bypass { juce::String (juce::CharPointer_UTF8 ("启用")) };
        juce::TextButton guiButton { juce::String (juce::CharPointer_UTF8 ("界面")) };
        juce::TextButton removeButton { juce::String (juce::CharPointer_UTF8 ("×")) };
    };

    class PathHeader : public juce::Component
    {
    public:
        PathHeader (PluginRackComponent& owner, int pathIndex);

        void paint (juce::Graphics&) override;
        void resized() override;
        void applyTheme();

        int getPathIndex() const { return pathIndex; }

        /** Re-reads the engine's solo/mute state into the M/S buttons. */
        void syncSoloMute();

    private:
        PluginRackComponent& owner;
        int pathIndex;
        juce::Label title;
        AnimatedToggle enable { juce::String (juce::CharPointer_UTF8 ("启用")) };
        VolumeSlider volume;
        juce::TextButton addButton { juce::String (juce::CharPointer_UTF8 ("添加插件")) };
        MsButton muteButton { "M", [] { return aur::Theme::danger(); } };
        MsButton soloButton { "S", [] { return aur::Theme::warn(); } };

        void pressMute();
        void pressSolo (bool altDown);
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

    /** Re-syncs every PathHeader's M/S buttons with the engine's solo/mute state. */
    void refreshSoloMute();

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

    // When set to a valid path index, the next change message from exactly
    // that chain is an enable toggle of a row: the rows are repainted in place
    // instead of being rebuilt, letting the switch animation play out.
    int skipEnableRebuildPath = -1;

    // Path the next "add plug-in" action targets (set by requestAddPlugin).
    int targetPath = 0;

    juce::Label title { {}, juce::String (juce::CharPointer_UTF8 ("效果链")) };
    juce::Label countLabel;
    juce::Label latencyLabel;
    juce::TextButton addButton { juce::String (juce::CharPointer_UTF8 ("插件管理")) };

    juce::Viewport viewport;
    juce::Component rowContainer;
    std::unique_ptr<DropIndicator> dropIndicator;

    juce::Slider mixSlider { juce::Slider::RotaryVerticalDrag, juce::Slider::TextBoxBelow };
    juce::Label mixLabel { {}, juce::String (juce::CharPointer_UTF8 ("干湿比")) };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginRackComponent)
};
