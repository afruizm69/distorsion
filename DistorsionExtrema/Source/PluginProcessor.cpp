#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

DistorsionExtremaAudioProcessor::DistorsionExtremaAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", AudioChannelSet::stereo(), true)
                          .withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", dex::createParameterLayout())
{
    auto p = [this] (const String& id)
    {
        auto* v = apvts.getRawParameterValue (id);
        jassert (v != nullptr);
        return v;
    };

    for (int i = 0; i < dex::numEqs; ++i)
        pEq[i] = { p (dex::eqId (i, "on")), p (dex::eqId (i, "type")), p (dex::eqId (i, "freq")),
                   p (dex::eqId (i, "gain")), p (dex::eqId (i, "q")), p (dex::eqId (i, "mode")) };

    for (int i = 0; i < dex::numPreStages; ++i)
    {
        pPdOn[i] = p (dex::pdId (i, "on"));
        pPdDrive[i] = p (dex::pdId (i, "drive"));
    }
    pWsOn = p ("ws_on"); pWsAmt = p ("ws_amt"); pWsShape = p ("ws_shape");
    pFinOn = p ("fin_on"); pFinType = p ("fin_type"); pFinDrive = p ("fin_drive");
    pIn = p ("in"); pOut = p ("out"); pMix = p ("mix"); pQuality = p ("quality");
    pLfoOn = p ("lfo_on"); pLfoTarget = p ("lfo_target"); pLfoDest = p ("lfo_dest");
    pLfoShape = p ("lfo_shape"); pLfoRate = p ("lfo_rate"); pLfoSync = p ("lfo_sync");
    pLfoDiv = p ("lfo_div"); pLfoDepth = p ("lfo_depth");
    pCapArm = p ("cap_arm"); pCapSlot = p ("cap_slot"); pCapPlay = p ("cap_play"); pCapSpeed = p ("cap_speed");
    pSecCrush = p ("sec_crush"); pSecRing = p ("sec_ring"); pSecRingFreq = p ("sec_ringfreq");
}

bool DistorsionExtremaAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != AudioChannelSet::mono() && out != AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

void DistorsionExtremaAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    fs = sampleRate;
    numChPrepared = jlimit (1, 2, getTotalNumOutputChannels());

    for (int k = 0; k < 3; ++k)
    {
        oversamplers[k] = std::make_unique<dsp::Oversampling<float>> (
            (size_t) numChPrepared, (size_t) (k + 1),
            dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true);
        oversamplers[k]->initProcessing ((size_t) kChunk);
    }
    osIndex = jlimit (0, 2, (int) pQuality->load());

    juce::ignoreUnused (samplesPerBlock);
    mixer.prepare ({ sampleRate, (uint32) kChunk, (uint32) numChPrepared });
    mixer.reset();

    for (auto& e : eqs) e.prepare (sampleRate);
    lfo.prepare (sampleRate);

    inGainS.reset (sampleRate, 0.03);
    outGainS.reset (sampleRate, 0.03);
    inGainS.setCurrentAndTargetValue (Decibels::decibelsToGain (pIn->load()));
    outGainS.setCurrentAndTargetValue (Decibels::decibelsToGain (pOut->load()));
    crushS.reset (sampleRate, 0.05);
    ringS.reset (sampleRate, 0.05);

    const double osRate = sampleRate * (double) (1 << (osIndex + 1));
    for (int i = 0; i < dex::numPreStages; ++i)
    {
        pdDriveS[i].reset (osRate, 0.03); pdDriveS[i].setCurrentAndTargetValue (pPdDrive[i]->load() * 0.01f);
        pdOnS[i].reset (osRate, 0.02);    pdOnS[i].setCurrentAndTargetValue (pPdOn[i]->load());
    }
    for (auto* s : { &wsAmtS, &wsShapeS, &wsOnS, &finDriveS, &finOnS })
        s->reset (osRate, 0.03);
    wsAmtS.setCurrentAndTargetValue (pWsAmt->load() * 0.01f);
    wsShapeS.setCurrentAndTargetValue (pWsShape->load() * 0.01f);
    wsOnS.setCurrentAndTargetValue (pWsOn->load());
    finDriveS.setCurrentAndTargetValue (pFinDrive->load() * 0.01f);
    finOnS.setCurrentAndTargetValue (pFinOn->load());

    for (int c = 0; c < 2; ++c) { dcX[c] = dcY[c] = 0.0f; crushHold[c] = 0.0f; }
    updateLatency();
}

void DistorsionExtremaAudioProcessor::updateLatency()
{
    const auto lat = oversamplers[osIndex] != nullptr ? oversamplers[osIndex]->getLatencyInSamples() : 0.0f;
    setLatencySamples ((int) std::round (lat));
    mixer.setWetLatency (lat);
}

void DistorsionExtremaAudioProcessor::beginTouch (int eqIndex, bool capture)
{
    captureRequested.store (capture);
    touchedEq.store (eqIndex);
}

void DistorsionExtremaAudioProcessor::endTouch()
{
    captureRequested.store (false);
    touchedEq.store (-1);
}

void DistorsionExtremaAudioProcessor::clearSlot (int slot)
{
    const SpinLock::ScopedLockType sl (motionLock);
    slots[(size_t) slot].length.store (0);
    slots[(size_t) slot].eq = -1;
}

// Graba / reproduce el movimiento de los puntos de EQ (a 100 puntos por segundo)
void DistorsionExtremaAudioProcessor::handleMotion (int len)
{
    overrideEq = -1;
    const SpinLock::ScopedTryLockType tl (motionLock);
    if (! tl.isLocked())
        return;

    const int touched = touchedEq.load();
    const bool wantRec = captureRequested.load() && touched >= 0;
    const double step = fs / dex::MotionSlot::kRate;

    if (wantRec && ! recording.load())
    {
        recSlot = jlimit (0, dex::numSlots - 1, (int) pCapSlot->load());
        auto& s = slots[(size_t) recSlot];
        s.eq = touched;
        s.freq[0] = pEq[touched].freq->load();
        s.gain[0] = pEq[touched].gain->load();
        s.length.store (1);
        recAccum = 0.0;
        recording.store (true);
    }
    else if (! wantRec && recording.load())
    {
        recording.store (false);
    }

    if (recording.load())
    {
        auto& s = slots[(size_t) recSlot];
        recAccum += len;
        while (recAccum >= step)
        {
            recAccum -= step;
            const int L = s.length.load();
            if (L < dex::MotionSlot::kMaxPoints)
            {
                s.freq[(size_t) L] = pEq[s.eq].freq->load();
                s.gain[(size_t) L] = pEq[s.eq].gain->load();
                s.length.store (L + 1);
            }
        }
    }

    const int play = (int) pCapPlay->load() - 1;
    if (play != lastPlay) { playPos = 0.0; lastPlay = play; }

    int pbEq = -1;
    if (play >= 0 && ! (recording.load() && recSlot == play))
    {
        auto& s = slots[(size_t) play];
        const int L = s.length.load();
        if (L > 1 && s.eq >= 0 && s.eq != touched)
        {
            playPos = std::fmod (playPos, (double) L);
            const int i0 = (int) playPos;
            const int i1 = (i0 + 1) % L;
            const float fr = (float) (playPos - i0);
            const float f0 = s.freq[(size_t) i0], f1 = s.freq[(size_t) i1];
            overrideFreq = f0 * std::pow (f1 / f0, fr);
            overrideGain = s.gain[(size_t) i0] + (s.gain[(size_t) i1] - s.gain[(size_t) i0]) * fr;
            overrideEq = s.eq;
            pbEq = s.eq;
            playPos += pCapSpeed->load() * dex::MotionSlot::kRate * len / fs;
        }
    }
    playbackEq.store (pbEq);
}

void DistorsionExtremaAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer&)
{
    ScopedNoDenormals noDenormals;
    const int numIn = getTotalNumInputChannels();
    const int numOut = getTotalNumOutputChannels();
    const int n = buffer.getNumSamples();
    for (int ch = numIn; ch < numOut; ++ch)
        buffer.clear (ch, 0, n);

    const int numCh = jmin (numChPrepared, numOut, 2);
    if (numCh <= 0 || n <= 0 || oversamplers[0] == nullptr)
        return;

    const int q = jlimit (0, 2, (int) pQuality->load());
    if (q != osIndex)
    {
        osIndex = q;
        oversamplers[osIndex]->reset();
        const double osRate = fs * (double) (1 << (osIndex + 1));
        for (int i = 0; i < dex::numPreStages; ++i) { pdDriveS[i].reset (osRate, 0.03); pdOnS[i].reset (osRate, 0.02); }
        for (auto* s : { &wsAmtS, &wsShapeS, &wsOnS, &finDriveS, &finOnS }) s->reset (osRate, 0.03);
        updateLatency();
    }

    double bpm = 120.0, ppq = 0.0;
    bool playing = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = *b;
            if (auto pp = pos->getPpqPosition()) ppq = *pp;
            playing = pos->getIsPlaying();
        }

    dsp::AudioBlock<float> block (buffer);
    auto io = block.getSubsetChannelBlock (0, (size_t) numCh);
    mixer.setWetMixProportion (pMix->load() * 0.01f);

    for (int start = 0; start < n; start += kChunk)
    {
        const int len = jmin (kChunk, n - start);
        auto sub = io.getSubBlock ((size_t) start, (size_t) len);
        mixer.pushDrySamples (sub);
        processChunk (buffer, start, len, numCh, bpm, ppq + (start / fs) * (bpm / 60.0), playing);
        mixer.mixWetSamples (sub);
    }

    outGainS.setTargetValue (Decibels::decibelsToGain (pOut->load()));
    auto* L = buffer.getWritePointer (0);
    auto* R = numCh > 1 ? buffer.getWritePointer (1) : nullptr;
    int w = analyserWrite.load();
    for (int i = 0; i < n; ++i)
    {
        const float g = outGainS.getNextValue();
        L[i] *= g;
        if (R != nullptr) R[i] *= g;
        analyserFifo[(size_t) w] = R != nullptr ? 0.5f * (L[i] + R[i]) : L[i];
        w = (w + 1) % fifoSize;
    }
    analyserWrite.store (w);

    for (int e = 0; e < dex::numEqs; ++e)
    {
        const auto& c = eqs[e].current();
        eqView[e].freq.store (c.freq);
        eqView[e].gain.store (c.gainDb);
        eqView[e].q.store (c.q);
        eqView[e].type.store (c.type);
        eqView[e].on.store (pEq[e].on->load() > 0.5f);
    }
}

void DistorsionExtremaAudioProcessor::processChunk (AudioBuffer<float>& buffer, int start, int len, int numCh,
                                                    double bpm, double ppq, bool playing)
{
    float* L = buffer.getWritePointer (0, start);
    float* R = numCh > 1 ? buffer.getWritePointer (1, start) : nullptr;
    const bool stereo = R != nullptr;

    // ---- LFO "Muevelo"
    const bool lfoOn = pLfoOn->load() > 0.5f;
    const int lfoTarget = (int) pLfoTarget->load();
    const int lfoDest = (int) pLfoDest->load();
    const int lfoShape = (int) pLfoShape->load();
    const float depth = lfoOn ? pLfoDepth->load() * 0.01f : 0.0f;
    double inc = pLfoRate->load() / fs;
    if (pLfoSync->load() > 0.5f)
    {
        const double beats = dex::lfoDivBeats[jlimit (0, 6, (int) pLfoDiv->load())];
        inc = (bpm / 60.0) / beats / fs;
        if (playing) lfo.syncTo (ppq / beats, lfoShape);
    }
    for (int i = 0; i < len; ++i)
        lfoBuf[(size_t) i] = lfo.tick (inc, lfoShape) * depth;

    // ---- Captura / reproduccion de movimiento
    handleMotion (len);

    // ---- Objetivos de los EQs
    int modes[dex::numEqs];
    for (int e = 0; e < dex::numEqs; ++e)
    {
        float f = pEq[e].freq->load(), g = pEq[e].gain->load();
        if (overrideEq == e) { f = overrideFreq; g = overrideGain; }
        eqs[e].setTarget ((int) pEq[e].type->load(), f, g, pEq[e].q->load(), pEq[e].on->load() > 0.5f);
        modes[e] = (int) pEq[e].mode->load();
    }
    auto modOct = [&] (int e, int i) { return (lfoTarget == e && lfoDest != 1) ? lfoBuf[(size_t) i] * 3.0f : 0.0f; };
    auto modDb  = [&] (int e, int i) { return (lfoTarget == e && lfoDest != 0) ? lfoBuf[(size_t) i] * 18.0f : 0.0f; };

    // ---- Entrada + EQ 1 (pre M/S) + EQ 2 (color)
    inGainS.setTargetValue (Decibels::decibelsToGain (pIn->load()));
    for (int i = 0; i < len; ++i)
    {
        const float ig = inGainS.getNextValue();
        float l = L[i] * ig;
        float r = stereo ? R[i] * ig : l;
        for (int e = 0; e < 2; ++e)
        {
            eqs[e].beginSample (modOct (e, i), modDb (e, i));
            dex::eqStereo (eqs[e], l, r, modes[e], stereo);
        }
        L[i] = l;
        if (stereo) R[i] = r;
    }

    // ---- Seccion no lineal sobremuestreada
    for (int s = 0; s < dex::numPreStages; ++s)
    {
        pdDriveS[s].setTargetValue (pPdDrive[s]->load() * 0.01f);
        pdOnS[s].setTargetValue (pPdOn[s]->load() > 0.5f ? 1.0f : 0.0f);
    }
    wsAmtS.setTargetValue (pWsAmt->load() * 0.01f);
    wsShapeS.setTargetValue (pWsShape->load() * 0.01f);
    wsOnS.setTargetValue (pWsOn->load() > 0.5f ? 1.0f : 0.0f);
    finDriveS.setTargetValue (pFinDrive->load() * 0.01f);
    finOnS.setTargetValue (pFinOn->load() > 0.5f ? 1.0f : 0.0f);
    const int finType = (int) pFinType->load();

    dsp::AudioBlock<float> chunk (buffer.getArrayOfWritePointers(), (size_t) numCh, (size_t) start, (size_t) len);
    auto& os = *oversamplers[osIndex];
    auto up = os.processSamplesUp (chunk);
    const int upN = (int) up.getNumSamples();
    float* u0 = up.getChannelPointer (0);
    float* u1 = numCh > 1 ? up.getChannelPointer (1) : nullptr;

    for (int i = 0; i < upN; ++i)
    {
        float drv[dex::numPreStages], on[dex::numPreStages];
        for (int s = 0; s < dex::numPreStages; ++s) { drv[s] = pdDriveS[s].getNextValue(); on[s] = pdOnS[s].getNextValue(); }
        const float wa = wsAmtS.getNextValue(), wsh = wsShapeS.getNextValue(), won = wsOnS.getNextValue();
        const float fd = finDriveS.getNextValue(), fon = finOnS.getNextValue();

        for (int c = 0; c < numCh; ++c)
        {
            float* d = c == 0 ? u0 : u1;
            float x = d[i];
            for (int s = 0; s < dex::numPreStages; ++s)
                if (on[s] > 0.0f) x += (dex::dist::preStage (s, x, drv[s]) - x) * on[s];
            if (won > 0.0f) x += (dex::dist::waveshaper (x, wa, wsh) - x) * won;
            if (fon > 0.0f) x += (dex::dist::finalStage (finType, x, fd) - x) * fon;
            d[i] = x;
        }
    }
    os.processSamplesDown (chunk);

    // ---- DC blocker + EQ 3 (post M/S) + efectos ocultos
    crushS.setTargetValue (pSecCrush->load() * 0.01f);
    ringS.setTargetValue (pSecRing->load() * 0.01f);
    const double ringInc = pSecRingFreq->load() / fs;
    for (int i = 0; i < len; ++i)
    {
        float l = L[i];
        float r = stereo ? R[i] : l;

        auto dc = [this] (int c, float x) { const float y = x - dcX[c] + 0.995f * dcY[c]; dcX[c] = x; dcY[c] = y; return y; };
        l = dc (0, l);
        r = stereo ? dc (1, r) : l;

        eqs[2].beginSample (modOct (2, i), modDb (2, i));
        dex::eqStereo (eqs[2], l, r, modes[2], stereo);

        const float ring = ringS.getNextValue();
        const float rmod = std::sin ((float) (2.0 * MathConstants<double>::pi * ringPhase));
        ringPhase += ringInc;
        if (ringPhase >= 1.0) ringPhase -= 1.0;
        if (ring > 0.0001f)
        {
            l += (l * rmod - l) * ring;
            r += (r * rmod - r) * ring;
        }

        const float crush = crushS.getNextValue();
        if (crush > 0.0001f)
        {
            const float holdLen = 1.0f + crush * 40.0f;
            crushCounter += 1.0f;
            if (crushCounter >= holdLen)
            {
                crushCounter -= holdLen;
                const float levels = std::pow (2.0f, 16.0f - crush * 13.0f);
                crushHold[0] = std::round (l * levels) / levels;
                crushHold[1] = std::round (r * levels) / levels;
            }
            l += (crushHold[0] - l) * crush;
            r += (crushHold[1] - r) * crush;
        }

        L[i] = l;
        if (stereo) R[i] = r;
    }
}

// ---- Estado (parametros + memorias de movimiento) --------------------------
void DistorsionExtremaAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<XmlElement> xml (state.createXml());
    if (xml == nullptr) return;

    auto* motion = xml->createNewChildElement ("MOTION");
    const SpinLock::ScopedLockType sl (motionLock);
    for (auto& s : slots)
    {
        auto* e = motion->createNewChildElement ("SLOT");
        const int len = s.length.load();
        e->setAttribute ("eq", s.eq);
        e->setAttribute ("len", len);
        MemoryBlock mb;
        mb.append (s.freq.data(), (size_t) len * sizeof (float));
        mb.append (s.gain.data(), (size_t) len * sizeof (float));
        e->setAttribute ("data", mb.toBase64Encoding());
    }
    copyXmlToBinary (*xml, destData);
}

void DistorsionExtremaAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml == nullptr) return;

    if (auto* motion = xml->getChildByName ("MOTION"))
    {
        const SpinLock::ScopedLockType sl (motionLock);
        int idx = 0;
        for (auto* e : motion->getChildIterator())
        {
            if (idx >= dex::numSlots) break;
            auto& s = slots[(size_t) idx++];
            const int len = jlimit (0, dex::MotionSlot::kMaxPoints, e->getIntAttribute ("len"));
            MemoryBlock mb;
            mb.fromBase64Encoding (e->getStringAttribute ("data"));
            if (len > 0 && mb.getSize() >= (size_t) len * 2 * sizeof (float))
            {
                auto* f = static_cast<const float*> (mb.getData());
                std::copy (f, f + len, s.freq.begin());
                std::copy (f + len, f + 2 * len, s.gain.begin());
                s.eq = jlimit (-1, dex::numEqs - 1, e->getIntAttribute ("eq", -1));
                s.length.store (s.eq >= 0 ? len : 0);
            }
            else
            {
                s.eq = -1;
                s.length.store (0);
            }
        }
        xml->removeChildElement (motion, true);
    }

    if (xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* DistorsionExtremaAudioProcessor::createEditor()
{
    return new DistorsionExtremaAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DistorsionExtremaAudioProcessor();
}
