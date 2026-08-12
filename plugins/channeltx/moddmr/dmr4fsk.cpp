///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2026 SDRangel-DMR-tx contributors                             //
///////////////////////////////////////////////////////////////////////////////////

#include "dmr4fsk.h"

#include <cmath>

// ETSI TS 102 361 frequency deviations (Hz) mapped to normalized modulation index
static const Real DMR_DEV_PLUS3 = 1944.0f;
static const Real DMR_DEV_PLUS1 = 648.0f;
static const Real DMR_DEV_MINUS1 = -648.0f;
static const Real DMR_DEV_MINUS3 = -1944.0f;

DMR4FSK::DMR4FSK() :
    m_sampleRate(48000),
    m_samplesPerSymbol(10),
    m_symbolSampleIdx(0),
    m_gain(1.0f),
    m_currentLevel(0.0f),
    m_nextLevel(0.0f)
{
    m_pulseShape.create(0.2f, 5, 48000.0f / 4800.0f);
}

void DMR4FSK::configure(int sampleRate, float gain)
{
    m_sampleRate = sampleRate;
    m_samplesPerSymbol = sampleRate / 4800;
    if (m_samplesPerSymbol < 1) {
        m_samplesPerSymbol = 1;
    }

    m_gain = gain;
    m_symbolSampleIdx = 0;
    m_pulseShape.create(0.2f, 5, (Real) sampleRate / 4800.0f);
}

Real DMR4FSK::dibitToLevel(uint8_t byte, int dibitIndex)
{
    const uint8_t mask = 0xC0U;
    uint8_t c = byte << (dibitIndex * 2);

    switch (c & mask) {
    case 0xC0U:
        return DMR_DEV_PLUS3;
    case 0x80U:
        return DMR_DEV_PLUS1;
    case 0x00U:
        return DMR_DEV_MINUS1;
    default:
        return DMR_DEV_MINUS3;
    }
}

Real DMR4FSK::nextSample(uint8_t dibitByte, int dibitIndex)
{
    if (m_symbolSampleIdx == 0) {
        m_nextLevel = dibitToLevel(dibitByte, dibitIndex);
        m_currentLevel = m_nextLevel;
    }

    Real shaped = m_pulseShape.filter(m_currentLevel);

    m_symbolSampleIdx++;
    if (m_symbolSampleIdx >= m_samplesPerSymbol) {
        m_symbolSampleIdx = 0;
    }

    // Normalize deviation to roughly unit magnitude for FM integration in source
    return shaped * m_gain / DMR_DEV_PLUS3;
}
