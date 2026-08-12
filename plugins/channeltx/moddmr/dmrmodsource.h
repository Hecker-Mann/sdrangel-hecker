///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2026 SDRangel-DMR-tx contributors                             //
///////////////////////////////////////////////////////////////////////////////////

#ifndef PLUGINS_CHANNELTX_MODDMR_DMRMODSOURCE_H_
#define PLUGINS_CHANNELTX_MODDMR_DMRMODSOURCE_H_

#include <QObject>
#include <QRecursiveMutex>

#include "dsp/channelsamplesource.h"
#include "dsp/nco.h"
#include "dsp/dsptypes.h"
#include "audio/audiofifo.h"
#include "util/movingaverage.h"
#include "dmrmodsettings.h"
#include "dmr4fsk.h"
#include "dmrframebuilder.h"
#include "dmrambeencoder.h"

class ChannelAPI;

class DMRModSource : public QObject, public ChannelSampleSource
{
    Q_OBJECT
public:
    DMRModSource();
    virtual ~DMRModSource();

    virtual void pull(SampleVector::iterator begin, unsigned int nbSamples);
    virtual void pullOne(Sample& sample);
    virtual void prefetch(unsigned int nbSamples);

    double getMagSq() const { return m_magsq; }
    void getLevels(qreal& rmsLevel, qreal& peakLevel, int& numSamples) const
    {
        rmsLevel = m_rmsLevel;
        peakLevel = m_peakLevelOut;
        numSamples = m_levelNbSamples;
    }

    void applySettings(const QStringList& settingsKeys, const DMRModSettings& settings, bool force = false);
    void applyChannelSettings(int channelSampleRate, int channelFrequencyOffset, bool force = false);
    void applyAudioSampleRate(int sampleRate);
    int getAudioSampleRate() const { return m_audioSampleRate; }
    void setChannel(ChannelAPI *channel) { m_channel = channel; }
    int getChannelSampleRate() const { return m_channelSampleRate; }
    AudioFifo *getAudioFifo() { return &m_audioFifo; }
    void startVoice();
    void stopVoice();

private:
    void modulateSample();
    void calculateLevel(Real sample);
    void pullAudio(unsigned int nbSamples);
    void feedAmbeFromAudio(unsigned int nbAudioSamples);

    int m_channelSampleRate;
    int m_channelFrequencyOffset;
    int m_audioSampleRate;
    DMRModSettings m_settings;
    ChannelAPI *m_channel;

    NCO m_carrierNco;
    Real m_linearGain;
    Complex m_modSample;
    Real m_modPhasor;

    DMR4FSK m_modem;
    DMRFrameBuilder m_frameBuilder;
    DMRAmbeEncoder m_ambeEncoder;

    AudioFifo m_audioFifo;
    AudioVector m_audioBuffer;
    unsigned int m_audioBufferFill;
    AudioVector m_audioReadBuffer;
    unsigned int m_audioReadBufferFill;
    int m_downsampleRatio;   // audioSampleRate / 8000
    int m_downsampleCount;
    Real m_downsampleAcc;
    QRecursiveMutex m_audioMutex;

    uint8_t m_frame[33];
    int m_frameByteIdx;
    int m_dibitIdx;

    double m_magsq;
    MovingAverageUtil<double, double, 16> m_movingAverage;
    int m_levelCalcCount;
    Real m_peakLevel;
    Real m_levelSum;
    qreal m_rmsLevel;
    qreal m_peakLevelOut;
    static const int m_levelNbSamples;

private slots:
    void handleAudio();
};

#endif
