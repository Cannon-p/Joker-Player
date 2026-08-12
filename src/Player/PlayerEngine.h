#pragma once

#include <JuceHeader.h>

#include "PluginChain.h"
#include "PluginManager.h"

//==============================================================================
/**
    The heart of the player.

    Responsibilities:
      * manage the audio device and drive playback from a decoded file
        (wav / aiff / flac / ogg / mp3 ...)
      * host up to three VST3 effect paths in realtime:

            input 鈹€鈹€鈻?path 1 鈹€鈹€鈻衡攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈻?sum
            input 鈹€鈹€鈻?path 2 鈹€鈹€鈻衡攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈻?sum
            path1 鈹€鈹€鈻?path 3 鈹€鈹€鈻衡攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈻?sum

        paths 1 and 2 run in parallel (both fed the dry input); path 3 is fed
        by path 1's output. The three path outputs are latency-aligned and
        summed, then scaled individually. Each path can be enabled/disabled
        and has its own volume. Default: only path 1 is enabled.
        All plugin processing happens synchronously on the audio thread.
      * automatic latency compensation: every path output (and the dry signal
        used by the dry/wet mix) is delayed by (maxLatency - pathLatency) so
        all contributions stay sample-aligned.
*/
class PlayerEngine : public juce::AudioSource,
                     public juce::ChangeListener
{
public:
    PlayerEngine();
    ~PlayerEngine() override;

    //==============================================================================
    // juce::AudioSource
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void releaseResources() override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo&) override;

    //==============================================================================
    // Transport control
    bool loadFile (const juce::File& file);
    void play();
    void pause();

    void stop();
    void restartFromStart();

    bool isPlaying() const        { return transport.isPlaying(); }
    bool hasTrack() const         { return readerSource != nullptr; }
    bool hasStreamFinished() const { return hasTrack() && transport.hasStreamFinished(); }

    double getPositionInSeconds() const { return transport.getCurrentPosition(); }
    double getLengthInSeconds() const   { return transport.getLengthInSeconds(); }
    void setPosition (double seconds);
    const juce::File& getCurrentFile() const { return currentFile; }

    //==============================================================================
    // Devices
    juce::AudioDeviceManager& getDeviceManager() { return deviceManager; }

    //==============================================================================
    // Plug-ins
    static constexpr int numPaths = 3;

    PluginChain& getChain (int path) { return chains[(size_t) path]; }
    PluginManager& getPluginManager() { return pluginManager; }

    /** Instantiates, prepares and adds a plug-in to a chain (message thread).
        `path` is 0..numPaths-1. */
    juce::String addPluginFromDescription (const juce::PluginDescription& description,
                                           int path);

    //==============================================================================
    // Path routing / per-path volume
    bool  isPathEnabled (int path) const { return pathEnabled[(size_t) path].load(); }
    void  setPathEnabled (int path, bool shouldBeEnabled)
    {
        pathEnabled[(size_t) path].store (shouldBeEnabled);
    }

    float getPathVolume (int path) const { return pathVolume[(size_t) path].load(); }
    void  setPathVolume (int path, float newVolume)
    {
        pathVolume[(size_t) path].store (juce::jlimit (0.0f, 2.0f, newVolume));
    }

    //==============================================================================
    // Mix / latency / volume
    float getMix() const          { return mix.load(); }
    void  setMix (float newMix)   { mix.store (juce::jlimit (0.0f, 1.0f, newMix)); }

    float getMasterVolume() const { return volume.load(); }
    void  setMasterVolume (float v) { volume.store (juce::jlimit (0.0f, 1.0f, v)); }

    /** Largest effective latency (in samples) among all enabled paths.
        Path 3 is fed by path 1, so its effective latency includes path 1's. */
    int getCurrentLatencySamples() const;

    //==============================================================================
    // Device configuration helpers
    double getSampleRate() const  { return sampleRate.load(); }
    int    getBlockSize() const   { return blockSize.load(); }

    /** Re-prepares all hosted plug-ins after the device configuration changed.
        Message thread only. */
    void reopenPlugIns();

private:
    class LatencyCompensator;

    void changeListenerCallback (juce::ChangeBroadcaster*) override;

    juce::AudioDeviceManager deviceManager;
    // NB: this must be declared before sourcePlayer so that it is destroyed
    // AFTER it. AudioSourcePlayer::setSource (nullptr) (called from its
    // destructor) triggers PlayerEngine::releaseResources(), which stops this
    // thread; the thread must still be alive at that point.
    juce::TimeSliceThread bufferedThread { "Joker Audio Reader" };
    juce::AudioSourcePlayer sourcePlayer;
    juce::AudioFormatManager formatManager;
    PluginManager pluginManager;
    PluginChain chains[(size_t) numPaths];

    std::unique_ptr<juce::AudioFormatReader> rawReader;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    juce::AudioTransportSource transport;

    juce::File currentFile;

    std::unique_ptr<LatencyCompensator> latencyComp;
    juce::AudioBuffer<float> dryBuffer;

    // One working buffer + one aligner per path.
    std::vector<juce::AudioBuffer<float>> pathBuffers;
    std::vector<std::unique_ptr<LatencyCompensator>> alignComps;
    juce::AudioBuffer<float> alignedSum;
    std::vector<juce::MidiBuffer> midiBuffers;

    std::atomic<float> mix { 1.0f };
    std::atomic<float> volume { 1.0f };
    std::atomic<bool> pathEnabled[(size_t) numPaths];
    std::atomic<float> pathVolume[(size_t) numPaths];

    std::atomic<double> sampleRate { 48000.0 };
    std::atomic<int> blockSize { 512 };

    // Which configuration the plug-in chain has actually been prepared for.
    std::atomic<int> preparedBlock { -1 };
    std::atomic<double> preparedRate { -1.0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PlayerEngine)
};