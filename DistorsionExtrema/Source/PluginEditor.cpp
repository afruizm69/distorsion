#include "PluginEditor.h"

using namespace juce;
// DISTORSION EXTREMA - interfaz

namespace dex
{
// =============================================================================
RedLookAndFeel::RedLookAndFeel()
{
    setColour (Slider::textBoxTextColourId, Col::text);
    setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
    setColour (Slider::textBoxBackgroundColourId, Colours::transparentBlack);
    setColour (ComboBox::backgroundColourId, Col::control);
    setColour (ComboBox::outlineColourId, Col::panelEdge);
    setColour (ComboBox::textColourId, Col::text);
    setColour (ComboBox::arrowColourId, Col::redHot);
    setColour (PopupMenu::backgroundColourId, Colour (0xff17171b));
    setColour (PopupMenu::highlightedBackgroundColourId, Col::red);
    setColour (PopupMenu::textColourId, Col::text);
    setColour (PopupMenu::highlightedTextColourId, Colours::white);
    setColour (Label::textColourId, Col::textDim);
    setColour (TextButton::buttonColourId, Col::control);
    setColour (TextButton::buttonOnColourId, Col::red);
    setColour (TextButton::textColourOffId, Col::text);
    setColour (TextButton::textColourOnId, Colours::white);
    setColour (ToggleButton::textColourId, Col::text);
}

void RedLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos,
                                       float startAngle, float endAngle, Slider&)
{
    auto bounds = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (3.0f);
    const float size = jmin (bounds.getWidth(), bounds.getHeight());
    auto r = bounds.withSizeKeepingCentre (size, size);
    const auto c = r.getCentre();
    const float rad = size * 0.5f - 3.0f;
    const float ang = startAngle + pos * (endAngle - startAngle);
    const float stroke = jmax (3.0f, size * 0.07f);

    Path track;
    track.addCentredArc (c.x, c.y, rad, rad, 0.0f, startAngle, endAngle, true);
    g.setColour (Col::panelEdge);
    g.strokePath (track, PathStrokeType (stroke, PathStrokeType::curved, PathStrokeType::rounded));

    Path val;
    val.addCentredArc (c.x, c.y, rad, rad, 0.0f, startAngle, ang, true);
    g.setColour (Col::red.withAlpha (0.25f));
    g.strokePath (val, PathStrokeType (stroke * 2.2f, PathStrokeType::curved, PathStrokeType::rounded));
    g.setColour (Col::redHot);
    g.strokePath (val, PathStrokeType (stroke, PathStrokeType::curved, PathStrokeType::rounded));

    auto inner = r.reduced (stroke * 2.4f);
    g.setGradientFill (ColourGradient (Colour (0xff2e2e34), inner.getTopLeft(),
                                       Colour (0xff0f0f12), inner.getBottomRight(), false));
    g.fillEllipse (inner);
    g.setColour (Colour (0xff3a3a42));
    g.drawEllipse (inner, 1.0f);

    const float pl = inner.getWidth() * 0.5f;
    Path p;
    p.addRoundedRectangle (-1.5f, -pl + 3.0f, 3.0f, pl * 0.5f, 1.5f);
    g.setColour (Colours::white);
    g.fillPath (p, AffineTransform::rotation (ang).translated (c.x, c.y));
}

Label* RedLookAndFeel::createSliderTextBox (Slider& s)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (s);
    l->setColour (Label::outlineColourId, Colours::transparentBlack);
    l->setColour (Label::outlineWhenEditingColourId, Col::red);
    l->setColour (Label::backgroundColourId, Colours::transparentBlack);
    l->setColour (Label::textColourId, Col::text);
    l->setFont (boldFont (12.0f));
    return l;
}

void RedLookAndFeel::drawToggleButton (Graphics& g, ToggleButton& b, bool highlighted, bool)
{
    auto r = b.getLocalBounds().toFloat();
    const bool on = b.getToggleState();
    auto led = Rectangle<float> (r.getX() + 2.0f, r.getCentreY() - 6.0f, 12.0f, 12.0f);
    if (on)
    {
        g.setColour (Col::red.withAlpha (0.35f));
        g.fillEllipse (led.expanded (3.0f));
        g.setColour (Col::redHot);
    }
    else
    {
        g.setColour (Col::redDark.withAlpha (highlighted ? 0.9f : 0.6f));
    }
    g.fillEllipse (led);
    g.setColour (Colours::black.withAlpha (0.5f));
    g.drawEllipse (led, 1.0f);

    g.setColour (on ? Col::text : Col::textDim);
    g.setFont (boldFont (12.5f));
    g.drawText (b.getButtonText(), r.withTrimmedLeft (20.0f), Justification::centredLeft, true);
}

void RedLookAndFeel::drawButtonBackground (Graphics& g, Button& b, const Colour&, bool highlighted, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    Colour fill = b.getToggleState() ? Col::red : Col::control;
    if (highlighted) fill = fill.brighter (0.12f);
    if (down) fill = fill.darker (0.2f);
    g.setColour (fill);
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (b.getToggleState() ? Col::redHot : Col::panelEdge);
    g.drawRoundedRectangle (r, 4.0f, 1.0f);
}

void RedLookAndFeel::drawComboBox (Graphics& g, int w, int h, bool, int, int, int, int, ComboBox& box)
{
    auto r = Rectangle<float> (0.0f, 0.0f, (float) w, (float) h).reduced (0.5f);
    g.setColour (box.isEnabled() ? Col::control : Col::control.darker (0.4f));
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (box.hasKeyboardFocus (true) ? Col::red : Col::panelEdge);
    g.drawRoundedRectangle (r, 4.0f, 1.0f);

    Path arrow;
    const float ax = (float) w - 14.0f, ay = (float) h * 0.5f;
    arrow.addTriangle (ax - 4.0f, ay - 2.0f, ax + 4.0f, ay - 2.0f, ax, ay + 3.0f);
    g.setColour (box.isEnabled() ? Col::redHot : Col::textDim);
    g.fillPath (arrow);
}

// =============================================================================
Knob::Knob (AudioProcessorValueTreeState& s, const String& id, const String& name, bool small)
{
    slider.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (Slider::TextBoxBelow, true, 80, small ? 14 : 16);
    slider.setRotaryParameters (MathConstants<float>::pi * 1.25f, MathConstants<float>::pi * 2.75f, true);
    label.setText (name, dontSendNotification);
    label.setJustificationType (Justification::centred);
    addAndMakeVisible (slider);
    addAndMakeVisible (label);
    att = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (s, id, slider);
    slider.setDoubleClickReturnValue (true, (double) s.getParameterRange (id).convertFrom0to1 (s.getParameter (id)->getDefaultValue()));
}

void Knob::resized()
{
    auto b = getLocalBounds();
    label.setBounds (b.removeFromTop (16));
    slider.setBounds (b);
}

Choice::Choice (AudioProcessorValueTreeState& s, const String& id, const String& name)
{
    if (auto* p = dynamic_cast<AudioParameterChoice*> (s.getParameter (id)))
        box.addItemList (p->choices, 1);
    label.setText (name, dontSendNotification);
    label.setJustificationType (Justification::centredLeft);
    addAndMakeVisible (box);
    addAndMakeVisible (label);
    att = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment> (s, id, box);
}

void Choice::resized()
{
    auto b = getLocalBounds();
    if (label.getText().isNotEmpty())
        label.setBounds (b.removeFromTop (15));
    box.setBounds (b.withSizeKeepingCentre (b.getWidth(), jmin (b.getHeight(), 26)));
}

Toggle::Toggle (AudioProcessorValueTreeState& s, const String& id, const String& name)
{
    button.setButtonText (name);
    addAndMakeVisible (button);
    att = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (s, id, button);
}

void SecretPanel::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (Colour (0xff120507));
    g.fillRoundedRectangle (r, 8.0f);
    g.setColour (Col::redHot);
    g.drawRoundedRectangle (r, 8.0f, 1.5f);
    g.setFont (boldFont (14.0f));
    g.drawText (U ("☠ SECRETO"), getLocalBounds().reduced (12, 6).removeFromTop (20), Justification::centredLeft);
    g.setColour (Col::textDim);
    g.setFont (plainFont (11.0f));
    g.drawText ("efectos ocultos", getLocalBounds().reduced (12, 6).removeFromTop (20), Justification::centredRight);
}

// =============================================================================
EqGraph::EqGraph (DistorsionExtremaAudioProcessor& p) : proc (p)
{
    spectrum.fill (-120.0f);
    startTimerHz (30);
}

Colour EqGraph::eqColour (int i)
{
    switch (i)
    {
        case 0: return Col::redHot;
        case 1: return Col::amber;
        default: return Colour (0xfff2f2f2);
    }
}

Rectangle<float> EqGraph::plotArea() const { return getLocalBounds().toFloat().reduced (1.0f).withTrimmedLeft (30.0f).withTrimmedBottom (18.0f); }

float EqGraph::freqToX (float f) const
{
    auto a = plotArea();
    return a.getX() + a.getWidth() * std::log (f / 20.0f) / std::log (1000.0f);
}
float EqGraph::xToFreq (float x) const
{
    auto a = plotArea();
    return 20.0f * std::pow (1000.0f, jlimit (0.0f, 1.0f, (x - a.getX()) / a.getWidth()));
}
float EqGraph::gainToY (float g) const
{
    auto a = plotArea();
    return a.getCentreY() - g / 27.0f * a.getHeight() * 0.5f;
}
float EqGraph::yToGain (float y) const
{
    auto a = plotArea();
    return jlimit (-24.0f, 24.0f, (a.getCentreY() - y) / (a.getHeight() * 0.5f) * 27.0f);
}

RangedAudioParameter* EqGraph::param (int eq, const char* name) const
{
    return proc.apvts.getParameter (eqId (eq, name));
}

void EqGraph::setParam (RangedAudioParameter* p, float value)
{
    p->setValueNotifyingHost (p->convertTo0to1 (value));
}

// Lo que se dibuja: el valor del parametro, o el valor modulado si el LFO / la captura lo mueven
static EqSettings displaySettings (DistorsionExtremaAudioProcessor& proc, int i, bool dragging)
{
    auto& s = proc.apvts;
    EqSettings e;
    e.type = (int) s.getRawParameterValue (eqId (i, "type"))->load();
    e.freq = s.getRawParameterValue (eqId (i, "freq"))->load();
    e.gainDb = s.getRawParameterValue (eqId (i, "gain"))->load();
    e.q = s.getRawParameterValue (eqId (i, "q"))->load();

    const bool lfoHere = s.getRawParameterValue ("lfo_on")->load() > 0.5f
                      && (int) s.getRawParameterValue ("lfo_target")->load() == i;
    const bool playHere = proc.playbackEq.load() == i;
    if (! dragging && (lfoHere || playHere))
    {
        e.freq = proc.eqView[i].freq.load();
        e.gainDb = proc.eqView[i].gain.load();
    }
    return e;
}

Point<float> EqGraph::pointPos (int i) const
{
    const auto e = displaySettings (proc, i, dragging == i);
    return { freqToX (e.freq), gainToY (jlimit (-24.0f, 24.0f, e.gainDb)) };
}

int EqGraph::hitPoint (Point<float> p, float radius) const
{
    int best = -1;
    float bestD = radius;
    for (int i = numEqs - 1; i >= 0; --i)
    {
        const float d = pointPos (i).getDistanceFrom (p);
        if (d < bestD) { bestD = d; best = i; }
    }
    return best;
}

void EqGraph::timerCallback()
{
    const double fs = proc.getSampleRate() > 0 ? proc.getSampleRate() : 44100.0;
    const int w = proc.analyserWrite.load();
    for (int i = 0; i < fftSize; ++i)
        fftData[(size_t) i] = proc.analyserFifo[(size_t) ((w - fftSize + i + DistorsionExtremaAudioProcessor::fifoSize) % DistorsionExtremaAudioProcessor::fifoSize)];
    std::fill (fftData.begin() + fftSize, fftData.end(), 0.0f);
    window.multiplyWithWindowingTable (fftData.data(), (size_t) fftSize);
    fft.performFrequencyOnlyForwardTransform (fftData.data());

    for (size_t k = 0; k < spectrum.size(); ++k)
    {
        const float f = 20.0f * std::pow (1000.0f, (float) k / (float) (spectrum.size() - 1));
        const int bin = jlimit (1, fftSize / 2 - 1, (int) (f / fs * fftSize));
        const float db = Decibels::gainToDecibels (fftData[(size_t) bin] / (fftSize * 0.25f), -120.0f);
        spectrum[k] = jmax (db, spectrum[k] - 1.2f);
    }
    repaint();
}

void EqGraph::paint (Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour (Colour (0xff0c0c0f));
    g.fillRoundedRectangle (bounds, 8.0f);

    auto a = plotArea();
    const double fs = proc.getSampleRate() > 0 ? proc.getSampleRate() : 44100.0;

    // Rejilla
    g.setFont (plainFont (10.5f));
    for (float f : { 20.0f, 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f, 20000.0f })
    {
        const float x = freqToX (f);
        g.setColour (Colour (0xff1c1c22));
        g.drawVerticalLine (roundToInt (x), a.getY(), a.getBottom());
        g.setColour (Col::textDim);
        const String txt = f >= 1000.0f ? String ((int) (f / 1000.0f)) + "k" : String ((int) f);
        auto lr = Rectangle<float> (x - 20.0f, a.getBottom() + 2.0f, 40.0f, 14.0f);
        if (lr.getRight() > a.getRight()) lr.setX (a.getRight() - lr.getWidth() - 2.0f);
        g.drawText (txt, lr, f >= 20000.0f ? Justification::centredRight : Justification::centred);
    }
    for (int db = -24; db <= 24; db += 6)
    {
        const float y = gainToY ((float) db);
        g.setColour (db == 0 ? Colour (0xff34343c) : Colour (0xff1c1c22));
        g.drawHorizontalLine (roundToInt (y), a.getX(), a.getRight());
        g.setColour (Col::textDim);
        g.drawText ((db > 0 ? "+" : "") + String (db), Rectangle<float> (bounds.getX() + 2.0f, y - 7.0f, 26.0f, 14.0f), Justification::centredRight);
    }

    // Analizador
    {
        Path spec;
        spec.startNewSubPath (a.getX(), a.getBottom());
        for (size_t k = 0; k < spectrum.size(); ++k)
        {
            const float x = a.getX() + a.getWidth() * (float) k / (float) (spectrum.size() - 1);
            const float y = jmap (jlimit (-90.0f, 0.0f, spectrum[k]), -90.0f, 0.0f, a.getBottom(), a.getY());
            spec.lineTo (x, y);
        }
        spec.lineTo (a.getRight(), a.getBottom());
        spec.closeSubPath();
        g.setGradientFill (ColourGradient (Col::red.withAlpha (0.28f), 0.0f, a.getY(), Col::red.withAlpha (0.03f), 0.0f, a.getBottom(), false));
        g.fillPath (spec);
    }

    // Curvas
    const int numPts = 220;
    std::vector<float> total ((size_t) numPts, 0.0f);
    EqSettings settings[numEqs];
    for (int i = 0; i < numEqs; ++i)
        settings[i] = displaySettings (proc, i, dragging == i);

    for (int i = 0; i < numEqs; ++i)
    {
        const bool on = proc.apvts.getRawParameterValue (eqId (i, "on"))->load() > 0.5f;
        if (! on) continue;
        Path p;
        for (int k = 0; k < numPts; ++k)
        {
            const float x = a.getX() + a.getWidth() * (float) k / (float) (numPts - 1);
            const float db = eqMagnitudeDb (settings[i], xToFreq (x), fs);
            total[(size_t) k] += db;
            const float y = jlimit (a.getY(), a.getBottom(), gainToY (db));
            if (k == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
        }
        g.setColour (eqColour (i).withAlpha (0.55f));
        g.strokePath (p, PathStrokeType (1.4f));
    }

    {
        Path p;
        for (int k = 0; k < numPts; ++k)
        {
            const float x = a.getX() + a.getWidth() * (float) k / (float) (numPts - 1);
            const float y = jlimit (a.getY(), a.getBottom(), gainToY (total[(size_t) k]));
            if (k == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
        }
        g.setColour (Col::red.withAlpha (0.35f));
        g.strokePath (p, PathStrokeType (6.0f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour (Colours::white);
        g.strokePath (p, PathStrokeType (2.0f, PathStrokeType::curved, PathStrokeType::rounded));
    }

    // Puntos
    const int playing = proc.playbackEq.load();
    for (int i = 0; i < numEqs; ++i)
    {
        const bool on = proc.apvts.getRawParameterValue (eqId (i, "on"))->load() > 0.5f;
        const auto c = pointPos (i);
        const float rad = (i == hover || i == dragging) ? 11.0f : 9.0f;
        auto circle = Rectangle<float> (rad * 2.0f, rad * 2.0f).withCentre (c);
        if (i == playing || (i == dragging && proc.isRecording()))
        {
            g.setColour (Col::red.withAlpha (0.35f));
            g.fillEllipse (circle.expanded (6.0f));
        }
        g.setColour (on ? eqColour (i) : Col::control);
        g.fillEllipse (circle);
        g.setColour (on ? Colours::black : eqColour (i));
        g.drawEllipse (circle, 1.5f);
        g.setFont (boldFont (12.0f));
        g.drawText (String (i + 1), circle, Justification::centred);
    }

    // Etiqueta del punto activo
    const int info = dragging >= 0 ? dragging : hover;
    if (info >= 0)
    {
        const auto& s = settings[info];
        const String txt = "EQ " + String (info + 1) + "  ·  " + eqTypeNames()[s.type] + "  ·  "
                         + (s.freq >= 1000.0f ? String (s.freq / 1000.0f, 2) + " kHz" : String (roundToInt (s.freq)) + " Hz")
                         + "  ·  " + String (s.gainDb, 1) + " dB  ·  Q " + String (s.q, 2);
        g.setColour (Col::text);
        g.setFont (boldFont (12.0f));
        g.drawText (txt, a.reduced (10.0f, 6.0f).removeFromTop (16.0f), Justification::centredRight);
    }
    else
    {
        g.setColour (Col::textDim);
        g.setFont (plainFont (11.0f));
        g.drawText (U ("Arrastra un punto · rueda = Q · doble clic = reiniciar"),
                    a.reduced (10.0f, 6.0f).removeFromTop (16.0f), Justification::centredRight);
    }

    if (proc.isRecording())
    {
        g.setColour (Col::redHot);
        g.setFont (boldFont (13.0f));
        g.drawText (U ("● GRABANDO MOVIMIENTO"), a.reduced (10.0f, 6.0f).removeFromTop (16.0f), Justification::centredLeft);
    }

    g.setColour (Col::panelEdge);
    g.drawRoundedRectangle (bounds.reduced (0.5f), 8.0f, 1.0f);
}

void EqGraph::mouseMove (const MouseEvent& e)
{
    const int h = hitPoint (e.position, 16.0f);
    if (h != hover)
    {
        hover = h;
        setMouseCursor (hover >= 0 ? MouseCursor::DraggingHandCursor : MouseCursor::NormalCursor);
        repaint();
    }
}

void EqGraph::mouseExit (const MouseEvent&)
{
    hover = -1;
    repaint();
}

void EqGraph::mouseDown (const MouseEvent& e)
{
    dragging = hitPoint (e.position, 16.0f);
    if (dragging < 0) return;

    auto* pf = param (dragging, "freq");
    auto* pg = param (dragging, "gain");

    // Si la captura estaba moviendo este punto, partimos desde donde esta ahora
    if (proc.playbackEq.load() == dragging)
    {
        setParam (pf, proc.eqView[dragging].freq.load());
        setParam (pg, jlimit (-24.0f, 24.0f, proc.eqView[dragging].gain.load()));
    }

    pf->beginChangeGesture();
    pg->beginChangeGesture();
    capturing = proc.apvts.getRawParameterValue ("cap_arm")->load() > 0.5f;
    proc.beginTouch (dragging, capturing);
    mouseDrag (e);
}

void EqGraph::mouseDrag (const MouseEvent& e)
{
    if (dragging < 0) return;
    auto a = plotArea();
    const float x = jlimit (a.getX(), a.getRight(), e.position.x);
    const float y = jlimit (a.getY(), a.getBottom(), e.position.y);
    setParam (param (dragging, "freq"), xToFreq (x));
    setParam (param (dragging, "gain"), yToGain (y));
    repaint();
}

void EqGraph::mouseUp (const MouseEvent&)
{
    if (dragging < 0) return;
    param (dragging, "freq")->endChangeGesture();
    param (dragging, "gain")->endChangeGesture();
    proc.endTouch();

    if (capturing)
    {
        // Al soltar: se reproduce la memoria recien grabada y se desarma la grabacion
        const int slot = (int) proc.apvts.getRawParameterValue ("cap_slot")->load();
        auto* play = proc.apvts.getParameter ("cap_play");
        play->beginChangeGesture();
        play->setValueNotifyingHost (play->convertTo0to1 ((float) (slot + 1)));
        play->endChangeGesture();
        auto* arm = proc.apvts.getParameter ("cap_arm");
        arm->beginChangeGesture();
        arm->setValueNotifyingHost (0.0f);
        arm->endChangeGesture();
    }
    capturing = false;
    dragging = -1;
}

void EqGraph::mouseDoubleClick (const MouseEvent& e)
{
    const int i = hitPoint (e.position, 16.0f);
    if (i < 0) return;
    for (auto* n : { "freq", "gain", "q" })
    {
        auto* p = param (i, n);
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->getDefaultValue());
        p->endChangeGesture();
    }
}

void EqGraph::mouseWheelMove (const MouseEvent& e, const MouseWheelDetails& wheel)
{
    const int i = dragging >= 0 ? dragging : hitPoint (e.position, 24.0f);
    if (i < 0) return;
    auto* p = param (i, "q");
    const float q = p->convertFrom0to1 (p->getValue());
    const float delta = wheel.deltaY != 0.0f ? wheel.deltaY : wheel.deltaX;
    p->beginChangeGesture();
    setParam (p, jlimit (0.1f, 18.0f, q * std::exp (delta * 1.5f)));
    p->endChangeGesture();
}
} // namespace dex

// =============================================================================
using namespace dex;

DistorsionExtremaAudioProcessorEditor::DistorsionExtremaAudioProcessorEditor (DistorsionExtremaAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p), graph (p),
      lfoOn (p.apvts, "lfo_on", "ACTIVO"),
      lfoTarget (p.apvts, "lfo_target", "DESTINO"),
      lfoDest (p.apvts, "lfo_dest", "MODULA"),
      lfoShape (p.apvts, "lfo_shape", "FORMA"),
      lfoDiv (p.apvts, "lfo_div", U ("DIVISIÓN")),
      lfoRate (p.apvts, "lfo_rate", "VELOCIDAD"),
      lfoDepth (p.apvts, "lfo_depth", "PROFUNDIDAD"),
      lfoSync (p.apvts, "lfo_sync", "SINCRO"),
      capArm (p.apvts, "cap_arm", "GRABAR"),
      capPlay (p.apvts, "cap_play", "REPRODUCIR"),
      capSpeed (p.apvts, "cap_speed", "VELOCIDAD"),
      wsOn (p.apvts, "ws_on", "ACTIVO"),
      wsAmt (p.apvts, "ws_amt", "CANTIDAD"),
      wsShape (p.apvts, "ws_shape", "FORMA"),
      finOn (p.apvts, "fin_on", "ACTIVO"),
      finType (p.apvts, "fin_type", "TIPO"),
      finDrive (p.apvts, "fin_drive", "DRIVE"),
      inGain (p.apvts, "in", "ENTRADA"),
      mix (p.apvts, "mix", "MEZCLA"),
      outGain (p.apvts, "out", "SALIDA"),
      quality (p.apvts, "quality", {}),
      secCrush (p.apvts, "sec_crush", "DESTROZO"),
      secRing (p.apvts, "sec_ring", "ANILLO"),
      secRingFreq (p.apvts, "sec_ringfreq", "FREC. ANILLO")
{
    addAndMakeVisible (graph);

    for (int i = 0; i < numEqs; ++i)
    {
        eqOn[i] = std::make_unique<Toggle> (p.apvts, eqId (i, "on"), "ACTIVO");
        eqType[i] = std::make_unique<Choice> (p.apvts, eqId (i, "type"), "TIPO");
        eqMode[i] = std::make_unique<Choice> (p.apvts, eqId (i, "mode"), "CANAL");
        eqQ[i] = std::make_unique<Knob> (p.apvts, eqId (i, "q"), "Q", true);
        addAndMakeVisible (*eqOn[i]);
        addAndMakeVisible (*eqType[i]);
        addAndMakeVisible (*eqMode[i]);
        addAndMakeVisible (*eqQ[i]);
    }

    for (auto* c : std::initializer_list<Component*> { &lfoOn, &lfoTarget, &lfoDest, &lfoShape, &lfoDiv, &lfoRate,
                                                       &lfoDepth, &lfoSync, &capArm, &capPlay, &capSpeed, &capStatus,
                                                       &clearButton, &wsOn, &wsAmt, &wsShape, &finOn, &finType,
                                                       &finDrive, &inGain, &mix, &outGain, &quality })
        addAndMakeVisible (c);

    for (int i = 0; i < numSlots; ++i)
    {
        slotButtons[i].setButtonText (String (i + 1));
        slotButtons[i].onClick = [this, i] { setChoiceParam ("cap_slot", i); };
        addAndMakeVisible (slotButtons[i]);
    }
    clearButton.onClick = [this] { proc.clearSlot ((int) proc.apvts.getRawParameterValue ("cap_slot")->load()); };
    capStatus.setJustificationType (Justification::centredRight);
    capStatus.setFont (boldFont (11.5f));

    const auto names = preStageNames();
    for (int i = 0; i < numPreStages; ++i)
    {
        pdOn[i] = std::make_unique<Toggle> (p.apvts, pdId (i, "on"), names[i]);
        pdDrive[i] = std::make_unique<Knob> (p.apvts, pdId (i, "drive"), "DRIVE");
        addAndMakeVisible (*pdOn[i]);
        addAndMakeVisible (*pdDrive[i]);
    }

    secretPanel.addAndMakeVisible (secCrush);
    secretPanel.addAndMakeVisible (secRing);
    secretPanel.addAndMakeVisible (secRingFreq);
    addChildComponent (secretPanel);

    // Se aplica al final, cuando todos los controles ya son hijos del editor
    setLookAndFeel (&lnf);
    setSize (1080, 720);
    startTimerHz (15);
}

DistorsionExtremaAudioProcessorEditor::~DistorsionExtremaAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void DistorsionExtremaAudioProcessorEditor::setChoiceParam (const String& id, int index)
{
    auto* p = proc.apvts.getParameter (id);
    p->beginChangeGesture();
    p->setValueNotifyingHost (p->convertTo0to1 ((float) index));
    p->endChangeGesture();
}

void DistorsionExtremaAudioProcessorEditor::timerCallback()
{
    auto& s = proc.apvts;
    const int sel = (int) s.getRawParameterValue ("cap_slot")->load();
    for (int i = 0; i < numSlots; ++i)
    {
        slotButtons[i].setToggleState (i == sel, dontSendNotification);
        slotButtons[i].setButtonText (String (i + 1) + (proc.isSlotFilled (i) ? U (" ●") : String()));
    }

    const bool sync = s.getRawParameterValue ("lfo_sync")->load() > 0.5f;
    lfoDiv.setEnabled (sync);
    lfoDiv.setAlpha (sync ? 1.0f : 0.4f);
    lfoRate.setEnabled (! sync);
    lfoRate.setAlpha (sync ? 0.4f : 1.0f);

    const int play = (int) s.getRawParameterValue ("cap_play")->load();
    if (proc.isRecording())
    {
        capStatus.setColour (Label::textColourId, Col::redHot);
        capStatus.setText (U ("● GRABANDO EN MEMORIA ") + String (sel + 1), dontSendNotification);
    }
    else if (s.getRawParameterValue ("cap_arm")->load() > 0.5f)
    {
        capStatus.setColour (Label::textColourId, Col::redHot);
        capStatus.setText ("Arrastra un punto del EQ", dontSendNotification);
    }
    else if (play > 0)
    {
        const bool filled = proc.isSlotFilled (play - 1);
        capStatus.setColour (Label::textColourId, filled ? Col::text : Col::textDim);
        capStatus.setText (filled ? U ("▶ Reproduciendo memoria ") + String (play)
                                  : "Memoria " + String (play) + U (" vacía"), dontSendNotification);
    }
    else
    {
        capStatus.setColour (Label::textColourId, Col::textDim);
        capStatus.setText ("Arma GRABAR y mueve un punto", dontSendNotification);
    }
}

void DistorsionExtremaAudioProcessorEditor::mouseDown (const MouseEvent& e)
{
    if (logoArea.contains (e.getPosition()))
    {
        secretVisible = ! secretVisible;
        secretPanel.setVisible (secretVisible);
        if (secretVisible) secretPanel.toFront (false);
        repaint();
    }
}

void DistorsionExtremaAudioProcessorEditor::drawPanel (Graphics& g, Rectangle<int> r, const String& title, const String& subtitle)
{
    auto rf = r.toFloat();
    g.setColour (Col::panel);
    g.fillRoundedRectangle (rf, 8.0f);
    g.setColour (Col::panelEdge);
    g.drawRoundedRectangle (rf.reduced (0.5f), 8.0f, 1.0f);

    auto t = r.reduced (12, 6).removeFromTop (18);
    g.setColour (Col::red);
    g.fillRect (t.getX(), t.getY() + 3, 3, 12);
    g.setColour (Col::text);
    g.setFont (boldFont (13.5f));
    g.drawText (title, t.withTrimmedLeft (9), Justification::centredLeft);
    if (subtitle.isNotEmpty())
    {
        g.setColour (Col::textDim);
        g.setFont (plainFont (11.0f));
        g.drawText (subtitle, t, Justification::centredRight);
    }
}

void DistorsionExtremaAudioProcessorEditor::paint (Graphics& g)
{
    g.setGradientFill (ColourGradient (Colour (0xff121215), 0.0f, 0.0f, Col::bg, 0.0f, (float) getHeight(), false));
    g.fillAll();

    // ---- Cabecera
    g.setGradientFill (ColourGradient (Col::redDark.withAlpha (0.9f), 0.0f, 0.0f,
                                       Colour (0xff0d0d10), (float) getWidth() * 0.6f, 0.0f, false));
    g.fillRect (headerArea);
    g.setColour (Col::red);
    g.fillRect (headerArea.getX(), headerArea.getBottom() - 2, headerArea.getWidth(), 2);

    // Logo: onda recortada en zig-zag
    auto logo = logoArea.toFloat();
    auto icon = logo.removeFromLeft (52.0f).reduced (6.0f, 12.0f);
    Path wave;
    const int segs = 6;
    for (int i = 0; i <= segs; ++i)
    {
        const float x = icon.getX() + icon.getWidth() * (float) i / segs;
        const float y = (i % 2 == 0) ? icon.getBottom() : icon.getY();
        if (i == 0) wave.startNewSubPath (x, icon.getCentreY()); else wave.lineTo (x, jlimit (icon.getY() + 4.0f, icon.getBottom() - 4.0f, y));
    }
    g.setColour (secretVisible ? Colours::white : Col::redHot);
    g.strokePath (wave, PathStrokeType (3.0f, PathStrokeType::mitered, PathStrokeType::square));

    g.setColour (Colours::white);
    g.setFont (Font (FontOptions (30.0f, Font::bold | Font::italic)));
    g.drawText ("DISTORSION", logo.removeFromLeft (190.0f), Justification::centredLeft);
    g.setColour (Col::redHot);
    g.drawText ("EXTREMA", logo, Justification::centredLeft);

    g.setColour (Col::textDim);
    g.setFont (plainFont (12.0f));
    g.drawText (U ("multi-etapa · EQ en movimiento · v1.0"),
                headerArea.withTrimmedLeft (480).withTrimmedRight (250), Justification::centredLeft);
    g.drawText ("CALIDAD", headerArea.withTrimmedRight (176).removeFromRight (70), Justification::centredRight);

    // ---- Paneles
    const String eqTitles[] = { U ("EQ 1 · PRE"), U ("EQ 2 · COLOR"), U ("EQ 3 · POST") };
    for (int i = 0; i < numEqs; ++i)
    {
        drawPanel (g, eqStripArea[i], eqTitles[i]);
        g.setColour (EqGraph::eqColour (i));
        g.fillEllipse ((float) eqStripArea[i].getRight() - 22.0f, (float) eqStripArea[i].getY() + 10.0f, 10.0f, 10.0f);
    }
    drawPanel (g, lfoArea, U ("MUÉVELO"), "modula un punto del EQ");
    drawPanel (g, capArea, "CAPTURA", "4 memorias de movimiento");
    drawPanel (g, preArea, U ("RÓMPELO · PRE-DISTORSIÓN"), "4 etapas en serie");
    drawPanel (g, wsArea, "WAVESHAPER", {});
    drawPanel (g, finArea, "FINAL", {});
    drawPanel (g, outArea, "SALIDA", {});
}

void DistorsionExtremaAudioProcessorEditor::resized()
{
    const int m = 14, gap = 10;
    auto b = getLocalBounds();
    headerArea = b.removeFromTop (64);
    logoArea = headerArea.reduced (m, 0).removeFromLeft (440);
    quality.setBounds (headerArea.withTrimmedRight (m).removeFromRight (160).withSizeKeepingCentre (160, 28));

    b.removeFromTop (gap);
    b = b.reduced (m, 0);
    b.removeFromBottom (m);

    auto bottom = b.removeFromBottom (206);
    b.removeFromBottom (gap);

    auto right = b.removeFromRight (350);
    b.removeFromRight (gap + 2);

    // Izquierda: grafica + tiras de EQ
    auto strips = b.removeFromBottom (124);
    b.removeFromBottom (gap);
    graph.setBounds (b);

    const int sw = (strips.getWidth() - 2 * gap) / 3;
    for (int i = 0; i < numEqs; ++i)
    {
        eqStripArea[i] = strips.removeFromLeft (sw);
        strips.removeFromLeft (gap);
        auto in = eqStripArea[i].reduced (10, 6);
        auto title = in.removeFromTop (20);
        eqOn[i]->setBounds (title.withTrimmedRight (20).removeFromRight (84));
        in.removeFromTop (2);
        eqQ[i]->setBounds (in.removeFromRight (66));
        in.removeFromRight (6);
        eqType[i]->setBounds (in.removeFromTop (42));
        in.removeFromTop (2);
        eqMode[i]->setBounds (in.removeFromTop (42));
    }

    // Derecha: Muevelo + Captura
    lfoArea = right.removeFromTop (212);
    right.removeFromTop (gap);
    capArea = right;

    {
        auto in = lfoArea.reduced (12, 8).withTrimmedTop (24);
        auto row1 = in.removeFromTop (22);
        lfoOn.setBounds (row1.removeFromLeft (100));
        lfoSync.setBounds (row1.removeFromLeft (100));
        in.removeFromTop (6);
        auto row2 = in.removeFromTop (42);
        const int cw = (row2.getWidth() - 12) / 3;
        lfoTarget.setBounds (row2.removeFromLeft (cw)); row2.removeFromLeft (6);
        lfoDest.setBounds (row2.removeFromLeft (cw)); row2.removeFromLeft (6);
        lfoShape.setBounds (row2);
        in.removeFromTop (6);
        const int kw = in.getWidth() / 3;
        lfoRate.setBounds (in.removeFromLeft (kw));
        lfoDepth.setBounds (in.removeFromLeft (kw));
        lfoDiv.setBounds (in.withSizeKeepingCentre (in.getWidth() - 6, 42));
        secretPanel.setBounds (lfoArea);
        auto sp = secretPanel.getLocalBounds().reduced (12, 8).withTrimmedTop (30);
        const int sk = sp.getWidth() / 3;
        secCrush.setBounds (sp.removeFromLeft (sk).withSizeKeepingCentre (sk, jmin (sp.getHeight(), 130)));
        secRing.setBounds (sp.removeFromLeft (sk).withSizeKeepingCentre (sk, jmin (sp.getHeight(), 130)));
        secRingFreq.setBounds (sp.withSizeKeepingCentre (sk, jmin (sp.getHeight(), 130)));
    }
    {
        auto in = capArea.reduced (12, 8).withTrimmedTop (24);
        auto row1 = in.removeFromTop (22);
        capArm.setBounds (row1.removeFromLeft (100));
        capStatus.setBounds (row1);
        in.removeFromTop (8);
        auto row2 = in.removeFromTop (30);
        clearButton.setBounds (row2.removeFromRight (74));
        row2.removeFromRight (8);
        const int bw = (row2.getWidth() - 3 * 6) / 4;
        for (int i = 0; i < numSlots; ++i)
        {
            slotButtons[i].setBounds (row2.removeFromLeft (bw));
            row2.removeFromLeft (6);
        }
        in.removeFromTop (4);
        capSpeed.setBounds (in.removeFromRight (100));
        in.removeFromRight (8);
        capPlay.setBounds (in.withSizeKeepingCentre (in.getWidth(), 42));
    }

    // Abajo: Rompelo
    preArea = bottom.removeFromLeft (370);
    bottom.removeFromLeft (gap);
    wsArea = bottom.removeFromLeft (190);
    bottom.removeFromLeft (gap);
    finArea = bottom.removeFromLeft (190);
    bottom.removeFromLeft (gap);
    outArea = bottom;

    {
        auto in = preArea.reduced (10, 8).withTrimmedTop (24);
        const int cw = in.getWidth() / numPreStages;
        for (int i = 0; i < numPreStages; ++i)
        {
            auto col = in.removeFromLeft (cw);
            pdOn[i]->setBounds (col.removeFromTop (22).reduced (4, 0));
            col.removeFromTop (6);
            pdDrive[i]->setBounds (col);
        }
    }
    {
        auto in = wsArea.reduced (10, 8).withTrimmedTop (24);
        wsOn.setBounds (in.removeFromTop (22).reduced (4, 0));
        in.removeFromTop (6);
        wsAmt.setBounds (in.removeFromLeft (in.getWidth() / 2));
        wsShape.setBounds (in);
    }
    {
        auto in = finArea.reduced (10, 8).withTrimmedTop (24);
        finOn.setBounds (in.removeFromTop (22).reduced (4, 0));
        in.removeFromTop (4);
        finType.setBounds (in.removeFromTop (42).reduced (4, 0));
        in.removeFromTop (2);
        finDrive.setBounds (in);
    }
    {
        auto in = outArea.reduced (10, 8).withTrimmedTop (30);
        const int kw = in.getWidth() / 3;
        inGain.setBounds (in.removeFromLeft (kw));
        mix.setBounds (in.removeFromLeft (kw));
        outGain.setBounds (in);
    }
}
