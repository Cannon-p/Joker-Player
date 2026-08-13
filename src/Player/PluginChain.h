#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
    A serial effect chain of hosted plug-in instances for the Joker player.

    The chain owns a list of AudioPluginInstance objects (VST3 et al.) that are
    processed one after another in realtime. Latency is reported by each plug-in
    and summed; the playback engine uses that sum for automatic latency
    compensation.

    All mutations (add / remove / enable) must be performed on the message
    thread; processBlock() is safe to be called from the audio thread.
*/
class PluginChain : public juce::ChangeBroadcaster
{
public:
    struct Slot
    {
        Slot (std::unique_ptr<juce::AudioPluginInstance> inst, juce::PluginDescription desc);

        std::unique_ptr<juce::AudioPluginInstance> instance;
        juce::PluginDescription description;
        bool enabled = true;

        // Message-thread only: hosts the plug-in's own GUI.
        std::unique_ptr<juce::DocumentWindow> editorWindow;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Slot)
    };

    PluginChain();
    ~PluginChain() override;

    /** Adds a prepared plug-in at the end of the chain. Message thread only. */
    void add (std::unique_ptr<juce::AudioPluginInstance> newInstance,
              juce::PluginDescription description);

    /** Removes a plug-in from the chain. Message thread only. */
    void remove (int index);

    /** Moves a plug-in to a new position in the chain. Message thread only. */
    void move (int fromIndex, int toIndex);

    /** Removes all plug-ins. Message thread only. */
    void clear();

    /** Enables / bypasses a plug-in. Message thread only. */
    void setEnabled (int index, bool shouldBeEnabled);

    int getNumSlots() const { return (int) slots.size(); }
    Slot* getSlot (int index) const;

    /** Sum of the latencies of all enabled plug-ins, in samples. */
    int getTotalLatencySamples() const;

    /** Prepares every instance for the given device configuration.
        Must be called on the message thread (and again after a device change). */
    void prepareToPlay (double sampleRate, int samplesPerBlock);

    /** Processes one block through all enabled plug-ins. Audio thread. */
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi);

private:
    /** Calls suspendProcessing(true) + releaseResources() on a slot's instance. */
    static void releaseInstance (Slot* slot);

    mutable juce::CriticalSection lock;
    std::vector<std::unique_ptr<Slot>> slots;
    double sampleRate = 48000.0;
    int blockSize = 512;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginChain)
};