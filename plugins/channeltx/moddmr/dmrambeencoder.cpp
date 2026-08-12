///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2026 SDRangel-DMR-tx contributors                             //
///////////////////////////////////////////////////////////////////////////////////

#include "dmrambeencoder.h"

#include "mbeenc.h"
#include "cgolay24128.h"

#include <cstring>
#include <cmath>
#include <cassert>

static const uint8_t BIT_MASK_TABLE[] = {0x80U, 0x40U, 0x20U, 0x10U, 0x08U, 0x04U, 0x02U, 0x01U};
#define WRITE_BIT(p,i,b) p[(i)>>3] = (b) ? (p[(i)>>3] | BIT_MASK_TABLE[(i)&7]) : (p[(i)>>3] & ~BIT_MASK_TABLE[(i)&7])
#define READ_BIT(p,i)    (p[(i)>>3] & BIT_MASK_TABLE[(i)&7])

// Interleave tables from MMDVMHost AMBEFEC (G4KLX)
static const unsigned int DMR_A_TABLE[] = {
    0U,  4U,  8U, 12U, 16U, 20U, 24U, 28U, 32U, 36U, 40U, 44U,
    48U, 52U, 56U, 60U, 64U, 68U,  1U,  5U,  9U, 13U, 17U, 21U};
static const unsigned int DMR_B_TABLE[] = {
    25U, 29U, 33U, 37U, 41U, 45U, 49U, 53U, 57U, 61U, 65U, 69U,
     2U,  6U, 10U, 14U, 18U, 22U, 26U, 30U, 34U, 38U, 42U};
static const unsigned int DMR_C_TABLE[] = {
    46U, 50U, 54U, 58U, 62U, 66U, 70U,  3U,  7U, 11U, 15U, 19U,
    23U, 27U, 31U, 35U, 39U, 43U, 47U, 51U, 55U, 59U, 63U, 67U, 71U};

// Standard DMR silence AMBE payload (3 frames) for fallback
static const uint8_t DMR_SILENCE_BURST[33] = {
    0xB9U, 0xE8U, 0x81U, 0x52U, 0x61U, 0x73U, 0x00U, 0x2AU, 0x6BU, 0xB9U, 0xE8U,
    0x81U, 0x52U, 0x60U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x01U, 0x73U, 0x00U,
    0x2AU, 0x6BU, 0xB9U, 0xE8U, 0x81U, 0x52U, 0x61U, 0x73U, 0x00U, 0x2AU, 0x6BU};

static uint32_t computePrngMask23(uint32_t aOrig)
{
    uint16_t pr[24];
    pr[0] = static_cast<uint16_t>(16U * aOrig);
    for (int i = 1; i < 24; i++) {
        pr[i] = static_cast<uint16_t>((173U * static_cast<uint32_t>(pr[i - 1]) + 13849U) % 65536U);
    }
    for (int i = 1; i < 24; i++) {
        pr[i] = pr[i] / 32768U;
    }
    uint32_t mask = 0;
    for (int i = 1; i <= 23; i++) {
        if (pr[i]) {
            mask |= (1U << (23 - i));
        }
    }
    return mask;
}

/** Pack 49-bit voice params (from b[9]) into FEC'd 72-bit AMBE+2 frame. */
static void encodeAmbeFrame(const int b[9], uint8_t frame72[9])
{
    static const int bLengths[9] = {7, 5, 5, 9, 7, 5, 4, 4, 3};
    uint8_t bits49[49];
    int pos = 0;
    for (int i = 0; i < 9; i++) {
        int val = b[i];
        for (int j = bLengths[i] - 1; j >= 0; j--) {
            bits49[pos++] = (val >> j) & 1;
        }
    }

    uint32_t c0 = 0, c1 = 0;
    for (int i = 0; i < 12; i++) {
        c0 = (c0 << 1) | bits49[i];
        c1 = (c1 << 1) | bits49[12 + i];
    }

    uint32_t a = CGolay24128::encode24128(c0);
    uint32_t bCode = CGolay24128::encode23127(c1);
    bCode ^= computePrngMask23(c0);

    uint32_t cBlock = 0;
    for (int i = 24; i < 49; i++) {
        cBlock = (cBlock << 1) | bits49[i];
    }

    std::memset(frame72, 0, 9);
    for (int i = 0; i < 24; i++) {
        if ((a >> (23 - i)) & 1) {
            frame72[i / 8] |= (1 << (7 - (i % 8)));
        }
    }
    for (int i = 0; i < 23; i++) {
        int p = 24 + i;
        if ((bCode >> (22 - i)) & 1) {
            frame72[p / 8] |= (1 << (7 - (p % 8)));
        }
    }
    for (int i = 0; i < 25; i++) {
        int p = 47 + i;
        if ((cBlock >> (24 - i)) & 1) {
            frame72[p / 8] |= (1 << (7 - (p % 8)));
        }
    }
}

static void extractABC(const uint8_t ambe[9], uint32_t& a, uint32_t& b, uint32_t& c)
{
    a = 0;
    for (int i = 0; i < 24; i++) {
        if ((ambe[i / 8] >> (7 - (i % 8))) & 1) {
            a |= (0x800000U >> i);
        }
    }
    b = 0;
    for (int i = 0; i < 23; i++) {
        int p = 24 + i;
        if ((ambe[p / 8] >> (7 - (p % 8))) & 1) {
            b |= (0x400000U >> i);
        }
    }
    c = 0;
    for (int i = 0; i < 25; i++) {
        int p = 47 + i;
        if ((ambe[p / 8] >> (7 - (p % 8))) & 1) {
            c |= (0x1000000U >> i);
        }
    }
}

static unsigned int slotBitPos(unsigned int basePos, unsigned int frameIndex)
{
    if (frameIndex == 0U) {
        return basePos;
    }
    if (frameIndex == 1U) {
        unsigned int pos = basePos + 72U;
        if (pos >= 108U) {
            pos += 48U; // skip sync/EMB region
        }
        return pos;
    }
    return basePos + 192U;
}

void DMRAmbeEncoder::packAmbeIntoBurst(const uint8_t ambe[3][9], uint8_t frame[33])
{
    std::memset(frame, 0, 33);

    for (unsigned int fi = 0; fi < 3U; fi++) {
        uint32_t a, b, c;
        extractABC(ambe[fi], a, b, c);

        uint32_t mask = 0x800000U;
        for (unsigned int i = 0U; i < 24U; i++, mask >>= 1) {
            WRITE_BIT(frame, slotBitPos(DMR_A_TABLE[i], fi), a & mask);
        }
        mask = 0x400000U;
        for (unsigned int i = 0U; i < 23U; i++, mask >>= 1) {
            WRITE_BIT(frame, slotBitPos(DMR_B_TABLE[i], fi), b & mask);
        }
        mask = 0x1000000U;
        for (unsigned int i = 0U; i < 25U; i++, mask >>= 1) {
            WRITE_BIT(frame, slotBitPos(DMR_C_TABLE[i], fi), c & mask);
        }
    }
}

DMRAmbeEncoder::DMRAmbeEncoder() :
    m_encoder(nullptr),
    m_gainDb(0)
{
    MBEEncoder *enc = new MBEEncoder();
    enc->set_dmr_mode();
    enc->set_gain_adjust(1.0f);
    m_encoder = enc;
    m_pcmBuffer.reserve(480 * 4);
}

DMRAmbeEncoder::~DMRAmbeEncoder()
{
    delete static_cast<MBEEncoder*>(m_encoder);
    m_encoder = nullptr;
}

void DMRAmbeEncoder::reset()
{
    QMutexLocker lock(&m_mutex);
    delete static_cast<MBEEncoder*>(m_encoder);
    MBEEncoder *enc = new MBEEncoder();
    enc->set_dmr_mode();
    enc->set_gain_adjust(std::pow(10.0f, m_gainDb / 20.0f));
    m_encoder = enc;
    m_pcmBuffer.clear();
}

void DMRAmbeEncoder::setGainDb(int gainDb)
{
    if (gainDb < -20) gainDb = -20;
    if (gainDb > 20) gainDb = 20;
    m_gainDb = gainDb;
    QMutexLocker lock(&m_mutex);
    if (m_encoder) {
        static_cast<MBEEncoder*>(m_encoder)->set_gain_adjust(std::pow(10.0f, m_gainDb / 20.0f));
    }
}

bool DMRAmbeEncoder::encodeFrame(const int16_t pcm[160], uint8_t ambe[9])
{
    QMutexLocker lock(&m_mutex);
    if (!m_encoder || !pcm || !ambe) {
        return false;
    }

    int b[9] = {0};
    static_cast<MBEEncoder*>(m_encoder)->encode_dmr_params(pcm, b);
    encodeAmbeFrame(b, ambe);
    return true;
}

bool DMRAmbeEncoder::encodeVoiceBurst(const int16_t pcm[480], uint8_t frame[33])
{
    uint8_t ambe[3][9];
    for (int i = 0; i < 3; i++) {
        if (!encodeFrame(pcm + i * 160, ambe[i])) {
            std::memcpy(frame, DMR_SILENCE_BURST, 33);
            return false;
        }
    }
    packAmbeIntoBurst(ambe, frame);
    return true;
}

void DMRAmbeEncoder::pushPcm8k(const int16_t *samples, int count)
{
    if (!samples || count <= 0) {
        return;
    }
    QMutexLocker lock(&m_mutex);
    m_pcmBuffer.insert(m_pcmBuffer.end(), samples, samples + count);
    // Cap buffer (~1 second)
    if (m_pcmBuffer.size() > 8000) {
        m_pcmBuffer.erase(m_pcmBuffer.begin(), m_pcmBuffer.begin() + (m_pcmBuffer.size() - 8000));
    }
}

bool DMRAmbeEncoder::hasBurstAudio() const
{
    QMutexLocker lock(&m_mutex);
    return m_pcmBuffer.size() >= 480;
}

bool DMRAmbeEncoder::takeVoiceBurst(uint8_t frame[33])
{
    int16_t pcm[480];
    {
        QMutexLocker lock(&m_mutex);
        if (m_pcmBuffer.size() < 480) {
            std::memcpy(frame, DMR_SILENCE_BURST, 33);
            return false;
        }
        std::copy(m_pcmBuffer.begin(), m_pcmBuffer.begin() + 480, pcm);
        m_pcmBuffer.erase(m_pcmBuffer.begin(), m_pcmBuffer.begin() + 480);
    }
    return encodeVoiceBurst(pcm, frame);
}
