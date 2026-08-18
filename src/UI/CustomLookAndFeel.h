#pragma once

#include <JuceHeader.h>

//==============================================================================
namespace aur
{
/** Central colour palette — a modern, dark, rounded theme. */
struct Theme
{
    enum class Mode { Night, Day };

    static Mode getMode() { return mode; }
    static void setMode (Mode m) { mode = m; }

    /** Loads the saved theme mode from disk (applied automatically at startup). */
    static void loadSavedMode();

    /** Persists the current theme mode so it survives restarts. */
    static void saveMode();

    static const juce::Colour& bg()
    {
        static const juce::Colour night (0xff161030);
        static const juce::Colour day (0xfff7f3fb);
        return (mode == Mode::Day) ? day : night;
    }
    static const juce::Colour& bgTop()
    {
        static const juce::Colour night (0xff221a44);
        static const juce::Colour day (0xfff6f8ff);
        return (mode == Mode::Day) ? day : night;
    }
    static const juce::Colour& panel()
    {
        static const juce::Colour night (0xff1e1640);
        static const juce::Colour day (0xfff2edf9);
        return (mode == Mode::Day) ? day : night;
    }
    static const juce::Colour& panelHover()
    {
        static const juce::Colour night (0xff2b2050);
        static const juce::Colour day (0xffe8e2f6);
        return (mode == Mode::Day) ? day : night;
    }
    static const juce::Colour& panelActive()
    {
        static const juce::Colour night (0xff382a66);
        static const juce::Colour day (0xffd9cff2);
        return (mode == Mode::Day) ? day : night;
    }
    static const juce::Colour& text()
    {
        static const juce::Colour night (0xffeef1fa);
        static const juce::Colour day (0xff2d2840);
        return (mode == Mode::Day) ? day : night;
    }
    static const juce::Colour& textDim()
    {
        static const juce::Colour night (0xff9aa6c4);
        static const juce::Colour day (0xff7a7391);
        return (mode == Mode::Day) ? day : night;
    }
    static const juce::Colour& accent()
    {
        static const juce::Colour night (0xff4d8dff);
        static const juce::Colour day (0xff6b5bff);
        return (mode == Mode::Day) ? day : night;
    }
    static const juce::Colour& accent2()
    {
        static const juce::Colour night (0xffb45cff);
        static const juce::Colour day (0xffff7ab0);
        return (mode == Mode::Day) ? day : night;
    }
    static const juce::Colour& accentLight()
    {
        static const juce::Colour night (0xffa85cff);
        static const juce::Colour day (0xffb9a9ff);
        return (mode == Mode::Day) ? day : night;
    }
    static const juce::Colour& accentSoft()
    {
        static const juce::Colour night (0x504d8dff);
        static const juce::Colour day (0x336b5bff);
        return (mode == Mode::Day) ? day : night;
    }
    static const juce::Colour& good()
    {
        static const juce::Colour night (0xff3ce8a5);
        static const juce::Colour day (0xff2fb877);
        return (mode == Mode::Day) ? day : night;
    }
    static const juce::Colour& warn()
    {
        static const juce::Colour night (0xffffb15c);
        static const juce::Colour day (0xffffa040);
        return (mode == Mode::Day) ? day : night;
    }
    static const juce::Colour& border()
    {
        static const juce::Colour night (0xff3a4564);
        static const juce::Colour day (0xffcfc6e6);
        return (mode == Mode::Day) ? day : night;
    }
    static const juce::Colour& inputBg()
    {
        static const juce::Colour night (0xff16213c);
        static const juce::Colour day (0xffffffff);
        return (mode == Mode::Day) ? day : night;
    }

    /** Returns the UI typeface (with CJK support where available). */
    static juce::Font uiFont (float height)
    {
        constexpr float scale = 1.2f;
        return juce::Font ("Microsoft YaHei UI", height * scale, juce::Font::plain);
    }

    /** A smooth diagonal gradient for the main window background. */
    static juce::ColourGradient windowBackgroundGradient (const juce::Rectangle<float>& area)
    {
        if (mode == Mode::Day)
        {
            juce::ColourGradient g (juce::Colour (0xffe9dbff), area.getTopLeft(),
                                    juce::Colour (0xffffe0f0), area.getBottomRight(), false);
            g.addColour (0.5f, juce::Colour (0xffd8e8ff));
            return g;
        }

        // Night: deep indigo fading into a deep crimson (绛红) glow near the
        // bottom-right, blending smoothly so no near-black corners remain.
        juce::ColourGradient g (juce::Colour (0xff2a2154), area.getTopLeft(),
                                juce::Colour (0xff4a1530), area.getBottomRight(), false);
        g.addColour (0.45f, juce::Colour (0xff251a4b));
        g.addColour (0.8f,  juce::Colour (0xff3a1634));
        return g;
    }

    /** A smooth vertical gradient for panel surfaces (lighter at top, deeper at bottom). */
    static juce::ColourGradient panelGradient (const juce::Rectangle<float>& area)
    {
        if (mode == Mode::Day)
        {
            juce::ColourGradient g (juce::Colour (0xffffffff), area.getTopLeft(),
                                    juce::Colour (0xfff3ecff), area.getBottomLeft(), false);
            g.addColour (0.6f, juce::Colour (0xfffff1f8));
            return g;
        }

        // Night: deep blue panels with a smooth vertical gradient, lighter at the
        // top and deepening downward, keeping the surfaces readable.
        juce::ColourGradient g (juce::Colour (0xff24345e), area.getTopLeft(),
                                juce::Colour (0xff141f3f), area.getBottomLeft(), false);
        g.addColour (0.5f, juce::Colour (0xff1c2a4e));
        return g;
    }

    static Mode mode;
};

/** A custom look-and-feel providing the Joker visual style. */
class CustomLookAndFeel : public juce::LookAndFeel_V4
{
public:
    CustomLookAndFeel();

    static CustomLookAndFeel& instance();

    /** Re-applies the current theme colours to the LAF colour scheme
        (call after aur::Theme::setMode). */
    void refreshScheme();

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
    juce::Button* createDocumentWindowButton (int buttonType) override;
    void positionDocumentWindowButtons (juce::DocumentWindow&,
                                        int titleBarX, int titleBarY,
                                        int titleBarW, int titleBarH,
                                        juce::Button* minimiseButton,
                                        juce::Button* maximiseButton,
                                        juce::Button* closeButton,
                                        bool positionTitleBarButtonsOnLeft) override;
    void drawDocumentWindowTitleBar (juce::DocumentWindow&, juce::Graphics&,
                                     int w, int h, int titleSpaceX, int titleSpaceW,
                                     const juce::Image* icon, bool drawTitleTextOnLeft) override;

    //==========================================================================
    static juce::Colour transparentWithAlpha (const juce::Colour& c, float alpha);

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CustomLookAndFeel)
};

} // namespace aur