///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2026 SDRangel-DMR-tx contributors                             //
//                                                                               //
// This program is free software; you can redistribute it and/or modify          //
// it under the terms of the GNU General Public License as published by          //
// the Free Software Foundation as version 3 of the License, or                  //
// (at your option) any later version.                                           //
///////////////////////////////////////////////////////////////////////////////////

#ifndef PLUGINS_CHANNELTX_MODDMR_DMR4FSK_H_
#define PLUGINS_CHANNELTX_MODDMR_DMR4FSK_H_

#include "dsp/dsptypes.h"
#include "dsp/raisedcosine.h"

/**
 * DMR 4FSK modulator at 4800 symbols/s.
 * Each input byte carries four dibits (MSB first).
 */
class DMR4FSK
{
public:
    DMR4FSK();

    void configure(int sampleRate, float gain);
    Real nextSample(uint8_t dibitByte, int dibitIndex);
    int getSamplesPerSymbol() const { return m_samplesPerSymbol; }
    int getSamplesPerFrame() const { return m_samplesPerSymbol * 4 * 33; }

private:
    static Real dibitToLevel(uint8_t byte, int dibitIndex);

    int m_sampleRate;
    int m_samplesPerSymbol;
    int m_symbolSampleIdx;
    Real m_gain;
    RaisedCosine<Real> m_pulseShape;
    Real m_currentLevel;
    Real m_nextLevel;
};

#endif
