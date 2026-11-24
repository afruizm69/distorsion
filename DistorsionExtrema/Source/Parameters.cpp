#include "Parameters.h"

namespace dex
{
using namespace juce;

StringArray eqTypeNames()
{
    return { "Campana", "Muesca", "Pasa-bajos", "Pasa-altos", "Banda",
             U ("Inclinación"), "Formante", "Peine" };
}

StringArray eqModeNames() { return { U ("Estéreo"), "Mid", "Side" }; }

StringArray preStageNames() { return { "SUAVE", "DURO", "PLIEGUE", "DIODO" }; }

static String fmtHz (float v, int)
{
    if (v >= 1000.0f)
        return String (v / 1000.0f, v >= 10000.0f ? 1 : 2) + " kHz";
    return String (v, v < 100.0f ? 1 : 0) + " Hz";
}
static String fmtDb (float v, int) { return (v > 0.05f ? "+" : "") + String (v, 1) + " dB"; }
static String fmtPct (float v, int) { return String (roundToInt (v)) + " %"; }
static String fmtQ (float v, int) { return String (v, 2); }
static String fmtX (float v, int) { return String (v, 2) + "x"; }

AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    AudioProcessorValueTreeState::ParameterLayout layout;

    auto addF = [&] (const String& id, const String& name, NormalisableRange<float> range, float def,
                     std::function<String (float, int)> fmt)
    {
        layout.add (std::make_unique<AudioParameterFloat> (
            ParameterID { id, 1 }, name, range, def,
            AudioParameterFloatAttributes().withStringFromValueFunction (std::move (fmt))));
    };
    auto addB = [&] (const String& id, const String& name, bool def)
    {
        layout.add (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, name, def));
    };
    auto addC = [&] (const String& id, const String& name, const StringArray& choices, int def)
    {
        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, name, choices, def));
    };

    auto pct = NormalisableRange<float> (0.0f, 100.0f, 0.1f);

    NormalisableRange<float> freqRange (20.0f, 20000.0f);
    freqRange.setSkewForCentre (632.0f);
    NormalisableRange<float> qRange (0.1f, 18.0f);
    qRange.setSkewForCentre (1.0f);
    NormalisableRange<float> gainRange (-24.0f, 24.0f, 0.01f);
    NormalisableRange<float> ioRange (-24.0f, 24.0f, 0.01f);

    // ---- Global
    addF ("in", "Entrada", ioRange, 0.0f, fmtDb);
    addF ("out", "Salida", ioRange, -6.0f, fmtDb);
    addF ("mix", "Mezcla", pct, 100.0f, fmtPct);
    addC ("quality", "Calidad", { "Normal (2x)", "Alta (4x)", "Extrema (8x)" }, 1);

    // ---- Tres ecualizadores
    const float defFreq[] = { 150.0f, 1200.0f, 5000.0f };
    const int defType[] = { 0, 4, 0 };
    const float defGain[] = { 0.0f, 0.0f, 0.0f };
    const float defQ[] = { 0.8f, 1.0f, 0.7f };
    const int defMode[] = { 1, 0, 2 };
    for (int i = 0; i < numEqs; ++i)
    {
        const String n = "EQ " + String (i + 1) + " ";
        addB (eqId (i, "on"), n + "Activo", true);
        addC (eqId (i, "type"), n + "Tipo", eqTypeNames(), defType[i]);
        addF (eqId (i, "freq"), n + "Frecuencia", freqRange, defFreq[i], fmtHz);
        addF (eqId (i, "gain"), n + "Ganancia", gainRange, defGain[i], fmtDb);
        addF (eqId (i, "q"), n + "Q", qRange, defQ[i], fmtQ);
        addC (eqId (i, "mode"), n + "Modo", eqModeNames(), defMode[i]);
    }

    // ---- Pre-distorsion (4 etapas)
    const auto stageNames = preStageNames();
    for (int i = 0; i < numPreStages; ++i)
    {
        addB (pdId (i, "on"), "Pre " + stageNames[i], i == 0);
        addF (pdId (i, "drive"), "Pre " + stageNames[i] + " Drive", pct, i == 0 ? 35.0f : 25.0f, fmtPct);
    }

    // ---- Waveshaper
    addB ("ws_on", "Waveshaper", true);
    addF ("ws_amt", "Waveshaper Cantidad", pct, 25.0f, fmtPct);
    addF ("ws_shape", "Waveshaper Forma", pct, 0.0f, fmtPct);

    // ---- Distorsion final
    addB ("fin_on", "Final", true);
    addC ("fin_type", "Final Tipo", { U ("Válvula"), "Fuzz", "Rectificador" }, 0);
    addF ("fin_drive", "Final Drive", pct, 30.0f, fmtPct);

    // ---- LFO "Muevelo"
    NormalisableRange<float> rateRange (0.02f, 20.0f);
    rateRange.setSkewForCentre (1.0f);
    addB ("lfo_on", "LFO Activo", false);
    addC ("lfo_target", "LFO Destino EQ", { "EQ 1", "EQ 2", "EQ 3" }, 0);
    addC ("lfo_dest", "LFO Modula", { "Frecuencia", "Ganancia", "Ambas" }, 0);
    addC ("lfo_shape", "LFO Forma", { "Seno", U ("Triángulo"), "Sierra", "Cuadrada", "Aleatoria" }, 0);
    addF ("lfo_rate", "LFO Velocidad", rateRange, 1.0f, [] (float v, int) { return String (v, v < 1.0f ? 2 : 1) + " Hz"; });
    addB ("lfo_sync", "LFO Sincro", false);
    addC ("lfo_div", "LFO Division", { "4 compases", "2 compases", U ("1 compás"), "1/2", "1/4", "1/8", "1/16" }, 4);
    addF ("lfo_depth", "LFO Profundidad", pct, 50.0f, fmtPct);

    // ---- Captura de movimiento
    NormalisableRange<float> speedRange (0.25f, 4.0f);
    speedRange.setSkewForCentre (1.0f);
    addB ("cap_arm", "Captura Grabar", false);
    addC ("cap_slot", "Captura Memoria", { "1", "2", "3", "4" }, 0);
    addC ("cap_play", "Captura Reproducir", { "Apagado", "Memoria 1", "Memoria 2", "Memoria 3", "Memoria 4" }, 0);
    addF ("cap_speed", "Captura Velocidad", speedRange, 1.0f, fmtX);

    // ---- Efectos ocultos
    NormalisableRange<float> ringRange (20.0f, 2000.0f);
    ringRange.setSkewForCentre (200.0f);
    addF ("sec_crush", "Secreto Destrozo", pct, 0.0f, fmtPct);
    addF ("sec_ring", "Secreto Anillo", pct, 0.0f, fmtPct);
    addF ("sec_ringfreq", "Secreto Frec Anillo", ringRange, 180.0f, fmtHz);

    return layout;
}
} // namespace dex
