#pragma once

#include <JuceHeader.h>

//==============================================================================
namespace aur
{
/** Central colour palette — a modern, dark, rounded theme. */
struct Theme
{
    static const juce::Colour& bg()        { static const juce::Colour c (0xff10131a); return c; }
    static const juce::Colour& bgTop()     { static const juce::Colour c (0xff1a1f2a); return c; }
    static const juce::Colour& panel()     { static const juce::Colour c (0xff171b25); return c; }
    static const juce::Colour& panelHover(){ static const juce::Colour c (0xff202636); return c; }
    static const juce::Colour& panelActive(){ static const juce::Colour c (0xff273043); return c; }
    static const juce::Colour& text()      { static const juce::Colour c (0xffe9edf4); return c; }
    static const juce::Colour& textDim()   { static const juce::Colour c (0xff8b94a8); return c; }
    static const juce::Colour& accent()    { static const juce::Colour c (0xff5e8cff); return c; }
    static const juce::Colour& accent2()   { static const juce::Colour c (0xff8a5cff); return c; }
    static const juce::Colour& accentSoft(){ static const juce::Colour c (0x405e8cff); return c; }
    static const juce::Colour& good()      { static const juce::Colour c (0xff38d08c); return c; }
    static const juce::Colour& warn()      { static const juce::Colour c (0xffffa94d); return c; }
    static const juce::Colour& border()    { static const juce::Colour c (0xff2a3140); return c; }
    static const juce::Colour& inputBg()   { static const juce::Colour c (0xff0e1116); return c; }

    /** Returns the UI typeface (with CJK support where available). */
    static juce::Font uiFont (float height)
    {
        constexpr float scale = 1.2f;
        return juce::Font ("Microsoft YaHei UI", height * scale, juce::Font::plain);
    }
};

/** A custom look-and-feel providing the Joker visual style. */
class CustomLookAndFeel : public juce::LookAndFeel_V4
{
public:
    CustomLookAndFeel();

    static CustomLookAndFeel& instance();

    //==========================================================================
    void drawButtonBackground (juce::Graphics&, juce::Button&,
                               const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted,
                               bool shouldDrawButtonAsDown) override;

    void drawToggleButton (juce::Graphics&, juce::ToggleButton&,
                           bool shouldDrawButtonAsHighlighted,
                           bool shouldDrawButtonAsDown) override;

    void drawComboBox (juce::Graphics&, int width, int height, bool isDown,
                       int buttonX, int buttonY, int buttonW, int buttonH,
                       juce::ComboBox&) override;

    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;

    juce::Label* createComboBoxTextBox (juce::ComboBox&) override;

    void drawPopupMenuBackground (juce::Graphics&, int width, int height) override;

    void drawScrollbar (juce::Graphics&, juce::ScrollBar&,
                        int x, int y, int w, int h,
                        bool isVertical, int thumbStart, int thumbSize,
                        bool isMouseOver, bool isMouseDown) override;

    void drawTextEditorOutline (juce::Graphics&, int width, int height,
                                juce::TextEditor&) override;
    void fillTextEditorBackground (juce::Graphics&, int width, int height,
                                   juce::TextEditor&) override;

    void drawLinearSlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           const juce::Slider::SliderStyle, juce::Slider&) override;

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider&) override;

    void drawTableHeaderBackground (juce::Graphics&, juce::TableHeaderComponent&) override;
    void drawTableHeaderColumn (juce::Graphics&, juce::TableHeaderComponent&,
                                const juce::String& columnName, int columnId,
                                int width, int height, bool isMouseOver,
                                bool isMouseDown, int columnFlags) override;

    void drawAlertBox (juce::Graphics&, juce::AlertWindow&, const juce::Rectangle<int>&,
                       juce::TextLayout&) override;

    //==========================================================================
    static juce::Colour transparentWithAlpha (const juce::Colour& c, float alpha);

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CustomLookAndFeel)
};

} // namespace aur