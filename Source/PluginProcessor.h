#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "BassEngine.h"
#include "MonoSampler.h"
#include "MidiExporter.h"
#include <atomic>
#include <mutex>

namespace bleek {

class BleekBassBoxProcessor : public juce::AudioProcessor,
                              public juce::AudioProcessorValueTreeState::Listener {
public:
    BleekBassBoxProcessor();
    ~BleekBassBoxProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "BLEEK BASS BOX"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.5; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    void parameterChanged(const juce::String& parameterID, float newValue) override;

    // UI Actions
    void triggerRegenerate(bool randomizeSeed = true);
    void applyEraPreset(EraPreset era);
    // Slot 0 = A (CALL), slot 1 = B (ANSWER). Slot B follows A until it gets its own sample.
    bool loadSampleFile(const juce::File& file, int slot = 0, bool setKeyFromSample = true);
    void resetToDefaultSine(int slot = 0);
    MonoSampler& getSampler(int slot) noexcept { return slot == 1 ? samplerB : sampler; }
    bool isSlotBFollowingA() const noexcept { return bFollowsA_.load(std::memory_order_relaxed); }

    std::vector<MidiNoteEvent> getPatternSnapshot() const;
    double getHostBpm() const noexcept { return currentBpm_.load(std::memory_order_relaxed); }
    double getPlayheadBarPosition() const noexcept { return playheadBarPos_.load(std::memory_order_relaxed); }
    bool isHostPlaying() const noexcept { return isPlaying_.load(std::memory_order_relaxed); }
    uint32_t getCurrentSeed() const noexcept { return currentSeed_.load(std::memory_order_relaxed); }
    int getSoundingNote() const noexcept { return soundingNoteForUi_.load(std::memory_order_relaxed); }

    juce::AudioProcessorValueTreeState apvts;
    MonoSampler sampler;   // slot A (CALL)
    MonoSampler samplerB;  // slot B (ANSWER)

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    GenerationParams buildGenerationParamsFromState() const;

    BassEngine bassEngine_;
    mutable std::mutex patternMutex_;
    std::vector<MidiNoteEvent> activePattern_;

    std::atomic<uint32_t> currentSeed_ { 19940901u };
    std::atomic<double> currentBpm_ { 170.0 };
    std::atomic<double> playheadBarPos_ { 0.0 };
    std::atomic<bool> isPlaying_ { false };
    std::atomic<bool> needsRegeneration_ { true };

    int currentlySoundingGeneratedNote_ = -1;
    std::atomic<int> soundingNoteForUi_ { -1 }; // generated or live-played note, for keyboard highlight
    int liveHeldNote_ = -1;
    int currentGenChannel_ = 1;
    std::atomic<bool> bFollowsA_ { true };

    void applyAdsrToSamplers();
    double lastBlockEndTick_ = -1.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BleekBassBoxProcessor)
};

} // namespace bleek
