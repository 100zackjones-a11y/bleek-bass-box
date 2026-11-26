#pragma once
#include "MidiExporter.h"
#include <array>
#include <vector>
#include <cstdint>
#include <string>

namespace bleek {

enum class EraPreset : int {
    RaggaJungle_94_95 = 0,
    DarkJungle_96_97,
    TechstepNeuro_98_99,
    MinimalRollers_00_01,
    LiquidJumpUp_02_04,
    Liminal_15_24,          // modern minimal: held sub, root <-> b2, slide-in grace notes
    Count
};

enum class BassArchetype : int {
    RootsJungleSub = 0,
    Sub808,
    DubInfluenced,
    ReeseBassline,
    Techstep,
    Neurofunk,
    RollingBassline,
    MinimalDarkRoller,
    LiquidFunk,
    JumpUp,
    StabbingBassline,
    PedalNotePulse,
    LiminalSubRoller,       // long held sub notes, b2 bar movement, grace slides, bar-4 fill
    LiminalPulse,           // 3-step (2 on / 1 off) root pulse, lower-neighbour fills
    Count
};

enum class DrumTemplate : int {
    TwoStepDnB = 0,
    AmenBreak,
    ThinkBreak,
    Rolling16th,
    Count
};

enum class DrumInteractionMode : int {
    ReinforceKick = 0,
    DubAvoidance,
    CounterpointPocket,
    AnticipateTransient
};

enum class DrumHit : uint8_t {
    None = 0,
    Kick,
    Snare,
    Ghost
};

struct EraConfig {
    const char* name;
    double defaultBpm;
    float defaultSwing; // 0.50 = straight, 0.53 = jungle pocket
    DrumTemplate defaultDrum;
    BassArchetype defaultArchetype;
};

struct GenerationParams {
    EraPreset era = EraPreset::RaggaJungle_94_95;
    BassArchetype archetype = BassArchetype::RootsJungleSub;
    DrumTemplate drumTemplate = DrumTemplate::AmenBreak;
    int rootMidiNote = 29;       // F1 (MIDI 29) classic jungle sub root
    int phraseBars = 4;          // 1, 2, 4, or 8
    uint32_t seed = 19940901u;

    float noteDensity = 0.50f;     // 0.0 - 1.0
    float syncopation = 0.55f;     // 0.0 - 1.0
    float phraseVariation = 0.35f; // 0.0 - 1.0
    float noteLength = 0.55f;      // 0.0 - 1.0
    float chromaticism = 0.30f;    // 0.0 - 1.0
    float octaveMovement = 0.35f;  // 0.0 - 1.0
    float drumAwareness = 0.75f;   // 0.0 - 1.0
    float swing = 0.53f;           // 0.50 - 0.62

    bool lockRhythm = false;
    bool lockPitch = false;
    bool lockPhrase = false;
    bool lockGroove = false;

    // Call & Response: odd bars answer the bar before them
    bool callResponse = false;
    float responseAmount = 0.5f;   // 0 = near echo, 1 = strongly contrasting answer
    int responseDirection = 0;     // 0 = Up, 1 = Down, 2 = Mirror
};

// Deterministic Mulberry32 PRNG for reproducible seeds across macOS/Windows
struct Mulberry32 {
    uint32_t state;
    explicit Mulberry32(uint32_t seed = 1u) : state(seed == 0u ? 0x9E3779B9u : seed) {}

    uint32_t nextU32() {
        uint32_t z = (state += 0x6D2B79F5u);
        z = (z ^ (z >> 15)) * (z | 1u);
        z ^= z + (z ^ (z >> 7)) * (z | 61u);
        return z ^ (z >> 14);
    }

    float nextFloat() {
        return static_cast<float>(nextU32() >> 8) * (1.0f / 16777216.0f);
    }

    int nextInt(int minInclusive, int maxInclusive) {
        if (maxInclusive <= minInclusive) return minInclusive;
        const uint32_t span = static_cast<uint32_t>(maxInclusive - minInclusive + 1);
        return minInclusive + static_cast<int>(nextU32() % span);
    }
};

class BassEngine {
public:
    static constexpr int kStepsPerBar = 16;
    static constexpr int kTicksPerStep = 120; // 480 PPQN / 4
    static constexpr int kTicksPerBar = 1920;

    BassEngine();

    static EraConfig getEraConfig(EraPreset era);
    static const char* getArchetypeName(BassArchetype arch);
    static const char* getDrumTemplateName(DrumTemplate tmpl);
    static std::array<DrumHit, 16> getDrumGrid(DrumTemplate tmpl);

    // Regenerates pattern honoring dimension locks (Rhythm, Pitch, Phrase, Groove)
    void generate(const GenerationParams& params);

    const std::vector<MidiNoteEvent>& getPattern() const noexcept { return currentNotes_; }
    std::vector<MidiNoteEvent> getNotesForBar(int barIndex) const;

private:
    struct StepSlot {
        bool active = false;
        int interval = 0;      // semitones relative to root
        int durationSteps = 1; // in 16th steps (0 = 32nd-note half-step turnaround)
        bool isTurnaround32nd = false;
        int velocity = 110;
    };

    using BarPattern = std::array<StepSlot, 16>;

    std::vector<BarPattern> cachedBars_;
    std::vector<MidiNoteEvent> currentNotes_;

    BarPattern generateBarRhythmAndPitch(const GenerationParams& p,
                                         int barIdx,
                                         Mulberry32& rng,
                                         const BarPattern* motifA,
                                         bool isTurnaroundBar);

    void applyPitchContour(BarPattern& bar,
                           const GenerationParams& p,
                           int barIdx,
                           Mulberry32& rng,
                           bool isTurnaroundBar);

    void compileAndValidateMidi(const std::vector<BarPattern>& bars,
                                const GenerationParams& p);

    void applyCallResponse(std::vector<BarPattern>& bars, const GenerationParams& p);

    // Whole-phrase generator for the Liminal archetypes (they think in phrases, not bars)
    std::vector<BarPattern> generateLiminalPhrase(const GenerationParams& p, Mulberry32& rng, int numBars);
    static bool isLiminal(BassArchetype a) noexcept {
        return a == BassArchetype::LiminalSubRoller || a == BassArchetype::LiminalPulse;
    }
};

} // namespace bleek
