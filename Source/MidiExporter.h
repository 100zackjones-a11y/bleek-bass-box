#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <vector>
#include <algorithm>

namespace bleek {

struct MidiNoteEvent {
    int midiNote = 36;       // 0-127
    int velocity = 110;      // 1-127
    int startTick = 0;       // 480 PPQN (1920 ticks per 4/4 bar)
    int durationTicks = 120; // >= 12 ticks
    int barIndex = 0;
    int stepIndex = 0;
};

class MidiExporter {
public:
    static constexpr int kPPQN = 480;

    // Serializes notes into a Standard MIDI File (SMF Type 0) at 480 PPQN
    static bool exportToFile(const std::vector<MidiNoteEvent>& notes,
                             double bpm,
                             int totalBars,
                             const juce::File& targetFile)
    {
        juce::MidiMessageSequence track;

        // Tempo meta-event (microseconds per quarter note)
        const int microsecondsPerBeat = static_cast<int>(60000000.0 / juce::jlimit(60.0, 240.0, bpm));
        auto tempoMsg = juce::MidiMessage::tempoMetaEvent(microsecondsPerBeat);
        tempoMsg.setTimeStamp(0.0);
        track.addEvent(tempoMsg);

        // Time signature 4/4
        auto tsMsg = juce::MidiMessage::timeSignatureMetaEvent(4, 4);
        tsMsg.setTimeStamp(0.0);
        track.addEvent(tsMsg);

        // Track name
        auto nameMsg = juce::MidiMessage::textMetaEvent(0x03, "BLEEK BASS BOX Pattern");
        nameMsg.setTimeStamp(0.0);
        track.addEvent(nameMsg);

        for (const auto& n : notes) {
            const int noteNum = juce::jlimit(0, 127, n.midiNote);
            const float vel = juce::jlimit(1, 127, n.velocity) / 127.0f;
            const double start = static_cast<double>(std::max(0, n.startTick));
            const double end = start + static_cast<double>(std::max(12, n.durationTicks));

            auto on = juce::MidiMessage::noteOn(1, noteNum, vel);
            on.setTimeStamp(start);
            track.addEvent(on);

            auto off = juce::MidiMessage::noteOff(1, noteNum, 0.0f);
            off.setTimeStamp(end);
            track.addEvent(off);
        }

        // End of track aligned to exact bar boundary for clean Ableton clip looping
        const double totalTicks = static_cast<double>(std::max(1, totalBars) * 4 * kPPQN);
        auto eot = juce::MidiMessage::endOfTrack();
        eot.setTimeStamp(totalTicks);
        track.addEvent(eot);

        track.updateMatchedPairs();

        juce::MidiFile midiFile;
        midiFile.setTicksPerQuarterNote(kPPQN);
        midiFile.addTrack(track);

        if (targetFile.existsAsFile())
            targetFile.deleteFile();

        juce::FileOutputStream outStream(targetFile);
        if (!outStream.openedOk())
            return false;

        return midiFile.writeTo(outStream, 0);
    }

    // Creates a temporary .mid file ready for OS Drag-and-Drop into Ableton Live
    static juce::File createTempDragFile(const std::vector<MidiNoteEvent>& notes,
                                         double bpm,
                                         int totalBars,
                                         uint32_t seed,
                                         const juce::String& archetypeTag)
    {
        auto tempDir = juce::File::getSpecialLocation(juce::File::tempDirectory)
                           .getChildFile("BleekBassBox_Drag");
        tempDir.createDirectory();

        // Clean stale temp files older than 2 hours
        for (const auto& f : tempDir.findChildFiles(juce::File::findFiles, false, "*.mid")) {
            if ((juce::Time::getCurrentTime() - f.getLastModificationTime()).inHours() >= 2)
                f.deleteFile();
        }

        const auto safeTag = archetypeTag.retainCharacters("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-");
        // Built by concatenation: String::formatted("%s") reads wide strings on Windows and
        // produced garbled filenames that Ableton refused to accept as a drop.
        const auto fileName = "BLEEK_" + safeTag
                            + "_" + juce::String(static_cast<int>(bpm + 0.5)) + "BPM"
                            + "_S" + juce::String(static_cast<juce::uint32>(seed)) + ".mid";
        auto outFile = tempDir.getChildFile(fileName);
        exportToFile(notes, bpm, totalBars, outFile);
        return outFile;
    }
};

} // namespace bleek
