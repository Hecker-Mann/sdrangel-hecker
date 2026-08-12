///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2026 SDRangel-DMR-tx contributors                             //
// Frame builder uses codec from MMDVMHost (G4KLX) - GPLv2                       //
///////////////////////////////////////////////////////////////////////////////////

#ifndef PLUGINS_CHANNELTX_MODDMR_DMRFRAMEBUILDER_H_
#define PLUGINS_CHANNELTX_MODDMR_DMRFRAMEBUILDER_H_

#include <cstdint>
#include <cstring>

#include "dmrcodec/DMRDefines.h"

class DMRAmbeEncoder;

class DMRFrameBuilder
{
public:
    enum Mode {
        ModeIdle = 0,
        ModeVoice = 1,
        ModeCal = 2
    };

    DMRFrameBuilder();

    void configure(
        unsigned int colorCode,
        unsigned int srcId,
        unsigned int dstId,
        bool groupCall,
        unsigned int slot,
        bool duplex,
        Mode mode);

    void setVoiceActive(bool active);
    bool isVoiceActive() const { return m_voiceActive; }
    void setAmbeEncoder(DMRAmbeEncoder *encoder) { m_ambeEncoder = encoder; }

    /** Returns 33-byte dibit frame. */
    bool nextFrame(uint8_t* frame);

private:
    enum TxState {
        StateSlot1,
        StateCach2,
        StateSlot2,
        StateCach1,
        StateCal
    };

    enum VoicePhase {
        VoiceIdle,
        VoiceHeader,
        VoiceSync,
        VoicePayload,
        VoiceTerminator,
        VoiceHang
    };

    void buildIdle(uint8_t* frame);
    void buildVoiceLcHeader(uint8_t* frame);
    void buildVoiceSync(uint8_t* frame, uint8_t seqN);
    void buildVoicePayload(uint8_t* frame, uint8_t seqN);
    void buildTerminator(uint8_t* frame);
    void buildCal(uint8_t* frame);
    void buildCach(uint8_t* frame, unsigned int txSlot, unsigned int rxSlot);
    void applySlotType(uint8_t* frame, unsigned char dataType);
    void fillVoicePayload(uint8_t* frame);
    bool nextVoiceFrame(uint8_t* frame);

    DMRAmbeEncoder *m_ambeEncoder;
    unsigned int m_colorCode;
    unsigned int m_srcId;
    unsigned int m_dstId;
    bool m_groupCall;
    unsigned int m_slot;
    bool m_duplex;
    Mode m_mode;

    bool m_voiceActive;
    VoicePhase m_voicePhase;
    unsigned int m_voiceSeq;
    unsigned int m_voicePayloadCount;
    unsigned int m_hangCount;

    TxState m_txState;
    unsigned int m_cachPtr;
    uint8_t m_idle[DMR_FRAME_LENGTH_BYTES];
};

#endif
