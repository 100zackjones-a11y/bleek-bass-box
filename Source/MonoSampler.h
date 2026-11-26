#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_dsp/juce_dsp.h>
#include <memory>
#include <atomic>
#include <cmath>
#include <array>

namespace bleek {

struct DetectedKeyInfo {
    int pitchClass = 5;              // 0 = C, 1 = C#, ..., 5 = F, ..., 11 = B
    bool isMinor = true;             // true = Minor, false = Major
    int recommendedRootMidi = 29;    // Octave-normalized bass root MIDI (C1=24 .. B1=35)
    float confidence = 1.0f;         // 0.0 - 1.0
    juce::String label { "F Min (F1)" };
};

struct SampleAsset {
    juce::AudioBuffer<float> buffer;
    double sampleRate = 44100.0;
    int detectedRootMidi = 29; // F1 default
    DetectedKeyInfo detectedKey;
    juce::String fileName { "INTERNAL PURE SUB (SINE)" };
    bool isFallbackSine = true;
};

class MonoSampler {
public:
    MonoSampler();

    void prepareToPlay(double sampleRate, int samplesPerBlock);
    void releaseResources();

    // Thread-safe sample loading from UI / Drag-and-Drop
    bool loadSampleFromFile(const juce::File& file);
    void loadFallbackSine();

    void setRootMidiNote(int rootNote) noexcept { userRootMidi_.store(rootNote, std::memory_order_relaxed); }
    int getRootMidiNote() const noexcept { return userRootMidi_.load(std::memory_order_relaxed); }

    void setADSR(float attackMs, float decayMs, float sustainLevel, float releaseMs);

    void noteOn(int midiNote, float velocity);
    void noteOff(int midiNote);
    void allNotesOff();

    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples);

    std::shared_ptr<const SampleAsset> getCurrentAsset() const {
        return std::atomic_load(&activeAsset_);
    }

private:
    struct Voice {
        bool active = false;
        int midiNote = 29;
        float velocity = 1.0f;
        double sourcePosition = 0.0;
        double pitchRatio = 1.0;
        juce::ADSR adsr;
        juce::LagrangeInterpolator interpL;
        juce::LagrangeInterpolator interpR;

        // De-click crossfade ramp for monophonic voice stealing (6 ms)
        float stealGain = 1.0f;
        float stealStep = 0.0f;
        bool isFadingOutForSteal = false;
    };

    double hostSampleRate_ = 44100.0;
    juce::ADSR::Parameters adsrParams_ { 0.008f, 0.450f, 0.85f, 0.040f };
    std::atomic<int> userRootMidi_ { 29 };

    std::shared_ptr<const SampleAsset> activeAsset_;
    Voice mainVoice_;
    Voice stealVoice_; // Smooth 6 ms anti-click voice-stealing tail

    static int estimateRootPitchFromBuffer(const juce::AudioBuffer<float>& buf, double sr, float& outConfidence);
    static DetectedKeyInfo detectMusicalKey(const juce::AudioBuffer<float>& buf, double sr, const juce::String& fileName);
    static bool parseKeyFromFilename(const juce::String& fileName, DetectedKeyInfo& outInfo);
};

} // namespace bleek
