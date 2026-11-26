#include "MonoSampler.h"
#include <algorithm>
#include <numeric>

namespace bleek {

namespace {
    const char* kPitchClassNames[12] = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
    };

    juce::String formatKeyLabel(int pitchClass, bool isMinor, int rootMidi) {
        const int pc = ((pitchClass % 12) + 12) % 12;
        const int octave = (rootMidi / 12) - 1;
        return juce::String(kPitchClassNames[pc])
             + (isMinor ? " Min" : " Maj")
             + " (" + juce::String(kPitchClassNames[rootMidi % 12]) + juce::String(octave) + ")";
    }
}

MonoSampler::MonoSampler() {
    loadFallbackSine();
}

void MonoSampler::prepareToPlay(double sampleRate, int /*samplesPerBlock*/) {
    hostSampleRate_ = (sampleRate > 0.0) ? sampleRate : 44100.0;
    mainVoice_.adsr.setSampleRate(hostSampleRate_);
    stealVoice_.adsr.setSampleRate(hostSampleRate_);
    mainVoice_.adsr.setParameters(adsrParams_);
    stealVoice_.adsr.setParameters(adsrParams_);
}

void MonoSampler::releaseResources() {
    allNotesOff();
}

void MonoSampler::loadFallbackSine() {
    auto asset = std::make_shared<SampleAsset>();
    const double sr = 48000.0;
    const int rootMidi = 29; // F1 (43.6535 Hz)
    const double freq = 440.0 * std::pow(2.0, (rootMidi - 69) / 12.0);
    const int numSamples = static_cast<int>(sr * 4.0); // 4-second seamless sub cycle

    asset->sampleRate = sr;
    asset->detectedRootMidi = rootMidi;
    asset->detectedKey = { 5, true, rootMidi, 1.0f, "F Min (F1)" };
    asset->fileName = "INTERNAL PURE SUB (SINE)";
    asset->isFallbackSine = true;
    asset->buffer.setSize(2, numSamples);

    const double twoPi = juce::MathConstants<double>::twoPi;
    for (int i = 0; i < numSamples; ++i) {
        const double phase = twoPi * freq * (static_cast<double>(i) / sr);
        // Pure fundamental sub with subtle 2nd harmonic warmth (-32 dB) for small speakers
        const float sample = static_cast<float>(0.92 * std::sin(phase) + 0.025 * std::sin(2.0 * phase));
        asset->buffer.setSample(0, i, sample);
        asset->buffer.setSample(1, i, sample);
    }

    std::atomic_store(&activeAsset_, std::shared_ptr<const SampleAsset>(asset));
}

bool MonoSampler::loadSampleFromFile(const juce::File& file) {
    if (!file.existsAsFile())
        return false;

    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats(); // WAV, AIFF, FLAC

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (reader == nullptr || reader->lengthInSamples <= 0)
        return false;

    // Cap at 30 seconds for bass one-shots
    const int maxSamples = static_cast<int>(std::min<juce::int64>(reader->lengthInSamples,
                                                                  static_cast<juce::int64>(reader->sampleRate * 30.0)));
    const int numChannels = std::min(2, static_cast<int>(reader->numChannels));

    auto asset = std::make_shared<SampleAsset>();
    asset->buffer.setSize(2, maxSamples);
    asset->buffer.clear();

    juce::AudioBuffer<float> tempBuf(numChannels, maxSamples);
    reader->read(&tempBuf, 0, maxSamples, 0, true, true);

    for (int i = 0; i < maxSamples; ++i) {
        const float L = tempBuf.getSample(0, i);
        const float R = (numChannels > 1) ? tempBuf.getSample(1, i) : L;
        asset->buffer.setSample(0, i, L);
        asset->buffer.setSample(1, i, R);
    }

    asset->sampleRate = reader->sampleRate;
    asset->fileName = file.getFileName();
    asset->isFallbackSine = false;
    asset->detectedKey = detectMusicalKey(asset->buffer, asset->sampleRate, asset->fileName);
    asset->detectedRootMidi = asset->detectedKey.recommendedRootMidi;

    userRootMidi_.store(asset->detectedRootMidi, std::memory_order_relaxed);
    std::atomic_store(&activeAsset_, std::shared_ptr<const SampleAsset>(asset));
    return true;
}

int MonoSampler::estimateRootPitchFromBuffer(const juce::AudioBuffer<float>& buf, double sr, float& outConfidence) {
    outConfidence = 0.0f;
    // Autocorrelation pitch detector over sustain region (25 ms - 125 ms) for bass hits
    const int start = std::min(buf.getNumSamples() / 4, static_cast<int>(sr * 0.025));
    const int window = std::min(buf.getNumSamples() - start, static_cast<int>(sr * 0.100));
    if (window < 512) return 29; // Default F1

    const int minLag = static_cast<int>(sr / 160.0); // ~E3 max bass fundamental
    const int maxLag = std::min(window / 2, static_cast<int>(sr / 27.5)); // A0 min
    if (maxLag <= minLag) return 29;

    const float* data = buf.getReadPointer(0, start);
    int bestLag = minLag;
    double bestCorr = -1.0;

    for (int lag = minLag; lag <= maxLag; ++lag) {
        double corr = 0.0;
        double energy = 0.0;
        for (int i = 0; i < window - lag; ++i) {
            corr += static_cast<double>(data[i]) * static_cast<double>(data[i + lag]);
            energy += static_cast<double>(data[i]) * static_cast<double>(data[i]);
        }
        const double norm = (energy > 1e-9) ? (corr / energy) : 0.0;
        if (norm > bestCorr) {
            bestCorr = norm;
            bestLag = lag;
        }
    }

    outConfidence = static_cast<float>(std::clamp(bestCorr, 0.0, 1.0));
    if (bestCorr < 0.45) return 29; // Unpitched/noisy hit -> default F1

    const double detectedHz = sr / static_cast<double>(bestLag);
    const int midi = static_cast<int>(std::round(69.0 + 12.0 * std::log2(detectedHz / 440.0)));
    return std::clamp(midi, 24, 48); // Clamp within C1 - C3 bass root range
}

bool MonoSampler::parseKeyFromFilename(const juce::String& fileName, DetectedKeyInfo& outInfo) {
    // Tokenize stem by common sample-pack delimiters (_, -, space, ., (, ))
    const auto stem = juce::File::createFileWithoutCheckingPath(fileName).getFileNameWithoutExtension();
    juce::StringArray tokens;
    tokens.addTokens(stem, "_-. ()[]", "");

    auto parsePitchClassToken = [](const juce::String& raw) -> std::pair<int, int> {
        if (raw.isEmpty()) return { -1, 0 };
        const juce::juce_wchar c0 = juce::CharacterFunctions::toUpperCase(raw[0]);
        int base = -1;
        switch (c0) {
            case 'C': base = 0; break;
            case 'D': base = 2; break;
            case 'E': base = 4; break;
            case 'F': base = 5; break;
            case 'G': base = 7; break;
            case 'A': base = 9; break;
            case 'B': base = 11; break;
            default: return { -1, 0 };
        }
        int consumed = 1;
        if (raw.length() >= 2) {
            const juce::juce_wchar c1 = raw[1];
            if (c1 == '#') {
                base = (base + 1) % 12;
                consumed = 2;
            } else if (c1 == 'b' || c1 == 'B') {
                if (raw.length() == 2 || !juce::CharacterFunctions::isLetter(raw[2]) ||
                    raw.substring(2).equalsIgnoreCase("m") ||
                    raw.substring(2).equalsIgnoreCase("min") ||
                    raw.substring(2).equalsIgnoreCase("minor") ||
                    raw.substring(2).equalsIgnoreCase("maj") ||
                    raw.substring(2).equalsIgnoreCase("major"))
                {
                    base = (base + 11) % 12;
                    consumed = 2;
                }
            }
        }
        return { base, consumed };
    };

    for (int i = 0; i < tokens.size(); ++i) {
        const auto tok = tokens[i];
        auto [pc, consumed] = parsePitchClassToken(tok);
        if (pc < 0) continue;

        const auto rest = tok.substring(consumed);
        if (rest.containsOnly("0123456789") && rest.length() >= 2)
            continue;

        bool foundQuality = false;
        bool isMinor = true;

        if (rest.equalsIgnoreCase("m") || rest.equalsIgnoreCase("min") || rest.equalsIgnoreCase("minor")) {
            foundQuality = true;
            isMinor = true;
        } else if (rest.equalsIgnoreCase("maj") || rest.equalsIgnoreCase("major")) {
            foundQuality = true;
            isMinor = false;
        } else if (rest.isEmpty() && i + 1 < tokens.size()) {
            const auto nextTok = tokens[i + 1];
            if (nextTok.equalsIgnoreCase("m") || nextTok.equalsIgnoreCase("min") || nextTok.equalsIgnoreCase("minor")) {
                foundQuality = true;
                isMinor = true;
            } else if (nextTok.equalsIgnoreCase("maj") || nextTok.equalsIgnoreCase("major")) {
                foundQuality = true;
                isMinor = false;
            }
        }

        if (foundQuality) {
            const int rootMidi = 24 + pc; // C1..B1 octave
            outInfo.pitchClass = pc;
            outInfo.isMinor = isMinor;
            outInfo.recommendedRootMidi = rootMidi;
            outInfo.confidence = 0.98f;
            outInfo.label = formatKeyLabel(pc, isMinor, rootMidi);
            return true;
        }
    }

    // Second pass: a bare note tag with no quality, e.g. "FSL_C#_808_Bass", "Sub_F", "Reese_Ab1".
    // Sample packs usually tag one-shots with just the root note; treat it as minor (DnB default).
    // Only single-letter notes need the token to stand alone, so words like "Bass"/"Deep" never match.
    for (int i = 0; i < tokens.size(); ++i) {
        const auto tok = tokens[i];
        auto [pc, consumed] = parsePitchClassToken(tok);
        if (pc < 0) continue;

        const auto rest = tok.substring(consumed);
        int octave = -1;
        if (rest.isEmpty()) {
            // A lone capital letter only counts if it is upper case (avoid "a", "e" words)
            if (consumed == 1 && !juce::CharacterFunctions::isUpperCase(tok[0]))
                continue;
        } else if (rest.length() == 1 && rest.containsOnly("0123456")) {
            octave = rest.getIntValue();
        } else {
            continue;
        }

        int rootMidi = 24 + pc; // C1..B1 octave by default
        if (octave >= 0) {
            rootMidi = 12 * (octave + 1) + pc;
            while (rootMidi > 47) rootMidi -= 12; // keep within the bass root range C1..B2
            while (rootMidi < 24) rootMidi += 12;
        }

        outInfo.pitchClass = pc;
        outInfo.isMinor = true;
        outInfo.recommendedRootMidi = rootMidi;
        outInfo.confidence = 0.90f;
        outInfo.label = formatKeyLabel(pc, true, rootMidi);
        return true;
    }
    return false;
}

DetectedKeyInfo MonoSampler::detectMusicalKey(const juce::AudioBuffer<float>& buf,
                                              double sr,
                                              const juce::String& fileName)
{
    DetectedKeyInfo tagInfo;
    if (parseKeyFromFilename(fileName, tagInfo)) {
        float pitchConf = 0.0f;
        const int estMidi = estimateRootPitchFromBuffer(buf, sr, pitchConf);
        if (pitchConf > 0.65f && (estMidi % 12) == tagInfo.pitchClass) {
            tagInfo.recommendedRootMidi = estMidi;
            tagInfo.label = formatKeyLabel(tagInfo.pitchClass, tagInfo.isMinor, estMidi);
        }
        return tagInfo;
    }

    std::array<double, 12> chroma {};
    const int totalSamples = buf.getNumSamples();
    const int analysisLimit = std::min(totalSamples, static_cast<int>(sr * 6.0));
    const int frameSize = 4096;
    const int hopSize = 2048;

    if (analysisLimit >= 1024 && sr > 0.0) {
        const float* monoL = buf.getReadPointer(0);
        const float* monoR = (buf.getNumChannels() > 1) ? buf.getReadPointer(1) : monoL;

        struct Bin { int pc; double coeff; double weight; };
        std::vector<Bin> bins;
        bins.reserve(48);
        for (int m = 24; m <= 71; ++m) {
            const double freq = 440.0 * std::pow(2.0, (m - 69) / 12.0);
            if (freq >= sr * 0.45) continue;
            const double omega = juce::MathConstants<double>::twoPi * freq / sr;
            const double w = (m <= 47) ? 1.45 : 0.85;
            bins.push_back({ m % 12, 2.0 * std::cos(omega), w });
        }

        std::vector<double> frame(frameSize, 0.0);
        for (int start = 0; start + frameSize <= analysisLimit; start += hopSize) {
            double frameEnergy = 0.0;
            for (int n = 0; n < frameSize; ++n) {
                const double win = 0.5 * (1.0 - std::cos(juce::MathConstants<double>::twoPi * n / (frameSize - 1)));
                const double s = 0.5 * (static_cast<double>(monoL[start + n]) + static_cast<double>(monoR[start + n])) * win;
                frame[n] = s;
                frameEnergy += s * s;
            }
            if (frameEnergy < 1e-7) continue;

            for (const auto& b : bins) {
                double s1 = 0.0, s2 = 0.0;
                for (int n = 0; n < frameSize; ++n) {
                    const double s0 = frame[n] + b.coeff * s1 - s2;
                    s2 = s1;
                    s1 = s0;
                }
                const double power = std::max(0.0, s1 * s1 + s2 * s2 - b.coeff * s1 * s2);
                chroma[b.pc] += std::sqrt(power) * b.weight;
            }
        }
    }

    const double chromaSum = std::accumulate(chroma.begin(), chroma.end(), 0.0);
    if (chromaSum > 1e-9) {
        for (double& c : chroma) c /= chromaSum;
    }

    static constexpr std::array<double, 12> kMinorProfile = {
        6.33, 2.68, 3.52, 5.38, 2.60, 3.53, 2.54, 4.75, 3.98, 2.69, 3.34, 3.17
    };
    static constexpr std::array<double, 12> kMajorProfile = {
        6.35, 2.23, 3.48, 2.33, 4.38, 4.09, 2.52, 5.19, 2.39, 3.66, 2.29, 2.88
    };

    auto correlateProfile = [&](int tonic, const std::array<double, 12>& prof) -> double {
        double meanC = 1.0 / 12.0;
        double meanP = std::accumulate(prof.begin(), prof.end(), 0.0) / 12.0;
        double num = 0.0, denC = 0.0, denP = 0.0;
        for (int i = 0; i < 12; ++i) {
            const double dc = chroma[(tonic + i) % 12] - meanC;
            const double dp = prof[i] - meanP;
            num += dc * dp;
            denC += dc * dc;
            denP += dp * dp;
        }
        const double denom = std::sqrt(denC * denP);
        return (denom > 1e-12) ? (num / denom) : 0.0;
    };

    int bestTonic = 5;
    bool bestIsMinor = true;
    double bestScore = -2.0;

    for (int t = 0; t < 12; ++t) {
        const double minorScore = correlateProfile(t, kMinorProfile) + 0.03;
        const double majorScore = correlateProfile(t, kMajorProfile);

        if (minorScore > bestScore) {
            bestScore = minorScore;
            bestTonic = t;
            bestIsMinor = true;
        }
        if (majorScore > bestScore) {
            bestScore = majorScore;
            bestTonic = t;
            bestIsMinor = false;
        }
    }

    float pitchConf = 0.0f;
    const int fundamentalMidi = estimateRootPitchFromBuffer(buf, sr, pitchConf);

    int finalPitchClass = bestTonic;
    int finalRootMidi = 24 + bestTonic;

    if (pitchConf >= 0.72f) {
        finalPitchClass = fundamentalMidi % 12;
        finalRootMidi = fundamentalMidi;
        bestIsMinor = true;
    } else if ((fundamentalMidi % 12) == bestTonic) {
        finalRootMidi = fundamentalMidi;
    }

    DetectedKeyInfo info;
    info.pitchClass = finalPitchClass;
    info.isMinor = bestIsMinor;
    info.recommendedRootMidi = std::clamp(finalRootMidi, 24, 47);
    info.confidence = static_cast<float>(std::clamp(std::max<double>(bestScore, pitchConf), 0.0, 1.0));
    info.label = formatKeyLabel(info.pitchClass, info.isMinor, info.recommendedRootMidi);
    return info;
}

void MonoSampler::setADSR(float attackMs, float decayMs, float sustainLevel, float releaseMs) {
    adsrParams_.attack  = std::clamp(attackMs, 1.0f, 500.0f) * 0.001f;
    adsrParams_.decay   = std::clamp(decayMs, 10.0f, 3000.0f) * 0.001f;
    adsrParams_.sustain = std::clamp(sustainLevel, 0.0f, 1.0f);
    adsrParams_.release = std::clamp(releaseMs, 5.0f, 2000.0f) * 0.001f;

    mainVoice_.adsr.setParameters(adsrParams_);
    stealVoice_.adsr.setParameters(adsrParams_);
}

void MonoSampler::noteOn(int midiNote, float velocity) {
    auto asset = std::atomic_load(&activeAsset_);
    if (!asset || asset->buffer.getNumSamples() < 4)
        return;

    // Monophonic Voice Stealing: transfer active voice to stealVoice_ for 6 ms linear de-click fade
    if (mainVoice_.active) {
        stealVoice_ = mainVoice_;
        stealVoice_.isFadingOutForSteal = true;
        stealVoice_.stealGain = 1.0f;
        const float fadeSamples = std::max(1.0f, static_cast<float>(hostSampleRate_ * 0.006)); // 6 ms
        stealVoice_.stealStep = 1.0f / fadeSamples;
    }

    const int root = userRootMidi_.load(std::memory_order_relaxed);
    const double semitoneOffset = static_cast<double>(midiNote - root);
    const double pitchFactor = std::pow(2.0, semitoneOffset / 12.0);
    const double srRatio = asset->sampleRate / hostSampleRate_;

    mainVoice_.active = true;
    mainVoice_.midiNote = midiNote;
    mainVoice_.velocity = std::clamp(velocity, 0.0f, 1.0f);
    mainVoice_.sourcePosition = 0.0;
    mainVoice_.pitchRatio = pitchFactor * srRatio;
    mainVoice_.stealGain = 1.0f;
    mainVoice_.stealStep = 0.0f;
    mainVoice_.isFadingOutForSteal = false;
    mainVoice_.interpL.reset();
    mainVoice_.interpR.reset();
    mainVoice_.adsr.setSampleRate(hostSampleRate_);
    mainVoice_.adsr.setParameters(adsrParams_);
    mainVoice_.adsr.noteOn();
}

void MonoSampler::noteOff(int midiNote) {
    if (mainVoice_.active && mainVoice_.midiNote == midiNote) {
        mainVoice_.adsr.noteOff();
    }
}

void MonoSampler::allNotesOff() {
    mainVoice_.active = false;
    mainVoice_.adsr.reset();
    stealVoice_.active = false;
    stealVoice_.adsr.reset();
}

void MonoSampler::renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) {
    auto asset = std::atomic_load(&activeAsset_);
    if (!asset || asset->buffer.getNumSamples() < 4)
        return;

    const int totalSrcSamples = asset->buffer.getNumSamples();
    const float* srcL = asset->buffer.getReadPointer(0);
    const float* srcR = asset->buffer.getReadPointer(1);

    auto renderVoice = [&](Voice& v) {
        if (!v.active) return;

        for (int i = 0; i < numSamples; ++i) {
            if (!v.adsr.isActive()) {
                v.active = false;
                break;
            }

            if (v.isFadingOutForSteal) {
                v.stealGain -= v.stealStep;
                if (v.stealGain <= 0.0f) {
                    v.active = false;
                    break;
                }
            }

            if (asset->isFallbackSine) {
                // Loop internal pure sub seamlessly
                while (v.sourcePosition >= static_cast<double>(totalSrcSamples - 4))
                    v.sourcePosition -= static_cast<double>(totalSrcSamples - 4);
            } else if (v.sourcePosition >= static_cast<double>(totalSrcSamples - 4)) {
                v.active = false;
                break;
            }

            // High-quality 4-point cubic Lagrange interpolation
            const int idx0 = static_cast<int>(v.sourcePosition);
            const float frac = static_cast<float>(v.sourcePosition - static_cast<double>(idx0));

            auto cubicSample = [&](const float* src) -> float {
                const int i0 = std::max(0, idx0 - 1);
                const int i1 = idx0;
                const int i2 = std::min(totalSrcSamples - 1, idx0 + 1);
                const int i3 = std::min(totalSrcSamples - 1, idx0 + 2);

                const float y0 = src[i0], y1 = src[i1], y2 = src[i2], y3 = src[i3];
                const float c0 = y1;
                const float c1 = 0.5f * (y2 - y0);
                const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
                const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
                return ((c3 * frac + c2) * frac + c1) * frac + c0;
            };

            const float env = v.adsr.getNextSample();
            const float gain = env * v.velocity * v.stealGain;

            const float outL = cubicSample(srcL) * gain;
            const float outR = cubicSample(srcR) * gain;

            outputBuffer.addSample(0, startSample + i, outL);
            if (outputBuffer.getNumChannels() > 1)
                outputBuffer.addSample(1, startSample + i, outR);

            v.sourcePosition += v.pitchRatio;
        }
    };

    renderVoice(stealVoice_);
    renderVoice(mainVoice_);
}

} // namespace bleek
