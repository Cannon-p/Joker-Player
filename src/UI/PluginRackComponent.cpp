#include "PluginRackComponent.h"
#include "CustomLookAndFeel.h"
#include "../Player/PlayerEngine.h"
#include "../Trace.h"

namespace
{
juce::String shorten (const juce::String& text, int maxChars)
{
    if (text.length() <= maxChars)
        return text;

    return text.substring (0, juce::jmax (0, maxChars - 1)) + "\u2026";
}

bool isBusPath (int path)
{
    return path == PlayerEngine::busPath;
}
} // namespace

//==============================================================================
PluginRackComponent::AnimatedToggle::AnimatedToggle (const juce::String& text)
    : juce::ToggleButton (text)
{
}

PluginRackComponent::AnimatedToggle::~AnimatedToggle()
{
    stopTimer();
}

void PluginRackComponent::AnimatedToggle::snapKnob()
{
    knob = getToggleState() ? 1.0f : 0.0f;
    target = knob;
    stopTimer();
    repaint();
}

void PluginRackComponent::AnimatedToggle::startAnimation()
{
    target = getToggleState() ? 1.0f : 0.0f;
    startTimerHz (60);
}

void PluginRackComponent::AnimatedToggle::timerCallback()
{
    constexpr float ease = 0.28f;

    knob += (target - knob) * ease;

    if (std::abs (target - knob) < 0.004f)
    {
        knob = target;
        stopTimer();
    }

    repaint();
}

void PluginRackComponent::AnimatedToggle::paintButton (juce::Graphics& g,
                                                       bool /*shouldDrawButtonAsHighlighted*/,
                                                       bool /*shouldDrawButtonAsDown*/)
{
    const auto bounds = getLocalBounds().toFloat();
    const float h = bounds.getHeight();
    const float pillH = juce::jmin (20.0f, h - 2.0f);
    const float pillW = 40.0f;
    const auto pill = juce::Rectangle<float> (8.0f, (h - pillH) * 0.5f, pillW, pillH);
    const bool on = getToggleState();

    // Monochrome track: no accent colouring, plain black/white contrast.
    g.setColour (on ? aur::Theme::bg() : aur::Theme::inputBg());
    g.fillRoundedRectangle (pill, pillH * 0.5f);
    g.setColour (on ? aur::Theme::text() : aur::Theme::border());
    g.drawRoundedRectangle (pill, pillH * 0.5f, 1.0f);

    const float knobSize = juce::jmin (16.0f, pillH - 4.0f);
    const float knobX = pill.getX() + 2.0f + knob * (pill.getWidth() - knobSize - 4.0f);

    g.setColour (on ? aur::Theme::text() : aur::Theme::textDim());
    g.fillEllipse (knobX, pill.getCentreY() - knobSize * 0.5f, knobSize, knobSize);

    g.setColour (on ? aur::Theme::text() : aur::Theme::textDim());
    g.setFont (aur::Theme::uiFont (12.0f));
    g.drawText (getButtonText(),
                juce::Rectangle<float> (pill.getRight() + 6.0f, 0,
                                        bounds.getWidth() - pill.getRight() - 6.0f, h),
                juce::Justification::centredLeft);
}

//==============================================================================
PluginRackComponent::PluginRackComponent (PlayerEngine& engineRef)
    : engine (engineRef)
{
    aur::traceStep ("PluginRackComponent ctor start");
    title.setFont (aur::Theme::uiFont (16.0f).boldened());
    title.setColour (juce::Label::textColourId, aur::Theme::text());
    addAndMakeVisible (title);

    countLabel.setFont (aur::Theme::uiFont (12.0f));
    countLabel.setColour (juce::Label::textColourId, aur::Theme::textDim());
    addAndMakeVisible (countLabel);

    latencyLabel.setFont (aur::Theme::uiFont (12.0f));
    latencyLabel.setColour (juce::Label::textColourId, aur::Theme::textDim());
    latencyLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (latencyLabel);

    addButton.onClick = [this]
    {
        requestAddPlugin (0);
    };
    addAndMakeVisible (addButton);

    viewport.setViewedComponent (&rowContainer, false);
    viewport.setScrollBarsShown (true, false);
    viewport.setScrollBarThickness (8);
    addAndMakeVisible (viewport);

    mixSlider.setRange (0.0, 1.0, 0.01);
    mixSlider.setValue (1.0, juce::dontSendNotification);
    mixSlider.setColour (juce::Slider::textBoxTextColourId, aur::Theme::text());
    mixSlider.setColour (juce::Slider::textBoxBackgroundColourId, aur::Theme::inputBg());
    mixSlider.setColour (juce::Slider::textBoxOutlineColourId, aur::Theme::border());
    mixSlider.onValueChange = [this]
    {
        engine.setMix ((float) mixSlider.getValue());
    };
    addAndMakeVisible (mixSlider);

    mixLabel.setFont (aur::Theme::uiFont (15.0f));
    mixLabel.setColour (juce::Label::textColourId, aur::Theme::textDim());
    mixLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (mixLabel);

    for (int p = 0; p < PlayerEngine::numPaths; ++p)
        engine.getChain (p).addChangeListener (this);

    engine.getBusChain().addChangeListener (this);

    rebuildRows();

    startTimer (250);
    aur::traceStep ("PluginRackComponent ctor done");
}

PluginRackComponent::~PluginRackComponent()
{
    stopTimer();

    for (int p = 0; p < PlayerEngine::numPaths; ++p)
        engine.getChain (p).removeChangeListener (this);

    engine.getBusChain().removeChangeListener (this);
}

void PluginRackComponent::requestAddPlugin (int path)
{
    targetPath = juce::jlimit (0, PlayerEngine::busPath, path);

    if (onAddPluginClicked)
        onAddPluginClicked (targetPath);
}

PluginChain& PluginRackComponent::chainFor (int pathIndex) const
{
    return (pathIndex == PlayerEngine::busPath) ? engine.getBusChain()
                                                : engine.getChain (pathIndex);
}

//==============================================================================
void PluginRackComponent::applyTheme()
{
    title.setColour (juce::Label::textColourId, aur::Theme::text());
    countLabel.setColour (juce::Label::textColourId, aur::Theme::textDim());
    latencyLabel.setColour (juce::Label::textColourId, aur::Theme::textDim());
    mixLabel.setColour (juce::Label::textColourId, aur::Theme::textDim());
    mixSlider.setColour (juce::Slider::textBoxTextColourId, aur::Theme::text());
    mixSlider.setColour (juce::Slider::textBoxBackgroundColourId, aur::Theme::inputBg());
    mixSlider.setColour (juce::Slider::textBoxOutlineColourId, aur::Theme::border());

    for (auto* child : rowContainer.getChildren())
    {
        if (auto* header = dynamic_cast<PathHeader*> (child))
            header->applyTheme();
    }

    repaint();
    rowContainer.repaint();
}

//==============================================================================
void PluginRackComponent::paint (juce::Graphics& g)
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
void PluginRackComponent::resized()
{
    auto b = getLocalBounds().reduced (12);

    auto header = b.removeFromTop (26);
    title.setBounds (header.removeFromLeft (header.getWidth() * 0.5));
    latencyLabel.setBounds (header.removeFromRight (b.getWidth() * 0.28).reduced (0, 2));
    countLabel.setBounds (header.reduced (4, 2));

    auto footer = b.removeFromBottom (140);

    auto addRow = footer.removeFromTop (40);
    addButton.setBounds (addRow.reduced (2, 4));

    auto mixArea = footer.removeFromTop (78);
    mixSlider.setBounds (mixArea.removeFromLeft (72).reduced (2, 12));
    auto labelBox = mixArea.removeFromLeft (72);
    mixLabel.setBounds (labelBox.withTrimmedTop ((labelBox.getHeight() - 20) / 2).withHeight (20));

    viewport.setBounds (b);

    // Layout the rows inside the viewport.
    int total = 0;
    for (int p = 0; p < PlayerEngine::numPaths; ++p)
    {
        total += 34;                              // path header
        total += engine.getChain (p).getNumSlots() * 70;   // plugin rows
    }

    total += 34;                                  // bus header
    total += engine.getBusChain().getNumSlots() * 70;

    auto containerBounds = juce::Rectangle<int> (0, 0, viewport.getWidth() - 8,
                                                 juce::jmax (viewport.getHeight(), total + 8));
    rowContainer.setBounds (containerBounds);

    int y = 4;

    for (auto* child : rowContainer.getChildren())
    {
        const int h = (dynamic_cast<PathHeader*> (child) != nullptr) ? 30 : 62;
        child->setBounds (4, y, containerBounds.getWidth() - 8, h);
        y += h + 4;
    }
}

//==============================================================================
void PluginRackComponent::rebuildRows()
{
    rowContainer.removeAllChildren();

    for (int p = 0; p < PlayerEngine::numPaths; ++p)
    {
        auto& chain = engine.getChain (p);

        rowContainer.addAndMakeVisible (new PathHeader (*this, p));

        for (int i = 0; i < chain.getNumSlots(); ++i)
            rowContainer.addAndMakeVisible (new RackRow (*this, p, i));
    }

    // Master bus section (sits after the three paths).
    {
        auto& chain = engine.getBusChain();

        rowContainer.addAndMakeVisible (new PathHeader (*this, PlayerEngine::busPath));

        for (int i = 0; i < chain.getNumSlots(); ++i)
            rowContainer.addAndMakeVisible (new RackRow (*this, PlayerEngine::busPath, i));
    }

    int count = 0;
    for (int p = 0; p < PlayerEngine::numPaths; ++p)
        count += engine.getChain (p).getNumSlots();

    count += engine.getBusChain().getNumSlots();

    countLabel.setText (juce::String (count) + juce::String (juce::CharPointer_UTF8 (" \u4E2A\u63D2\u4EF6")), juce::dontSendNotification);

    resized();
}

//==============================================================================
void PluginRackComponent::changeListenerCallback (juce::ChangeBroadcaster* source)
{
    for (int p = 0; p < PlayerEngine::numPaths; ++p)
    {
        if (source == &engine.getChain (p))
        {
            if (skipEnableRebuildPath == p)
            {
                skipEnableRebuildPath = -1;
                rowContainer.repaint();
            }
            else
            {
                rebuildRows();
            }
            return;
        }
    }

    if (source == &engine.getBusChain())
    {
        if (skipEnableRebuildPath == PlayerEngine::busPath)
        {
            skipEnableRebuildPath = -1;
            rowContainer.repaint();
        }
        else
        {
            rebuildRows();
        }
    }
}

void PluginRackComponent::timerCallback()
{
    const double rate = engine.getSampleRate();
    const double latencyMs = rate > 0.0 ? (double) engine.getCurrentLatencySamples() * 1000.0 / rate
                                        : 0.0;

    latencyLabel.setText (juce::String (juce::CharPointer_UTF8 ("\u94FE\u5EF6\u8FDF ")) + juce::String::formatted ("%.1f ms", latencyMs),
                          juce::dontSendNotification);
}

//==============================================================================
// A window hosting a plug-in's own editor. Closing it hides (not destroys) the
// window so that the editor state is preserved for when it is reopened.
class PluginEditorWindow : public juce::DocumentWindow
{
public:
    PluginEditorWindow (const juce::String& name, juce::Colour backgroundColour, int requiredButtons)
        : juce::DocumentWindow (name, backgroundColour, requiredButtons)
    {
    }

    void closeButtonPressed() override
    {
        setVisible (false);
    }
};

void PluginRackComponent::RackRow::mouseDown (const juce::MouseEvent& e)
{
    const auto rel = e.getEventRelativeTo (&owner.rowContainer);
    owner.beginRowDrag (pathIndex, slotIndex, rel.getPosition().y);
}

void PluginRackComponent::RackRow::mouseDrag (const juce::MouseEvent& e)
{
    const auto rel = e.getEventRelativeTo (&owner.rowContainer);
    owner.updateRowDrag (rel.getPosition().y);
}

void PluginRackComponent::RackRow::mouseUp (const juce::MouseEvent&)
{
    owner.endRowDrag();
}

//==============================================================================
void PluginRackComponent::beginRowDrag (int pathIndex, int slotIndex, int mouseY)
{
    dragPath = pathIndex;
    dragSlot = slotIndex;
    dropSlot = slotIndex;

    if (dropIndicator == nullptr)
    {
        dropIndicator = std::make_unique<DropIndicator>();
        rowContainer.addAndMakeVisible (dropIndicator.get());
    }

    updateRowDrag (mouseY);
}

void PluginRackComponent::updateRowDrag (int mouseY)
{
    if (dragPath < 0)
        return;

    dropSlot = getDropSlotForY (mouseY);
    updateDropIndicator();
    repaint();
}

void PluginRackComponent::endRowDrag()
{
    if (dragPath >= 0 && dragSlot >= 0 && dropSlot >= 0)
    {
        // dropSlot is a gap index (0..N). Convert to the final slot index the
        // dragged row would occupy after it is removed.
        int to = dropSlot;

        if (dropSlot > dragSlot)
            --to;

        if (to != dragSlot)
            chainFor (dragPath).move (dragSlot, to);
    }

    dragPath = -1;
    dragSlot = -1;
    dropSlot = -1;

    rowContainer.removeChildComponent (dropIndicator.get());
    dropIndicator = nullptr;
}

int PluginRackComponent::getDropSlotForY (int mouseY) const
{
    if (dragPath < 0)
        return -1;

    const int numSlots = chainFor (dragPath).getNumSlots();

    // Map the mouse Y to the gap above/below each plugin row of the dragged path.
    int slot = 0;
    int y = 4;

    for (auto* child : rowContainer.getChildren())
    {
        const int h = (dynamic_cast<const PathHeader*> (child) != nullptr) ? 30 : 62;

        if (auto* row = dynamic_cast<RackRow*> (child))
        {
            if (row->getPathIndex() == dragPath)
            {
                // This row occupies [y, y+h); the gap above it maps to slot.
                if (mouseY <= y + h / 2)
                    return slot;

                ++slot;
            }
        }

        y += h + 4;
    }

    return numSlots;
}

void PluginRackComponent::updateDropIndicator()
{
    if (dropIndicator == nullptr)
        return;

    const int numSlots = chainFor (dragPath).getNumSlots();
    const int target = juce::jlimit (0, numSlots, dropSlot);

    // Compute the vertical position of the drop gap.
    int slot = 0;
    int y = 4;

    for (auto* child : rowContainer.getChildren())
    {
        const int h = (dynamic_cast<const PathHeader*> (child) != nullptr) ? 30 : 62;

        if (auto* row = dynamic_cast<RackRow*> (child))
        {
            if (row->getPathIndex() == dragPath)
            {
                if (slot == target)
                {
                    dropIndicator->setBounds (8, y - 2, rowContainer.getWidth() - 16, 4);
                    return;
                }

                ++slot;
            }
        }

        y += h + 4;
    }

    dropIndicator->setBounds (8, y - 2, rowContainer.getWidth() - 16, 4);
}

//==============================================================================
PluginRackComponent::DropIndicator::DropIndicator()
{
    setInterceptsMouseClicks (false, false);
}

void PluginRackComponent::DropIndicator::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (aur::Theme::accent());
    g.fillRoundedRectangle (b, 2.0f);
}

//==============================================================================
void PluginRackComponent::openEditor (int pathIndex, int slotIndex)
{
    auto& chain = chainFor (pathIndex);
    auto* slot = chain.getSlot (slotIndex);

    if (slot == nullptr)
        return;

    // Re-show an already-open editor window instead of creating a new one.
    if (slot->editorWindow != nullptr)
    {
        slot->editorWindow->setVisible (true);
        slot->editorWindow->toFront (true);
        return;
    }

    auto* editor = slot->instance->createEditorIfNeeded();

    if (editor == nullptr)
    {
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon,
                                                "\u8BE5\u63D2\u4EF6",
                                                "\u8BE5\u63D2\u4EF6\u6CA1\u6709\u53EF\u7528\u7684\u56FE\u5F62\u754C\u9762\u3002");
        return;
    }

    auto* window = new PluginEditorWindow (slot->description.name,
                                           aur::Theme::bg(),
                                           juce::DocumentWindow::allButtons);

    window->setUsingNativeTitleBar (true);
    window->setContentOwned (editor, true);
    window->setResizable (true, true);
    window->setResizeLimits (220, 140, 4096, 4096);
    window->centreWithSize (juce::jmax (editor->getWidth(), 360),
                            juce::jmax (editor->getHeight(), 220));
    window->setVisible (true);

    slot->editorWindow.reset (window);
}

//==============================================================================
PluginRackComponent::PathHeader::PathHeader (PluginRackComponent& ownerRef, int pathIndex_)
    : owner (ownerRef), pathIndex (pathIndex_)
{
    const bool bus = isBusPath (pathIndex);

    static const juce::String pathNames[] = {
        juce::String (juce::CharPointer_UTF8 ("\u63D2\u5165 1")),
        juce::String (juce::CharPointer_UTF8 ("\u63D2\u5165 2")),
        juce::String (juce::CharPointer_UTF8 ("\u53D1\u9001"))
    };
    const juce::String pathName = (pathIndex >= 0 && pathIndex < 3)
                                      ? pathNames[pathIndex]
                                      : juce::String();

    title.setText (bus ? juce::String (juce::CharPointer_UTF8 ("\u603B\u7EBF"))
                       : pathName,
                   juce::dontSendNotification);
    title.setFont (aur::Theme::uiFont (12.5f).boldened());
    title.setColour (juce::Label::textColourId, aur::Theme::text());
    addAndMakeVisible (title);

    enable.setToggleState (bus ? owner.engine.isBusEnabled()
                               : owner.engine.isPathEnabled (pathIndex),
                           juce::dontSendNotification);
    enable.snapKnob();
    enable.onClick = [this, bus]
    {
        const bool newEnabled = enable.getToggleState();

        enable.startAnimation();

        if (bus)
            owner.engine.setBusEnabled (newEnabled);
        else
            owner.engine.setPathEnabled (pathIndex, newEnabled);

        // Keep the header's highlight/colouring in sync with the toggle.
        repaint();
        owner.rowContainer.repaint();
    };
    addAndMakeVisible (enable);

    volume.setRange (0.0, 2.0, 0.01);
    volume.setValue (bus ? owner.engine.getBusVolume()
                         : owner.engine.getPathVolume (pathIndex),
                     juce::dontSendNotification);
    volume.onValueChange = [this, bus]
    {
        if (bus)
            owner.engine.setBusVolume ((float) volume.getValue());
        else
            owner.engine.setPathVolume (pathIndex, (float) volume.getValue());
    };
    addAndMakeVisible (volume);

    addButton.setTooltip (bus ? "\u6D4B\u8BD5\u5E76\u6DFB\u52A0\u5230\u603B\u7EBF"
                              : "\u6D4F\u89C8\u5DF2\u626B\u63CF\u7684\u63D2\u4EF6\u5E76\u6DFB\u52A0\u5230\u8BE5\u8DEF\u5F84");
    addButton.onClick = [this]
    {
        owner.requestAddPlugin (pathIndex);
    };
    addAndMakeVisible (addButton);

    setInterceptsMouseClicks (true, true);
}

void PluginRackComponent::PathHeader::applyTheme()
{
    title.setColour (juce::Label::textColourId, aur::Theme::text());
    repaint();
}

void PluginRackComponent::PathHeader::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat().reduced (1.0f);

    const bool bus = isBusPath (pathIndex);
    const bool on = bus ? owner.engine.isBusEnabled()
                        : owner.engine.isPathEnabled (pathIndex);

    g.setColour (on ? aur::Theme::panelActive() : aur::Theme::panel());
    g.fillRoundedRectangle (b, 8.0f);

    g.setColour (on ? aur::Theme::accent() : aur::Theme::border());
    g.fillRoundedRectangle (b.withWidth (3.0f), 1.5f);
}

void PluginRackComponent::PathHeader::resized()
{
    auto b = getLocalBounds().reduced (8, 4);
    b.removeFromLeft (2);

    auto volArea = b.removeFromRight (b.getWidth() * 0.34);
    volume.setBounds (volArea.reduced (0, 6));

    auto toggleArea = b.removeFromRight (90);
    enable.setBounds (toggleArea.reduced (2, 6));

    auto addArea = b.removeFromRight (74);
    addButton.setBounds (addArea.reduced (4, 2));

    title.setBounds (b.reduced (0, 2));
}

//==============================================================================
PluginRackComponent::RackRow::RackRow (PluginRackComponent& ownerRef, int pathIndex_, int slotIndex_)
    : owner (ownerRef), pathIndex (pathIndex_), slotIndex (slotIndex_)
{
    auto& chain = owner.chainFor (pathIndex);
    auto* slot = chain.getSlot (slotIndex);
    jassert (slot != nullptr);

    bypass.setToggleState (slot != nullptr && slot->enabled, juce::dontSendNotification);
    bypass.snapKnob();
    bypass.onClick = [this]
    {
        const bool newEnabled = bypass.getToggleState();

        bypass.startAnimation();

        // Mark the next chain change as enable-only so the rows are repainted
        // in place (letting the knob animation play) instead of rebuilt.
        owner.skipEnableRebuildPath = pathIndex;
        owner.chainFor (pathIndex).setEnabled (slotIndex, newEnabled);
    };
    addAndMakeVisible (bypass);

    guiButton.onClick = [this]
    {
        owner.openEditor (pathIndex, slotIndex);
    };
    addAndMakeVisible (guiButton);

    removeButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xffff5f6b));
    removeButton.setColour (juce::TextButton::textColourOnId, juce::Colour (0xffff5f6b));

    removeButton.onClick = [this]
    {
        owner.chainFor (pathIndex).remove (slotIndex);
    };
    addAndMakeVisible (removeButton);

    setInterceptsMouseClicks (true, true);
}

void PluginRackComponent::RackRow::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat().reduced (1.0f);
    const auto* slot = owner.chainFor (pathIndex).getSlot (slotIndex);

    const bool pathOn = isBusPath (pathIndex) ? owner.engine.isBusEnabled()
                                              : owner.engine.isPathEnabled (pathIndex);
    const bool isEnabled = slot != nullptr && slot->enabled && pathOn;

    // Panel background.
    g.setColour (isEnabled ? aur::Theme::panelHover() : aur::Theme::panel());
    g.fillRoundedRectangle (b, 10.0f);

    // Bypass / enabled indicator.
    if (isEnabled)
    {
        g.setColour (aur::Theme::accent());
        g.fillRoundedRectangle (b.withWidth (3.0f), 1.5f);
    }
    else
    {
        g.setColour (aur::Theme::border());
        g.fillRoundedRectangle (b.withWidth (3.0f), 1.5f);
    }

    if (slot == nullptr)
        return;

    // Order chip.
    auto left = b.reduced (14, 0);
    auto chip = left.removeFromLeft (26.0f).withTrimmedTop (b.getHeight() * 0.5f - 11.0f);
    chip.setHeight (22.0f);
    g.setColour (isEnabled ? aur::Theme::accentSoft() : aur::Theme::panelActive());
    g.fillRoundedRectangle (chip, 6.0f);
    g.setColour (isEnabled ? aur::Theme::accent() : aur::Theme::textDim());
    g.setFont (aur::Theme::uiFont (11.0f).boldened());
    g.drawText (juce::String (slotIndex + 1), chip, juce::Justification::centred);

    // Name + meta.
    auto text = left.reduced (6, 0).withTrimmedRight (6);
    auto nameBox = text.removeFromTop (text.getHeight() * 0.55f);
    auto metaBox = text;

    const auto nameColour = isEnabled ? aur::Theme::text() : aur::Theme::textDim();

    g.setColour (nameColour);
    g.setFont (aur::Theme::uiFont (13.5f));
    g.drawText (shorten (slot->description.name, 26), nameBox.reduced (0, 2),
                juce::Justification::centredLeft);

    g.setColour (aur::Theme::textDim());
    g.setFont (aur::Theme::uiFont (11.0f));
    const auto vendor = shorten (slot->description.manufacturerName, 20);
    g.drawText (vendor.isEmpty() ? slot->description.pluginFormatName
                                 : vendor + " \u00B7 " + slot->description.pluginFormatName,
                metaBox.reduced (0, 2), juce::Justification::centredLeft);
}

void PluginRackComponent::RackRow::resized()
{
    auto b = getLocalBounds().reduced (14, 8);

    // Reserve space for the left chip.
    b.removeFromLeft (26);

    auto buttons = b.removeFromRight (210).reduced (0, 6);

    removeButton.setBounds (buttons.removeFromRight (32).reduced (4, 8));
    guiButton.setBounds (buttons.removeFromRight (56).reduced (4, 10));
    bypass.setBounds (buttons.reduced (2, 8));
}
