#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>
#include <complex>
#include <vector>
#include <array>
#include <algorithm>

namespace dex
{
constexpr float kPi = 3.14159265358979f;

enum EqType { Bell = 0, Notch, LowPass, HighPass, BandPass, Tilt, Formant, Comb };
enum EqMode { Stereo = 0, Mid, Side };

struct EqSettings
{
    int type = Bell;
    float freq = 1000.0f, gainDb = 0.0f, q = 0.7f;
};

// ---- Reglas compartidas por el audio y por la grafica -----------------------
// En pasa-bajos/pasa-altos/muesca, mover el punto hacia arriba aumenta la resonancia.
inline float resonanceQ (const EqSettings& s) { return s.q * std::pow (10.0f, s.gainDb / 40.0f); }
inline float combFeedback (const EqSettings& s) { return juce::jlimit (-0.95f, 0.95f, s.gainDb / 24.0f * 0.95f); }
inline float formantSecondFreq (float f, double fs) { return std::min (f * 2.2f, (float) (fs * 0.45)); }

// ---- Filtro SVF (topologia de Simper): se puede modular muestra a muestra sin clicks
struct SvfCoeffs { float a1 = 0, a2 = 0, a3 = 0, m0 = 1, m1 = 0, m2 = 0; };

enum SvfMode { SvfBell = 0, SvfNotch, SvfLP, SvfHP, SvfBP, SvfHighShelf };

inline SvfCoeffs makeSvf (int mode, float f, float q, float gainDb, double fs)
{
    f = juce::jlimit (10.0f, (float) (fs * 0.49), f);
    q = std::max (0.05f, q);
    const float A = std::pow (10.0f, gainDb / 40.0f);
    float g = std::tan (kPi * f / (float) fs);
    float k = 1.0f / q;
    SvfCoeffs c;
    switch (mode)
    {
        case SvfBell:      k = 1.0f / (q * A); c.m0 = 1; c.m1 = k * (A * A - 1.0f); c.m2 = 0; break;
        case SvfNotch:     c.m0 = 1; c.m1 = -k; c.m2 = 0; break;
        case SvfLP:        c.m0 = 0; c.m1 = 0; c.m2 = 1; break;
        case SvfHP:        c.m0 = 1; c.m1 = -k; c.m2 = -1; break;
        case SvfBP:        c.m0 = 0; c.m1 = k; c.m2 = 0; break;
        case SvfHighShelf: g *= std::sqrt (A); c.m0 = A * A; c.m1 = k * (1.0f - A) * A; c.m2 = 1.0f - A * A; break;
        default: break;
    }
    c.a1 = 1.0f / (1.0f + g * (g + k));
    c.a2 = g * c.a1;
    c.a3 = g * c.a2;
    return c;
}

struct Svf
{
    float ic1 = 0, ic2 = 0;
    void reset() { ic1 = ic2 = 0; }
    inline float tick (float v0, const SvfCoeffs& c)
    {
        const float v3 = v0 - ic2;
        const float v1 = c.a1 * ic1 + c.a2 * v3;
        const float v2 = ic2 + c.a2 * ic1 + c.a3 * v3;
        ic1 = 2.0f * v1 - ic1;
        ic2 = 2.0f * v2 - ic2;
        return c.m0 * v0 + c.m1 * v1 + c.m2 * v2;
    }
};

// ---- Respuesta en frecuencia (para dibujar la curva) ------------------------
inline std::complex<double> analogResponse (int mode, double w, double q, double gainDb)
{
    using C = std::complex<double>;
    const C s (0.0, w);
    const double A = std::pow (10.0, gainDb / 40.0);
    const C den = s * s + s / q + 1.0;
    switch (mode)
    {
        case SvfBell:  return (s * s + s * (A / q) + 1.0) / (s * s + s / (A * q) + 1.0);
        case SvfNotch: return (s * s + 1.0) / den;
        case SvfLP:    return 1.0 / den;
        case SvfHP:    return s * s / den;
        case SvfBP:    return (s / q) / den;
        case SvfHighShelf:
        {
            const double sa = std::sqrt (A);
            return A * (A * s * s + (sa / q) * s + 1.0) / (s * s + (sa / q) * s + A);
        }
        default: return 1.0;
    }
}

inline float eqMagnitudeDb (const EqSettings& s, float f, double fs)
{
    const double nyq = fs * 0.4999;
    auto warp = [&] (double f0)
    {
        f0 = juce::jlimit (10.0, fs * 0.49, f0);
        return std::tan (juce::MathConstants<double>::pi * std::min ((double) f, nyq) / fs)
             / std::tan (juce::MathConstants<double>::pi * f0 / fs);
    };
    double mag = 1.0;
    switch (s.type)
    {
        case Bell:     mag = std::abs (analogResponse (SvfBell, warp (s.freq), s.q, s.gainDb)); break;
        case Notch:    mag = std::abs (analogResponse (SvfNotch, warp (s.freq), resonanceQ (s), 0)); break;
        case LowPass:  mag = std::abs (analogResponse (SvfLP, warp (s.freq), resonanceQ (s), 0)); break;
        case HighPass: mag = std::abs (analogResponse (SvfHP, warp (s.freq), resonanceQ (s), 0)); break;
        case BandPass: mag = std::abs (analogResponse (SvfBP, warp (s.freq), s.q, 0)) * std::pow (10.0, s.gainDb / 20.0); break;
        case Tilt:     mag = std::abs (analogResponse (SvfHighShelf, warp (s.freq), s.q, s.gainDb)) * std::pow (10.0, -s.gainDb / 40.0); break;
        case Formant:
            mag = std::abs (analogResponse (SvfBell, warp (s.freq), s.q, s.gainDb))
                * std::abs (analogResponse (SvfBell, warp (formantSecondFreq (s.freq, fs)), s.q, s.gainDb * 0.8));
            break;
        case Comb:
        {
            const double fb = combFeedback (s);
            const double D = fs / std::max (20.0f, s.freq);
            const double om = 2.0 * juce::MathConstants<double>::pi * f / fs;
            mag = (1.0 - std::abs (fb)) / std::abs (1.0 - fb * std::exp (std::complex<double> (0.0, -om * D)));
            break;
        }
        default: break;
    }
    return (float) (20.0 * std::log10 (std::max (mag, 1.0e-6)));
}

// ---- Ecualizador con parametros suavizados y modulables ---------------------
class EqUnit
{
public:
    static constexpr int kCombSize = 16384;

    void prepare (double sampleRate)
    {
        fs = sampleRate;
        freqS.reset (fs, 0.03);
        gainS.reset (fs, 0.03);
        qS.reset (fs, 0.03);
        onS.reset (fs, 0.02);
        for (auto& b : combBuf) b.assign (kCombSize, 0.0f);
        reset();
    }

    void reset()
    {
        for (auto& s : svfA) s.reset();
        for (auto& s : svfB) s.reset();
        for (auto& b : combBuf) std::fill (b.begin(), b.end(), 0.0f);
        combW = { 0, 0 };
    }

    void setTarget (int type, float f, float gainDb, float q, bool on)
    {
        if (type != curType)
        {
            if (type == Comb || curType == Comb) reset();
            curType = type;
        }
        if (firstTarget)
        {
            freqS.setCurrentAndTargetValue (f);
            gainS.setCurrentAndTargetValue (gainDb);
            qS.setCurrentAndTargetValue (q);
            onS.setCurrentAndTargetValue (on ? 1.0f : 0.0f);
            firstTarget = false;
        }
        freqS.setTargetValue (f);
        gainS.setTargetValue (gainDb);
        qS.setTargetValue (q);
        onS.setTargetValue (on ? 1.0f : 0.0f);
    }

    // Avanza el suavizado una muestra y recalcula coeficientes (con modulacion del LFO).
    void beginSample (float modOctaves, float modDb)
    {
        float f = freqS.getNextValue();
        if (modOctaves != 0.0f) f *= std::exp2 (modOctaves);
        cur.type = curType;
        cur.freq = juce::jlimit (20.0f, 20000.0f, f);
        cur.gainDb = juce::jlimit (-30.0f, 30.0f, gainS.getNextValue() + modDb);
        cur.q = qS.getNextValue();
        onAmt = onS.getNextValue();
        outGain = 1.0f;

        switch (curType)
        {
            case Bell:     cA = makeSvf (SvfBell, cur.freq, cur.q, cur.gainDb, fs); break;
            case Notch:    cA = makeSvf (SvfNotch, cur.freq, resonanceQ (cur), 0, fs); break;
            case LowPass:  cA = makeSvf (SvfLP, cur.freq, resonanceQ (cur), 0, fs); break;
            case HighPass: cA = makeSvf (SvfHP, cur.freq, resonanceQ (cur), 0, fs); break;
            case BandPass: cA = makeSvf (SvfBP, cur.freq, cur.q, 0, fs); outGain = juce::Decibels::decibelsToGain (cur.gainDb); break;
            case Tilt:     cA = makeSvf (SvfHighShelf, cur.freq, cur.q, cur.gainDb, fs); outGain = std::pow (10.0f, -cur.gainDb / 40.0f); break;
            case Formant:
                cA = makeSvf (SvfBell, cur.freq, cur.q, cur.gainDb, fs);
                cB = makeSvf (SvfBell, formantSecondFreq (cur.freq, fs), cur.q, cur.gainDb * 0.8f, fs);
                break;
            case Comb:
                fb = combFeedback (cur);
                delay = juce::jlimit (1.0f, (float) kCombSize - 2.0f, (float) (fs / cur.freq));
                break;
            default: break;
        }
    }

    float processChannel (int ch, float x)
    {
        if (onAmt <= 0.0f) return x;
        float y;
        switch (curType)
        {
            case Formant: y = svfB[ch].tick (svfA[ch].tick (x, cA), cB); break;
            case Comb:
            {
                auto& buf = combBuf[(size_t) ch];
                int& w = combW[(size_t) ch];
                float rp = (float) w - delay;
                if (rp < 0.0f) rp += (float) kCombSize;
                const int i0 = (int) rp;
                const float fr = rp - (float) i0;
                const int i1 = (i0 + 1) % kCombSize;
                const float del = buf[(size_t) i0] + (buf[(size_t) i1] - buf[(size_t) i0]) * fr;
                const float yy = x + fb * del;
                buf[(size_t) w] = yy;
                w = (w + 1) % kCombSize;
                y = yy * (1.0f - std::abs (fb));
                break;
            }
            default: y = svfA[ch].tick (x, cA) * outGain; break;
        }
        return x + (y - x) * onAmt;
    }

    const EqSettings& current() const { return cur; }

private:
    double fs = 44100.0;
    int curType = Bell;
    bool firstTarget = true;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> freqS { 1000.0f }, qS { 0.7f };
    juce::SmoothedValue<float> gainS, onS;
    EqSettings cur;
    SvfCoeffs cA, cB;
    float outGain = 1.0f, onAmt = 1.0f, fb = 0.0f, delay = 100.0f;
    Svf svfA[2], svfB[2];
    std::array<std::vector<float>, 2> combBuf;
    std::array<int, 2> combW { 0, 0 };
};

inline void eqStereo (EqUnit& eq, float& l, float& r, int mode, bool stereo)
{
    if (! stereo)
    {
        if (mode != Side) l = eq.processChannel (0, l);
        r = l;
        return;
    }
    switch (mode)
    {
        case Mid:
        {
            const float m = (l + r) * 0.5f, s = (l - r) * 0.5f;
            const float m2 = eq.processChannel (0, m);
            l = m2 + s; r = m2 - s;
            break;
        }
        case Side:
        {
            const float m = (l + r) * 0.5f, s = (l - r) * 0.5f;
            const float s2 = eq.processChannel (0, s);
            l = m + s2; r = m - s2;
            break;
        }
        default:
            l = eq.processChannel (0, l);
            r = eq.processChannel (1, r);
            break;
    }
}

// ---- LFO -------------------------------------------------------------------
class Lfo
{
public:
    void prepare (double fs) { smoothCoef = std::exp (-1.0f / (0.003f * (float) fs)); }

    // inc = ciclos por muestra
    float tick (double inc, int shape)
    {
        phase += inc;
        if (phase >= 1.0) { phase -= std::floor (phase); newRandom(); }
        return smooth (value (shape));
    }

    void syncTo (double newPhase, int shape)
    {
        newPhase -= std::floor (newPhase);
        if (newPhase < phase - 0.5) newRandom();
        phase = newPhase;
        juce::ignoreUnused (shape);
    }

    float value (int shape) const
    {
        const float p = (float) phase;
        switch (shape)
        {
            case 0: return std::sin (2.0f * kPi * p);
            case 1: return 1.0f - 4.0f * std::abs (p - 0.5f);
            case 2: return 2.0f * p - 1.0f;
            case 3: return p < 0.5f ? 1.0f : -1.0f;
            default:
            {
                const float t = p * p * (3.0f - 2.0f * p);
                return rndPrev + (rndNext - rndPrev) * t;
            }
        }
    }

private:
    float smooth (float v) { state = v + (state - v) * smoothCoef; return state; }
    void newRandom() { rndPrev = rndNext; rndNext = rng.nextFloat() * 2.0f - 1.0f; }

    double phase = 0.0;
    float state = 0.0f, smoothCoef = 0.99f, rndPrev = 0.0f, rndNext = 0.0f;
    juce::Random rng;
};

// ---- Distorsiones ----------------------------------------------------------
namespace dist
{
inline float soft (float x, float d)  { return std::tanh ((1.0f + d * 29.0f) * x); }
inline float hard (float x, float d)  { return juce::jlimit (-1.0f, 1.0f, (1.0f + d * 29.0f) * x); }
inline float fold (float x, float d)  { return std::sin ((1.0f + d * 11.0f) * x * 1.5707963f); }
inline float diode (float x, float d)
{
    x *= 1.0f + d * 19.0f;
    return x >= 0.0f ? 1.0f - std::exp (-x) : -0.6f * (1.0f - std::exp (1.6f * x));
}

inline float preStage (int i, float x, float d)
{
    switch (i)
    {
        case 0: return soft (x, d);
        case 1: return hard (x, d);
        case 2: return fold (x, d);
        default: return diode (x, d);
    }
}

// amt: 0..1 dureza; shape: 0 = saturacion racional agresiva, 1 = plegado senoidal
inline float waveshaper (float x, float amt, float shape)
{
    x = juce::jlimit (-4.0f, 4.0f, x);
    const float k = amt * 60.0f;
    const float a = (1.0f + k) * x / (1.0f + k * std::abs (x));
    const float b = std::sin (x * (1.0f + amt * 9.0f));
    return a + (b - a) * shape;
}

inline float finalStage (int type, float x, float d)
{
    switch (type)
    {
        case 0: // Valvula: saturacion asimetrica
        {
            const float k = 1.0f + d * 24.0f;
            return (std::tanh (k * x + 0.3f) - std::tanh (0.3f)) * 0.9f;
        }
        case 1: // Fuzz: muy agresivo
        {
            const float z = (1.0f + d * 80.0f) * x;
            const float y = z >= 0.0f ? 1.0f - std::exp (-z) : -1.0f + std::exp (z);
            return y * 0.8f;
        }
        default: // Rectificador: anade octava
        {
            const float y = std::tanh ((1.0f + d * 24.0f) * x);
            return y * (1.0f - d) + std::abs (y) * d;
        }
    }
}
} // namespace dist

// ---- Memoria de movimiento -------------------------------------------------
struct MotionSlot
{
    static constexpr int kRate = 100;          // puntos por segundo
    static constexpr int kMaxPoints = 100 * 60; // 60 s
    int eq = -1;
    std::atomic<int> length { 0 };
    std::vector<float> freq = std::vector<float> (kMaxPoints, 1000.0f);
    std::vector<float> gain = std::vector<float> (kMaxPoints, 0.0f);
};

} // namespace dex
