// Banco de pruebas offline: procesa audio con distintas configuraciones,
// comprueba la captura de movimiento y el guardado de estado, y renderiza la interfaz a PNG.
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include <iostream>

using namespace juce;

static int failures = 0;
#define CHECK(cond, msg) do { if (! (cond)) { std::cout << "FALLO: " << msg << std::endl; ++failures; } else std::cout << "ok: " << msg << std::endl; } while (0)

static void setP (DistorsionExtremaAudioProcessor& p, const String& id, float value)
{
    auto* prm = p.apvts.getParameter (id);
    prm->setValueNotifyingHost (prm->convertTo0to1 (value));
}

struct Stats { float peak = 0; bool finite = true; double rms = 0; };

static Stats run (DistorsionExtremaAudioProcessor& p, int channels, double seconds, double fs = 48000.0, int block = 512)
{
    Stats st;
    AudioBuffer<float> buf (channels, block);
    MidiBuffer midi;
    const int total = (int) (seconds * fs);
    double ph1 = 0, ph2 = 0, sum = 0;
    int count = 0;
    for (int pos = 0; pos < total; pos += block)
    {
        for (int i = 0; i < block; ++i)
        {
            ph1 += 110.0 / fs; ph1 -= std::floor (ph1);
            ph2 += 164.8 / fs; ph2 -= std::floor (ph2);
            const float s = 0.3f * (float) (2.0 * ph1 - 1.0) + 0.2f * (float) std::sin (2.0 * MathConstants<double>::pi * ph2);
            for (int c = 0; c < channels; ++c) buf.setSample (c, i, c == 0 ? s : s * 0.8f);
        }
        p.processBlock (buf, midi);
        for (int c = 0; c < channels; ++c)
            for (int i = 0; i < block; ++i)
            {
                const float v = buf.getSample (c, i);
                if (! std::isfinite (v)) st.finite = false;
                st.peak = jmax (st.peak, std::abs (v));
                sum += (double) v * v; ++count;
            }
    }
    st.rms = std::sqrt (sum / jmax (1, count));
    return st;
}

int main (int argc, char** argv)
{
    ScopedJuceInitialiser_GUI gui;
    const String outDir = argc > 1 ? String (argv[1]) : String (".");

    DistorsionExtremaAudioProcessor p;
    p.setPlayConfigDetails (2, 2, 48000.0, 512);
    p.prepareToPlay (48000.0, 512);

    auto s = run (p, 2, 2.0);
    CHECK (s.finite && s.peak > 0.05f && s.peak < 4.0f, "por defecto: pico " + String (s.peak) + " rms " + String (s.rms));
    std::cout << "latencia (muestras): " << p.getLatencySamples() << std::endl;

    for (int t = 0; t < 3; ++t)
    {
        setP (p, "fin_type", (float) t);
        setP (p, "fin_drive", 100.0f);
        s = run (p, 2, 0.5);
        CHECK (s.finite && s.peak < 4.0f, "final tipo " + String (t) + " a tope: pico " + String (s.peak));
    }

    for (int i = 0; i < 4; ++i) { setP (p, dex::pdId (i, "on"), 1.0f); setP (p, dex::pdId (i, "drive"), 100.0f); }
    setP (p, "ws_amt", 100.0f); setP (p, "ws_shape", 60.0f);
    for (int q = 0; q < 3; ++q)
    {
        setP (p, "quality", (float) q);
        s = run (p, 2, 0.5);
        CHECK (s.finite && s.peak < 4.0f, "todo a tope, calidad " + String (q) + ": pico " + String (s.peak) + " latencia " + String (p.getLatencySamples()));
    }

    for (int e = 0; e < 3; ++e)
        for (int type = 0; type < 8; ++type)
        {
            setP (p, dex::eqId (e, "type"), (float) type);
            setP (p, dex::eqId (e, "gain"), 24.0f);
            setP (p, dex::eqId (e, "q"), 18.0f);
            setP (p, dex::eqId (e, "freq"), type == 7 ? 20.0f : 3000.0f);
            s = run (p, 2, 0.25);
            CHECK (s.finite, "EQ " + String (e + 1) + " tipo " + String (type) + " extremo: pico " + String (s.peak));
        }

    // LFO en todas las formas, a velocidad maxima
    setP (p, "lfo_on", 1.0f); setP (p, "lfo_dest", 2.0f); setP (p, "lfo_depth", 100.0f); setP (p, "lfo_rate", 20.0f);
    for (int sh = 0; sh < 5; ++sh)
    {
        setP (p, "lfo_shape", (float) sh);
        s = run (p, 2, 0.3);
        CHECK (s.finite, "LFO forma " + String (sh) + ": pico " + String (s.peak));
    }
    setP (p, "lfo_on", 0.0f);

    setP (p, "sec_crush", 80.0f); setP (p, "sec_ring", 70.0f);
    s = run (p, 2, 0.5);
    CHECK (s.finite, "efectos ocultos: pico " + String (s.peak));
    setP (p, "sec_crush", 0.0f); setP (p, "sec_ring", 0.0f);

    // Reinicio a valores por defecto
    for (auto* prm : p.getParameters())
        prm->setValueNotifyingHost (prm->getDefaultValue());

    // Captura de movimiento: grabamos el EQ 1 barriendo de 100 Hz a 5 kHz durante 1 s
    setP (p, "cap_arm", 1.0f);
    setP (p, "cap_slot", 2.0f);
    p.beginTouch (0, true);
    for (int k = 0; k < 20; ++k)
    {
        setP (p, dex::eqId (0, "freq"), 100.0f * std::pow (50.0f, k / 19.0f));
        setP (p, dex::eqId (0, "gain"), -12.0f + 24.0f * k / 19.0f);
        run (p, 2, 0.05);
    }
    p.endTouch();
    run (p, 2, 0.02);
    CHECK (p.isSlotFilled (2) && ! p.isSlotFilled (0), "memoria 3 grabada");
    setP (p, "cap_arm", 0.0f);
    setP (p, "cap_play", 3.0f);
    setP (p, dex::eqId (0, "freq"), 1000.0f);
    float minF = 1e9f, maxF = 0;
    for (int k = 0; k < 40; ++k)
    {
        run (p, 2, 0.05);
        minF = jmin (minF, p.eqView[0].freq.load());
        maxF = jmax (maxF, p.eqView[0].freq.load());
    }
    CHECK (p.playbackEq.load() == 0 && minF < 200.0f && maxF > 3000.0f,
           "reproduccion mueve el EQ 1 entre " + String (minF) + " y " + String (maxF) + " Hz");

    // Guardar / cargar estado
    MemoryBlock state;
    p.getStateInformation (state);
    DistorsionExtremaAudioProcessor p2;
    p2.setPlayConfigDetails (2, 2, 48000.0, 512);
    p2.prepareToPlay (48000.0, 512);
    p2.setStateInformation (state.getData(), (int) state.getSize());
    CHECK (p2.isSlotFilled (2) && (int) p2.apvts.getRawParameterValue ("cap_play")->load() == 3, "estado restaurado con la memoria");

    // Mono
    DistorsionExtremaAudioProcessor pm;
    AudioProcessor::BusesLayout mono;
    mono.inputBuses.add (AudioChannelSet::mono());
    mono.outputBuses.add (AudioChannelSet::mono());
    CHECK (pm.setBusesLayout (mono), "acepta mono");
    pm.prepareToPlay (44100.0, 256);
    s = run (pm, 1, 0.5, 44100.0, 256);
    CHECK (s.finite && s.peak > 0.01f, "mono: pico " + String (s.peak));

    // Bloques irregulares y grandes
    DistorsionExtremaAudioProcessor pb;
    pb.setPlayConfigDetails (2, 2, 96000.0, 4096);
    pb.prepareToPlay (96000.0, 4096);
    s = run (pb, 2, 0.5, 96000.0, 3001);
    CHECK (s.finite, "96 kHz, bloques de 3001: pico " + String (s.peak));

    // Interfaz -> PNG
    {
        DistorsionExtremaAudioProcessor pv;
        pv.setPlayConfigDetails (2, 2, 48000.0, 512);
        pv.prepareToPlay (48000.0, 512);
        setP (pv, dex::eqId (0, "gain"), 6.0f);
        setP (pv, dex::eqId (1, "gain"), 9.0f);
        setP (pv, dex::eqId (1, "type"), 0.0f);
        setP (pv, dex::eqId (2, "gain"), -5.0f);
        setP (pv, dex::eqId (2, "type"), 5.0f);
        setP (pv, "pd3_on", 1.0f);
        run (pv, 2, 1.0);
        std::unique_ptr<AudioProcessorEditor> ed (pv.createEditor());
        ed->setSize (1080, 720);
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
        File out = File (outDir).getChildFile ("interfaz.png");
        out.deleteFile();
        FileOutputStream os (out);
        PNGImageFormat().writeImageToStream (img, os);
        std::cout << "captura: " << out.getFullPathName() << std::endl;
        ed.reset();
    }

    std::cout << (failures == 0 ? "TODAS LAS PRUEBAS OK" : "HAY FALLOS: " + String (failures)) << std::endl;
    return failures == 0 ? 0 : 1;
}
