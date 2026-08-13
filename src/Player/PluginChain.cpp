#include "PluginChain.h"

#include <cassert>

//==============================================================================
/**
    Configures a plug-in instance's buses to match what this host can actually
    provide, then prepares it for processing.

    This host only ever supplies a stereo buffer.  Many VST3 plug-ins enable
    extra input buses by default (side-chains etc.).  Under JUCE's VST3 wrapper
    such an enabled extra bus maps onto host channels ((numMainChannels-1) + i)
    which do not exist in a stereo buffer: `getWritePointer (2)` on a 2-channel
    buffer reads its (out-of-range) channel pointer array, and plug-ins like
    FabFilter Pro-Q 3 dereference that garbage and crash.  So we must reduce
    the plug-in to a single stereo in/out pair and disable everything else
    BEFORE prepareToPlay.
*/
static void prepareInstanceForStereoHost (juce::AudioPluginInstance* instance,
                                          double sampleRate, int samplesPerBlock)
{
    jassert (instance != nullptr);

    juce::AudioProcessor::BusesLayout hostsLayout;

    for (int i = 0; i < instance->getBusCount (true); ++i)
        hostsLayout.inputBuses.add (i == 0 ? juce::AudioChannelSet::stereo()
                                           : juce::AudioChannelSet::disabled());

    for (int i = 0; i < instance->getBusCount (false); ++i)
        hostsLayout.outputBuses.add (i == 0 ? juce::AudioChannelSet::stereo()
                                            : juce::AudioChannelSet::disabled());

    if (instance->setBusesLayout (hostsLayout))
        instance->setRateAndBufferSizeDetails (sampleRate, samplesPerBlock);
    else
    {
        // A mono (single-channel) plug-in can't be forced to stereo.  Use the
        // main bus exactly as it is, but still disable every extra bus so the
        // wrapper never maps onto non-existent host channels.
        juce::AudioProcessor::BusesLayout original = instance->getBusesLayout();

        for (int i = 1; i < original.inputBuses.size(); ++i)
            original.inputBuses.getReference (i) = juce::AudioChannelSet::disabled();

        for (int i = 1; i < original.outputBuses.size(); ++i)
            original.outputBuses.getReference (i) = juce::AudioChannelSet::disabled();

        instance->setBusesLayout (original);
        instance->setRateAndBufferSizeDetails (sampleRate, samplesPerBlock);
    }

    instance->prepareToPlay (sampleRate, samplesPerBlock);
}

//==============================================================================
PluginChain::Slot::Slot (std::unique_ptr<juce::AudioPluginInstance> inst,
                         juce::PluginDescription desc)
    : instance (std::move (inst)), description (std::move (desc))
{
    jassert (instance != nullptr);
}

//==============================================================================
PluginChain::PluginChain() = default;

PluginChain::~PluginChain()
{
    // Release the realtime resources of every instance before they are
    // destroyed.  Some plug-ins (e.g. FabFilter) crash during teardown if
    // releaseResources() is never called.
    const juce::ScopedLock sl (lock);

    for (auto& slot : slots)
        releaseInstance (slot.get());

    slots.clear();
}

//==============================================================================
void PluginChain::releaseInstance (Slot* slot)
{
    if (slot == nullptr || slot->instance == nullptr)
        return;

    slot->instance->suspendProcessing (true);
    slot->instance->releaseResources();
}

//==============================================================================
void PluginChain::prepareToPlay (double rate, int samplesPerBlock)
{
    if (rate == sampleRate && samplesPerBlock == blockSize)
        return;

    const juce::ScopedLock sl (lock);

    sampleRate = rate;
    blockSize = samplesPerBlock;

    for (auto& slot : slots)
    {
        if (auto* instance = slot->instance.get())
        {
            instance->setRateAndBufferSizeDetails (rate, samplesPerBlock);
            instance->prepareToPlay (rate, samplesPerBlock);
        }
    }
}

//==============================================================================
void PluginChain::add (std::unique_ptr<juce::AudioPluginInstance> newInstance,
                       juce::PluginDescription description)
{
    if (newInstance == nullptr)
    {
        jassertfalse;
        return;
    }

    // Prepare the new instance on the message thread (it's not reachable from
    // the audio thread yet, so this is safe). The instance is created in an
    // unprepared state (see PluginManager::createInstance). We reduce it to a
    // stereo bus layout first (see prepareInstanceForStereoHost): this host only
    // provides a stereo buffer, and leaving extra buses (sidechains) enabled
    // makes the VST3 wrapper hand the plug-in out-of-range channel pointers,
    // crashing plug-ins like FabFilter Pro-Q 3.
    prepareInstanceForStereoHost (newInstance.get(), sampleRate, blockSize);

    {
        const juce::ScopedLock sl (lock);
        slots.push_back (std::make_unique<Slot> (std::move (newInstance), std::move (description)));
    }

    sendChangeMessage();
}

void PluginChain::remove (int index)
{
    std::unique_ptr<Slot> removed;

    {
        const juce::ScopedLock sl (lock);

        if (index < 0 || index >= getNumSlots())
            return;

        removed = std::move (slots[(size_t) index]);
        slots.erase (slots.begin() + index);
    }

    // Safe to do outside the lock: the audio thread can no longer see this slot.
    releaseInstance (removed.get());

    sendChangeMessage();
}

void PluginChain::move (int fromIndex, int toIndex)
{
    if (fromIndex == toIndex)
        return;

    {
        const juce::ScopedLock sl (lock);

        if (fromIndex < 0 || fromIndex >= getNumSlots() || toIndex < 0 || toIndex >= getNumSlots())
            return;

        auto slot = std::move (slots[(size_t) fromIndex]);
        slots.erase (slots.begin() + fromIndex);
        slots.insert (slots.begin() + toIndex, std::move (slot));
    }

    sendChangeMessage();
}

void PluginChain::clear()
{
    std::vector<std::unique_ptr<Slot>> removed;

    {
        const juce::ScopedLock sl (lock);
        removed.swap (slots);
    }

    for (auto& slot : removed)
        releaseInstance (slot.get());

    sendChangeMessage();
}

void PluginChain::setEnabled (int index, bool shouldBeEnabled)
{
    if (auto* slot = getSlot (index))
    {
        if (slot->enabled == shouldBeEnabled)
            return;

        {
            const juce::ScopedLock sl (lock);
            slot->enabled = shouldBeEnabled;
        }

        sendChangeMessage();
    }
}

//==============================================================================
PluginChain::Slot* PluginChain::getSlot (int index) const
{
    const juce::ScopedLock sl (lock);

    if (index < 0 || index >= (int) slots.size())
        return nullptr;

    return slots[(size_t) index].get();
}

int PluginChain::getTotalLatencySamples() const
{
    const juce::ScopedLock sl (lock);

    int lat = 0;
    for (auto& slot : slots)
        if (slot->enabled)
            lat += slot->instance->getLatencySamples();

    return lat;
}

//==============================================================================
void PluginChain::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    const juce::ScopedLock sl (lock);

    for (auto& slot : slots)
        if (slot->enabled)
            slot->instance->processBlock (buffer, midi);
}