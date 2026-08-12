///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2026 SDRangel-DMR-tx contributors                             //
///////////////////////////////////////////////////////////////////////////////////

#include "dmrframebuilder.h"

#include "dmrcodec/DMRFullLC.h"
#include "dmrcodec/DMRLC.h"
#include "dmrcodec/DMRSlotType.h"
#include "dmrcodec/DMREMB.h"
#include "dmrcodec/Sync.h"
#include "dmrambeencoder.h"

// DMRDefines.h prefixes these with TAG_DATA + 0x00; skip that for the 33-byte air frame.
static const unsigned char *const DMR_IDLE_FRAME = DMR_IDLE_DATA + 2;
static const unsigned char *const DMR_SILENCE_FRAME = DMR_SILENCE_DATA + 2;

static const uint8_t CACH_INTERLEAVE[] = {
    1U,  2U,  3U,  5U,  6U,  7U,  9U, 10U, 11U, 13U, 15U, 16U, 17U, 19U, 20U, 21U, 23U,
    25U, 26U, 27U, 29U, 30U, 31U, 33U, 34U, 35U, 37U, 39U, 40U, 41U, 43U, 44U, 45U, 47U,
    49U, 50U, 51U, 53U, 54U, 55U, 57U, 58U, 59U, 61U, 63U, 64U, 65U, 67U, 68U, 69U, 71U,
    73U, 74U, 75U, 77U, 78U, 79U, 81U, 82U, 83U, 85U, 87U, 88U, 89U, 91U, 92U, 93U, 95U};

static const uint8_t EMPTY_SHORT_LC[12] = {0};

DMRFrameBuilder::DMRFrameBuilder() :
    m_ambeEncoder(nullptr),
    m_colorCode(1),
    m_srcId(1234567),
    m_dstId(9),
    m_groupCall(true),
    m_slot(1),
    m_duplex(false),
    m_mode(ModeIdle),
    m_voiceActive(false),
    m_voicePhase(VoiceIdle),
    m_voiceSeq(0),
    m_voicePayloadCount(0),
    m_hangCount(0),
    m_txState(StateSlot1),
    m_cachPtr(0)
{
    std::memcpy(m_idle, DMR_IDLE_FRAME, DMR_FRAME_LENGTH_BYTES);
}

void DMRFrameBuilder::configure(
    unsigned int colorCode,
    unsigned int srcId,
    unsigned int dstId,
    bool groupCall,
    unsigned int slot,
    bool duplex,
    Mode mode)
{
    m_colorCode = colorCode & 0x0FU;
    m_srcId = srcId & 0xFFFFFFU;
    m_dstId = dstId & 0xFFFFFFU;
    m_groupCall = groupCall;
    m_slot = (slot == 2U) ? 2U : 1U;
    m_duplex = duplex;
    m_mode = mode;

    std::memcpy(m_idle, DMR_IDLE_FRAME, DMR_FRAME_LENGTH_BYTES);
    applySlotType(m_idle, DT_IDLE);
}

void DMRFrameBuilder::setVoiceActive(bool active)
{
    if (active && !m_voiceActive) {
        m_voicePhase = VoiceHeader;
        m_voiceSeq = 0;
        m_voicePayloadCount = 0;
        m_hangCount = 0;
        if (m_ambeEncoder) {
            m_ambeEncoder->reset();
        }
    } else if (!active && m_voiceActive) {
        // Finish current superframe then send terminator
        if (m_voicePhase == VoiceSync || m_voicePhase == VoicePayload || m_voicePhase == VoiceHeader) {
            m_voicePhase = VoiceTerminator;
        }
    }

    m_voiceActive = active;
}

void DMRFrameBuilder::applySlotType(uint8_t* frame, unsigned char dataType)
{
    CDMRSlotType slotType;
    slotType.setColorCode(m_colorCode);
    slotType.setDataType(dataType);
    slotType.getData(frame);
}

void DMRFrameBuilder::fillVoicePayload(uint8_t* frame)
{
    if (m_ambeEncoder && m_ambeEncoder->takeVoiceBurst(frame)) {
        return;
    }
    // Fallback: standard silence AMBE burst
    std::memcpy(frame, DMR_SILENCE_FRAME, DMR_FRAME_LENGTH_BYTES);
}

void DMRFrameBuilder::buildIdle(uint8_t* frame)
{
    std::memcpy(frame, m_idle, DMR_FRAME_LENGTH_BYTES);
}

void DMRFrameBuilder::buildVoiceLcHeader(uint8_t* frame)
{
    std::memset(frame, 0, DMR_FRAME_LENGTH_BYTES);
    CSync::addDMRDataSync(frame, m_duplex);

    FLCO flco = m_groupCall ? FLCO::GROUP : FLCO::USER_USER;
    CDMRLC lc(flco, m_srcId, m_dstId);
    CDMRFullLC fullLC;
    fullLC.encode(lc, frame, DT_VOICE_LC_HEADER);

    applySlotType(frame, DT_VOICE_LC_HEADER);
}

void DMRFrameBuilder::buildVoiceSync(uint8_t* frame, uint8_t seqN)
{
    fillVoicePayload(frame);
    CSync::addDMRAudioSync(frame, m_duplex);
    (void) seqN;
}

void DMRFrameBuilder::buildVoicePayload(uint8_t* frame, uint8_t seqN)
{
    fillVoicePayload(frame);

    CDMREMB emb;
    emb.setColorCode(m_colorCode);
    emb.setPI(false);
    // LCSS rotates through embedded LC fragments; use non-zero for mid-superframe
    emb.setLCSS((seqN % 4U) == 0U ? 1U : 2U);
    emb.getData(frame);
}

void DMRFrameBuilder::buildTerminator(uint8_t* frame)
{
    std::memset(frame, 0, DMR_FRAME_LENGTH_BYTES);
    CSync::addDMRDataSync(frame, m_duplex);

    FLCO flco = m_groupCall ? FLCO::GROUP : FLCO::USER_USER;
    CDMRLC lc(flco, m_srcId, m_dstId);
    CDMRFullLC fullLC;
    fullLC.encode(lc, frame, DT_TERMINATOR_WITH_LC);

    applySlotType(frame, DT_TERMINATOR_WITH_LC);
}

void DMRFrameBuilder::buildCal(uint8_t* frame)
{
    for (unsigned int i = 0U; i < DMR_FRAME_LENGTH_BYTES; i++) {
        frame[i] = 0x5FU;
    }
}

void DMRFrameBuilder::buildCach(uint8_t* frame, unsigned int txSlot, unsigned int rxSlot)
{
    uint8_t shortLC[12];
    std::memcpy(shortLC, EMPTY_SHORT_LC, 12U);

    if (m_cachPtr >= 12U) {
        m_cachPtr = 0U;
    }

    std::memcpy(frame, shortLC + m_cachPtr, 3U);

    bool at = m_voiceActive;
    bool tc = (txSlot == 1U);
    bool ls0 = true;
    bool ls1 = true;

    if (m_cachPtr == 0U) {
        ls1 = false;
    } else if (m_cachPtr == 9U) {
        ls0 = false;
    }

    bool h0 = at ^ tc ^ ls1;
    bool h1 = tc ^ ls1 ^ ls0;
    bool h2 = at ^ tc ^ ls0;

    frame[0U] |= at ? 0x80U : 0x00U;
    frame[0U] |= tc ? 0x08U : 0x00U;
    frame[1U] |= ls1 ? 0x80U : 0x00U;
    frame[1U] |= ls0 ? 0x08U : 0x00U;
    frame[1U] |= h0 ? 0x02U : 0x00U;
    frame[2U] |= h1 ? 0x20U : 0x00U;
    frame[2U] |= h2 ? 0x02U : 0x00U;

    m_cachPtr += 3U;
    (void) rxSlot;
}

bool DMRFrameBuilder::nextVoiceFrame(uint8_t* frame)
{
    switch (m_voicePhase) {
    case VoiceHeader:
        buildVoiceLcHeader(frame);
        m_voicePhase = VoiceSync;
        return true;
    case VoiceSync:
        buildVoiceSync(frame, 0U);
        m_voicePhase = VoicePayload;
        m_voiceSeq = 1U;
        return true;
    case VoicePayload:
        buildVoicePayload(frame, m_voiceSeq);
        m_voiceSeq++;
        if (m_voiceSeq > 5U) {
            // Next superframe starts with audio sync again while PTT held
            if (m_voiceActive) {
                m_voicePhase = VoiceSync;
                m_voiceSeq = 0U;
            } else {
                m_voicePhase = VoiceTerminator;
            }
        }
        return true;
    case VoiceTerminator:
        buildTerminator(frame);
        m_voicePhase = VoiceHang;
        m_hangCount = 3U;
        return true;
    case VoiceHang:
        buildIdle(frame);
        if (m_hangCount > 0U) {
            m_hangCount--;
        } else {
            m_voiceActive = false;
            m_voicePhase = VoiceIdle;
        }
        return true;
    default:
        buildIdle(frame);
        return true;
    }
}

bool DMRFrameBuilder::nextFrame(uint8_t* frame)
{
    if (m_mode == ModeCal) {
        buildCal(frame);
        return true;
    }

    // TDMA structure: slot1 / CACH / slot2 / CACH
    switch (m_txState) {
    case StateSlot1:
        if (m_mode == ModeVoice && m_voiceActive && m_slot == 1U) {
            nextVoiceFrame(frame);
        } else {
            buildIdle(frame);
        }
        m_txState = StateCach2;
        return true;
    case StateCach2:
        buildCach(frame, 1U, 0U);
        m_txState = StateSlot2;
        return true;
    case StateSlot2:
        if (m_mode == ModeVoice && m_voiceActive && m_slot == 2U) {
            nextVoiceFrame(frame);
        } else {
            buildIdle(frame);
        }
        m_txState = StateCach1;
        return true;
    case StateCach1:
        buildCach(frame, 0U, 1U);
        m_txState = StateSlot1;
        return true;
    default:
        m_txState = StateSlot1;
        buildIdle(frame);
        return true;
    }
}
