///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2026 SDRangel-DMR-tx contributors                             //
// AMBE+2 encode path uses OpenDMR/OP25 MBEEncoder (GPLv2)                     //
///////////////////////////////////////////////////////////////////////////////////

#ifndef PLUGINS_CHANNELTX_MODDMR_DMRAMBEENCODER_H_
#define PLUGINS_CHANNELTX_MODDMR_DMRAMBEENCODER_H_

#include <cstdint>
#include <vector>
#include <QRecursiveMutex>

/**
 * Encodes 8 kHz PCM to DMR AMBE+2 and packs three 20 ms frames into a
 * 33-byte DMR voice burst (without sync/EMB).
 *
 * Patent note: AMBE+2 is patent-encumbered. This uses the OpenDMR/OP25
 * software encoder for amateur/experimental use — same class of software
 * vocoder as mbelib (decode) already used by SDRangel.
 */
class DMRAmbeEncoder
{
public:
    DMRAmbeEncoder();
    ~DMRAmbeEncoder();

    void reset();
    void setGainDb(int gainDb);

    /** Encode one 20 ms frame (160 samples @ 8 kHz) to 9 AMBE bytes. */
    bool encodeFrame(const int16_t pcm[160], uint8_t ambe[9]);

    /**
     * Encode 60 ms of audio (480 samples @ 8 kHz) into a 33-byte DMR voice
     * burst payload. Sync/EMB must be applied by the caller afterwards.
     */
    bool encodeVoiceBurst(const int16_t pcm[480], uint8_t frame[33]);

    /** Pack three already-encoded AMBE frames into a DMR voice burst. */
    static void packAmbeIntoBurst(const uint8_t ambe[3][9], uint8_t frame[33]);

    /** Push PCM at 8 kHz into the internal buffer (thread-safe). */
    void pushPcm8k(const int16_t *samples, int count);

    /** True when at least 480 samples are buffered for a voice burst. */
    bool hasBurstAudio() const;

    /**
     * Consume 480 samples and encode a voice burst into frame[33].
     * Returns false if not enough audio (writes silence AMBE instead).
     */
    bool takeVoiceBurst(uint8_t frame[33]);

private:
    void *m_encoder; // MBEEncoder*
    int m_gainDb;
    mutable QRecursiveMutex m_mutex;
    std::vector<int16_t> m_pcmBuffer;
};

#endif
