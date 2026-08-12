#include "PluginBrowserDialog.h"
#include "CustomLookAndFeel.h"
#include "../Player/PlayerEngine.h"

//==============================================================================
namespace
{
constexpr int kTopButtonsHeight = 44;
constexpr int kBottomBarHeight = 52;
} // namespace

//==============================================================================
PluginBrowserDialog::PluginBrowserDialog (PlayerEngine& engineRef,
                                          std::function<void (const juce::PluginDescription&)> onAdd,
                                          std::function<int()> getTargetPath)
    : DocumentWindow ("Joker Player · 插件管理器", aur::Theme::bg(),
                      DocumentWindow::closeButton, true),
      engine (engineRef)
{
    setUsingNativeTitleBar (true);

    auto* content = new ContentComponent (*this, engine, std::move (onAdd), std::move (getTargetPath));
    setContentOwned (content, true);

    setResizable (true, true);
    setResizeLimits (460, 520, 960, 960);
    centreWithSize (720, 640);
    setVisible (true);
}

PluginBrowserDialog::~PluginBrowserDialog() = default;

void PluginBrowserDialog::closeButtonPressed()
{
    engine.getPluginManager().saveCache();

    if (onClose)
        onClose();
}

//==============================================================================
PluginBrowserDialog::ContentComponent::ContentComponent (
    PluginBrowserDialog& ownerRef,
    PlayerEngine& engineRef,
    std::function<void (const juce::PluginDescription&)> onAddCallback,
    std::function<int()> getTargetPathCallback)
    : owner (ownerRef), engine (engineRef), onAdd (std::move (onAddCallback)),
      getTargetPath (std::move (getTargetPathCallback))
{
    title.setFont (aur::Theme::uiFont (16.0f).boldened());
    title.setColour (juce::Label::textColourId, aur::Theme::text());
    addAndMakeVisible (title);

    // -- properties file for remembering scan paths --------------------------
    juce::PropertiesFile::Options propsOptions;
    propsOptions.applicationName = "JokerPlayer";
    propsOptions.filenameSuffix = ".settings";
    propsOptions.osxLibrarySubFolder = "Application Support";
    properties = std::make_unique<juce::PropertiesFile> (propsOptions);

    // -- plug-in list component ----------------------------------------------
    pluginList = std::make_unique<juce::PluginListComponent> (
        engine.getPluginManager().getFormatManager(),
        engine.getPluginManager().getKnownPlugins(),
        engine.getPluginManager().getDataDirectory().getChildFile ("DeadMansPedal.txt"),
        properties.get());

    if (auto* vst3 = findVst3Format (engine.getPluginManager().getFormatManager()))
    {
        auto searchPath = juce::PluginListComponent::getLastSearchPath (*properties, *vst3);
        for (auto& dir : getDefaultScanFolders())
            searchPath.addIfNotAlreadyThere (juce::File (dir));

        juce::PluginListComponent::setLastSearchPath (*properties, *vst3, searchPath);
    }

    pluginList->setOptionsButtonText ("扫描选项");
    pluginList->setScanDialogText ("扫描插件", "正在扫描插件，请稍候…");
    pluginList->setNumberOfThreadsForScanning (1);
    addAndMakeVisible (pluginList.get());

    // -- buttons -------------------------------------------------------------
    scanDefaultButton.onClick = [this] { scanDefaultFolders(); };
    addAndMakeVisible (scanDefaultButton);

    browseButton.onClick = [this] { chooseAndScanFolder(); };
    addAndMakeVisible (browseButton);

    addSelectedButton.onClick = [this] { addSelectedPlugin(); };
    addAndMakeVisible (addSelectedButton);

    doneButton.onClick = [this]
    {
        engine.getPluginManager().saveCache();
        owner.closeButtonPressed();
    };
    addAndMakeVisible (doneButton);

    statusLabel.setFont (aur::Theme::uiFont (12.0f));
    statusLabel.setColour (juce::Label::textColourId, aur::Theme::textDim());
    statusLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (statusLabel);
}

PluginBrowserDialog::ContentComponent::~ContentComponent() = default;

//==============================================================================
void PluginBrowserDialog::ContentComponent::paint (juce::Graphics& g)
{
    g.fillAll (aur::Theme::bg());
}

void PluginBrowserDialog::ContentComponent::resized()
{
    auto b = getLocalBounds().reduced (12);

    auto titleArea = b.removeFromTop (30);
    title.setBounds (titleArea);

    auto buttons = b.removeFromTop (kTopButtonsHeight).reduced (0, 2);
    scanDefaultButton.setBounds (buttons.removeFromLeft (110).reduced (2, 6));
    browseButton.setBounds (buttons.removeFromLeft (110).reduced (2, 6));
    buttons.removeFromLeft (buttons.getWidth() * 0.3f);
    doneButton.setBounds (buttons.reduced (2, 6));

    auto bottom = b.removeFromBottom (kBottomBarHeight);
    statusLabel.setBounds (bottom.removeFromLeft (bottom.getWidth() * 0.55f).reduced (2, 8));
    addSelectedButton.setBounds (bottom.reduced (2, 7));

    pluginList->setBounds (b.reduced (2, 6));
}

//==============================================================================
void PluginBrowserDialog::ContentComponent::addSelectedPlugin()
{
    auto& table = pluginList->getTableListBox();
    const int row = table.getSelectedRow();

    if (row < 0)
    {
        statusLabel.setText ("请先在列表中选择一个插件", juce::dontSendNotification);
        return;
    }

    auto types = engine.getPluginManager().getKnownPlugins().getTypes();

    if (row >= types.size())
    {
        statusLabel.setText ("该项不是有效的插件描述", juce::dontSendNotification);
        return;
    }

    const auto& desc = types.getReference (row);

    const juce::String error = engine.addPluginFromDescription (desc,
                                                                getTargetPath ? getTargetPath() : 0);

    if (error.isEmpty())
    {
        statusLabel.setText ("已添加： " + desc.name, juce::dontSendNotification);
        engine.getPluginManager().saveCache();

        if (onAdd)
            onAdd (desc);
    }
    else
    {
        statusLabel.setText ("添加失败： " + error, juce::dontSendNotification);
    }
}

void PluginBrowserDialog::ContentComponent::scanDefaultFolders()
{
    if (auto* vst3 = findVst3Format (engine.getPluginManager().getFormatManager()))
        pluginList->scanFor (*vst3, getDefaultScanFolders());
}

void PluginBrowserDialog::ContentComponent::chooseAndScanFolder()
{
    juce::FileChooser chooser ("选择包含插件的目录",
                               juce::File::getSpecialLocation (juce::File::userHomeDirectory),
                               "");

    if (chooser.browseForDirectory())
    {
        if (auto* vst3 = findVst3Format (engine.getPluginManager().getFormatManager()))
            pluginList->scanFor (*vst3, juce::StringArray (chooser.getResult().getFullPathName()));
    }
}

//==============================================================================
juce::AudioPluginFormat* PluginBrowserDialog::ContentComponent::findVst3Format (
    const juce::AudioPluginFormatManager& manager)
{
    for (auto* format : manager.getFormats())
        if (format->getName().containsIgnoreCase ("VST3"))
            return format;

    return nullptr;
}

juce::StringArray PluginBrowserDialog::ContentComponent::getDefaultScanFolders()
{
    juce::StringArray dirs;

#if JUCE_WINDOWS
    dirs.add ("C:\\Program Files\\Common Files\\VST3");
    dirs.add ("C:\\Program Files\\Steinberg\\Vst3Plugins");
#endif

#if JUCE_MAC
    dirs.add ("/Library/Audio/Plug-Ins/VST3");
    dirs.add ("~/Library/Audio/Plug-Ins/VST3");
#endif

    return dirs;
}