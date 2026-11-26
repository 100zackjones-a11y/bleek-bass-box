#pragma once
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"

namespace bleek {

// Retro 1990s Hardware Rack / Akai S-Series & E-mu E4 Inspired LookAndFeel
class BleekLookAndFeel : public juce::LookAndFeel_V4 {
public:
    BleekLookAndFeel();
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider& slider) override;
    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          const juce::Slider::SliderStyle style, juce::Slider& slider) override;
    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                              const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override;
    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                          bool shouldDrawButtonAsHighlighted,
                          bool shouldDrawButtonAsDown) override;
    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox& box) override;

    static void drawHardwareBezel(juce::Graphics& g, juce::Rectangle<float> bounds, bool recessed);
    static void drawPhillipsScrew(juce::Graphics& g, float cx, float cy, float radius);
};

// Native OS File Drag-Out Badge (DRAG .MID)
class MidiDragHandle : public juce::Component {
public:
    explicit MidiDragHandle(BleekBassBoxProcessor& proc);
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;

private:
    BleekBassBoxProcessor& processor_;
    bool dragTriggered_ = false;
};

// Drag-and-Drop Retro CRT/LCD Waveform Display Zone
class SampleDropZone : public juce::Component,
                       public juce::FileDragAndDropTarget {
public:
    explicit SampleDropZone(BleekBassBoxProcessor& proc);
    void paint(juce::Graphics& g) override;
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void fileDragEnter(const juce::StringArray&, int, int) override { isDraggingOver_ = true; repaint(); }
    void fileDragExit(const juce::StringArray&) override { isDraggingOver_ = false; repaint(); }
    void filesDropped(const juce::StringArray& files, int, int) override;

private:
    BleekBassBoxProcessor& processor_;
    bool isDraggingOver_ = false;
};

// 1990s Backlit Emerald LCD Piano Roll + Kick/Snare Markers + Scanlines
class LcdPianoRoll : public juce::Component {
public:
    explicit LcdPianoRoll(BleekBassBoxProcessor& proc);
    void paint(juce::Graphics& g) override;
    void setSelectedBar(int bar) { selectedBar_ = bar; repaint(); }
    int getSelectedBar() const noexcept { return selectedBar_; }

private:
    BleekBassBoxProcessor& processor_;
    int selectedBar_ = 0;
};

class BleekBassBoxEditor : public juce::AudioProcessorEditor,
                           public juce::DragAndDropContainer,
                           private juce::Timer {
public:
    explicit BleekBassBoxEditor(BleekBassBoxProcessor& p);
    ~BleekBassBoxEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;
    void exportMidiWithFileChooser();
    void browseSampleWithFileChooser();

    BleekBassBoxProcessor& processor_;
    BleekLookAndFeel lookAndFeel_;

    // Top Bar
    juce::TextButton generateBtn_ { "GENERATE" };
    MidiDragHandle dragMidBadge_;
    juce::TextButton exportMidBtn_ { "EXPORT .MID" };
    juce::ToggleButton liveMidiOutToggle_ { "LIVE MIDI OUT" };
    juce::Label seedReadout_;

    // Sampler & ADSR Strip
    SampleDropZone sampleDropZone_;
    juce::TextButton loadSampleBtn_ { "LOAD SAMPLE" };
    juce::TextButton sineResetBtn_ { "PURE SUB" };
    juce::ComboBox rootNoteBox_;
    juce::Slider attackKnob_, decayKnob_, sustainKnob_, releaseKnob_;
    juce::Label attackLbl_, decayLbl_, sustainLbl_, releaseLbl_;

    // LCD Piano Roll & Bar Tabs
    LcdPianoRoll pianoRoll_;
    std::array<juce::TextButton, 8> barSelectBtns_;

    // Compositional Controls & Dimension Locks
    juce::ComboBox eraBox_, archetypeBox_, drumContextBox_, phraseLengthBox_;
    juce::Slider densitySlider_, syncopationSlider_, variationSlider_, lengthSlider_;
    juce::Slider chromaticSlider_, octaveSlider_, drumAwareSlider_, swingSlider_;
    juce::Label densityLbl_, syncopationLbl_, variationLbl_, lengthLbl_;
    juce::Label chromaticLbl_, octaveLbl_, drumAwareLbl_, swingLbl_;

    juce::ToggleButton lockRhythmBtn_ { "LOCK RHYTHM" };
    juce::ToggleButton lockPitchBtn_ { "LOCK PITCH" };
    juce::ToggleButton lockPhraseBtn_ { "LOCK PHRASE" };
    juce::ToggleButton lockGrooveBtn_ { "LOCK GROOVE" };

    // Call & Response
    juce::ToggleButton callResponseBtn_ { "CALL & RESPONSE" };
    juce::ComboBox responseDirBox_;
    juce::Slider responseAmountSlider_;
    juce::Label responseAmountLbl_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attCallResp_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attRespDir_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attRespAmt_;

    // APVTS Attachments
    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAtt  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAtt> attAttack_, attDecay_, attSustain_, attRelease_;
    std::unique_ptr<SliderAtt> attDensity_, attSync_, attVar_, attLen_;
    std::unique_ptr<SliderAtt> attChrom_, attOct_, attDrumAware_, attSwing_;
    std::unique_ptr<ComboAtt>  attEra_, attArch_, attDrum_, attRoot_, attPhrase_;
    std::unique_ptr<ButtonAtt> attLockRhythm_, attLockPitch_, attLockPhrase_, attLockGroove_, attLiveOut_;

    std::unique_ptr<juce::FileChooser> activeFileChooser_;

    std::array<float, 4> lastAdsr_ { -1.0f, -1.0f, -1.0f, -1.0f };
    const SampleAsset* lastAsset_ = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BleekBassBoxEditor)
};

} // namespace bleek
