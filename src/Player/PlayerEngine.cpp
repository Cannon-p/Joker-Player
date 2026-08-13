#include "PlayerEngine.h"
#include "../Trace.h"

//==============================================================================
/**
    A simple variable-delay ring buffer with a smoothed crossfade, used to keep
    the dry signal aligned with the (latency-introducing) plug-in chain.
*/
class PlayerEngine::LatencyCompensator
{
public:
    void reset (int numChannels, int maxDelaySamples, double newSampleRate, int bufferSize)
    {
        juce::ignoreUnused (bufferSize);

        maxDelay = juce::jmax (0, maxDelaySamples);
        const double rate = newSampleRate > 0.0 ? newSampleRate : 48000.0;

        ring.setSize (numChannels, maxDelay + 2);
        ring.clear();
        writePos = 0;
        currentDelay = 0;
        targetDelay = 0;
        crossfade = -1.0f;

        const double rampSeconds = 0.03;
        crossfadeStep = (float) (1.0 / (rampSeconds * rate));
    }

    void setTargetDelay (int samples)
    {
        samples = juce::jlimit (0, maxDelay, samples);

        if (samples == targetDelay)
            return;

        if (crossfade < 0.0f)
            currentDelay = targetDelay;

        targetDelay = samples;
        crossfade = 0.0f;
    }

    // Writes the input into the ring buffer while reading a delayed copy out.
    void process (const float* const* inData,
                  float* const* outData,
                  int numChannels,
                  int numSamples)
    {
        if (numChannels <= 0)
            return;

        const auto ringSize = ring.getNumSamples();

        for (int i = 0; i < numSamples; ++i)
        {
            for (int ch = 0; ch < numChannels; ++ch)
                ring.getWritePointer (ch)[writePos] = inData[ch][i];

            const int writePosBefore = writePos;
            writePos = (writePos + 1) % ringSize;

            if (crossfade >= 0.0f)
            {
                crossfade += crossfadeStep;
                if (crossfade >= 1.0f)
                {
                    crossfade = -1.0f;
                    currentDelay = targetDelay;
                }
            }

            const int readNew = ((writePosBefore - targetDelay) % ringSize + ringSize) % ringSize;

            if (crossfade >= 0.0f && crossfade < 1.0f)
            {
                const int readOld = ((writePosBefore - currentDelay) % ringSize + ringSize) % ringSize;
                const float f = crossfade;
                const float g = 1.0f - f;

                for (int ch = 0; ch < numChannels; ++ch)
                {
                    const auto* r = ring.getReadPointer (ch);
                    outData[ch][i] = r[readOld] * g + r[readNew] * f;
                }
            }
            else
            {
                for (int ch = 0; ch < numChannels; ++ch)
                    outData[ch][i] = ring.getReadPointer (ch)[readNew];
            }
        }
    }

private:
    juce::AudioBuffer<float> ring;
    int writePos = 0;
    int maxDelay = 0;
    int currentDelay = 0;
    int targetDelay = 0;
    float crossfade = -1.0f;
    float crossfadeStep = 0.0f;
};

//==============================================================================
PlayerEngine::PlayerEngine()
{
    aur::traceStep ("PlayerEngine ctor start");

    formatManager.registerBasicFormats(); // wav/aiff/flac/vorbis (+ mp3 when JUCE_USE_MP3AUDIOFORMAT=1)
    aur::traceStep ("formats registered");

    latencyComp = std::make_unique<LatencyCompensator>();

    for (int p = 0; p < numPaths; ++p)
    {
        alignComps.emplace_back (std::make_unique<LatencyCompensator>());
        midiBuffers.emplace_back ();
        pathEnabled[p] = (p == 0);
        pathVolume[p] = 1.0f;
    }

    deviceManager.addChangeListener (this);

    // Restore the audio device that was used last time, if any.
    std::unique_ptr<juce::XmlElement> savedDeviceState;
    auto stateFile = pluginManager.getDataDirectory().getChildFile ("device_state.xml");
    if (stateFile.existsAsFile())
        savedDeviceState = juce::XmlDocument::parse (stateFile);
    aur::traceStep ("device state read");

    sourcePlayer.setSource (this);
    aur::traceStep ("pre deviceManager.initialise");
    deviceManager.addAudioCallback (&sourcePlayer);
    deviceManager.initialise (0, 2, savedDeviceState.get(), true);
    aur::traceStep ("deviceManager.initialise done");

    // Many VST3 plug-ins assume a power-of-two block size and crash or misbehave
    // with the non-power-of-two periods that WASAPI shared mode often reports
    // (e.g. 480 samples at 48 kHz). Coerce the device to a power of two.
    auto setup = deviceManager.getAudioDeviceSetup();

    if (setup.bufferSize > 0 && (setup.bufferSize & (setup.bufferSize - 1)) != 0)
    {
        setup.bufferSize = 512;
        deviceManager.setAudioDeviceSetup (setup, true);
    }
}

PlayerEngine::~PlayerEngine()
{
    deviceManager.removeChangeListener (this);
    deviceManager.removeAudioCallback (&sourcePlayer);

    // Remember the chosen audio device for next launch.
    if (auto state = deviceManager.createStateXml())
    {
        if (state->getNumChildElements() > 0)
            state->writeTo (pluginManager.getDataDirectory().getChildFile ("device_state.xml"));
    }
}

//==============================================================================
void PlayerEngine::prepareToPlay (int samplesPerBlockExpected, double rate)
{
    aur::traceStep ("prepareToPlay");
    sampleRate = rate;
    blockSize = samplesPerBlockExpected;

    latencyComp->reset (2, (int) (rate * 4.0), rate, samplesPerBlockExpected);

    for (auto& comp : alignComps)
        comp->reset (2, (int) (rate * 4.0), rate, samplesPerBlockExpected);

    transport.prepareToPlay (samplesPerBlockExpected, rate);

    if (! bufferedThread.isThreadRunning())
        bufferedThread.startThread();

    // The plug-in chain is (re)prepared on the message thread.
    juce::MessageManager::callAsync ([this]
                                     {
                                         reopenPlugIns();
                                     });
}

void PlayerEngine::releaseResources()
{
    bufferedThread.stopThread (1000);
}

//==============================================================================
void PlayerEngine::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    transport.getNextAudioBlock (bufferToFill);

    if (bufferToFill.buffer == nullptr || bufferToFill.numSamples <= 0)
        return;

    const int numChannels = bufferToFill.buffer->getNumChannels();
    const int numSamples = bufferToFill.numSamples;
    const int startSample = bufferToFill.startSample;

    // Transport output peak, measured BEFORE any engine processing, so the
    // log can separate "transport produced silence" from "engine killed it".
    float tpk = 0.0f;
    for (int ch = 0; ch < numChannels; ++ch)
    {
        const float* r = bufferToFill.buffer->getReadPointer (ch, startSample);
        for (int i = 0; i < numSamples; ++i)
            tpk = juce::jmax (tpk, std::abs (r[i]));
    }

    // --- 0) snapshot path states ----------------------------------------------
    bool enabled[(size_t) numPaths];
    float volumes[(size_t) numPaths];

    for (int p = 0; p < numPaths; ++p)
    {
        enabled[(size_t) p] = pathEnabled[(size_t) p].load();
        volumes[(size_t) p] = pathVolume[(size_t) p].load();
    }

    // --- 1) ensure working buffers are large enough ---------------------------
    if (dryBuffer.getNumChannels() != numChannels || dryBuffer.getNumSamples() < numSamples)
    {
        dryBuffer.setSize (numChannels, numSamples, false, false, true);
        dryBuffer.clear();
    }

    if (pathBuffers.size() != (size_t) numPaths)
        pathBuffers.resize ((size_t) numPaths);

    for (auto& buf : pathBuffers)
        if (buf.getNumChannels() != numChannels || buf.getNumSamples() < numSamples)
            buf.setSize (numChannels, numSamples, false, false, true);

    if (alignedSum.getNumChannels() != numChannels || alignedSum.getNumSamples() < numSamples)
        alignedSum.setSize (numChannels, numSamples, false, false, true);

    // --- 2) latency of every path ----------------------------------------------
    // Path 3 is fed by path 1's output, so its effective latency is the sum of
    // both chains. We align every contribution to the *largest* latency so the
    // sum stays sample-accurate.
    int lat[(size_t) numPaths];
    lat[0] = chains[0].getTotalLatencySamples();
    lat[1] = chains[1].getTotalLatencySamples();
    lat[2] = lat[0] + chains[2].getTotalLatencySamples();

    int maxLat = 0;
    for (int p = 0; p < numPaths; ++p)
        if (enabled[(size_t) p])
            maxLat = juce::jmax (maxLat, lat[(size_t) p]);

    latencyComp->setTargetDelay (maxLat);

    // --- 3) dry/wet mix is computed against the dry input ----------------------
    // Capture the input (dry) reference *before* any path writes into the output
    // buffer, delayed so it lines up with the wet paths.
    std::vector<const float*> inPtrs ((size_t) numChannels);
    for (int ch = 0; ch < numChannels; ++ch)
        inPtrs[(size_t) ch] = bufferToFill.buffer->getReadPointer (ch, startSample);

    latencyComp->process (inPtrs.data(),
                          dryBuffer.getArrayOfWritePointers(),
                          numChannels,
                          numSamples);

    // --- 4) run the two parallel paths (1 and 2) on the dry input --------------
    // All processing happens synchronously on the audio thread. There is no
    // other thread involved (no worker round-trip per block), which keeps the
    // callback latency minimal.
    if (enabled[0])
    {
        for (int ch = 0; ch < numChannels; ++ch)
            pathBuffers[0].copyFrom (ch, 0, *bufferToFill.buffer, ch, startSample, numSamples);

        midiBuffers[0].clear();
        chains[0].processBlock (pathBuffers[0], midiBuffers[0]);
    }

    if (enabled[1])
    {
        for (int ch = 0; ch < numChannels; ++ch)
            pathBuffers[1].copyFrom (ch, 0, *bufferToFill.buffer, ch, startSample, numSamples);

        midiBuffers[1].clear();
        chains[1].processBlock (pathBuffers[1], midiBuffers[1]);
    }

    // --- 5) path 3 is fed by path 1's OUTPUT -----------------------------------
    if (enabled[2])
    {
        if (enabled[0])
        {
            // path 1's processed output is path 3's input
            for (int ch = 0; ch < numChannels; ++ch)
                pathBuffers[2].copyFrom (ch, 0, pathBuffers[0], ch, 0, numSamples);
        }
        else
        {
            // path 1 disabled: path 3 sees the dry input (passthrough behaviour)
            for (int ch = 0; ch < numChannels; ++ch)
                pathBuffers[2].copyFrom (ch, 0, *bufferToFill.buffer, ch, startSample, numSamples);
        }

        midiBuffers[2].clear();
        chains[2].processBlock (pathBuffers[2], midiBuffers[2]);
    }

    // --- 6) align every enabled path and sum with per-path volume ---------------
    alignedSum.clear();

    for (int p = 0; p < numPaths; ++p)
    {
        if (! enabled[(size_t) p])
            continue;

        alignComps[(size_t) p]->setTargetDelay (maxLat - lat[(size_t) p]);
        alignComps[(size_t) p]->process (pathBuffers[(size_t) p].getArrayOfReadPointers(),
                                         pathBuffers[(size_t) p].getArrayOfWritePointers(),
                                         numChannels,
                                         numSamples);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            juce::FloatVectorOperations::addWithMultiply (alignedSum.getWritePointer (ch),
                                                          pathBuffers[(size_t) p].getReadPointer (ch),
                                                          volumes[(size_t) p],
                                                          numSamples);
        }
    }

    // --- 7) dry/wet mix + master volume ----------------------------------------
    const float m = mix.load();
    const float dryGain = 1.0f - m;

    if (m < 1.0f && m > 0.0f)
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* dst = bufferToFill.buffer->getWritePointer (ch, startSample);
            const auto* wet = alignedSum.getReadPointer (ch);
            const auto* dry = dryBuffer.getReadPointer (ch);

            juce::FloatVectorOperations::multiply (dst, wet, m, numSamples);
            juce::FloatVectorOperations::addWithMultiply (dst, dry, dryGain, numSamples);
        }
    }
    else if (m <= 0.0f)
    {
        for (int ch = 0; ch < numChannels; ++ch)
            bufferToFill.buffer->copyFrom (ch,
                                           startSample,
                                           dryBuffer,
                                           ch,
                                           0,
                                           numSamples);
    }
    else
    {
        for (int ch = 0; ch < numChannels; ++ch)
            bufferToFill.buffer->copyFrom (ch,
                                           startSample,
                                           alignedSum,
                                           ch,
                                           0,
                                           numSamples);
    }

    bufferToFill.buffer->applyGain (startSample, numSamples, volume.load());

    // --- DIAGNOSTIC (temp): per-block output peak while playing ----------------
    {
        static int diagCount = 0;
        static int sincePlay = 0;

        const bool playing = transport.isPlaying();
        if (playing)
            ++sincePlay;
        else
            sincePlay = 0;

        if (playing && sincePlay <= 600)
        {
            float peak = 0.0f;
            for (int ch = 0; ch < numChannels; ++ch)
            {
                const float* r = bufferToFill.buffer->getReadPointer (ch, startSample);
                for (int i = 0; i < numSamples; ++i)
                    peak = juce::jmax (peak, std::abs (r[i]));
            }

            char buf[192];
            snprintf (buf, sizeof (buf),
                      "DIAGB blk=%d play=%d pos=%.2f tpk=%.4f pk=%.4f rpos=%.2f lat0=%d lat1=%d lat2=%d max=%d smp=%d ch=%d",
                      (int) diagCount, (int) playing,
                      transport.getCurrentPosition(), tpk, peak,
                      (readerSource != nullptr ? (double) readerSource->getNextReadPosition() / sampleRate.load() : -1.0),
                      lat[0], lat[1], lat[2], maxLat,
                      numSamples, numChannels);
            aur::traceStep (buf);
        }
        else
        {
            ++diagCount;
        }
    }
}

//==============================================================================
bool PlayerEngine::loadFile (const juce::File& file)
{
    std::unique_ptr<juce::AudioFormatReader> newReader (formatManager.createReaderFor (file));

    if (newReader == nullptr)
        return false;

    const auto keepPlaying = isPlaying();
    transport.stop();

    rawReader = std::move (newReader);

    readerSource = std::make_unique<juce::AudioFormatReaderSource> (rawReader.get(), false);

    // Read the file directly (no BufferingAudioSource read-ahead thread): the
    // playback had multi-second output silence with read-ahead enabled, which
    // was a transport/thread race; for local files a direct read is fine.
    transport.setSource (readerSource.get(),
                         0,
                         nullptr,
                         rawReader->sampleRate);

    currentFile = file;

    if (keepPlaying)
        play();

    return true;
}

//==============================================================================
void PlayerEngine::play()
{
    if (hasStreamFinished())
        transport.setPosition (0.0);

    transport.start();
}

void PlayerEngine::pause()
{
    transport.stop();
}

void PlayerEngine::stop()
{
    transport.stop();
    transport.setPosition (0.0);
}

void PlayerEngine::restartFromStart()
{
    transport.setPosition (0.0);
    transport.start();
}

void PlayerEngine::setPosition (double seconds)
{
    transport.setPosition (juce::jmax (0.0, seconds));
}

//==============================================================================
juce::String PlayerEngine::addPluginFromDescription (const juce::PluginDescription& description,
                                                     int path)
{
    path = juce::jlimit (0, numPaths - 1, path);

    const double rate = sampleRate.load();
    const int block = blockSize.load();

    // Make sure the target chain is prepared with the current device config.
    chains[(size_t) path].prepareToPlay (rate, block);

    juce::String error;
    auto instance = pluginManager.createInstance (description, rate, block, error);

    if (instance == nullptr)
        return error.isNotEmpty() ? error
                                  : juce::String ("鏃犳硶杞藉叆鎻掍欢: ") + description.name;

    chains[(size_t) path].add (std::move (instance), description);
    return {};
}

//==============================================================================
void PlayerEngine::reopenPlugIns()
{
    const double newRate = sampleRate.load();
    const int newBlock = blockSize.load();

    if (preparedRate.load() == newRate && preparedBlock.load() == newBlock)
        return;

    for (int p = 0; p < numPaths; ++p)
        chains[(size_t) p].prepareToPlay (newRate, newBlock);

    preparedRate = newRate;
    preparedBlock = newBlock;
}

//==============================================================================
int PlayerEngine::getCurrentLatencySamples() const
{
    int result = 0;

    for (int p = 0; p < numPaths; ++p)
    {
        if (! pathEnabled[(size_t) p].load())
            continue;

        const int lat = chains[(size_t) p].getTotalLatencySamples()
                        + (p == 2 ? chains[0].getTotalLatencySamples() : 0);

        result = juce::jmax (result, lat);
    }

    return result;
}

//==============================================================================
void PlayerEngine::changeListenerCallback (juce::ChangeBroadcaster* source)
{
    if (source == &deviceManager)
    {
        juce::MessageManager::callAsync ([this]
                                         {
                                             reopenPlugIns();
                                         });
    }
}