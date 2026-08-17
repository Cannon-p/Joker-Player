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
/**
    A TableListBoxModel that displays a filtered subset of the known plug-in
    list, supports double-click to add, and keeps row mapping to real indices.
*/
class FilteredPluginTableModel : public juce::TableListBoxModel
{
public:
    FilteredPluginTableModel (juce::KnownPluginList& listRef,
                              juce::Component& ownerComponent,
                              std::function<void (int realTypeIndex)> addCallback,
                              std::function<void (int realTypeIndex)> removeCallback)
        : list (listRef), ownerComp (ownerComponent),
          onAdd (std::move (addCallback)), onRemove (std::move (removeCallback))
    {
    }

    void setFilterText (const juce::String& t)
    {
        filterText = t.trim().toLowerCase();
    }

    int getNumRows() override
    {
        rebuild();
        return mapping.size();
    }

    int realIndexForRow (int row) const
    {
        return row >= 0 && row < mapping.size() ? mapping[row] : -1;
    }

    void paintRowBackground (juce::Graphics& g, int row, int width, int height, bool selected) override
    {
        g.fillAll (selected ? aur::Theme::panelActive() : aur::Theme::panel());
    }

    enum
    {
        nameCol = 1,
        typeCol = 2,
        categoryCol = 3,
        manufacturerCol = 4,
        descCol = 5
    };

    void paintCell (juce::Graphics& g, int row, int columnId, int width, int height, bool /*rowIsSelected*/) override
    {
        const int real = mapping[row];
        if (real < 0 || real >= list.getNumTypes())
            return;

        auto desc = list.getTypes()[real];

        juce::String text;
        switch (columnId)
        {
            case nameCol:         text = desc.name; break;
            case typeCol:         text = desc.pluginFormatName; break;
            case categoryCol:     text = desc.category.isNotEmpty() ? desc.category : "-"; break;
            case manufacturerCol: text = desc.manufacturerName; break;
            case descCol:         text = getPluginDescription (desc); break;
            default:              return;
        }

        g.setColour (columnId == nameCol ? aur::Theme::text() : aur::Theme::textDim());
        g.setFont (aur::Theme::uiFont ((float) height * 0.7f));
        g.drawFittedText (text, 4, 0, width - 6, height, juce::Justification::centredLeft, 1, 0.9f);
    }

    void cellDoubleClicked (int row, int /*columnId*/, const juce::MouseEvent&) override
    {
        if (row >= 0 && row < mapping.size() && onAdd)
            onAdd (mapping[row]);
    }

    void cellClicked (int rowNumber, int columnId, const juce::MouseEvent& e) override
    {
        if (rowNumber >= 0 && rowNumber < mapping.size() && e.mods.isPopupMenu())
        {
            const int real = mapping[rowNumber];

            juce::PopupMenu menu;
            menu.addItem (juce::String (juce::CharPointer_UTF8 ("从列表移除")), [this, real]
            {
                if (onRemove)
                    onRemove (real);
            });
            menu.addItem (juce::String (juce::CharPointer_UTF8 ("显示所在文件夹")), [this, real]
            {
                if (real >= 0 && real < list.getNumTypes())
                    juce::File (list.getTypes()[real].fileOrIdentifier).revealToUser();
            });
            menu.showMenuAsync (juce::PopupMenu::Options().withDeletionCheck (ownerComp));
        }
    }

    void sortOrderChanged (int newSortColumnId, bool isForwards) override
    {
        switch (newSortColumnId)
        {
            case nameCol:         list.sort (juce::KnownPluginList::sortAlphabetically, isForwards); break;
            case typeCol:         list.sort (juce::KnownPluginList::sortByFormat, isForwards); break;
            case categoryCol:     list.sort (juce::KnownPluginList::sortByCategory, isForwards); break;
            case manufacturerCol: list.sort (juce::KnownPluginList::sortByManufacturer, isForwards); break;
            default:              break;
        }
    }

private:
    void rebuild()
    {
        mapping.clear();
        auto types = list.getTypes();

        for (int i = 0; i < types.size(); ++i)
            if (matches (types[i]))
                mapping.add (i);
    }

    bool matches (const juce::PluginDescription& d) const
    {
        if (filterText.isEmpty())
            return true;

        return d.name.toLowerCase().contains (filterText)
            || d.manufacturerName.toLowerCase().contains (filterText)
            || d.pluginFormatName.toLowerCase().contains (filterText)
            || d.category.toLowerCase().contains (filterText);
    }

    static juce::String getPluginDescription (const juce::PluginDescription& desc)
    {
        juce::StringArray items;

        if (desc.descriptiveName != desc.name)
            items.add (desc.descriptiveName);

        items.add (desc.version);
        items.removeEmptyStrings();
        return items.joinIntoString (" - ");
    }

    juce::KnownPluginList& list;
    juce::Component& ownerComp;
    std::function<void (int)> onAdd;
    std::function<void (int)> onRemove;
    juce::String filterText;
    juce::Array<int> mapping;
};

//==============================================================================
PluginBrowserDialog::PluginBrowserDialog (PlayerEngine& engineRef,
                                          std::function<void (const juce::PluginDescription&)> onAdd,
                                          std::function<int()> getTargetPath)
    : DocumentWindow (juce::String (juce::CharPointer_UTF8 ("Joker Player · 插件管理器")), aur::Theme::bg(),
                      DocumentWindow::closeButton, true),
      engine (engineRef)
{
    setUsingNativeTitleBar (true);
    setLookAndFeel (&aur::CustomLookAndFeel::instance());

    auto* content = new ContentComponent (*this, engine, std::move (onAdd), std::move (getTargetPath));
    setContentOwned (content, true);

    setResizable (true, true);
    setResizeLimits (460, 520, 960, 960);
    centreWithSize (720, 640);
    setVisible (true);
}

PluginBrowserDialog::~PluginBrowserDialog() = default;

void PluginBrowserDialog::applyTheme()
{
    aur::CustomLookAndFeel::instance().refreshScheme();
    setLookAndFeel (&aur::CustomLookAndFeel::instance());

    if (auto* content = dynamic_cast<ContentComponent*> (getContentComponent()))
        content->applyTheme();

    sendLookAndFeelChange();
    repaint();
}

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

    pluginList->setOptionsButtonText (juce::String (juce::CharPointer_UTF8 ("扫描选项")));
    pluginList->setScanDialogText (juce::String (juce::CharPointer_UTF8 ("扫描插件")),
                                   juce::String (juce::CharPointer_UTF8 ("正在扫描插件，请稍候…")));
    pluginList->setNumberOfThreadsForScanning (1);
    addAndMakeVisible (pluginList.get());

    // -- buttons -------------------------------------------------------------
    filteredModel = new FilteredPluginTableModel (
        engine.getPluginManager().getKnownPlugins(),
        *pluginList,
        [this] (int realTypeIndex)
        {
            auto& known = engine.getPluginManager().getKnownPlugins();
            auto types = known.getTypes();
            if (realTypeIndex >= 0 && realTypeIndex < types.size())
                addPlugin (types.getReference (realTypeIndex));
        },
        [this] (int realTypeIndex)
        {
            auto& known = engine.getPluginManager().getKnownPlugins();
            auto types = known.getTypes();
            if (realTypeIndex >= 0 && realTypeIndex < types.size())
                known.removeType (types.getReference (realTypeIndex));
        });

    pluginList->setTableModel (filteredModel);

    searchBox.setFont (aur::Theme::uiFont (14.0f));
    searchBox.setColour (juce::TextEditor::textColourId, aur::Theme::text());
    searchBox.setColour (juce::TextEditor::backgroundColourId, aur::Theme::inputBg());
    searchBox.setColour (juce::TextEditor::outlineColourId, aur::Theme::border());
    searchBox.setColour (juce::TextEditor::focusedOutlineColourId, aur::Theme::accent());
    searchBox.setTextToShowWhenEmpty (juce::String (juce::CharPointer_UTF8 ("搜索插件名称、厂商、分类…")), aur::Theme::textDim());
    searchBox.setJustification (juce::Justification::centredLeft);
    searchBox.setReturnKeyStartsNewLine (false);
    searchBox.onTextChange = [this]
    {
        filteredModel->setFilterText (searchBox.getText());
        pluginList->getTableListBox().updateContent();
        pluginList->getTableListBox().repaint();
    };
    addAndMakeVisible (searchBox);

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
void PluginBrowserDialog::ContentComponent::applyTheme()
{
    title.setColour (juce::Label::textColourId, aur::Theme::text());
    statusLabel.setColour (juce::Label::textColourId, aur::Theme::textDim());
    searchBox.setColour (juce::TextEditor::textColourId, aur::Theme::text());
    searchBox.setColour (juce::TextEditor::backgroundColourId, aur::Theme::inputBg());
    searchBox.setColour (juce::TextEditor::outlineColourId, aur::Theme::border());
    searchBox.setColour (juce::TextEditor::focusedOutlineColourId, aur::Theme::accent());
    searchBox.applyColourToAllText (aur::Theme::text());
    searchBox.setTextToShowWhenEmpty (juce::String (juce::CharPointer_UTF8 ("搜索插件名称、厂商、分类…")), aur::Theme::textDim());
    repaint();
}

//==============================================================================
void PluginBrowserDialog::ContentComponent::paint (juce::Graphics& g)
{
    g.setGradientFill (aur::Theme::windowBackgroundGradient (getLocalBounds().toFloat()));
    g.fillAll();
}

void PluginBrowserDialog::ContentComponent::resized()
{
    auto b = getLocalBounds().reduced (12);

    auto titleArea = b.removeFromTop (30);
    title.setBounds (titleArea);

    auto buttons = b.removeFromTop (kTopButtonsHeight).reduced (0, 2);
    searchBox.setBounds (buttons.removeFromLeft (buttons.getWidth() * 0.6f).reduced (2, 6));
    buttons.removeFromLeft (buttons.getWidth() * 0.3f);
    doneButton.setBounds (buttons.reduced (2, 6));

    auto bottom = b.removeFromBottom (kBottomBarHeight);
    statusLabel.setBounds (bottom.reduced (2, 8));

    pluginList->setBounds (b.reduced (2, 6));
}

//==============================================================================
void PluginBrowserDialog::ContentComponent::addPlugin (const juce::PluginDescription& desc)
{
    const juce::String error = engine.addPluginFromDescription (desc,
                                                                getTargetPath ? getTargetPath() : 0);

    if (error.isEmpty())
    {
        statusLabel.setText (juce::String (juce::CharPointer_UTF8 ("已添加： ")) + desc.name, juce::dontSendNotification);
        engine.getPluginManager().saveCache();

        if (onAdd)
            onAdd (desc);
    }
    else
    {
        statusLabel.setText (juce::String (juce::CharPointer_UTF8 ("添加失败： ")) + error, juce::dontSendNotification);
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
    dirs.add ("C:\\Program Files\\Common Files\\VST2");
    dirs.add ("C:\\Program Files\\VstPlugins");
    dirs.add ("C:\\Program Files (x86)\\VstPlugins");
    dirs.add ("C:\\Program Files\\Steinberg\\VstPlugins");
#endif

#if JUCE_MAC
    dirs.add ("/Library/Audio/Plug-Ins/VST3");
    dirs.add ("~/Library/Audio/Plug-Ins/VST3");
#endif

    return dirs;
}