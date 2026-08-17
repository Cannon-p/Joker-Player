juce_add_gui_app(JokerPlayer
    PRODUCT_NAME "Joker Player"
    BUNDLE_ID "com.joker.player"
    COMPANY_NAME "Joker"
    COMPANY_COPYRIGHT "Copyright (c) 2026 Joker"
    COMPANY_WEBSITE ""
    COMPANY_EMAIL ""
    VERSION 1.0.0
    NEEDS_MIDI_INPUT FALSE
    MICROPHONE_PERMISSION_ENABLED FALSE
)

target_compile_features(JokerPlayer PRIVATE cxx_std_17)

if (MSVC)
    target_compile_options(JokerPlayer PRIVATE /utf-8)  # our sources are UTF-8 (Chinese UI strings)
endif ()

target_compile_definitions(JokerPlayer PRIVATE
    JUCE_WEB_BROWSER=0       # we don't use the web browser module
    JUCE_USE_CURL=0
    JUCE_USE_MP3AUDIOFORMAT=1 # software MP3 decoding (wav/aiff/flac/vorbis are always available)
    JUCE_ASIO=1               # enable ASIO audio devices (uses the bundled ASIO SDK)
    JUCE_PLUGINHOST_VST3=1    # enable VST3 plug-in hosting
    JUCE_PLUGINHOST_VST=1     # enable VST2 plug-in hosting (uses the bundled VST2 SDK headers)
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_MODAL_LOOPS_PERMITTED=1 # FileChooser::browseForDirectory etc. (GUI app)
)

target_include_directories(JokerPlayer PRIVATE
    ${CMAKE_SOURCE_DIR}/asiosdk_2.3.3_2019-06-14/common
    ${CMAKE_SOURCE_DIR}/vst2_sdk
)

target_sources(JokerPlayer PRIVATE
    Main.cpp
    Player/PluginChain.cpp
    Player/PluginManager.cpp
    Player/PlayerEngine.cpp
    UI/CustomLookAndFeel.cpp
    UI/MainComponentImpl.cpp
    UI/PlaylistComponent.cpp
    UI/PluginBrowserDialog.cpp
    UI/PluginRackComponent.cpp
)

target_link_libraries(JokerPlayer PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_data_structures
    juce::juce_dsp
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
)

juce_generate_juce_header(JokerPlayer)