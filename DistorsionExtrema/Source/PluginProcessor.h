#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "Parameters.h"
#include "DSP.h"

class DistorsionExtremaAudioProcessor : public juce::AudioProcessor
{
public:
    DistorsionExtremaAudioProcessor();
    ~DistorsionExtremaAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.1; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // ---- Datos para la interfaz (se leen desde el hilo de mensajes)
    struct EqView
    {
        std::atomic<float> freq { 1000.0f }, gain { 0.0f }, q { 0.7f };
        std::atomic<int> type { 0 };
        std::atomic<bool> on { true };
    };
    EqView eqView[dex::numEqs];
    std::atomic<int> playbackEq { -1 };

    void beginTouch (int eqIndex, bool capture);
    void endTouch();
    bool isRecording() const { return recording.load(); }
    bool isSlotFilled (int slot) const { return slots[(size_t) slot].length.load() > 1; }
    void clearSlot (int slot);

    static constexpr int fifoSize = 4096;
    std::array<float, fifoSize> analyserFifo {};
    std::atomic<int> analyserWrite { 0 };

private:
    static constexpr int kChunk = 64;

    void processChunk (juce::AudioBuffer<float>& buffer, int start, int len, int numCh,
                       double bpm, double ppq, bool playing);
    void handleMotion (int len);
    void updateLatency();

    struct EqParams { std::atomic<float> *on, *type, *freq, *gain, *q, *mode; };
    EqParams pEq[dex::numEqs];
    std::atomic<float>* pPdOn[dex::numPreStages];
    std::atomic<float>* pPdDrive[dex::numPreStages];
    std::atomic<float> *pWsOn, *pWsAmt, *pWsShape, *pFinOn, *pFinType, *pFinDrive;
    std::atomic<float> *pIn, *pOut, *pMix, *pQuality;
    std::atomic<float> *pLfoOn, *pLfoTarget, *pLfoDest, *pLfoShape, *pLfoRate, *pLfoSync, *pLfoDiv, *pLfoDepth;
    std::atomic<float> *pCapArm, *pCapSlot, *pCapPlay, *pCapSpeed;
    std::atomic<float> *pSecCrush, *pSecRing, *pSecRingFreq;

    double fs = 44100.0;
    int numChPrepared = 2;

    dex::EqUnit eqs[dex::numEqs];
    dex::Lfo lfo;
    std::array<float, kChunk> lfoBuf {};

    std::unique_ptr<juce::dsp::Oversampling<float>> oversamplers[3];
    int osIndex = 1;

    juce::dsp::DryWetMixer<float> mixer { 2048 };
    juce::SmoothedValue<float> inGainS, outGainS;

    // Suavizado de la seccion no lineal (a frecuencia sobremuestreada)
    juce::SmoothedValue<float> pdDriveS[dex::numPreStages], pdOnS[dex::numPreStages];
    juce::SmoothedValue<float> wsAmtS, wsShapeS, wsOnS, finDriveS, finOnS;

    // DC blocker
    float dcX[2] {}, dcY[2] {};

    // Secretos
    float crushHold[2] {}, crushCounter = 0.0f;
    double ringPhase = 0.0;
    juce::SmoothedValue<float> crushS, ringS;

    // Captura de movimiento
    std::array<dex::MotionSlot, dex::numSlots> slots;
    juce::SpinLock motionLock;
    std::atomic<int> touchedEq { -1 };
    std::atomic<bool> captureRequested { false };
    std::atomic<bool> recording { false };
    int recSlot = 0;
    double recAccum = 0.0, playPos = 0.0;
    int lastPlay = -1;
    int overrideEq = -1;
    float overrideFreq = 1000.0f, overrideGain = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DistorsionExtremaAudioProcessor)
};
