#include <JuceHeader.h>

#include "Trace.h"
#include "UI/CustomLookAndFeel.h"
#include "UI/MainComponent.h"

//==============================================================================
class JokerPlayerApplication : public juce::JUCEApplication
{
public:
    JokerPlayerApplication() = default;

    const juce::String getApplicationName() override { return ProjectInfo::projectName; }
    const juce::String getApplicationVersion() override { return ProjectInfo::versionString; }
    bool moreThanOneInstanceAllowed() override { return false; }

    void initialise (const juce::String& /*commandLine*/) override
    {
        aur::traceStep ("initialise start");
        aur::Theme::loadSavedMode();
        juce::LookAndFeel::setDefaultLookAndFeel (&aur::CustomLookAndFeel::instance());
        aur::traceStep ("default LAF set");
        mainWindow.reset (new MainWindow());
        aur::traceStep ("main window created");
    }

    void shutdown() override
    {
        mainWindow.reset();
        juce::LookAndFeel::setDefaultLookAndFeel (nullptr);
    }

    void systemRequestedQuit() override { quit(); }
    void anotherInstanceStarted (const juce::String&) override {}

private:
    class MainWindow : public juce::DocumentWindow
    {
    public:
        MainWindow()
            : DocumentWindow ("Joker Player",
                              aur::Theme::bg(),
                              DocumentWindow::allButtons)
        {
            aur::traceStep ("MainWindow ctor body");
            setUsingNativeTitleBar (false);
            setTitleBarHeight (36);
            aur::traceStep ("custom title bar");

            // App icon, embedded as a binary resource (from "Joker Player.png").
            auto iconImage = juce::ImageCache::getFromMemory (BinaryData::app_icon_png,
                                                              BinaryData::app_icon_pngSize);
            if (iconImage.isValid())
                setIcon (iconImage);

            setContentOwned (new MainComponent(), true);
            aur::traceStep ("content owned");

            themeButton.setTooltip (juce::String (juce::CharPointer_UTF8 ("切换日间 / 夜间模式")));
            themeButton.onClick = [this]
            {
                aur::Theme::setMode (aur::Theme::getMode() == aur::Theme::Mode::Night
                                         ? aur::Theme::Mode::Day
                                         : aur::Theme::Mode::Night);
                aur::Theme::saveMode();
                themeButton.setTheme (aur::Theme::getMode());
                if (auto* mc = dynamic_cast<MainComponent*> (getContentComponent()))
                    mc->applyTheme();
                repaint();
            };
            themeButton.setTheme (aur::Theme::getMode());
            addAndMakeVisible (themeButton);

            centreWithSize (1180, 760);
            setResizable (true, true);
            setResizeLimits (980, 640, 2560, 1600);
            setVisible (true);

            // Also apply the icon to the OS window / taskbar.
            if (auto* peer = getPeer())
                peer->setIcon (iconImage);
        }

        void resized() override
        {
            juce::DocumentWindow::resized();

            // Theme toggle sits on the title bar, just left of the window buttons.
            if (auto* minButton = getMinimiseButton())
            {
                const int buttonW = 30;
                const int gap = 2;
                themeButton.setBounds (minButton->getX() - buttonW - gap,
                                       minButton->getY(), buttonW, minButton->getHeight());
            }
        }

        void closeButtonPressed() override
        {
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
        }

    private:
        MainComponent::ThemeButton themeButton;
    };

    std::unique_ptr<MainWindow> mainWindow;
};

//==============================================================================
START_JUCE_APPLICATION (JokerPlayerApplication)
