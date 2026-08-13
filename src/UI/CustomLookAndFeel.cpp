#include "CustomLookAndFeel.h"
#include "../Trace.h"

//==============================================================================
namespace
{
juce::Colour mixWithBg (const juce::Colour& c, float amount)
{
    return c.interpolatedWith (aur::Theme::panel(), amount);
}
} // namespace

//==============================================================================
aur::CustomLookAndFeel::CustomLookAndFeel()
{
    aur::traceStep ("CustomLookAndFeel ctor start");
    // Ensure every widget falls back to a CJK-capable typeface (@see uiFont).
    setDefaultSansSerifTypefaceName ("Microsoft YaHei UI");

    auto& scheme = getCurrentColourScheme();

    scheme.setUIColour (juce::LookAndFeel_V4::ColourScheme::outline, aur::Theme::border());
    scheme.setUIColour (juce::LookAndFeel_V4::ColourScheme::defaultText, aur::Theme::text());
    scheme.setUIColour (juce::LookAndFeel_V4::ColourScheme::defaultFill, aur::Theme::panel());
    scheme.setUIColour (juce::LookAndFeel_V4::ColourScheme::highlightedFill, aur::Theme::accent());
    scheme.setUIColour (juce::LookAndFeel_V4::ColourScheme::highlightedText, juce::Colour (0xffffffff));

    setColour (juce::AlertWindow::backgroundColourId, aur::Theme::panel());
    setColour (juce::AlertWindow::textColourId, aur::Theme::text());
    setColour (juce::AlertWindow::outlineColourId, aur::Theme::border());

    setColour (juce::ScrollBar::thumbColourId, aur::Theme::panelActive());
}

aur::CustomLookAndFeel& aur::CustomLookAndFeel::instance()
{
    static aur::CustomLookAndFeel instance;
    return instance;
}

//==============================================================================
void aur::CustomLookAndFeel::drawButtonBackground (juce::Graphics& g,
                                              juce::Button& button,
                                              const juce::Colour& /*backgroundColour*/,
                                              bool shouldDrawButtonAsHighlighted,
                                              bool shouldDrawButtonAsDown)
{
    const auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
    const auto corner = juce::jmin (8.0f, bounds.getHeight() * 0.5f);

    juce::Colour base = button.isEnabled() ? juce::Colour (0xff222837) : juce::Colour (0xff1c212c);

    if (button.isEnabled())
    {
        if (button.getToggleState())
            base = mixWithBg (aur::Theme::accent(), 0.05f);
        else if (shouldDrawButtonAsDown)
            base = aur::Theme::panelActive();
        else if (shouldDrawButtonAsHighlighted)
            base = aur::Theme::panelHover();
    }

    const bool toggled = button.getToggleState();

    if (toggled)
    {
        g.setColour (aur::Theme::accent());
        g.drawRoundedRectangle (bounds, corner, 1.2f);
    }

    g.setColour (base);
    g.fillRoundedRectangle (bounds.reduced (toggled ? 1.0f : 0.0f), corner);

    if (button.isEnabled() && ! toggled)
    {
        const juce::Colour border = shouldDrawButtonAsHighlighted
                                        ? juce::Colour (0xff3a4360)
                                        : aur::Theme::border();

        g.setColour (border);
        g.drawRoundedRectangle (bounds, corner, 1.0f);
    }
}

//==============================================================================
void aur::CustomLookAndFeel::drawToggleButton (juce::Graphics& g,
                                          juce::ToggleButton& button,
                                          bool shouldDrawButtonAsHighlighted,
                                          bool shouldDrawButtonAsDown)
{
    juce::ignoreUnused (shouldDrawButtonAsDown);

    const float h = (float) button.getHeight();
    const auto bounds = button.getLocalBounds().toFloat();
    const bool on = button.getToggleState();

    if (button.getHeight() >= 22 && button.getWidth() >= 56 && button.getButtonText().isNotEmpty())
    {
        // Pill switch (e.g. bypass).
        const float pillH = 20.0f;
        const float pillW = 40.0f;
        const auto pill = juce::Rectangle<float> (8.0f, (h - pillH) * 0.5f, pillW, pillH);

        g.setColour (on ? juce::Colour (0xff2f57b8) : juce::Colour (0xff2a3040));
        g.fillRoundedRectangle (pill, pillH * 0.5f);
        g.setColour (on ? aur::Theme::accent() : aur::Theme::border());
        g.drawRoundedRectangle (pill, pillH * 0.5f, 1.0f);

        const float knobX = on ? pill.getRight() - pillH : pill.getX();
        g.setColour (on ? juce::Colours::white : aur::Theme::textDim());
        g.fillEllipse (knobX - 4.0f, pill.getCentreY() - 8.0f, 16.0f, 16.0f);

        g.setColour (on ? juce::Colours::white : aur::Theme::textDim());
        g.setFont (aur::Theme::uiFont (12.0f));
        g.drawText (button.getButtonText(), juce::Rectangle<float> (pill.getRight() + 6.0f, 0, bounds.getWidth() - pill.getRight() - 6.0f, h),
                    juce::Justification::centredLeft);

        juce::ignoreUnused (shouldDrawButtonAsHighlighted);
        return;
    }

    // Plain toggle indicator.
    const float size = juce::jmin (16.0f, h * 0.6f);
    auto box = juce::Rectangle<float> (4.0f, (h - size) * 0.5f, size, size);

    g.setColour (on ? aur::Theme::accent() : aur::Theme::panelActive());
    g.fillRoundedRectangle (box, 4.0f);
    g.setColour (on ? aur::Theme::accent().brighter (0.15f) : aur::Theme::border());
    g.drawRoundedRectangle (box, 4.0f, 1.0f);

    if (on)
    {
        g.setColour (juce::Colours::white);
        g.drawText ("\u2713", box, juce::Justification::centred);
    }
}

//==============================================================================
void aur::CustomLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height,
                                      bool isDown, int buttonX, int buttonY,
                                      int buttonW, int buttonH, juce::ComboBox& box)
{
    juce::ignoreUnused (isDown, buttonX, buttonY, buttonW, buttonH);

    const auto bounds = juce::Rectangle<int> (0, 0, width, height).reduced (0, 2).toFloat();
    const float corner = 6.0f;

    g.setColour (aur::Theme::inputBg());
    g.fillRoundedRectangle (bounds, corner);
    g.setColour (aur::Theme::border());
    g.drawRoundedRectangle (bounds, corner, 1.0f);

    if (box.isEnabled())
    {
        g.setColour (aur::Theme::textDim());
        const float cx = (float) width - 22.0f;
        const float cy = (float) height * 0.5f;
        juce::Path arrow;
        arrow.addTriangle (cx - 5.0f, cy - 2.0f, cx + 5.0f, cy - 2.0f, cx, cy + 4.0f);
        g.fillPath (arrow);
    }
}

juce::Label* aur::CustomLookAndFeel::createComboBoxTextBox (juce::ComboBox&)
{
    auto* label = new juce::Label ({}, {});
    label->setJustificationType (juce::Justification::centredLeft);
    label->setInterceptsMouseClicks (false, false);
    return label;
}

juce::Font aur::CustomLookAndFeel::getTextButtonFont (juce::TextButton& button, int buttonHeight)
{
    const auto text = button.getButtonText();

    if (text == "插件管理")
        return aur::Theme::uiFont ((float) buttonHeight * 0.45f);

    if (text == "界面")
        return aur::Theme::uiFont (15.0f);

    if (text == "扫描选项")
        return aur::Theme::uiFont ((float) buttonHeight * 0.65f);

    if (text == "＋ 添加所选插件")
        return aur::Theme::uiFont ((float) buttonHeight * 0.35f);

    if (text == "添加插件")
        return aur::Theme::uiFont (13.0f);

    if (text == "×")
        return aur::Theme::uiFont (13.0f);

    return aur::Theme::uiFont ((float) buttonHeight * 0.55f);
}

juce::Font aur::CustomLookAndFeel::getComboBoxFont (juce::ComboBox& box)
{
    return aur::Theme::uiFont ((float) box.getHeight() * 0.5f);
}

//==============================================================================
void aur::CustomLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int width, int height)
{
    const auto r = juce::Rectangle<int> (width, height).toFloat();

    g.setColour (juce::Colour (0xee20242e));
    g.fillRoundedRectangle (r.reduced (2.0f), 8.0f);

    g.setColour (aur::Theme::border());
    g.drawRoundedRectangle (r.reduced (2.0f), 8.0f, 1.0f);
}

//==============================================================================
void aur::CustomLookAndFeel::drawScrollbar (juce::Graphics& g, juce::ScrollBar&, int x, int y, int w, int h,
                                       bool isVertical, int thumbStart, int thumbSize,
                                       bool isMouseOver, bool isMouseDown)
{
    juce::ignoreUnused (isMouseDown);

    if (thumbSize <= 0)
        return;

    const auto thumbCol = isMouseOver ? aur::Theme::accent().withAlpha (0.7f)
                                      : aur::Theme::panelActive();

    g.setColour (thumbCol);
    if (isVertical)
        g.fillRoundedRectangle ((float) x + 2.0f, (float) y + thumbStart,
                                (float) w - 4.0f, (float) thumbSize, 3.0f);
    else
        g.fillRoundedRectangle ((float) x + thumbStart, (float) y + 2.0f,
                                (float) thumbSize, (float) h - 4.0f, 3.0f);
}

//==============================================================================
void aur::CustomLookAndFeel::fillTextEditorBackground (juce::Graphics& g, int width, int height,
                                                  juce::TextEditor& ed)
{
    juce::ignoreUnused (ed);
    g.setColour (aur::Theme::inputBg());
    g.fillRoundedRectangle (0.0f, 0.0f, (float) width, (float) height, 6.0f);
}

void aur::CustomLookAndFeel::drawTextEditorOutline (juce::Graphics& g, int width, int height,
                                               juce::TextEditor& ed)
{
    const auto hasFocus = ed.hasKeyboardFocus (true);
    g.setColour (hasFocus ? aur::Theme::accent() : aur::Theme::border());
    g.drawRoundedRectangle (0.5f, 0.5f, (float) width - 1.0f, (float) height - 1.0f, 6.0f, 1.0f);
}

//==============================================================================
void aur::CustomLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y,
                                          int width, int height, float sliderPos,
                                          float minSliderPos, float maxSliderPos,
                                          const juce::Slider::SliderStyle style,
                                          juce::Slider& slider)
{
    juce::ignoreUnused (minSliderPos, maxSliderPos);

    const float trackThickness = 4.0f;
    const float thumbRadius = 6.0f;

    if (style == juce::Slider::LinearVertical)
    {
        const float trackX = x + width * 0.5f - trackThickness * 0.5f;
        juce::Rectangle<float> track (trackX, y, trackThickness, height);
        g.setColour (aur::Theme::border());
        g.fillRoundedRectangle (track, trackThickness * 0.5f);

        const float thumbY = sliderPos;
        const juce::Rectangle<float> fill (trackX, thumbY, trackThickness, (float) y + height - thumbY);
        g.setColour (aur::Theme::accent());
        g.fillRoundedRectangle (fill, trackThickness * 0.5f);

        g.setColour (slider.isMouseOverOrDragging() ? juce::Colours::white : aur::Theme::accent());
        g.fillEllipse (x + width * 0.5f - thumbRadius, thumbY - thumbRadius, thumbRadius * 2, thumbRadius * 2);
    }
    else
    {
        const float trackY = y + height * 0.5f - trackThickness * 0.5f;
        juce::Rectangle<float> track (x, trackY, width, trackThickness);
        g.setColour (aur::Theme::border());
        g.fillRoundedRectangle (track, trackThickness * 0.5f);

        const float pos = juce::jlimit ((float) x, (float) x + width, sliderPos);

        juce::ColourGradient grad (aur::Theme::accent(), (float) x, 0.0f,
                                   aur::Theme::accent2(), (float) (x + width), 0.0f, false);
        juce::Rectangle<float> fill (x, trackY, pos - x, trackThickness);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (fill, trackThickness * 0.5f);

        const float thumbX = pos;
        g.setColour (juce::Colours::white.withAlpha (slider.isMouseOverOrDragging() ? 1.0f : 0.9f));
        g.fillEllipse (thumbX - thumbRadius, trackY + trackThickness * 0.5f - thumbRadius,
                       thumbRadius * 2, thumbRadius * 2);
    }
}

//==============================================================================
void aur::CustomLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y,
                                          int width, int height,
                                          float sliderPosProportional,
                                          float rotaryStartAngle, float rotaryEndAngle,
                                          juce::Slider& slider)
{
    const auto area = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (4.0f);
    const auto centre = area.getCentre();
    const float radius = juce::jmin (area.getWidth(), area.getHeight()) * 0.5f;

    // Background arc.
    juce::Path bgArc;
    bgArc.addArc (area.getX(), area.getY(), area.getWidth(), area.getHeight(),
                  rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (aur::Theme::border());
    g.strokePath (bgArc, juce::PathStrokeType (4.0f));

    // Value arc with gradient.
    const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    juce::Path valueArc;
    valueArc.addArc (area.getX(), area.getY(), area.getWidth(), area.getHeight(),
                     rotaryStartAngle, angle, true);

    juce::ColourGradient grad (aur::Theme::accent(), centre.translated (-radius, 0.0f),
                               aur::Theme::accent2(), centre.translated (radius, 0.0f), false);
    g.setGradientFill (grad);
    g.strokePath (valueArc, juce::PathStrokeType (4.0f));

    // Pointer.
    const float angle2 = slider.getProperties().contains ("pointerAngle")
                             ? (float) slider.getProperties() ["pointerAngle"]
                             : angle;
    juce::ignoreUnused (angle2);

    juce::Path pointer;
    pointer.startNewSubPath (centre);
    pointer.lineTo (centre.translated (std::cos (angle), std::sin (angle)) * (float) (radius * 0.65));
    pointer.closeSubPath();

    const float th = juce::jmax (1.0f, radius * 0.10f);
    g.setColour (aur::Theme::text());
    g.strokePath (pointer, juce::PathStrokeType (th, juce::PathStrokeType::curved));

    g.setColour (slider.isMouseOverOrDragging() ? juce::Colours::white : aur::Theme::textDim());
    g.fillEllipse (centre.getX() - th * 0.5f, centre.getY() - th * 0.5f, th, th);
}

//==============================================================================
void aur::CustomLookAndFeel::drawTableHeaderBackground (juce::Graphics& g,
                                                   juce::TableHeaderComponent&)
{
    g.fillAll (aur::Theme::panelHover());
}

void aur::CustomLookAndFeel::drawTableHeaderColumn (juce::Graphics& g,
                                               juce::TableHeaderComponent&,
                                               const juce::String& columnName,
                                               int /*columnId*/,
                                               int width, int height,
                                               bool isMouseOver,
                                               bool /*isMouseDown*/,
                                               int columnFlags)
{
    juce::ignoreUnused (isMouseOver);

    auto cellArea = juce::Rectangle<int> (width, height);

    g.saveState();
    g.setFont (aur::Theme::uiFont (12.0f));
    g.setColour (aur::Theme::textDim());
    g.drawText (columnName, cellArea.reduced (8, 0), juce::Justification::centredLeft);

    if ((columnFlags & juce::TableHeaderComponent::sortedForwards)
        || (columnFlags & juce::TableHeaderComponent::sortedBackwards))
    {
        g.setColour (aur::Theme::accent());
        const auto r = cellArea.removeFromRight (16).reduced (4);
        juce::Path arrow;
        const bool forwards = (columnFlags & juce::TableHeaderComponent::sortedForwards) != 0;
        if (forwards)
            arrow.addTriangle (r.getX(), r.getBottom() - 3, r.getRight(), r.getBottom() - 3,
                               r.getCentreX(), r.getY() + 2);
        else
            arrow.addTriangle (r.getX(), r.getY() + 3, r.getRight(), r.getY() + 3,
                               r.getCentreX(), r.getBottom() - 2);
        g.fillPath (arrow);
    }

    g.setColour (aur::Theme::border());
    g.fillRect (cellArea.removeFromRight (1));
    g.restoreState();
}

//==============================================================================
void aur::CustomLookAndFeel::drawAlertBox (juce::Graphics& g, juce::AlertWindow& alert,
                                      const juce::Rectangle<int>& textArea,
                                      juce::TextLayout& textLayout)
{
    auto bounds = alert.getLocalBounds().toFloat();
    g.setColour (aur::Theme::panel());
    g.fillRoundedRectangle (bounds, 8.0f);
    g.setColour (aur::Theme::border());
    g.drawRoundedRectangle (bounds, 8.0f, 1.0f);

    juce::ignoreUnused (textArea, textLayout);
}

//==============================================================================
juce::Colour aur::CustomLookAndFeel::transparentWithAlpha (const juce::Colour& c, float alpha)
{
    return c.withAlpha (alpha);
}
