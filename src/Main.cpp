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
            setUsingNativeTitleBar (true);
            aur::traceStep ("native title bar");
            setContentOwned (new MainComponent(), true);
            aur::traceStep ("content owned");
            centreWithSize (1180, 760);
            setResizable (true, true);
            setResizeLimits (980, 640, 2560, 1600);
            setVisible (true);
        }

        void closeButtonPressed() override
        {
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
        }
    };

    std::unique_ptr<MainWindow> mainWindow;
};

//==============================================================================
START_JUCE_APPLICATION (JokerPlayerApplication)
