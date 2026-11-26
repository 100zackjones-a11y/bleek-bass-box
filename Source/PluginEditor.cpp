#include "PluginEditor.h"

namespace bleek {

namespace Palette {
    // 1990s Studio Rack / Akai S1000 & E-mu E4X Hardware Palette
    static const juce::Colour kRackEarBg    { 0xFF1A1C20 };
    static const juce::Colour kChassisMetal { 0xFF2B2F36 };
    static const juce::Colour kChassisDark  { 0xFF1E2127 };
    static const juce::Colour kPanelRecess  { 0xFF22252C };
    static const juce::Colour kBevelLight   { 0xFF474D58 };
    static const juce::Colour kBevelShadow  { 0xFF101216 };
    static const juce::Colour kTactileKey   { 0xFF363B44 };
    static const juce::Colour kAmberLed     { 0xFFFF9E1B };
    static const juce::Colour kRedLed       { 0xFFFF3B30 };
    static const juce::Colour kLcdPhosphor  { 0xFF26F79A };
    static const juce::Colour kLcdBg        { 0xFF061B14 };
    static const juce::Colour kSilkscreen   { 0xFFE2E6EC };
    static const juce::Colour kSilkMuted    { 0xFFA4ABB6 };
}

// -----------------------------------------------------------------------------
// BleekLookAndFeel (1990s Hardware Aesthetics)
// -----------------------------------------------------------------------------
BleekLookAndFeel::BleekLookAndFeel() {
    setColour(juce::ResizableWindow::backgroundColourId, Palette::kChassisMetal);
    setColour(juce::TextButton::buttonColourId, Palette::kTactileKey);
    setColour(juce::TextButton::textColourOffId, Palette::kSilkscreen);
    setColour(juce::ComboBox::backgroundColourId, Palette::kChassisDark);
    setColour(juce::ComboBox::outlineColourId, Palette::kBevelShadow);
    setColour(juce::ComboBox::textColourId, Palette::kLcdPhosphor);
    setColour(juce::PopupMenu::backgroundColourId, Palette::kChassisDark);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, Palette::kAmberLed);
    setColour(juce::PopupMenu::highlightedTextColourId, Palette::kBevelShadow);
    setColour(juce::Label::textColourId, Palette::kSilkscreen);
    setColour(juce::ToggleButton::textColourId, Palette::kSilkscreen);
    setColour(juce::ToggleButton::tickColourId, Palette::kAmberLed);
}

void BleekLookAndFeel::drawHardwareBezel(juce::Graphics& g, juce::Rectangle<float> b, bool recessed) {
    const auto topCol = recessed ? Palette::kBevelShadow : Palette::kBevelLight;
    const auto botCol = recessed ? Palette::kBevelLight  : Palette::kBevelShadow;

    g.setColour(topCol);
    g.drawLine(b.getX(), b.getY(), b.getRight(), b.getY(), 1.5f);
    g.drawLine(b.getX(), b.getY(), b.getX(), b.getBottom(), 1.5f);

    g.setColour(botCol);
    g.drawLine(b.getX(), b.getBottom(), b.getRight(), b.getBottom(), 1.5f);
    g.drawLine(b.getRight(), b.getY(), b.getRight(), b.getBottom(), 1.5f);
}

void BleekLookAndFeel::drawPhillipsScrew(juce::Graphics& g, float cx, float cy, float r) {
    // Recessed screw socket
    g.setColour(Palette::kBevelShadow);
    g.fillEllipse(cx - r - 1.0f, cy - r - 1.0f, (r + 1.0f) * 2.0f, (r + 1.0f) * 2.0f);

    // Metallic screw head gradient
    juce::ColourGradient grad(juce::Colour(0xFF6B7280), cx - r * 0.5f, cy - r * 0.5f,
                              juce::Colour(0xFF1F242D), cx + r * 0.7f, cy + r * 0.7f, false);
    g.setGradientFill(grad);
    g.fillEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f);

    // Cross-slot
    g.setColour(juce::Colour(0xFF0E1014));
    g.drawLine(cx - r * 0.55f, cy, cx + r * 0.55f, cy, 1.4f);
    g.drawLine(cx, cy - r * 0.55f, cx, cy + r * 0.55f, 1.4f);
}

void BleekLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                        juce::Slider&)
{
    const float radius = juce::jmin(width / 2.0f, height / 2.0f) - 6.0f;
    const float cx = x + width * 0.5f;
    const float cy = y + height * 0.5f;
    const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    // 1. Silkscreen dial tick marks around knob perimeter
    g.setColour(Palette::kSilkMuted);
    for (int i = 0; i <= 10; ++i) {
        const float tickNorm = static_cast<float>(i) / 10.0f;
        const float tickAngle = rotaryStartAngle + tickNorm * (rotaryEndAngle - rotaryStartAngle);
        const float rIn = radius + 1.5f;
        const float rOut = radius + ((i == 0 || i == 5 || i == 10) ? 5.0f : 3.2f);
        const float x1 = cx + std::sin(tickAngle) * rIn;
        const float y1 = cy - std::cos(tickAngle) * rIn;
        const float x2 = cx + std::sin(tickAngle) * rOut;
        const float y2 = cy - std::cos(tickAngle) * rOut;
        g.drawLine(x1, y1, x2, y2, (i == 0 || i == 5 || i == 10) ? 1.4f : 1.0f);
    }

    // 2. Drop shadow beneath hardware knob
    g.setColour(juce::Colours::black.withAlpha(0.55f));
    g.fillEllipse(cx - radius + 1.5f, cy - radius + 2.5f, radius * 2.0f, radius * 2.0f);

    // 3. Classic 90s molded synth/sampler knob skirt
    juce::ColourGradient skirtGrad(juce::Colour(0xFF434954), cx - radius * 0.6f, cy - radius * 0.6f,
                                   juce::Colour(0xFF14161A), cx + radius * 0.8f, cy + radius * 0.8f, false);
    g.setGradientFill(skirtGrad);
    g.fillEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);

    // 4. Inner knurled aluminum top cap
    const float capR = radius * 0.72f;
    juce::ColourGradient capGrad(juce::Colour(0xFF5A616E), cx - capR * 0.5f, cy - capR * 0.5f,
                                 juce::Colour(0xFF23272F), cx + capR * 0.7f, cy + capR * 0.7f, false);
    g.setGradientFill(capGrad);
    g.fillEllipse(cx - capR, cy - capR, capR * 2.0f, capR * 2.0f);
    g.setColour(Palette::kBevelShadow);
    g.drawEllipse(cx - capR, cy - capR, capR * 2.0f, capR * 2.0f, 1.0f);

    // 5. High-visibility 90s amber/white pointer line
    juce::Path pointer;
    pointer.addRoundedRectangle(-1.6f, -radius + 1.5f, 3.2f, radius * 0.58f, 1.0f);
    pointer.applyTransform(juce::AffineTransform::rotation(angle).translated(cx, cy));
    g.setColour(Palette::kAmberLed);
    g.fillPath(pointer);
}

void BleekLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPos, float, float,
                                        const juce::Slider::SliderStyle, juce::Slider&)
{
    const float trackY = y + height * 0.5f;
    auto slotRect = juce::Rectangle<float>(static_cast<float>(x), trackY - 3.5f, static_cast<float>(width), 7.0f);

    // Recessed metal fader slot
    g.setColour(Palette::kBevelShadow);
    g.fillRoundedRectangle(slotRect, 2.0f);
    drawHardwareBezel(g, slotRect, true);

    // Center detent tick
    g.setColour(Palette::kSilkMuted.withAlpha(0.5f));
    g.drawVerticalLine(x + width / 2, trackY - 8.0f, trackY + 8.0f);

    // 1990s Molded Fader Cap with center white/amber groove
    auto cap = juce::Rectangle<float>(sliderPos - 6.0f, trackY - 9.0f, 12.0f, 18.0f);
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.fillRoundedRectangle(cap.translated(1.0f, 1.5f), 2.0f);

    juce::ColourGradient capGrad(juce::Colour(0xFF565D6B), cap.getX(), cap.getY(),
                                 juce::Colour(0xFF232730), cap.getRight(), cap.getBottom(), false);
    g.setGradientFill(capGrad);
    g.fillRoundedRectangle(cap, 2.0f);
    drawHardwareBezel(g, cap, false);

    // Center indicator stripe on fader cap
    g.setColour(Palette::kAmberLed);
    g.fillRect(sliderPos - 1.0f, trackY - 7.0f, 2.0f, 14.0f);
}

void BleekLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                            const juce::Colour& backgroundColour,
                                            bool isHighlighted, bool isDown)
{
    auto b = button.getLocalBounds().toFloat().reduced(0.5f);
    const bool active = button.getToggleState();

    // Tactile rubber/plastic 90s button body
    auto base = backgroundColour;
    if (isDown) base = base.darker(0.15f);
    else if (isHighlighted) base = base.brighter(0.08f);

    juce::ColourGradient btnGrad(base.brighter(0.12f), b.getX(), b.getY(),
                                 base.darker(0.18f), b.getX(), b.getBottom(), false);
    g.setGradientFill(btnGrad);
    g.fillRoundedRectangle(b, 3.0f);
    drawHardwareBezel(g, b, isDown);

    // Classic 90s embedded rectangular LED lens on top-left or top edge of button
    auto ledRect = juce::Rectangle<float>(b.getX() + 5.0f, b.getY() + 4.0f, 8.0f, 3.5f);
    if (active) {
        g.setColour(Palette::kAmberLed.withAlpha(0.45f));
        g.fillRoundedRectangle(ledRect.expanded(2.0f), 2.0f);
        g.setColour(Palette::kAmberLed);
        g.fillRoundedRectangle(ledRect, 1.0f);
    } else {
        g.setColour(juce::Colour(0xFF15181E));
        g.fillRoundedRectangle(ledRect, 1.0f);
    }
}

void BleekLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                                        bool isHighlighted, bool isDown)
{
    drawButtonBackground(g, button, Palette::kTactileKey, isHighlighted, isDown);

    g.setColour(button.getToggleState() ? Palette::kAmberLed : Palette::kSilkscreen);
    g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    g.drawText(button.getButtonText(), button.getLocalBounds().withTrimmedLeft(12),
               juce::Justification::centred, true);
}

void BleekLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool,
                                    int, int, int, int, juce::ComboBox&)
{
    auto b = juce::Rectangle<float>(0, 0, static_cast<float>(width), static_cast<float>(height)).reduced(0.5f);
    // Dark backlit mini-LCD readout style for parameter selectors
    g.setColour(Palette::kLcdBg);
    g.fillRoundedRectangle(b, 2.5f);
    drawHardwareBezel(g, b, true);

    // Classic dropdown triangle arrow
    juce::Path arrow;
    const float ax = width - 14.0f;
    const float ay = height * 0.5f - 2.0f;
    arrow.addTriangle(ax, ay, ax + 7.0f, ay, ax + 3.5f, ay + 5.0f);
    g.setColour(Palette::kLcdPhosphor);
    g.fillPath(arrow);
}

// -----------------------------------------------------------------------------
// MidiDragHandle (OS File Drag-and-Drop into Ableton Live)
// -----------------------------------------------------------------------------
MidiDragHandle::MidiDragHandle(BleekBassBoxProcessor& proc) : processor_(proc) {
    setMouseCursor(juce::MouseCursor::DraggingHandCursor);
}

void MidiDragHandle::paint(juce::Graphics& g) {
    auto b = getLocalBounds().toFloat().reduced(0.5f);
    juce::ColourGradient grad(juce::Colour(0xFF47371B), b.getX(), b.getY(),
                              juce::Colour(0xFF281E0E), b.getX(), b.getBottom(), false);
    g.setGradientFill(grad);
    g.fillRoundedRectangle(b, 3.0f);
    BleekLookAndFeel::drawHardwareBezel(g, b, false);

    g.setColour(Palette::kAmberLed);
    g.setFont(juce::FontOptions(11.5f, juce::Font::bold));
    g.drawText("DRAG .MID", b, juce::Justification::centred);
}

void MidiDragHandle::mouseDown(const juce::MouseEvent&) {
    dragTriggered_ = false;
}

void MidiDragHandle::mouseDrag(const juce::MouseEvent& e) {
    if (!dragTriggered_ && e.getDistanceFromDragStart() > 4) {
        dragTriggered_ = true;
        const auto notes = processor_.getPatternSnapshot();
        const int phraseIdx = static_cast<int>(processor_.apvts.getRawParameterValue("phraseBars")->load());
        const int barChoices[4] = { 1, 2, 4, 8 };
        const int totalBars = barChoices[std::clamp(phraseIdx, 0, 3)];
        const auto archIdx = static_cast<BassArchetype>(static_cast<int>(processor_.apvts.getRawParameterValue("archetype")->load()));

        const auto tempMid = MidiExporter::createTempDragFile(
            notes,
            processor_.getHostBpm(),
            totalBars,
            processor_.getCurrentSeed(),
            BassEngine::getArchetypeName(archIdx));

        if (tempMid.existsAsFile()) {
            juce::DragAndDropContainer::performExternalDragDropOfFiles({ tempMid.getFullPathName() }, false);
        }
    }
}

// -----------------------------------------------------------------------------
// SampleDropZone (90s Backlit CRT/LCD Waveform Window)
// -----------------------------------------------------------------------------
SampleDropZone::SampleDropZone(BleekBassBoxProcessor& proc) : processor_(proc) {}

bool SampleDropZone::isInterestedInFileDrag(const juce::StringArray& files) {
    for (const auto& f : files) {
        const auto ext = juce::File(f).getFileExtension().toLowerCase();
        if (ext == ".wav" || ext == ".aif" || ext == ".aiff" || ext == ".flac")
            return true;
    }
    return false;
}

void SampleDropZone::filesDropped(const juce::StringArray& files, int, int) {
    isDraggingOver_ = false;
    for (const auto& f : files) {
        juce::File file(f);
        if (processor_.loadSampleFile(file))
            break;
    }
    repaint();
}

void SampleDropZone::paint(juce::Graphics& g) {
    auto b = getLocalBounds().toFloat().reduced(0.5f);
    g.setColour(Palette::kLcdBg);
    g.fillRoundedRectangle(b, 3.0f);
    BleekLookAndFeel::drawHardwareBezel(g, b, true);

    if (isDraggingOver_) {
        g.setColour(Palette::kAmberLed);
        g.drawRoundedRectangle(b.reduced(1.5f), 2.0f, 1.5f);
    }

    auto asset = processor_.sampler.getCurrentAsset();
    if (!asset) return;

    // Draw retro oscilloscope waveform preview
    auto waveArea = b.reduced(8.0f, 16.0f).translated(0.0f, 5.0f);
    // Center zero-crossing hairline
    g.setColour(Palette::kLcdPhosphor.withAlpha(0.18f));
    g.drawHorizontalLine(static_cast<int>(waveArea.getCentreY()), waveArea.getX(), waveArea.getRight());

    const int numSamples = asset->buffer.getNumSamples();
    if (numSamples > 0 && waveArea.getWidth() > 4.0f) {
        const float* data = asset->buffer.getReadPointer(0);
        const int w = static_cast<int>(waveArea.getWidth());
        const float midY = waveArea.getCentreY();
        const float halfH = waveArea.getHeight() * 0.45f;

        // ADSR timeline (ms). The view spans the whole sample, extended if the
        // envelope is longer; note-off sits so the release ends at the right edge.
        const float aMs = std::max(0.0f, processor_.apvts.getRawParameterValue("attack")->load());
        const float dMs = std::max(0.0f, processor_.apvts.getRawParameterValue("decay")->load());
        const float sLv = juce::jlimit(0.0f, 1.0f, processor_.apvts.getRawParameterValue("sustain")->load());
        const float rMs = std::max(0.0f, processor_.apvts.getRawParameterValue("release")->load());
        const float sampleMs = static_cast<float>(numSamples / std::max(1.0, asset->sampleRate) * 1000.0);
        const float minHoldMs = 0.15f * std::max(sampleMs, aMs + dMs + rMs);
        const float totalMs = std::max(sampleMs, aMs + dMs + minHoldMs + rMs);
        const float noteOffMs = totalMs - rMs;

        auto envAt = [&](float tMs) -> float {
            float level;
            if (tMs < aMs) level = aMs > 0.0f ? tMs / aMs : 1.0f;
            else if (tMs < aMs + dMs) level = 1.0f - (1.0f - sLv) * ((tMs - aMs) / std::max(0.001f, dMs));
            else level = sLv;
            if (tMs >= noteOffMs) {
                const float atOff = (noteOffMs < aMs) ? (aMs > 0.0f ? noteOffMs / aMs : 1.0f)
                                  : (noteOffMs < aMs + dMs) ? 1.0f - (1.0f - sLv) * ((noteOffMs - aMs) / std::max(0.001f, dMs))
                                  : sLv;
                level = atOff * (1.0f - juce::jlimit(0.0f, 1.0f, (tMs - noteOffMs) / std::max(0.001f, rMs)));
            }
            return juce::jlimit(0.0f, 1.0f, level);
        };

        const juce::Colour waveCol = asset->isFallbackSine ? Palette::kLcdPhosphor : Palette::kAmberLed;
        const float samplesPerMs = static_cast<float>(asset->sampleRate / 1000.0);

        for (int x = 0; x < w; ++x) {
            const float t0 = totalMs * static_cast<float>(x) / static_cast<float>(w);
            const float t1 = totalMs * static_cast<float>(x + 1) / static_cast<float>(w);
            const int i0 = static_cast<int>(t0 * samplesPerMs);
            const int i1 = std::min(numSamples, std::max(i0 + 1, static_cast<int>(t1 * samplesPerMs)));
            if (i0 >= numSamples) break;

            float peak = 0.0f;
            const int stride = std::max(1, (i1 - i0) / 16);
            for (int i = i0; i < i1; i += stride)
                peak = std::max(peak, std::abs(data[i]));

            const int px = static_cast<int>(waveArea.getX()) + x;
            const float raw = std::max(1.0f, peak * halfH);
            g.setColour(waveCol.withAlpha(0.18f)); // unshaped sample, ghosted
            g.drawVerticalLine(px, midY - raw, midY + raw);

            const float shaped = std::max(0.5f, peak * envAt(0.5f * (t0 + t1)) * halfH);
            g.setColour(waveCol); // what you actually hear
            g.drawVerticalLine(px, midY - shaped, midY + shaped);
        }

        // Envelope curve over the waveform
        const float top = waveArea.getY() + 2.0f;
        const float bottom = waveArea.getBottom() - 2.0f;
        auto xFor = [&](float ms) { return waveArea.getX() + (ms / totalMs) * waveArea.getWidth(); };
        auto yFor = [&](float lv) { return bottom - lv * (bottom - top); };

        juce::Path env;
        env.startNewSubPath(xFor(0.0f), yFor(0.0f));
        for (int x = 1; x <= w; ++x) {
            const float t = totalMs * static_cast<float>(x) / static_cast<float>(w);
            env.lineTo(xFor(t), yFor(envAt(t)));
        }

        juce::Path fill(env);
        fill.lineTo(xFor(totalMs), bottom);
        fill.closeSubPath();
        g.setColour(Palette::kLcdPhosphor.withAlpha(0.07f));
        g.fillPath(fill);
        g.setColour(Palette::kLcdPhosphor);
        g.strokePath(env, juce::PathStrokeType(1.5f));

        // Stage handles + labels
        const float attackEnd = std::min(aMs, noteOffMs);
        const float decayEnd = std::min(aMs + dMs, noteOffMs);
        const std::array<std::pair<float, const char*>, 4> marks {{
            { attackEnd, "A" }, { decayEnd, "D" }, { noteOffMs, "S" }, { totalMs, "R" } }};
        g.setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 9.5f, juce::Font::bold));
        for (const auto& [ms, label] : marks) {
            const float hx = juce::jlimit(waveArea.getX() + 3.0f, waveArea.getRight() - 3.0f, xFor(ms));
            const float hy = yFor(envAt(std::min(ms, totalMs - 0.001f)));
            g.setColour(Palette::kLcdPhosphor.withAlpha(0.25f));
            g.drawVerticalLine(static_cast<int>(hx), top, bottom);
            g.setColour(Palette::kLcdPhosphor);
            g.fillEllipse(hx - 3.0f, hy - 3.0f, 6.0f, 6.0f);
            g.drawText(label, juce::Rectangle<float>(hx - 8.0f, bottom - 12.0f, 16.0f, 12.0f),
                       juce::Justification::centred, false);
        }
    }

    // Subtle CRT scanline overlay
    g.setColour(juce::Colours::black.withAlpha(0.22f));
    for (int y = static_cast<int>(b.getY()) + 2; y < static_cast<int>(b.getBottom()); y += 3) {
        g.drawHorizontalLine(y, b.getX() + 2.0f, b.getRight() - 2.0f);
    }

    // Header strip: Sample filename + Detected Key pill badge
    auto headerStrip = b.reduced(8.0f, 4.0f).removeFromTop(17.0f);
    auto keyBadgeArea = headerStrip.removeFromRight(124.0f);

    g.setColour(Palette::kLcdPhosphor.withAlpha(0.16f));
    g.fillRoundedRectangle(keyBadgeArea, 2.0f);
    g.setColour(Palette::kLcdPhosphor);
    g.drawRoundedRectangle(keyBadgeArea, 2.0f, 1.0f);
    g.setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 10.5f, juce::Font::bold));
    g.drawText("KEY:" + asset->detectedKey.label, keyBadgeArea, juce::Justification::centred, true);

    g.setColour(Palette::kLcdPhosphor);
    g.setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 11.0f, juce::Font::bold));
    g.drawText(asset->fileName,
               headerStrip.withTrimmedRight(6.0f),
               juce::Justification::centredLeft, true);
}

// -----------------------------------------------------------------------------
// LcdPianoRoll (1990s Backlit Phosphor LCD Sequencer Grid)
// -----------------------------------------------------------------------------
LcdPianoRoll::LcdPianoRoll(BleekBassBoxProcessor& proc) : processor_(proc) {}

void LcdPianoRoll::paint(juce::Graphics& g) {
    auto b = getLocalBounds().toFloat().reduced(0.5f);
    g.setColour(Palette::kLcdBg);
    g.fillRoundedRectangle(b, 3.0f);
    BleekLookAndFeel::drawHardwareBezel(g, b, true);

    auto inner = b.reduced(10.0f);
    const float keyboardW = 38.0f;
    const float drumStripH = 18.0f;
    auto drumStrip = inner.removeFromTop(drumStripH).withTrimmedLeft(keyboardW + 4.0f);
    auto gridArea = inner.withTrimmedTop(4.0f);
    auto keysArea = gridArea.removeFromLeft(keyboardW);
    gridArea.removeFromLeft(4.0f);

    const float stepW = gridArea.getWidth() / 16.0f;

    // Drum Kick/Snare context markers
    const auto drumTmpl = static_cast<DrumTemplate>(
        static_cast<int>(processor_.apvts.getRawParameterValue("drumContext")->load()));
    const auto drums = BassEngine::getDrumGrid(drumTmpl);

    for (int s = 0; s < 16; ++s) {
        const float sx = gridArea.getX() + s * stepW;
        if (s % 4 == 0) {
            g.setColour(Palette::kLcdPhosphor.withAlpha(0.24f));
            g.drawVerticalLine(static_cast<int>(sx), gridArea.getY(), gridArea.getBottom());
        } else {
            g.setColour(Palette::kLcdPhosphor.withAlpha(0.08f));
            g.drawVerticalLine(static_cast<int>(sx), gridArea.getY(), gridArea.getBottom());
        }

        auto markerRect = juce::Rectangle<float>(sx + 2.0f, drumStrip.getY() + 2.0f, stepW - 4.0f, drumStripH - 4.0f);
        if (drums[s] == DrumHit::Kick) {
            g.setColour(Palette::kAmberLed);
            g.fillRoundedRectangle(markerRect, 1.5f);
            g.setColour(Palette::kBevelShadow);
            g.setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 10.0f, juce::Font::bold));
            g.drawText("K", markerRect, juce::Justification::centred);
        } else if (drums[s] == DrumHit::Snare) {
            g.setColour(Palette::kLcdPhosphor);
            g.fillRoundedRectangle(markerRect, 1.5f);
            g.setColour(Palette::kBevelShadow);
            g.setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 10.0f, juce::Font::bold));
            g.drawText("S", markerRect, juce::Justification::centred);
        }
    }

    // Draw MIDI notes for selected bar (pitch range: root - 7 to root + 15)
    const int rootMidi = 24 + static_cast<int>(processor_.apvts.getRawParameterValue("rootNote")->load());
    const int minNote = rootMidi - 7;
    const int maxNote = rootMidi + 15;
    const int noteSpan = std::max(1, maxNote - minNote + 1);
    const float rowH = gridArea.getHeight() / static_cast<float>(noteSpan);

    // Highlight root row
    const float rootY = gridArea.getBottom() - (rootMidi - minNote + 1) * rowH;
    g.setColour(Palette::kLcdPhosphor.withAlpha(0.12f));
    g.fillRect(gridArea.getX(), rootY, gridArea.getWidth(), rowH);

    // Side piano keyboard (one key per pitch row), sounding note lit amber
    {
        static const bool kIsBlack[12] = { false, true, false, true, false, false, true, false, true, false, true, false };
        static const char* kNames[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        const int soundingNote = processor_.getSoundingNote();

        for (int note = minNote; note <= maxNote; ++note) {
            const float ky = gridArea.getBottom() - (note - minNote + 1) * rowH;
            auto key = juce::Rectangle<float>(keysArea.getX(), ky, keysArea.getWidth(), rowH).reduced(0.0f, 0.5f);
            const bool black = kIsBlack[note % 12];

            juce::Colour fill = black ? juce::Colour(0xFF14181C) : juce::Colour(0xFFD9DED6);
            if (note == soundingNote)
                fill = Palette::kAmberLed;
            else if (note == rootMidi)
                fill = black ? juce::Colour(0xFF1F4A33) : juce::Colour(0xFFB8E6C9);

            g.setColour(fill);
            g.fillRect(black ? key.withTrimmedRight(keysArea.getWidth() * 0.35f) : key);
            g.setColour(juce::Colour(0xFF0B0E10));
            g.drawRect(key, 0.5f);

            if (note % 12 == 0 || note == rootMidi || note == soundingNote) {
                g.setColour(note == soundingNote || !black ? juce::Colour(0xFF101418) : Palette::kLcdPhosphor);
                g.setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(),
                                            juce::jlimit(7.0f, 10.0f, rowH - 1.0f), juce::Font::bold));
                g.drawText(juce::String(kNames[note % 12]) + juce::String(note / 12 - 1),
                           key.withTrimmedLeft(2.0f), juce::Justification::centredLeft, false);
            }

            // faint row line across the grid
            g.setColour(Palette::kLcdPhosphor.withAlpha(black ? 0.05f : 0.025f));
            g.fillRect(gridArea.getX(), ky, gridArea.getWidth(), rowH);
        }
    }

    const auto allNotes = processor_.getPatternSnapshot();
    const int barStartTick = selectedBar_ * BassEngine::kTicksPerBar;
    const int barEndTick = barStartTick + BassEngine::kTicksPerBar;

    for (const auto& n : allNotes) {
        if (n.startTick >= barEndTick || (n.startTick + n.durationTicks) <= barStartTick)
            continue;

        const int clampedStart = std::max(n.startTick, barStartTick) - barStartTick;
        const int clampedEnd = std::min(n.startTick + n.durationTicks, barEndTick) - barStartTick;

        const float nx = gridArea.getX() + (static_cast<float>(clampedStart) / BassEngine::kTicksPerBar) * gridArea.getWidth();
        const float nw = std::max(4.0f, (static_cast<float>(clampedEnd - clampedStart) / BassEngine::kTicksPerBar) * gridArea.getWidth());
        const int clampedPitch = std::clamp(n.midiNote, minNote, maxNote);
        const float ny = gridArea.getBottom() - (clampedPitch - minNote + 1) * rowH;

        // Note under the playhead glows amber
        bool underPlayhead = false;
        if (processor_.isHostPlaying()) {
            const double tick = processor_.getPlayheadBarPosition() * BassEngine::kTicksPerBar;
            underPlayhead = tick >= n.startTick && tick < n.startTick + n.durationTicks;
        }
        g.setColour(underPlayhead ? Palette::kAmberLed : Palette::kLcdPhosphor);
        g.fillRect(nx, ny + 1.0f, nw, std::max(4.0f, rowH - 2.0f));
    }

    // Retro LCD horizontal scanlines across display
    g.setColour(juce::Colours::black.withAlpha(0.20f));
    for (int y = static_cast<int>(b.getY()) + 2; y < static_cast<int>(b.getBottom()); y += 3) {
        g.drawHorizontalLine(y, b.getX() + 2.0f, b.getRight() - 2.0f);
    }

    // Real-time Playhead
    if (processor_.isHostPlaying()) {
        const double barPos = processor_.getPlayheadBarPosition();
        const int currentPlayingBar = static_cast<int>(std::floor(barPos));
        if (currentPlayingBar == selectedBar_) {
            const float frac = static_cast<float>(barPos - std::floor(barPos));
            const float px = gridArea.getX() + frac * gridArea.getWidth();
            g.setColour(Palette::kAmberLed.withAlpha(0.18f));
            g.fillRect(px - 5.0f, drumStrip.getY(), 10.0f, gridArea.getBottom() - drumStrip.getY());
            g.setColour(Palette::kAmberLed);
            g.drawLine(px, drumStrip.getY(), px, gridArea.getBottom(), 2.0f);
        }
    }
}

// -----------------------------------------------------------------------------
// BleekBassBoxEditor (19" Rackmount Hardware Chassis)
// -----------------------------------------------------------------------------
BleekBassBoxEditor::BleekBassBoxEditor(BleekBassBoxProcessor& p)
    : AudioProcessorEditor(&p),
      processor_(p),
      dragMidBadge_(p),
      sampleDropZone_(p),
      pianoRoll_(p)
{
    setLookAndFeel(&lookAndFeel_);
    setSize(980, 676);

    // Top Bar
    generateBtn_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF9A3412));
    generateBtn_.setColour(juce::TextButton::textColourOffId, Palette::kSilkscreen);
    generateBtn_.onClick = [this] {
        processor_.triggerRegenerate(true);
        pianoRoll_.repaint();
    };
    addAndMakeVisible(generateBtn_);
    addAndMakeVisible(dragMidBadge_);

    exportMidBtn_.onClick = [this] { exportMidiWithFileChooser(); };
    addAndMakeVisible(exportMidBtn_);
    addAndMakeVisible(liveMidiOutToggle_);

    seedReadout_.setJustificationType(juce::Justification::centred);
    seedReadout_.setColour(juce::Label::textColourId, Palette::kLcdPhosphor);
    seedReadout_.setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 11.5f, juce::Font::bold));
    addAndMakeVisible(seedReadout_);

    // Sampler & ADSR Strip
    addAndMakeVisible(sampleDropZone_);
    loadSampleBtn_.onClick = [this] { browseSampleWithFileChooser(); };
    addAndMakeVisible(loadSampleBtn_);
    sineResetBtn_.onClick = [this] {
        processor_.resetToDefaultSine();
        sampleDropZone_.repaint();
    };
    addAndMakeVisible(sineResetBtn_);

    const char* noteNames[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    for (int m = 24; m <= 47; ++m)
        rootNoteBox_.addItem("ROOT: " + juce::String(noteNames[m % 12]) + juce::String((m / 12) - 1), m - 23);
    addAndMakeVisible(rootNoteBox_);

    auto setupKnob = [this](juce::Slider& s, juce::Label& l, const juce::String& text) {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 56, 16);
        s.setColour(juce::Slider::textBoxBackgroundColourId, Palette::kLcdBg);
        s.setColour(juce::Slider::textBoxTextColourId, Palette::kLcdPhosphor);
        s.setColour(juce::Slider::textBoxOutlineColourId, Palette::kBevelShadow);
        addAndMakeVisible(s);
        l.setText(text, juce::dontSendNotification);
        l.setFont(juce::FontOptions(10.5f, juce::Font::bold));
        l.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(l);
    };
    setupKnob(attackKnob_, attackLbl_, "ATTACK");
    setupKnob(decayKnob_, decayLbl_, "DECAY");
    setupKnob(sustainKnob_, sustainLbl_, "SUSTAIN");
    setupKnob(releaseKnob_, releaseLbl_, "RELEASE");

    // LCD Piano Roll & Bar Selector
    addAndMakeVisible(pianoRoll_);
    for (int i = 0; i < 8; ++i) {
        barSelectBtns_[i].setButtonText("BAR " + juce::String(i + 1));
        barSelectBtns_[i].setClickingTogglesState(false);
        barSelectBtns_[i].onClick = [this, i] {
            pianoRoll_.setSelectedBar(i);
            for (int k = 0; k < 8; ++k)
                barSelectBtns_[k].setToggleState(k == i, juce::dontSendNotification);
        };
        addAndMakeVisible(barSelectBtns_[i]);
    }
    barSelectBtns_[0].setToggleState(true, juce::dontSendNotification);

    // Compositional Selectors
    eraBox_.addItemList({
        "1994-1995 Foundation / Ragga Jungle",
        "1996-1997 Dark Jungle / Early Techstep",
        "1998-1999 Techstep / Early Neurofunk",
        "2000-2001 Minimal Dark Rollers",
        "2002-2004 Liquid Funk & Jump-Up"
    }, 1);
    addAndMakeVisible(eraBox_);

    archetypeBox_.addItemList({
        "Roots Jungle Sub-Bass", "808-Style Sub", "Dub-Influenced Bass",
        "Reese Bassline", "Techstep", "Neurofunk", "Rolling Bassline",
        "Minimal Dark Roller", "Liquid Funk", "Jump-Up",
        "Stabbing Bassline", "Pedal-Note Pulse"
    }, 1);
    addAndMakeVisible(archetypeBox_);

    drumContextBox_.addItemList({ "Two-Step DnB", "Amen Break", "Think Break", "Rolling 16th Break" }, 1);
    addAndMakeVisible(drumContextBox_);

    phraseLengthBox_.addItemList({ "1 Bar", "2 Bars", "4 Bars", "8 Bars" }, 1);
    addAndMakeVisible(phraseLengthBox_);

    // Compositional Sliders
    auto setupHSlider = [this](juce::Slider& s, juce::Label& l, const juce::String& text) {
        s.setSliderStyle(juce::Slider::LinearHorizontal);
        s.setTextBoxStyle(juce::Slider::TextBoxRight, false, 44, 18);
        s.setColour(juce::Slider::textBoxBackgroundColourId, Palette::kLcdBg);
        s.setColour(juce::Slider::textBoxTextColourId, Palette::kLcdPhosphor);
        s.setColour(juce::Slider::textBoxOutlineColourId, Palette::kBevelShadow);
        addAndMakeVisible(s);
        l.setText(text, juce::dontSendNotification);
        l.setFont(juce::FontOptions(10.5f, juce::Font::bold));
        addAndMakeVisible(l);
    };
    setupHSlider(densitySlider_, densityLbl_, "NOTE DENSITY");
    setupHSlider(syncopationSlider_, syncopationLbl_, "SYNCOPATION");
    setupHSlider(variationSlider_, variationLbl_, "PHRASE VARIATION");
    setupHSlider(lengthSlider_, lengthLbl_, "NOTE LENGTH");
    setupHSlider(chromaticSlider_, chromaticLbl_, "CHROMATICISM");
    setupHSlider(octaveSlider_, octaveLbl_, "OCTAVE MOVEMENT");
    setupHSlider(drumAwareSlider_, drumAwareLbl_, "DRUM AWARENESS");
    setupHSlider(swingSlider_, swingLbl_, "SWING");
    setupHSlider(responseAmountSlider_, responseAmountLbl_, "ANSWER AMOUNT");

    // Call & Response
    addAndMakeVisible(callResponseBtn_);
    responseDirBox_.addItemList({ "Answer Up", "Answer Down", "Answer Mirror" }, 1);
    addAndMakeVisible(responseDirBox_);

    // Dimension Locks
    addAndMakeVisible(lockRhythmBtn_);
    addAndMakeVisible(lockPitchBtn_);
    addAndMakeVisible(lockPhraseBtn_);
    addAndMakeVisible(lockGrooveBtn_);

    // Bind APVTS Attachments
    attAttack_  = std::make_unique<SliderAtt>(processor_.apvts, "attack", attackKnob_);
    attDecay_   = std::make_unique<SliderAtt>(processor_.apvts, "decay", decayKnob_);
    attSustain_ = std::make_unique<SliderAtt>(processor_.apvts, "sustain", sustainKnob_);
    attRelease_ = std::make_unique<SliderAtt>(processor_.apvts, "release", releaseKnob_);

    attDensity_   = std::make_unique<SliderAtt>(processor_.apvts, "noteDensity", densitySlider_);
    attSync_      = std::make_unique<SliderAtt>(processor_.apvts, "syncopation", syncopationSlider_);
    attVar_       = std::make_unique<SliderAtt>(processor_.apvts, "phraseVariation", variationSlider_);
    attLen_       = std::make_unique<SliderAtt>(processor_.apvts, "noteLength", lengthSlider_);
    attChrom_     = std::make_unique<SliderAtt>(processor_.apvts, "chromaticism", chromaticSlider_);
    attOct_       = std::make_unique<SliderAtt>(processor_.apvts, "octaveMovement", octaveSlider_);
    attDrumAware_ = std::make_unique<SliderAtt>(processor_.apvts, "drumAwareness", drumAwareSlider_);
    attSwing_     = std::make_unique<SliderAtt>(processor_.apvts, "swing", swingSlider_);

    attEra_    = std::make_unique<ComboAtt>(processor_.apvts, "era", eraBox_);
    attArch_   = std::make_unique<ComboAtt>(processor_.apvts, "archetype", archetypeBox_);
    attDrum_   = std::make_unique<ComboAtt>(processor_.apvts, "drumContext", drumContextBox_);
    attRoot_   = std::make_unique<ComboAtt>(processor_.apvts, "rootNote", rootNoteBox_);
    attPhrase_ = std::make_unique<ComboAtt>(processor_.apvts, "phraseBars", phraseLengthBox_);

    attLockRhythm_ = std::make_unique<ButtonAtt>(processor_.apvts, "lockRhythm", lockRhythmBtn_);
    attLockPitch_  = std::make_unique<ButtonAtt>(processor_.apvts, "lockPitch", lockPitchBtn_);
    attLockPhrase_ = std::make_unique<ButtonAtt>(processor_.apvts, "lockPhrase", lockPhraseBtn_);
    attLockGroove_ = std::make_unique<ButtonAtt>(processor_.apvts, "lockGroove", lockGrooveBtn_);
    attLiveOut_    = std::make_unique<ButtonAtt>(processor_.apvts, "liveMidiOut", liveMidiOutToggle_);

    attCallResp_ = std::make_unique<ButtonAtt>(processor_.apvts, "callResponse", callResponseBtn_);
    attRespDir_  = std::make_unique<ComboAtt>(processor_.apvts, "responseDir", responseDirBox_);
    attRespAmt_  = std::make_unique<SliderAtt>(processor_.apvts, "responseAmount", responseAmountSlider_);

    startTimerHz(30);
}

BleekBassBoxEditor::~BleekBassBoxEditor() {
    stopTimer();
    setLookAndFeel(nullptr);
}

void BleekBassBoxEditor::timerCallback() {
    seedReadout_.setText(
        juce::String::formatted("%.1f BPM | S:%u", processor_.getHostBpm(), processor_.getCurrentSeed()),
        juce::dontSendNotification);

    const int phraseIdx = static_cast<int>(processor_.apvts.getRawParameterValue("phraseBars")->load());
    const int barChoices[4] = { 1, 2, 4, 8 };
    const int activeBarCount = barChoices[std::clamp(phraseIdx, 0, 3)];

    for (int i = 0; i < 8; ++i)
        barSelectBtns_[i].setEnabled(i < activeBarCount);

    // While the host plays, follow the playhead through the phrase bars
    int wantedBar = pianoRoll_.getSelectedBar();
    if (processor_.isHostPlaying())
        wantedBar = static_cast<int>(std::floor(processor_.getPlayheadBarPosition()));
    if (wantedBar >= activeBarCount || wantedBar < 0)
        wantedBar = 0;

    if (wantedBar != pianoRoll_.getSelectedBar()) {
        pianoRoll_.setSelectedBar(wantedBar);
        for (int k = 0; k < 8; ++k)
            barSelectBtns_[k].setToggleState(k == wantedBar, juce::dontSendNotification);
    }

    pianoRoll_.repaint();

    // Redraw the sample/ADSR display when the envelope knobs or sample change
    const std::array<float, 4> adsrNow {
        processor_.apvts.getRawParameterValue("attack")->load(),
        processor_.apvts.getRawParameterValue("decay")->load(),
        processor_.apvts.getRawParameterValue("sustain")->load(),
        processor_.apvts.getRawParameterValue("release")->load() };
    const auto assetNow = processor_.sampler.getCurrentAsset().get();
    if (adsrNow != lastAdsr_ || assetNow != lastAsset_) {
        lastAdsr_ = adsrNow;
        lastAsset_ = assetNow;
        sampleDropZone_.repaint();
    }
}

void BleekBassBoxEditor::exportMidiWithFileChooser() {
    activeFileChooser_ = std::make_unique<juce::FileChooser>(
        "Export BLEEK BASS BOX Pattern (.mid)",
        juce::File::getSpecialLocation(juce::File::userDesktopDirectory).getChildFile("BLEEK_BASS_PATTERN.mid"),
        "*.mid");

    activeFileChooser_->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& fc) {
            auto file = fc.getResult();
            if (file != juce::File{}) {
                const int phraseIdx = static_cast<int>(processor_.apvts.getRawParameterValue("phraseBars")->load());
                const int barChoices[4] = { 1, 2, 4, 8 };
                const int totalBars = barChoices[std::clamp(phraseIdx, 0, 3)];
                MidiExporter::exportToFile(processor_.getPatternSnapshot(), processor_.getHostBpm(), totalBars, file);
            }
        });
}

void BleekBassBoxEditor::browseSampleWithFileChooser() {
    activeFileChooser_ = std::make_unique<juce::FileChooser>(
        "Load Monophonic Bass Sample",
        juce::File::getSpecialLocation(juce::File::userHomeDirectory),
        "*.wav;*.aif;*.aiff;*.flac");

    activeFileChooser_->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& fc) {
            auto file = fc.getResult();
            if (file.existsAsFile()) {
                processor_.loadSampleFile(file);
                sampleDropZone_.repaint();
            }
        });
}

void BleekBassBoxEditor::paint(juce::Graphics& g) {
    const auto full = getLocalBounds().toFloat();
    const float earW = 24.0f;

    // 1. 19-Inch Studio Rack Ears (Left & Right)
    g.setColour(Palette::kRackEarBg);
    g.fillRect(0.0f, 0.0f, earW, full.getHeight());
    g.fillRect(full.getWidth() - earW, 0.0f, earW, full.getHeight());

    // Rack mounting slots + Phillips screws
    auto drawRackEarHardware = [&](float xCenter) {
        for (float yPos : { 36.0f, full.getHeight() - 36.0f }) {
            g.setColour(Palette::kBevelShadow);
            g.fillRoundedRectangle(xCenter - 6.0f, yPos - 9.0f, 12.0f, 18.0f, 5.0f);
            BleekLookAndFeel::drawPhillipsScrew(g, xCenter, yPos, 5.5f);
        }
    };
    drawRackEarHardware(earW * 0.5f);
    drawRackEarHardware(full.getWidth() - earW * 0.5f);

    // 2. Anodized Brushed Steel Faceplate
    auto faceplate = full.reduced(earW, 0.0f);
    juce::ColourGradient metalGrad(juce::Colour(0xFF333842), faceplate.getX(), 0.0f,
                                   juce::Colour(0xFF23272E), faceplate.getX(), faceplate.getHeight(), false);
    g.setGradientFill(metalGrad);
    g.fillRect(faceplate);
    BleekLookAndFeel::drawHardwareBezel(g, faceplate.reduced(0.5f), false);

    // Subtle horizontal brushed-metal grain lines
    g.setColour(juce::Colours::white.withAlpha(0.015f));
    for (int y = 2; y < getHeight(); y += 4) {
        g.drawHorizontalLine(y, faceplate.getX() + 2.0f, faceplate.getRight() - 2.0f);
    }

    // 3. Recessed Hardware Sub-Panel Groupings
    auto samplerPanel = juce::Rectangle<float>(faceplate.getX() + 10.0f, 62.0f, faceplate.getWidth() - 20.0f, 108.0f);
    g.setColour(Palette::kPanelRecess);
    g.fillRoundedRectangle(samplerPanel, 3.0f);
    BleekLookAndFeel::drawHardwareBezel(g, samplerPanel, true);

    auto enginePanel = juce::Rectangle<float>(faceplate.getX() + 10.0f, 404.0f, faceplate.getWidth() - 20.0f, 260.0f);
    g.setColour(Palette::kPanelRecess);
    g.fillRoundedRectangle(enginePanel, 3.0f);
    BleekLookAndFeel::drawHardwareBezel(g, enginePanel, true);

    // 4. Classic 1990s Silkscreen Branding & Model Badge
    const float headerX = faceplate.getX() + 14.0f;
    g.setColour(Palette::kAmberLed);
    g.fillRect(headerX, 14.0f, 4.0f, 34.0f);

    g.setColour(Palette::kSilkscreen);
    g.setFont(juce::FontOptions(17.5f, juce::Font::bold));
    g.drawText("BLEEK BASS BOX", static_cast<int>(headerX + 10.0f), 11, 210, 22, juce::Justification::centredLeft);

    g.setColour(Palette::kAmberLed);
    g.setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 9.5f, juce::Font::bold));
    g.drawText("MODEL S-9404 // DIGITAL BASS SYNTHESIS", static_cast<int>(headerX + 10.0f), 32, 250, 16, juce::Justification::centredLeft);

    // 5. Mini-LCD Tempo/Seed Readout Backing Box
    auto lcdSeedRect = seedReadout_.getBounds().toFloat();
    g.setColour(Palette::kLcdBg);
    g.fillRoundedRectangle(lcdSeedRect, 2.5f);
    BleekLookAndFeel::drawHardwareBezel(g, lcdSeedRect, true);

    // Silkscreen section labels
    g.setColour(Palette::kSilkMuted);
    g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
    g.drawText("AMP ENVELOPE GENERATOR [ADSR]", static_cast<int>(faceplate.getX() + 505.0f), 65, 390, 14, juce::Justification::centred);
}

void BleekBassBoxEditor::resized() {
    auto area = getLocalBounds().reduced(24, 0).reduced(14, 12);

    // 1. Top Bar
    auto topBar = area.removeFromTop(42);
    topBar.removeFromLeft(255); // Brand header space
    generateBtn_.setBounds(topBar.removeFromLeft(105).reduced(2));
    dragMidBadge_.setBounds(topBar.removeFromLeft(98).reduced(2));
    exportMidBtn_.setBounds(topBar.removeFromLeft(98).reduced(2));
    liveMidiOutToggle_.setBounds(topBar.removeFromLeft(118).reduced(2));
    seedReadout_.setBounds(topBar.reduced(2, 4));

    area.removeFromTop(16);

    // 2. Sampler & ADSR Strip
    auto samplerRow = area.removeFromTop(96);
    auto dropArea = samplerRow.removeFromLeft(465);
    sampleDropZone_.setBounds(dropArea.removeFromTop(68));
    auto sampleBtns = dropArea.withTrimmedTop(4);
    loadSampleBtn_.setBounds(sampleBtns.removeFromLeft(118));
    sampleBtns.removeFromLeft(6);
    sineResetBtn_.setBounds(sampleBtns.removeFromLeft(92));
    sampleBtns.removeFromLeft(6);
    rootNoteBox_.setBounds(sampleBtns);

    samplerRow.removeFromLeft(14);
    samplerRow.removeFromTop(8);
    const int knobW = samplerRow.getWidth() / 4;
    auto placeKnob = [&](juce::Slider& s, juce::Label& l) {
        auto cell = samplerRow.removeFromLeft(knobW);
        l.setBounds(cell.removeFromTop(14));
        s.setBounds(cell);
    };
    placeKnob(attackKnob_, attackLbl_);
    placeKnob(decayKnob_, decayLbl_);
    placeKnob(sustainKnob_, sustainLbl_);
    placeKnob(releaseKnob_, releaseLbl_);

    area.removeFromTop(16);

    // 3. LCD Piano Roll & Bar Selector
    auto barTabsRow = area.removeFromTop(24);
    const int tabW = 72;
    for (int i = 0; i < 8; ++i)
        barSelectBtns_[i].setBounds(barTabsRow.removeFromLeft(tabW).reduced(1));

    area.removeFromTop(4);
    pianoRoll_.setBounds(area.removeFromTop(182));

    area.removeFromTop(18);

    // 4. Compositional Selectors & Dimension Locks
    auto selectorRow = area.removeFromTop(28);
    eraBox_.setBounds(selectorRow.removeFromLeft(260).reduced(2));
    archetypeBox_.setBounds(selectorRow.removeFromLeft(220).reduced(2));
    drumContextBox_.setBounds(selectorRow.removeFromLeft(175).reduced(2));
    phraseLengthBox_.setBounds(selectorRow.removeFromLeft(110).reduced(2));

    area.removeFromTop(8);

    auto lockRow = area.removeFromTop(28);
    const int lockW = lockRow.getWidth() / 4;
    lockRhythmBtn_.setBounds(lockRow.removeFromLeft(lockW).reduced(2));
    lockPitchBtn_.setBounds(lockRow.removeFromLeft(lockW).reduced(2));
    lockPhraseBtn_.setBounds(lockRow.removeFromLeft(lockW).reduced(2));
    lockGrooveBtn_.setBounds(lockRow.removeFromLeft(lockW).reduced(2));

    area.removeFromTop(8);

    // Call & Response row
    auto crRow = area.removeFromTop(30);
    callResponseBtn_.setBounds(crRow.removeFromLeft(lockW).reduced(2));
    responseDirBox_.setBounds(crRow.removeFromLeft(170).reduced(2, 3));
    crRow.removeFromLeft(16);
    responseAmountLbl_.setBounds(crRow.removeFromLeft(135));
    responseAmountSlider_.setBounds(crRow.removeFromLeft(300));

    area.removeFromTop(10);

    // 5. 2-Column Compositional Sliders
    auto leftCol = area.removeFromLeft(area.getWidth() / 2).reduced(6, 0);
    auto rightCol = area.reduced(6, 0);

    auto placeSlider = [](juce::Rectangle<int>& col, juce::Slider& s, juce::Label& l) {
        auto row = col.removeFromTop(33);
        l.setBounds(row.removeFromLeft(135));
        s.setBounds(row);
    };

    placeSlider(leftCol, densitySlider_, densityLbl_);
    placeSlider(leftCol, syncopationSlider_, syncopationLbl_);
    placeSlider(leftCol, variationSlider_, variationLbl_);
    placeSlider(leftCol, lengthSlider_, lengthLbl_);

    placeSlider(rightCol, chromaticSlider_, chromaticLbl_);
    placeSlider(rightCol, octaveSlider_, octaveLbl_);
    placeSlider(rightCol, drumAwareSlider_, drumAwareLbl_);
    placeSlider(rightCol, swingSlider_, swingLbl_);
}

} // namespace bleek
