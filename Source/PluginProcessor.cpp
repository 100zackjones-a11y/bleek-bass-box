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
        "attack", "decay", "sustain", "release", "liveMidiOut"
    };
    for (const auto* id : paramIds)
        apvts.addParameterListener(id, this);

    triggerRegenerate(false);
}

BleekBassBoxProcessor::~BleekBassBoxProcessor() {
    const char* paramIds[] = {
        "era", "archetype", "drumContext", "rootNote", "phraseBars",
        "noteDensity", "syncopation", "phraseVariation", "noteLength",
        "chromaticism", "octaveMovement", "drumAwareness", "swing",
        "lockRhythm", "lockPitch", "lockPhrase", "lockGroove",
        "attack", "decay", "sustain", "release", "liveMidiOut"
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
        parameterID == "sustain" || parameterID == "release")
    {
        sampler.setADSR(apvts.getRawParameterValue("attack")->load(),
                        apvts.getRawParameterValue("decay")->load(),
                        apvts.getRawParameterValue("sustain")->load(),
                        apvts.getRawParameterValue("release")->load());
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

bool BleekBassBoxProcessor::loadSampleFile(const juce::File& file) {
    if (sampler.loadSampleFromFile(file)) {
        const int detected = sampler.getRootMidiNote();
        if (auto* p = apvts.getParameter("rootNote"))
            p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(detected - 24)));
        needsRegeneration_.store(true, std::memory_order_release);
        return true;
    }
    return false;
}

void BleekBassBoxProcessor::resetToDefaultSine() {
    sampler.loadFallbackSine();
    sampler.setRootMidiNote(29);
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
    sampler.setADSR(apvts.getRawParameterValue("attack")->load(),
                    apvts.getRawParameterValue("decay")->load(),
                    apvts.getRawParameterValue("sustain")->load(),
                    apvts.getRawParameterValue("release")->load());
}

void BleekBassBoxProcessor::releaseResources() {
    sampler.releaseResources();
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
        if (msg.isNoteOn())
            sampler.noteOn(msg.getNoteNumber(), msg.getFloatVelocity());
        else if (msg.isNoteOff())
            sampler.noteOff(msg.getNoteNumber());
        else if (msg.isAllNotesOff() || msg.isAllSoundOff())
            sampler.allNotesOff();
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

        for (const auto& n : notesCopy) {
            const double noteOnTick = static_cast<double>(n.startTick);
            const double noteOffTick = wrapTick(static_cast<double>(n.startTick + n.durationTicks));

            const int offSample = isTickInWindow(noteOffTick);
            if (offSample >= 0 && offSample < numSamples) {
                sampler.noteOff(n.midiNote);
                if (liveOutEnabled)
                    midiMessages.addEvent(juce::MidiMessage::noteOff(1, n.midiNote, 0.0f), offSample);
                if (currentlySoundingGeneratedNote_ == n.midiNote)
                    currentlySoundingGeneratedNote_ = -1;
            }

            const int onSample = isTickInWindow(noteOnTick);
            if (onSample >= 0 && onSample < numSamples) {
                const float velNorm = static_cast<float>(n.velocity) / 127.0f;
                sampler.noteOn(n.midiNote, velNorm);
                currentlySoundingGeneratedNote_ = n.midiNote;
                if (liveOutEnabled)
                    midiMessages.addEvent(juce::MidiMessage::noteOn(1, n.midiNote, velNorm), onSample);
            }
        }
    } else {
        // Transport stopped: ensure no stuck generated notes
        if (currentlySoundingGeneratedNote_ >= 0) {
            sampler.noteOff(currentlySoundingGeneratedNote_);
            if (liveOutEnabled)
                midiMessages.addEvent(juce::MidiMessage::noteOff(1, currentlySoundingGeneratedNote_, 0.0f), 0);
            currentlySoundingGeneratedNote_ = -1;
        }
    }

    // 3. Render monophonic bass sampler
    sampler.renderNextBlock(buffer, 0, numSamples);
}

void BleekBassBoxProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto state = apvts.copyState();
    state.setProperty("seed", static_cast<int64_t>(currentSeed_.load()), nullptr);
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
        apvts.replaceState(tree);
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
