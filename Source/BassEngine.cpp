#include "BassEngine.h"
#include <algorithm>
#include <cmath>
#include <numeric>

namespace bleek {

BassEngine::BassEngine() {
    GenerationParams defaults;
    generate(defaults);
}

EraConfig BassEngine::getEraConfig(EraPreset era) {
    switch (era) {
        case EraPreset::RaggaJungle_94_95:
            return { "1994–1995 Foundation / Ragga Jungle", 164.0, 0.530f, DrumTemplate::AmenBreak, BassArchetype::RootsJungleSub };
        case EraPreset::DarkJungle_96_97:
            return { "1996–1997 Dark Jungle / Early Techstep", 170.0, 0.505f, DrumTemplate::TwoStepDnB, BassArchetype::Techstep };
        case EraPreset::TechstepNeuro_98_99:
            return { "1998–1999 Techstep / Early Neurofunk", 172.0, 0.500f, DrumTemplate::TwoStepDnB, BassArchetype::Neurofunk };
        case EraPreset::MinimalRollers_00_01:
            return { "2000–2001 Minimal Dark Rollers", 173.0, 0.500f, DrumTemplate::TwoStepDnB, BassArchetype::MinimalDarkRoller };
        case EraPreset::LiquidJumpUp_02_04:
            return { "2002–2004 Liquid Funk & Jump-Up", 174.0, 0.500f, DrumTemplate::Rolling16th, BassArchetype::LiquidFunk };
        default:
            return { "1994–1995 Foundation / Ragga Jungle", 164.0, 0.530f, DrumTemplate::AmenBreak, BassArchetype::RootsJungleSub };
    }
}

const char* BassEngine::getArchetypeName(BassArchetype arch) {
    switch (arch) {
        case BassArchetype::RootsJungleSub:    return "Roots Jungle Sub-Bass";
        case BassArchetype::Sub808:            return "808-Style Sub";
        case BassArchetype::DubInfluenced:     return "Dub-Influenced Bass";
        case BassArchetype::ReeseBassline:     return "Reese Bassline";
        case BassArchetype::Techstep:          return "Techstep";
        case BassArchetype::Neurofunk:         return "Neurofunk";
        case BassArchetype::RollingBassline:   return "Rolling Bassline";
        case BassArchetype::MinimalDarkRoller: return "Minimal Dark Roller";
        case BassArchetype::LiquidFunk:        return "Liquid Funk";
        case BassArchetype::JumpUp:            return "Jump-Up";
        case BassArchetype::StabbingBassline:  return "Stabbing Bassline";
        case BassArchetype::PedalNotePulse:    return "Pedal-Note Pulse";
        default:                               return "Roots Jungle Sub-Bass";
    }
}

const char* BassEngine::getDrumTemplateName(DrumTemplate tmpl) {
    switch (tmpl) {
        case DrumTemplate::TwoStepDnB:  return "Two-Step DnB";
        case DrumTemplate::AmenBreak:   return "Amen Break";
        case DrumTemplate::ThinkBreak:  return "Think Break";
        case DrumTemplate::Rolling16th: return "Rolling 16th Break";
        default:                        return "Two-Step DnB";
    }
}

std::array<DrumHit, 16> BassEngine::getDrumGrid(DrumTemplate tmpl) {
    using H = DrumHit;
    switch (tmpl) {
        case DrumTemplate::TwoStepDnB:
            return { H::Kick, H::None, H::None, H::None, H::Snare, H::None, H::Kick, H::Ghost,
                     H::None, H::Ghost, H::None, H::None, H::Snare, H::None, H::None, H::Ghost };
        case DrumTemplate::AmenBreak:
            return { H::Kick, H::None, H::Kick, H::None, H::Snare, H::None, H::Ghost, H::Ghost,
                     H::None, H::Ghost, H::Kick, H::Kick, H::Snare, H::None, H::Ghost, H::Ghost };
        case DrumTemplate::ThinkBreak:
            return { H::Kick, H::None, H::Ghost, H::Kick, H::Snare, H::None, H::Ghost, H::Ghost,
                     H::Kick, H::Ghost, H::Kick, H::None, H::Snare, H::None, H::Ghost, H::None };
        case DrumTemplate::Rolling16th:
            return { H::Kick, H::Ghost, H::Ghost, H::Ghost, H::Snare, H::Ghost, H::Kick, H::Ghost,
                     H::Ghost, H::Ghost, H::Kick, H::Ghost, H::Snare, H::Ghost, H::Ghost, H::Ghost };
        default:
            return { H::Kick, H::None, H::None, H::None, H::Snare, H::None, H::Kick, H::None,
                     H::None, H::None, H::None, H::None, H::Snare, H::None, H::None, H::None };
    }
}

void BassEngine::generate(const GenerationParams& p) {
    const int numBars = std::clamp(p.phraseBars, 1, 8);
    if (static_cast<int>(cachedBars_.size()) != numBars) {
        cachedBars_.assign(numBars, BarPattern{});
    }

    Mulberry32 rng(p.seed);

    // If Phrase is locked and we already have bars, preserve phrase topology
    std::vector<BarPattern> workingBars = cachedBars_;

    for (int bar = 0; bar < numBars; ++bar) {
        const bool isFinalBar = (numBars > 1 && bar == numBars - 1);
        const BarPattern* motifA = (bar > 0) ? &workingBars[0] : nullptr;

        BarPattern generated = generateBarRhythmAndPitch(p, bar, rng, motifA, isFinalBar);

        if (p.lockRhythm && bar < static_cast<int>(cachedBars_.size())) {
            // Keep existing onset positions & durations, re-roll pitches unless Pitch is also locked
            for (int s = 0; s < 16; ++s) {
                generated[s].active = cachedBars_[bar][s].active;
                generated[s].durationSteps = cachedBars_[bar][s].durationSteps;
                generated[s].isTurnaround32nd = cachedBars_[bar][s].isTurnaround32nd;
            }
            if (!p.lockPitch) {
                applyPitchContour(generated, p, bar, rng, isFinalBar);
            }
        }

        if (p.lockPitch && bar < static_cast<int>(cachedBars_.size())) {
            // Preserve previous pitch sequence mapped across active notes
            std::vector<int> prevIntervals;
            for (int s = 0; s < 16; ++s) {
                if (cachedBars_[bar][s].active)
                    prevIntervals.push_back(cachedBars_[bar][s].interval);
            }
            if (!prevIntervals.empty()) {
                size_t idx = 0;
                for (int s = 0; s < 16; ++s) {
                    if (generated[s].active) {
                        generated[s].interval = prevIntervals[idx % prevIntervals.size()];
                        ++idx;
                    }
                }
            }
        }

        if (p.lockPhrase && bar > 0) {
            // Enforce strict A-A-A-A or locked phrase structure
            generated = workingBars[0];
        }

        workingBars[bar] = generated;
    }

    if (p.callResponse && numBars >= 2)
        applyCallResponse(workingBars, p);

    cachedBars_ = workingBars;
    compileAndValidateMidi(cachedBars_, p);
}

void BassEngine::applyCallResponse(std::vector<BarPattern>& bars, const GenerationParams& p) {
    // Rule-based (no randomness): an answer repeats its call, then rewrites only the
    // tail of the phrase so it walks - in scale - to a cadence note.
    //   UP     : tail climbs to the 5th above (final bar: home to the root / octave)
    //   DOWN   : tail falls to the 5th below  (final bar: home to the root)
    //   MIRROR : tail inverts the call's shape around the root, then cadences
    // ANSWER AMOUNT = how many tail notes are rewritten (1..4); above ~66% the answer
    // also gets a pickup note on step 15 (off the snare) to push into the next bar.
    const float amt = std::clamp(p.responseAmount, 0.0f, 1.0f);
    const int numBars = static_cast<int>(bars.size());
    const int dir = std::clamp(p.responseDirection, 0, 2);

    static constexpr std::array<int, 4> kLiquidCycle = { 0, 8, 10, 7 };
    auto anchorFor = [&](int bar) {
        return (p.archetype == BassArchetype::LiquidFunk) ? kLiquidCycle[static_cast<size_t>(bar % 4)] : 0;
    };

    // Snap an interval (semitones from the root) to the minor pentatonic: 0 3 5 7 10
    auto snapToScale = [](int rel) {
        static constexpr std::array<int, 6> kDeg = { 0, 3, 5, 7, 10, 12 };
        const int oct = (rel >= 0) ? rel / 12 : -((-rel + 11) / 12);
        const int pc = rel - oct * 12;
        int best = 0, bestDist = 99;
        for (int d : kDeg) {
            if (std::abs(d - pc) < bestDist) { bestDist = std::abs(d - pc); best = d; }
        }
        return oct * 12 + best;
    };

    const auto drums = getDrumGrid(p.drumTemplate);

    for (int b = 1; b < numBars; b += 2) {
        const BarPattern& call = bars[static_cast<size_t>(b - 1)];
        const BarPattern original = bars[static_cast<size_t>(b)];
        BarPattern answer = call; // start as an exact echo of the call

        const int callAnchor = anchorFor(b - 1);
        const int ansAnchor = anchorFor(b);
        const bool isFinalBar = (b == numBars - 1);

        // Keep the answer in its own harmonic place (Liquid Funk chord cycle)
        for (auto& slot : answer)
            if (slot.active) slot.interval = ansAnchor + (slot.interval - callAnchor);

        // ---- Rhythm: identical groove; only an optional pickup at high amounts ----
        if (!p.lockRhythm && amt >= 0.66f && !isFinalBar) {
            int last = -1;
            for (int s = 15; s >= 0; --s) if (answer[static_cast<size_t>(s)].active) { last = s; break; }
            if (last >= 0 && last < 13 && !answer[14].active && drums[14] != DrumHit::Snare) {
                answer[14] = answer[static_cast<size_t>(last)];
                answer[14].durationSteps = 1;
                answer[14].isTurnaround32nd = false;
                answer[14].velocity = 112;
                // shorten the note before it so the pickup speaks clearly
                auto& prev = answer[static_cast<size_t>(last)];
                prev.durationSteps = std::max(1, std::min(prev.durationSteps, 14 - last));
            }
        }

        // ---- Pitch: rewrite the tail so it walks in scale to the cadence ----
        if (!p.lockPitch) {
            std::vector<int> act;
            for (int s = 0; s < 16; ++s) if (answer[static_cast<size_t>(s)].active) act.push_back(s);

            if (act.size() >= 2) { // a one-note bar keeps its downbeat as is
                const int n = static_cast<int>(act.size());
                const int k = std::clamp(1 + static_cast<int>(std::round(amt * 3.0f)), 1, std::max(1, n - 1 > 0 ? n - 1 : 1));

                int target;
                if (isFinalBar) target = (dir == 0 && amt > 0.5f) ? 12 : 0;
                else            target = (dir == 1) ? -5 : 7;

                // Where the tail starts from: the last note that is kept as an echo
                const int headIdx = n - k - 1;
                const int startRel = (headIdx >= 0)
                    ? answer[static_cast<size_t>(act[static_cast<size_t>(headIdx)])].interval - ansAnchor
                    : 0;

                for (int j = 0; j < k; ++j) {
                    const int s = act[static_cast<size_t>(n - k + j)];
                    int rel;
                    if (j == k - 1) {
                        rel = target;
                    } else if (dir == 2) {
                        rel = snapToScale(-(call[static_cast<size_t>(s)].interval - callAnchor));
                        if (rel < -7) rel += 12;
                    } else {
                        const float t = static_cast<float>(j + 1) / static_cast<float>(k);
                        rel = snapToScale(static_cast<int>(std::round(startRel + (target - startRel) * t)));
                        // Approach the cadence from the direction of travel, never sit on it early
                        if (rel == target)
                            rel = snapToScale(target + ((dir == 1) ? 2 : -2));
                    }
                    answer[static_cast<size_t>(s)].interval = ansAnchor + std::clamp(rel, -12, 24);
                }
            }
        }

        // Keep the end-of-phrase turnaround pickup from the original final bar
        if (isFinalBar) {
            for (int s = 14; s < 16; ++s)
                if (original[static_cast<size_t>(s)].active && original[static_cast<size_t>(s)].isTurnaround32nd)
                    answer[static_cast<size_t>(s)] = original[static_cast<size_t>(s)];
        }

        bars[static_cast<size_t>(b)] = answer;
    }
}

BassEngine::BarPattern BassEngine::generateBarRhythmAndPitch(
    const GenerationParams& p,
    int barIdx,
    Mulberry32& rng,
    const BarPattern* motifA,
    bool isTurnaroundBar)
{
    BarPattern bar{};

    // 1. Weighted 16-Step Onset Priors per Archetype
    std::array<float, 16> weights{};
    int minNotes = 3, maxNotes = 6;
    int minDur = 2, maxDur = 5;
    DrumInteractionMode interaction = DrumInteractionMode::ReinforceKick;

    switch (p.archetype) {
        case BassArchetype::RootsJungleSub:
            weights = { 1.4f, 0.1f, 0.8f, 0.9f, 0.1f, 0.4f, 1.1f, 0.3f, 0.9f, 0.2f, 1.2f, 0.7f, 0.1f, 0.3f, 0.9f, 0.2f };
            minNotes = 3; maxNotes = 6; minDur = 2; maxDur = 6;
            interaction = DrumInteractionMode::CounterpointPocket;
            break;
        case BassArchetype::Sub808:
            weights = { 1.8f, 0.05f, 0.4f, 0.3f, 0.05f, 0.1f, 1.5f, 0.1f, 0.6f, 0.1f, 1.3f, 0.2f, 0.05f, 0.1f, 0.4f, 0.1f };
            minNotes = 2; maxNotes = 5; minDur = 4; maxDur = 8;
            interaction = DrumInteractionMode::ReinforceKick;
            break;
        case BassArchetype::DubInfluenced:
            weights = { 0.15f, 0.1f, 1.6f, 0.5f, 0.1f, 0.2f, 1.5f, 0.3f, 0.4f, 0.2f, 1.6f, 0.4f, 0.1f, 0.2f, 1.4f, 0.2f };
            minNotes = 2; maxNotes = 5; minDur = 2; maxDur = 5;
            interaction = DrumInteractionMode::DubAvoidance;
            break;
        case BassArchetype::ReeseBassline:
            weights = { 1.9f, 0.05f, 0.3f, 0.4f, 0.1f, 0.2f, 1.2f, 0.2f, 0.9f, 0.1f, 0.8f, 0.2f, 0.1f, 0.1f, 0.5f, 0.1f };
            minNotes = 1; maxNotes = 4; minDur = 6; maxDur = 12;
            interaction = DrumInteractionMode::ReinforceKick;
            break;
        case BassArchetype::Techstep:
            weights = { 1.8f, 0.2f, 0.5f, 0.9f, 0.15f, 0.3f, 1.7f, 0.4f, 0.6f, 0.3f, 1.1f, 0.8f, 0.15f, 0.4f, 0.9f, 0.6f };
            minNotes = 3; maxNotes = 7; minDur = 1; maxDur = 4;
            interaction = DrumInteractionMode::ReinforceKick;
            break;
        case BassArchetype::Neurofunk:
            weights = { 1.3f, 0.4f, 0.6f, 1.3f, 0.2f, 0.7f, 1.2f, 0.8f, 0.7f, 0.5f, 0.9f, 1.3f, 0.2f, 0.6f, 1.2f, 1.1f };
            minNotes = 4; maxNotes = 8; minDur = 1; maxDur = 4;
            interaction = DrumInteractionMode::AnticipateTransient;
            break;
        case BassArchetype::RollingBassline:
            weights = { 1.5f, 0.7f, 1.2f, 0.8f, 0.3f, 0.8f, 1.3f, 0.9f, 1.2f, 0.7f, 1.3f, 0.8f, 0.3f, 0.8f, 1.6f, 0.9f };
            minNotes = 6; maxNotes = 11; minDur = 1; maxDur = 2;
            interaction = DrumInteractionMode::CounterpointPocket;
            break;
        case BassArchetype::MinimalDarkRoller:
            weights = { 1.8f, 0.05f, 0.2f, 0.6f, 0.05f, 0.1f, 1.3f, 0.15f, 0.3f, 0.1f, 0.9f, 0.3f, 0.05f, 0.1f, 0.4f, 0.2f };
            minNotes = 2; maxNotes = 4; minDur = 2; maxDur = 6;
            interaction = DrumInteractionMode::CounterpointPocket;
            break;
        case BassArchetype::LiquidFunk:
            weights = { 1.6f, 0.2f, 0.9f, 0.8f, 0.2f, 0.4f, 1.2f, 0.6f, 1.1f, 0.3f, 1.2f, 0.7f, 0.2f, 0.5f, 1.0f, 0.4f };
            minNotes = 4; maxNotes = 8; minDur = 2; maxDur = 5;
            interaction = DrumInteractionMode::CounterpointPocket;
            break;
        case BassArchetype::JumpUp:
            weights = { 1.7f, 0.3f, 1.1f, 1.2f, 0.2f, 0.5f, 1.5f, 0.6f, 1.2f, 0.4f, 1.4f, 1.1f, 0.2f, 0.5f, 1.3f, 0.7f };
            minNotes = 6; maxNotes = 10; minDur = 1; maxDur = 2;
            interaction = DrumInteractionMode::ReinforceKick;
            break;
        case BassArchetype::StabbingBassline:
            weights = { 1.5f, 0.2f, 1.0f, 1.1f, 0.2f, 0.4f, 1.4f, 0.5f, 1.0f, 0.3f, 1.2f, 1.0f, 0.2f, 0.4f, 1.1f, 0.5f };
            minNotes = 4; maxNotes = 9; minDur = 1; maxDur = 2;
            interaction = DrumInteractionMode::CounterpointPocket;
            break;
        case BassArchetype::PedalNotePulse:
            weights = { 1.6f, 0.5f, 1.1f, 0.9f, 0.2f, 0.6f, 1.3f, 0.7f, 1.2f, 0.5f, 1.2f, 0.9f, 0.2f, 0.6f, 1.3f, 0.7f };
            minNotes = 4; maxNotes = 10; minDur = 1; maxDur = 3;
            interaction = DrumInteractionMode::ReinforceKick;
            break;
        default:
            break;
    }

    // 2. Drum-Aware Conditioning
    const auto drums = getDrumGrid(p.drumTemplate);
    const float aware = std::clamp(p.drumAwareness, 0.0f, 1.0f);

    for (int s = 0; s < 16; ++s) {
        const DrumHit hit = drums[s];
        const DrumHit nextHit = drums[(s + 1) % 16];
        float mult = 1.0f;

        switch (interaction) {
            case DrumInteractionMode::ReinforceKick:
                if (hit == DrumHit::Kick) mult = 1.45f;
                else if (hit == DrumHit::Snare) mult = 0.25f;
                break;
            case DrumInteractionMode::DubAvoidance:
                if (s == 0 || hit == DrumHit::Snare) mult = 0.20f;
                else if (s == 2 || s == 6 || s == 10 || s == 14) mult = 1.50f;
                break;
            case DrumInteractionMode::CounterpointPocket:
                if (hit == DrumHit::Snare) mult = 0.15f;
                else if (hit == DrumHit::None) mult = 1.35f;
                break;
            case DrumInteractionMode::AnticipateTransient:
                if (nextHit == DrumHit::Kick || nextHit == DrumHit::Snare) mult = 1.45f;
                else if (hit == DrumHit::Snare) mult = 0.25f;
                break;
        }

        // Syncopation slider boosts offbeat 16ths (odd indices) vs on-beat indices
        const bool isOffbeat16th = (s % 2 == 1);
        const float syncMult = isOffbeat16th
            ? (0.55f + 0.95f * p.syncopation)
            : (1.25f - 0.45f * p.syncopation);

        weights[s] *= ((1.0f - aware) + aware * mult) * syncMult;
    }

    // 3. Multi-Bar Phrase Continuity (A - A' - A'' - B turnaround)
    if (motifA != nullptr && barIdx > 0) {
        bar = *motifA;

        // Minimal Dark Roller isolates variations strictly to Bar 4 (final bar)
        const bool suppressEarlyMutation = (p.archetype == BassArchetype::MinimalDarkRoller && !isTurnaroundBar);
        const bool strictJumpUpLock = (p.archetype == BassArchetype::JumpUp && !isTurnaroundBar);

        const float mutationProb = suppressEarlyMutation ? 0.0f
            : (strictJumpUpLock ? p.phraseVariation * 0.15f
            : (isTurnaroundBar ? std::max(0.45f, p.phraseVariation * 1.35f) : p.phraseVariation * 0.55f));

        for (int s = 0; s < 16; ++s) {
            if (rng.nextFloat() < mutationProb) {
                if (bar[s].active && s != 0) {
                    bar[s].active = false;
                } else if (!bar[s].active && weights[s] > 0.65f && rng.nextFloat() < p.noteDensity) {
                    bar[s].active = true;
                }
            }
        }
    } else {
        // Bar 0 (Motif A): Weighted sampling without replacement to hit target note count
        const int targetNotes = static_cast<int>(std::round(
            minNotes + (maxNotes - minNotes) * std::clamp(p.noteDensity, 0.0f, 1.0f)));

        std::array<float, 16> pool = weights;
        for (int n = 0; n < targetNotes; ++n) {
            float totalW = std::accumulate(pool.begin(), pool.end(), 0.0f);
            if (totalW <= 0.0001f) break;

            float pick = rng.nextFloat() * totalW;
            float acc = 0.0f;
            int chosen = 0;
            for (int s = 0; s < 16; ++s) {
                acc += pool[s];
                if (pick <= acc) {
                    chosen = s;
                    break;
                }
            }
            bar[chosen].active = true;
            pool[chosen] = 0.0f;
        }
    }

    // Turnaround fill on final bar (step 14/15 pickup or 32nd-note Techstep/Neuro roll)
    if (isTurnaroundBar) {
        bar[14].active = true;
        if (p.archetype == BassArchetype::Techstep || p.archetype == BassArchetype::Neurofunk || p.noteDensity > 0.6f) {
            bar[15].active = true;
            bar[15].isTurnaround32nd = true;
        }
    }

    // Assign step durations & dynamics
    for (int s = 0; s < 16; ++s) {
        if (!bar[s].active) continue;

        if (p.archetype == BassArchetype::StabbingBassline) {
            bar[s].durationSteps = (rng.nextFloat() < 0.75f) ? 1 : 2;
        } else if (p.archetype == BassArchetype::JumpUp) {
            bar[s].durationSteps = (rng.nextFloat() < 0.70f) ? 1 : 2;
        } else {
            const float lenScale = 0.4f + 1.2f * p.noteLength;
            const int rawDur = rng.nextInt(minDur, maxDur);
            bar[s].durationSteps = std::clamp(static_cast<int>(std::round(rawDur * lenScale)), 1, 12);
        }

        const bool accented = (s == 0 || s == 6 || s == 10 || s == 14);
        bar[s].velocity = accented ? rng.nextInt(112, 127) : rng.nextInt(88, 108);
    }

    // 4. Pitch Contour via First-Order Markov Interval Transition Matrix
    applyPitchContour(bar, p, barIdx, rng, isTurnaroundBar);

    return bar;
}

void BassEngine::applyPitchContour(BarPattern& bar,
                                   const GenerationParams& p,
                                   int barIdx,
                                   Mulberry32& rng,
                                   bool isTurnaroundBar)
{
    // Liquid Funk 4-bar minor chord cycle: i (0) -> VI (+8) -> VII (+10) -> v (+7)
    static constexpr std::array<int, 4> kLiquidCycle = { 0, 8, 10, 7 };
    const int harmonicAnchor = (p.archetype == BassArchetype::LiquidFunk)
        ? kLiquidCycle[barIdx % 4]
        : 0;

    int prevInterval = harmonicAnchor;
    bool firstNote = true;

    for (int s = 0; s < 16; ++s) {
        if (!bar[s].active) continue;

        if (firstNote) {
            bar[s].interval = harmonicAnchor;
            prevInterval = harmonicAnchor;
            firstNote = false;
            continue;
        }

        // Pedal-Note Pulse keeps root interval fixed across Bars 1-3, resolves on Bar 4
        if (p.archetype == BassArchetype::PedalNotePulse && !isTurnaroundBar) {
            bar[s].interval = 0;
            continue;
        }

        // Candidate intervals & archetype transition weights
        std::vector<int> candidates;
        std::vector<float> probs;

        switch (p.archetype) {
            case BassArchetype::RootsJungleSub:
                // Pentatonic descents (-2, -5) & root returns
                candidates = { 0, -2, -5, 3, 7, 10 };
                probs      = { 0.35f, 0.25f, 0.20f, 0.08f, 0.07f, 0.05f };
                break;
            case BassArchetype::Sub808:
                candidates = { 0, 5, 7, -5, 12 };
                probs      = { 0.50f, 0.15f, 0.15f, 0.10f, 0.10f };
                break;
            case BassArchetype::DubInfluenced:
                candidates = { 0, 7, 10, -2, 3, 12 };
                probs      = { 0.35f, 0.22f, 0.18f, 0.10f, 0.08f, 0.07f };
                break;
            case BassArchetype::ReeseBassline:
                // 0 -> +1 -> +3 -> +6 Phrygian movement
                candidates = { 0, 1, 3, 6, 7 };
                probs      = { 0.25f, 0.30f, 0.22f, 0.15f, 0.08f };
                break;
            case BassArchetype::Techstep:
                // 0 <-> +1 minor-second & +6 tritone tension
                candidates = { 0, 1, 6, 3, 7 };
                probs      = { 0.32f, 0.32f, 0.20f, 0.08f, 0.08f };
                break;
            case BassArchetype::Neurofunk:
                candidates = { 0, 1, 3, 6, 7, 8, 12 };
                probs      = { 0.25f, 0.24f, 0.14f, 0.16f, 0.09f, 0.06f, 0.06f };
                break;
            case BassArchetype::RollingBassline:
                candidates = { 0, 3, 5, 7, 10, 12 };
                probs      = { 0.36f, 0.18f, 0.14f, 0.16f, 0.10f, 0.06f };
                break;
            case BassArchetype::MinimalDarkRoller:
                candidates = { 0, 1, 3, 7 };
                probs      = { 0.45f, 0.28f, 0.17f, 0.10f };
                break;
            case BassArchetype::LiquidFunk:
                // Chord-tone tracking (+0, +3, +7, +12 relative to cycle anchor)
                candidates = { harmonicAnchor, harmonicAnchor + 3, harmonicAnchor + 7, harmonicAnchor + 12, harmonicAnchor - 5 };
                probs      = { 0.32f, 0.22f, 0.24f, 0.14f, 0.08f };
                break;
            case BassArchetype::JumpUp:
                // Crisp +12 octave jumps and +13 flat-9 hooks
                candidates = { 0, 12, 13, 7, 3, 1 };
                probs      = { 0.28f, 0.30f, 0.20f, 0.10f, 0.06f, 0.06f };
                break;
            case BassArchetype::StabbingBassline:
                candidates = { 0, 3, 7, 10, 12, 1 };
                probs      = { 0.32f, 0.20f, 0.20f, 0.12f, 0.10f, 0.06f };
                break;
            case BassArchetype::PedalNotePulse:
            default:
                candidates = { 0, 7, 10, 12 };
                probs      = { 0.45f, 0.25f, 0.18f, 0.12f };
                break;
        }

        // Modulate weights by Chromaticism & Octave Movement controls
        for (size_t i = 0; i < candidates.size(); ++i) {
            const int iv = candidates[i];
            const int rel = iv - harmonicAnchor;
            if (rel == 1 || rel == 6 || rel == 13) {
                probs[i] *= (0.35f + 1.45f * p.chromaticism);
            }
            if (std::abs(rel) >= 12) {
                probs[i] *= (0.25f + 1.65f * p.octaveMovement);
            }
            // First-order Markov inertia: avoid repeating exact non-root note back-to-back
            if (iv == prevInterval && iv != harmonicAnchor) {
                probs[i] *= 0.45f;
            }
        }

        // Final turnaround resolution on step 14/15 -> 5th (+7) or b7th (+10)
        if (isTurnaroundBar && s >= 14) {
            bar[s].interval = harmonicAnchor + ((rng.nextFloat() < 0.55f) ? 7 : 10);
        } else {
            float sum = std::accumulate(probs.begin(), probs.end(), 0.0f);
            float r = rng.nextFloat() * sum;
            float acc = 0.0f;
            int chosen = harmonicAnchor;
            for (size_t i = 0; i < candidates.size(); ++i) {
                acc += probs[i];
                if (r <= acc) {
                    chosen = candidates[i];
                    break;
                }
            }
            bar[s].interval = chosen;
        }

        prevInterval = bar[s].interval;
    }
}

void BassEngine::compileAndValidateMidi(const std::vector<BarPattern>& bars,
                                        const GenerationParams& p)
{
    currentNotes_.clear();
    const float swingAmount = std::clamp(p.swing, 0.50f, 0.65f);
    // Swing shifts odd 16th-note subdivisions forward in ticks
    const int swingOffsetTicks = static_cast<int>(std::round((swingAmount - 0.50f) * 2.0f * kTicksPerStep));

    for (int b = 0; b < static_cast<int>(bars.size()); ++b) {
        const int barStartTick = b * kTicksPerBar;

        for (int s = 0; s < 16; ++s) {
            const auto& slot = bars[b][s];
            if (!slot.active) continue;

            int onsetTick = barStartTick + s * kTicksPerStep;
            if (s % 2 == 1) {
                onsetTick += swingOffsetTicks;
            }

            int durTicks = slot.isTurnaround32nd
                ? (kTicksPerStep / 2)
                : (slot.durationSteps * kTicksPerStep);

            // Prevent spilling past total phrase length
            const int phraseEndTick = static_cast<int>(bars.size()) * kTicksPerBar;
            if (onsetTick + durTicks > phraseEndTick) {
                durTicks = std::max(24, phraseEndTick - onsetTick - 12);
            }

            MidiNoteEvent ev;
            ev.midiNote = std::clamp(p.rootMidiNote + slot.interval, 12, 84);
            ev.velocity = std::clamp(slot.velocity, 1, 127);
            ev.startTick = onsetTick;
            ev.durationTicks = std::max(24, durTicks);
            ev.barIndex = b;
            ev.stepIndex = s;
            currentNotes_.push_back(ev);
        }
    }

    // Strict Monophonic MIDI Validation: enforce >= 12 tick gap before next onset
    // (unless Reese Bassline sustained overlap is intentionally active)
    const bool allowOverlap = (p.archetype == BassArchetype::ReeseBassline);
    for (size_t i = 0; i + 1 < currentNotes_.size(); ++i) {
        auto& curr = currentNotes_[i];
        const auto& next = currentNotes_[i + 1];

        if (!allowOverlap) {
            const int maxAllowedEnd = next.startTick - 12;
            if (curr.startTick + curr.durationTicks > maxAllowedEnd) {
                curr.durationTicks = std::max(12, maxAllowedEnd - curr.startTick);
            }
        }
    }
}

std::vector<MidiNoteEvent> BassEngine::getNotesForBar(int barIndex) const {
    std::vector<MidiNoteEvent> out;
    for (const auto& n : currentNotes_) {
        if (n.barIndex == barIndex)
            out.push_back(n);
    }
    return out;
}

} // namespace bleek
