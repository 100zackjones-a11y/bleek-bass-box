#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <random>

namespace bleek {

BleekBassBoxProcessor::BleekBassBoxProcessor()
    : AudioProcessor(BusesProperties()
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "BLEEK_STATE", createParameterLayout())
{
    const char* paramIds[] = {
        "era", "archetype", "drumContext", "rootNote", "phraseBars",
        "noteDensity", "syncopation", "phraseVariation", "noteLength",
        "chromaticism", "octaveMovement", "drumAwareness", "swing",
        "lockRhythm", "lockPitch", "lockPhrase", "lockGroove",
        "attack", "decay", "sustain", "release", "liveMidiOut",
        "callResponse", "responseAmount", "responseDir",
        "attackB", "decayB", "sustainB", "releaseB"
    };
    for (const auto* id : paramIds)
        apvts.addParameterListener(id, this);

    samplerB.setAsset(sampler.getCurrentAsset()); // slot B starts by following slot A
    triggerRegenerate(false);
}

BleekBassBoxProcessor::~BleekBassBoxProcessor() {
    const char* paramIds[] = {
        "era", "archetype", "drumContext", "rootNote", "phraseBars",
        "noteDensity", "syncopation", "phraseVariation", "noteLength",
        "chromaticism", "octaveMovement", "drumAwareness", "swing",
        "lockRhythm", "lockPitch", "lockPhrase", "lockGroove",
        "attack", "decay", "sustain", "release", "liveMidiOut",
        "callResponse", "responseAmount", "responseDir",
        "attackB", "decayB", "sustainB", "releaseB"
    };
    for (const auto* id : paramIds)
        apvts.removeParameterListener(id, this);
}

juce::AudioProcessorValueTreeState::ParameterLayout BleekBassBoxProcessor::createParameterLayout() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // 4 Core ADSR Envelope Parameters (Full Ableton Automation)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("attack", 1), "Attack",
        juce::NormalisableRange<float>(1.0f, 500.0f, 0.1f, 0.4f), 8.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("decay", 1), "Decay",
        juce::NormalisableRange<float>(10.0f, 3000.0f, 1.0f, 0.4f), 450.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("sustain", 1), "Sustain",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.85f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("release", 1), "Release",
        juce::NormalisableRange<float>(5.0f, 2000.0f, 1.0f, 0.4f), 40.0f));

    // Slot B (ANSWER) has its own envelope
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("attackB", 1), "Attack B",
        juce::NormalisableRange<float>(1.0f, 500.0f, 0.1f, 0.4f), 8.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("decayB", 1), "Decay B",
        juce::NormalisableRange<float>(10.0f, 3000.0f, 1.0f, 0.4f), 450.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("sustainB", 1), "Sustain B",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.85f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("releaseB", 1), "Release B",
        juce::NormalisableRange<float>(5.0f, 2000.0f, 1.0f, 0.4f), 40.0f));

    // Compositional Selectors
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("era", 1), "Historical Era",
        juce::StringArray{
            "1994-1995 Foundation / Ragga Jungle",
            "1996-1997 Dark Jungle / Early Techstep",
            "1998-1999 Techstep / Early Neurofunk",
            "2000-2001 Minimal Dark Rollers",
            "2002-2004 Liquid Funk & Jump-Up"
        }, 0));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("archetype", 1), "Archetype",
        juce::StringArray{
            "Roots Jungle Sub-Bass", "808-Style Sub", "Dub-Influenced Bass",
            "Reese Bassline", "Techstep", "Neurofunk", "Rolling Bassline",
            "Minimal Dark Roller", "Liquid Funk", "Jump-Up",
            "Stabbing Bassline", "Pedal-Note Pulse"
        }, 0));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("drumContext", 1), "Drum Context",
        juce::StringArray{ "Two-Step DnB", "Amen Break", "Think Break", "Rolling 16th Break" }, 1));

    // Root Notes C1 (24) through B2 (47)
    juce::StringArray rootNames;
    const char* noteNames[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    for (int m = 24; m <= 47; ++m)
        rootNames.add(juce::String(noteNames[m % 12]) + juce::String((m / 12) - 1));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("rootNote", 1), "Root Note", rootNames, 5)); // index 5 = F1 (MIDI 29)

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("phraseBars", 1), "Phrase Length",
        juce::StringArray{ "1 Bar", "2 Bars", "4 Bars", "8 Bars" }, 2)); // Default 4 Bars

    // Compositional Sliders
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("noteDensity", 1), "Note Density", 0.0f, 1.0f, 0.50f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("syncopation", 1), "Syncopation", 0.0f, 1.0f, 0.55f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("phraseVariation", 1), "Phrase Variation", 0.0f, 1.0f, 0.35f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("noteLength", 1), "Note Length", 0.0f, 1.0f, 0.55f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("chromaticism", 1), "Chromaticism", 0.0f, 1.0f, 0.30f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("octaveMovement", 1), "Octave Movement", 0.0f, 1.0f, 0.35f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("drumAwareness", 1), "Drum Awareness", 0.0f, 1.0f, 0.75f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("swing", 1), "Swing", 0.50f, 0.62f, 0.53f));

    // 4 Dimension Locks & Live MIDI Out
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("lockRhythm", 1), "Lock Rhythm", false));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("lockPitch", 1), "Lock Pitch", false));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("lockPhrase", 1), "Lock Phrase", false));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("lockGroove", 1), "Lock Groove", false));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("liveMidiOut", 1), "Live MIDI Out", true));

    // Call & Response
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("callResponse", 1), "Call & Response", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("responseAmount", 1), "Answer Amount", 0.0f, 1.0f, 0.50f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("responseDir", 1), "Answer Direction",
        juce::StringArray{ "Answer Up", "Answer Down", "Answer Mirror" }, 0));

    return { params.begin(), params.end() };
}

void BleekBassBoxProcessor::parameterChanged(const juce::String& parameterID, float newValue) {
    if (parameterID == "era") {
        applyEraPreset(static_cast<EraPreset>(static_cast<int>(newValue)));
        return;
    }

    if (parameterID == "rootNote") {
        sampler.setRootMidiNote(24 + static_cast<int>(newValue));
    }

    if (parameterID == "attack" || parameterID == "decay" ||
        parameterID == "sustain" || parameterID == "release" ||
        parameterID == "attackB" || parameterID == "decayB" ||
        parameterID == "sustainB" || parameterID == "releaseB")
    {
        applyAdsrToSamplers();
        return;
    }

    needsRegeneration_.store(true, std::memory_order_release);
}

void BleekBassBoxProcessor::applyEraPreset(EraPreset era) {
    const auto cfg = BassEngine::getEraConfig(era);
    currentBpm_.store(cfg.defaultBpm, std::memory_order_relaxed);

    const bool grooveLocked = apvts.getRawParameterValue("lockGroove")->load() > 0.5f;
    if (!grooveLocked) {
        if (auto* p = apvts.getParameter("swing"))
            p->setValueNotifyingHost(p->convertTo0to1(cfg.defaultSwing));
    }
    if (auto* p = apvts.getParameter("drumContext"))
        p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(cfg.defaultDrum)));
    if (auto* p = apvts.getParameter("archetype"))
        p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(cfg.defaultArchetype)));

    needsRegeneration_.store(true, std::memory_order_release);
}

GenerationParams BleekBassBoxProcessor::buildGenerationParamsFromState() const {
    GenerationParams g;
    g.era = static_cast<EraPreset>(static_cast<int>(apvts.getRawParameterValue("era")->load()));
    g.archetype = static_cast<BassArchetype>(static_cast<int>(apvts.getRawParameterValue("archetype")->load()));
    g.drumTemplate = static_cast<DrumTemplate>(static_cast<int>(apvts.getRawParameterValue("drumContext")->load()));
    g.rootMidiNote = 24 + static_cast<int>(apvts.getRawParameterValue("rootNote")->load());

    const int phraseIdx = static_cast<int>(apvts.getRawParameterValue("phraseBars")->load());
    const int barChoices[4] = { 1, 2, 4, 8 };
    g.phraseBars = barChoices[std::clamp(phraseIdx, 0, 3)];

    g.seed = currentSeed_.load(std::memory_order_relaxed);
    g.noteDensity = apvts.getRawParameterValue("noteDensity")->load();
    g.syncopation = apvts.getRawParameterValue("syncopation")->load();
    g.phraseVariation = apvts.getRawParameterValue("phraseVariation")->load();
    g.noteLength = apvts.getRawParameterValue("noteLength")->load();
    g.chromaticism = apvts.getRawParameterValue("chromaticism")->load();
    g.octaveMovement = apvts.getRawParameterValue("octaveMovement")->load();
    g.drumAwareness = apvts.getRawParameterValue("drumAwareness")->load();
    g.swing = apvts.getRawParameterValue("swing")->load();

    g.lockRhythm = apvts.getRawParameterValue("lockRhythm")->load() > 0.5f;
    g.lockPitch = apvts.getRawParameterValue("lockPitch")->load() > 0.5f;
    g.lockPhrase = apvts.getRawParameterValue("lockPhrase")->load() > 0.5f;
    g.lockGroove = apvts.getRawParameterValue("lockGroove")->load() > 0.5f;

    g.callResponse = apvts.getRawParameterValue("callResponse")->load() > 0.5f;
    g.responseAmount = apvts.getRawParameterValue("responseAmount")->load();
    g.responseDirection = static_cast<int>(apvts.getRawParameterValue("responseDir")->load());
    return g;
}

void BleekBassBoxProcessor::triggerRegenerate(bool randomizeSeed) {
    if (randomizeSeed) {
        std::random_device rd;
        uint32_t nextSeed = (static_cast<uint32_t>(rd()) ^ static_cast<uint32_t>(juce::Time::getHighResolutionTicks()));
        if (nextSeed == 0u) nextSeed = 1994u;
        currentSeed_.store(nextSeed, std::memory_order_relaxed);
    }

    const auto params = buildGenerationParamsFromState();
    bassEngine_.generate(params);

    std::lock_guard<std::mutex> lock(patternMutex_);
    activePattern_ = bassEngine_.getPattern();
}

void BleekBassBoxProcessor::applyAdsrToSamplers() {
    sampler.setADSR(apvts.getRawParameterValue("attack")->load(),
                    apvts.getRawParameterValue("decay")->load(),
                    apvts.getRawParameterValue("sustain")->load(),
                    apvts.getRawParameterValue("release")->load());
    samplerB.setADSR(apvts.getRawParameterValue("attackB")->load(),
                     apvts.getRawParameterValue("decayB")->load(),
                     apvts.getRawParameterValue("sustainB")->load(),
                     apvts.getRawParameterValue("releaseB")->load());
}

bool BleekBassBoxProcessor::loadSampleFile(const juce::File& file, int slot, bool setKeyFromSample) {
    if (slot == 1) {
        // Slot B keeps its own pitch reference; the key stays set by slot A
        if (!samplerB.loadSampleFromFile(file))
            return false;
        bFollowsA_.store(false, std::memory_order_relaxed);
        return true;
    }

    if (!sampler.loadSampleFromFile(file))
        return false;

    if (bFollowsA_.load(std::memory_order_relaxed))
        samplerB.setAsset(sampler.getCurrentAsset());

    if (setKeyFromSample) {
        const int detected = sampler.getRootMidiNote();
        if (auto* p = apvts.getParameter("rootNote"))
            p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(detected - 24)));
    }
    needsRegeneration_.store(true, std::memory_order_release);
    return true;
}

void BleekBassBoxProcessor::resetToDefaultSine(int slot) {
    if (slot == 1) {
        // "USE A": slot B goes back to following slot A's sample
        bFollowsA_.store(true, std::memory_order_relaxed);
        samplerB.setAsset(sampler.getCurrentAsset());
        return;
    }

    sampler.loadFallbackSine();
    sampler.setRootMidiNote(29);
    if (bFollowsA_.load(std::memory_order_relaxed))
        samplerB.setAsset(sampler.getCurrentAsset());
    if (auto* p = apvts.getParameter("rootNote"))
        p->setValueNotifyingHost(p->convertTo0to1(5.0f)); // F1
    needsRegeneration_.store(true, std::memory_order_release);
}

std::vector<MidiNoteEvent> BleekBassBoxProcessor::getPatternSnapshot() const {
    std::lock_guard<std::mutex> lock(patternMutex_);
    return activePattern_;
}

void BleekBassBoxProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    sampler.prepareToPlay(sampleRate, samplesPerBlock);
    samplerB.prepareToPlay(sampleRate, samplesPerBlock);
    if (bFollowsA_.load(std::memory_order_relaxed))
        samplerB.setAsset(sampler.getCurrentAsset());
    applyAdsrToSamplers();
}

void BleekBassBoxProcessor::releaseResources() {
    sampler.releaseResources();
    samplerB.releaseResources();
}

bool BleekBassBoxProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono();
}

void BleekBassBoxProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    if (needsRegeneration_.exchange(false, std::memory_order_acquire)) {
        triggerRegenerate(false);
    }

    const int numSamples = buffer.getNumSamples();
    const double sr = getSampleRate() > 0.0 ? getSampleRate() : 44100.0;

    // 1. Handle incoming live MIDI keyboard notes (for auditioning dropped samples)
    for (const auto metadata : midiMessages) {
        const auto msg = metadata.getMessage();
        // Incoming MIDI channel 2 plays slot B, everything else plays slot A
        auto& target = (msg.getChannel() == 2) ? samplerB : sampler;
        auto& other  = (msg.getChannel() == 2) ? sampler : samplerB;
        if (msg.isNoteOn()) {
            other.releaseHeld();
            target.noteOn(msg.getNoteNumber(), msg.getFloatVelocity());
            liveHeldNote_ = msg.getNoteNumber();
        } else if (msg.isNoteOff()) {
            target.noteOff(msg.getNoteNumber());
            if (liveHeldNote_ == msg.getNoteNumber())
                liveHeldNote_ = -1;
        } else if (msg.isAllNotesOff() || msg.isAllSoundOff()) {
            sampler.allNotesOff();
            samplerB.allNotesOff();
            liveHeldNote_ = -1;
        }
    }

    // 2. Lock to Ableton Live's AudioPlayHead::PositionInfo
    const bool liveOutEnabled = apvts.getRawParameterValue("liveMidiOut")->load() > 0.5f;
    if (!liveOutEnabled)
        midiMessages.clear();

    double bpm = currentBpm_.load(std::memory_order_relaxed);
    bool hostPlaying = false;
    double ppqPos = 0.0;

    if (auto* playHead = getPlayHead()) {
        if (auto posInfo = playHead->getPosition()) {
            if (auto hostBpm = posInfo->getBpm()) {
                bpm = *hostBpm;
                currentBpm_.store(bpm, std::memory_order_relaxed);
            }
            hostPlaying = posInfo->getIsPlaying();
            if (auto ppq = posInfo->getPpqPosition()) {
                ppqPos = *ppq;
            }
        }
    }

    isPlaying_.store(hostPlaying, std::memory_order_relaxed);

    const int phraseIdx = static_cast<int>(apvts.getRawParameterValue("phraseBars")->load());
    const int barChoices[4] = { 1, 2, 4, 8 };
    const int totalBars = barChoices[std::clamp(phraseIdx, 0, 3)];
    const double totalPhraseTicks = static_cast<double>(totalBars * BassEngine::kTicksPerBar);

    if (hostPlaying && totalPhraseTicks > 0.0) {
        const double ticksPerSample = (bpm / 60.0) * static_cast<double>(MidiExporter::kPPQN) / sr;
        const double blockStartTickRaw = ppqPos * static_cast<double>(MidiExporter::kPPQN);
        const double blockEndTickRaw = blockStartTickRaw + static_cast<double>(numSamples) * ticksPerSample;

        // Wrap position into active 1/2/4/8-bar phrase loop
        auto wrapTick = [totalPhraseTicks](double t) -> double {
            double m = std::fmod(t, totalPhraseTicks);
            if (m < 0.0) m += totalPhraseTicks;
            return m;
        };

        const double startMod = wrapTick(blockStartTickRaw);
        const double endMod = wrapTick(blockEndTickRaw);
        playheadBarPos_.store(startMod / static_cast<double>(BassEngine::kTicksPerBar), std::memory_order_relaxed);

        std::vector<MidiNoteEvent> notesCopy;
        {
            std::lock_guard<std::mutex> lock(patternMutex_);
            notesCopy = activePattern_;
        }

        auto isTickInWindow = [&](double tick) -> int {
            if (startMod <= endMod) {
                if (tick >= startMod && tick < endMod)
                    return static_cast<int>((tick - startMod) / ticksPerSample);
            } else {
                // Loop wrap boundary inside this audio block
                if (tick >= startMod)
                    return static_cast<int>((tick - startMod) / ticksPerSample);
                if (tick < endMod)
                    return static_cast<int>((totalPhraseTicks - startMod + tick) / ticksPerSample);
            }
            return -1;
        };

        // Call & Response: answer bars (odd bar index) play slot B on MIDI channel 2
        const bool crOn = apvts.getRawParameterValue("callResponse")->load() > 0.5f;
        auto isAnswer = [crOn](const MidiNoteEvent& n) { return crOn && (n.barIndex % 2 == 1); };

        for (const auto& n : notesCopy) {
            const double noteOnTick = static_cast<double>(n.startTick);
            const double noteOffTick = wrapTick(static_cast<double>(n.startTick + n.durationTicks));
            const bool answer = isAnswer(n);
            auto& target = answer ? samplerB : sampler;
            auto& other  = answer ? sampler : samplerB;
            const int channel = answer ? 2 : 1;

            const int offSample = isTickInWindow(noteOffTick);
            if (offSample >= 0 && offSample < numSamples) {
                target.noteOff(n.midiNote);
                if (liveOutEnabled)
                    midiMessages.addEvent(juce::MidiMessage::noteOff(channel, n.midiNote, 0.0f), offSample);
                if (currentlySoundingGeneratedNote_ == n.midiNote)
                    currentlySoundingGeneratedNote_ = -1;
            }

            const int onSample = isTickInWindow(noteOnTick);
            if (onSample >= 0 && onSample < numSamples) {
                const float velNorm = static_cast<float>(n.velocity) / 127.0f;
                other.releaseHeld(); // monophonic hand-off between the two slots
                target.noteOn(n.midiNote, velNorm);
                currentlySoundingGeneratedNote_ = n.midiNote;
                currentGenChannel_ = channel;
                if (liveOutEnabled)
                    midiMessages.addEvent(juce::MidiMessage::noteOn(channel, n.midiNote, velNorm), onSample);
            }
        }
    } else {
        // Transport stopped: ensure no stuck generated notes
        if (currentlySoundingGeneratedNote_ >= 0) {
            sampler.releaseHeld();
            samplerB.releaseHeld();
            if (liveOutEnabled)
                midiMessages.addEvent(juce::MidiMessage::noteOff(currentGenChannel_, currentlySoundingGeneratedNote_, 0.0f), 0);
            currentlySoundingGeneratedNote_ = -1;
        }
    }

    soundingNoteForUi_.store(liveHeldNote_ >= 0 ? liveHeldNote_ : currentlySoundingGeneratedNote_,
                             std::memory_order_relaxed);

    // 3. Render both monophonic sampler slots
    sampler.renderNextBlock(buffer, 0, numSamples);
    samplerB.renderNextBlock(buffer, 0, numSamples);
}

void BleekBassBoxProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto state = apvts.copyState();
    state.setProperty("seed", static_cast<int64_t>(currentSeed_.load()), nullptr);

    // Remember which samples were loaded (full paths), so the set reopens with them
    auto pathOf = [](const MonoSampler& s) -> juce::String {
        auto a = s.getCurrentAsset();
        return (a && !a->isFallbackSine) ? a->filePath : juce::String();
    };
    state.setProperty("sampleA", pathOf(sampler), nullptr);
    state.setProperty("sampleB", bFollowsA_.load() ? juce::String() : pathOf(samplerB), nullptr);

    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void BleekBassBoxProcessor::setStateInformation(const void* data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState && xmlState->hasTagName(apvts.state.getType())) {
        auto tree = juce::ValueTree::fromXml(*xmlState);
        if (tree.hasProperty("seed")) {
            currentSeed_.store(static_cast<uint32_t>(static_cast<int64_t>(tree.getProperty("seed"))));
        }
        const juce::String pathA = tree.getProperty("sampleA").toString();
        const juce::String pathB = tree.getProperty("sampleB").toString();

        apvts.replaceState(tree);

        // Reload saved samples without overriding the saved key/root
        if (pathA.isNotEmpty() && juce::File(pathA).existsAsFile())
            loadSampleFile(juce::File(pathA), 0, false);
        if (pathB.isNotEmpty() && juce::File(pathB).existsAsFile())
            loadSampleFile(juce::File(pathB), 1, false);
        else
            resetToDefaultSine(1); // B follows A

        applyAdsrToSamplers();
        triggerRegenerate(false);
    }
}

juce::AudioProcessorEditor* BleekBassBoxProcessor::createEditor() {
    return new BleekBassBoxEditor(*this);
}

} // namespace bleek

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new bleek::BleekBassBoxProcessor();
}
