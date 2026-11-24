#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "PluginProcessor.h"

namespace dex
{
namespace Col
{
    const juce::Colour bg        { 0xff0a0a0c };
    const juce::Colour panel     { 0xff141417 };
    const juce::Colour panelEdge { 0xff2a2a30 };
    const juce::Colour red       { 0xffe3161f };
    const juce::Colour redHot    { 0xffff3434 };
    const juce::Colour redDark   { 0xff4a0a0d };
    const juce::Colour amber     { 0xffffb020 };
    const juce::Colour text      { 0xffeeeeee };
    const juce::Colour textDim   { 0xff8a8a92 };
    const juce::Colour control   { 0xff1d1d22 };
}

inline juce::Font boldFont (float h) { return juce::Font (juce::FontOptions (h, juce::Font::bold)); }
inline juce::Font plainFont (float h) { return juce::Font (juce::FontOptions (h)); }

class RedLookAndFeel : public juce::LookAndFeel_V4
{
public:
    RedLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool highlighted, bool down) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    juce::Label* createSliderTextBox (juce::Slider&) override;
    juce::Font getTextButtonFont (juce::TextButton&, int) override { return boldFont (13.0f); }
    juce::Font getComboBoxFont (juce::ComboBox&) override { return boldFont (12.5f); }
    juce::Font getPopupMenuFont() override { return plainFont (14.0f); }
    juce::Font getLabelFont (juce::Label&) override { return boldFont (11.5f); }
};

// ---- Controles con su enlace al parametro -----------------------------------
struct Knob : juce::Component
{
    Knob (juce::AudioProcessorValueTreeState&, const juce::String& id, const juce::String& name, bool small = false);
    void resized() override;
    juce::Slider slider;
    juce::Label label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
};

struct Choice : juce::Component
{
    Choice (juce::AudioProcessorValueTreeState&, const juce::String& id, const juce::String& name);
    void resized() override;
    juce::ComboBox box;
    juce::Label label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> att;
};

struct Toggle : juce::Component
{
    Toggle (juce::AudioProcessorValueTreeState&, const juce::String& id, const juce::String& name);
    void resized() override { button.setBounds (getLocalBounds()); }
    juce::ToggleButton button;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> att;
};

struct SecretPanel : juce::Component
{
    void paint (juce::Graphics&) override;
};

// ---- Pantalla del EQ con puntos arrastrables y analizador --------------------
class EqGraph : public juce::Component, private juce::Timer
{
public:
    explicit EqGraph (DistorsionExtremaAudioProcessor&);
    ~EqGraph() override { stopTimer(); }

    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

    static juce::Colour eqColour (int i);

private:
    void timerCallback() override;
    juce::Rectangle<float> plotArea() const;
    float freqToX (float f) const;
    float xToFreq (float x) const;
    float gainToY (float g) const;
    float yToGain (float y) const;
    juce::Point<float> pointPos (int i) const;
    int hitPoint (juce::Point<float> p, float radius) const;
    juce::RangedAudioParameter* param (int eq, const char* name) const;
    void setParam (juce::RangedAudioParameter*, float value);

    DistorsionExtremaAudioProcessor& proc;
    int dragging = -1, hover = -1;
    bool capturing = false;

    static constexpr int fftOrder = 11, fftSize = 1 << fftOrder;
    juce::dsp::FFT fft { fftOrder };
    juce::dsp::WindowingFunction<float> window { (size_t) fftSize, juce::dsp::WindowingFunction<float>::hann };
    std::array<float, fftSize * 2> fftData {};
    std::array<float, 256> spectrum {};
};
} // namespace dex

class DistorsionExtremaAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit DistorsionExtremaAudioProcessorEditor (DistorsionExtremaAudioProcessor&);
    ~DistorsionExtremaAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    void timerCallback() override;
    void drawPanel (juce::Graphics&, juce::Rectangle<int>, const juce::String& title, const juce::String& subtitle = {});
    void setChoiceParam (const juce::String& id, int index);

    DistorsionExtremaAudioProcessor& proc;
    dex::RedLookAndFeel lnf;

    dex::EqGraph graph;

    // EQ strips
    std::unique_ptr<dex::Toggle> eqOn[dex::numEqs];
    std::unique_ptr<dex::Choice> eqType[dex::numEqs], eqMode[dex::numEqs];
    std::unique_ptr<dex::Knob> eqQ[dex::numEqs];

    // Muevelo (LFO)
    dex::Toggle lfoOn;
    dex::Choice lfoTarget, lfoDest, lfoShape, lfoDiv;
    dex::Knob lfoRate, lfoDepth;
    dex::Toggle lfoSync;

    // Captura
    dex::Toggle capArm;
    juce::TextButton slotButtons[dex::numSlots];
    juce::TextButton clearButton { "BORRAR" };
    dex::Choice capPlay;
    dex::Knob capSpeed;
    juce::Label capStatus;

    // Rompelo
    std::unique_ptr<dex::Toggle> pdOn[dex::numPreStages];
    std::unique_ptr<dex::Knob> pdDrive[dex::numPreStages];
    dex::Toggle wsOn;
    dex::Knob wsAmt, wsShape;
    dex::Toggle finOn;
    dex::Choice finType;
    dex::Knob finDrive;
    dex::Knob inGain, mix, outGain;
    dex::Choice quality;

    // Secreto
    dex::SecretPanel secretPanel;
    dex::Knob secCrush, secRing, secRingFreq;
    bool secretVisible = false;

    // Zonas de layout
    juce::Rectangle<int> headerArea, logoArea, eqStripArea[dex::numEqs], lfoArea, capArea,
                         preArea, wsArea, finArea, outArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DistorsionExtremaAudioProcessorEditor)
};
