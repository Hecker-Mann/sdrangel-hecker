///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2026 SDRangel-DMR-tx contributors                             //
///////////////////////////////////////////////////////////////////////////////////

#include <cmath>
#include <algorithm>
#include <QDebug>

#include "util/db.h"
#include "dmrmodsource.h"

const int DMRModSource::m_levelNbSamples = 480;

DMRModSource::DMRModSource() :
    m_channelSampleRate(48000),
    m_channelFrequencyOffset(0),
    m_audioSampleRate(48000),
    m_channel(nullptr),
    m_linearGain(1.0f),
    m_modPhasor(0.0f),
    m_audioFifo(12000),
    m_audioBufferFill(0),
    m_audioReadBufferFill(0),
    m_downsampleRatio(6),
    m_downsampleCount(0),
    m_downsampleAcc(0.0f),
    m_frameByteIdx(33),
    m_dibitIdx(0),
    m_magsq(0.0),
    m_levelCalcCount(0),
    m_peakLevel(0.0f),
    m_levelSum(0.0f),
    m_rmsLevel(0.0),
    m_peakLevelOut(0.0)
{
    m_audioFifo.setLabel("DMRModSource.m_audioFifo");
    m_audioBuffer.resize(4800);
    m_audioReadBuffer.resize(16384);

    m_frameBuilder.setAmbeEncoder(&m_ambeEncoder);
    applyAudioSampleRate(m_audioSampleRate);
    applySettings(QStringList(), m_settings, true);
    applyChannelSettings(m_channelSampleRate, m_channelFrequencyOffset, true);
}

DMRModSource::~DMRModSource()
{
}

void DMRModSource::pull(SampleVector::iterator begin, unsigned int nbSamples)
{
    std::for_each(
        begin,
        begin + nbSamples,
        [this](Sample& s) {
            pullOne(s);
        }
    );
}

void DMRModSource::pullOne(Sample& sample)
{
    if (m_settings.m_channelMute) {
        sample.m_real = 0.0f;
        sample.m_imag = 0.0f;
        return;
    }

    modulateSample();

    Complex ci = m_modSample;
    ci *= m_carrierNco.nextIQ();

    double magsq = ci.real() * ci.real() + ci.imag() * ci.imag();
    m_movingAverage(magsq);
    m_magsq = m_movingAverage.asDouble();

    sample.m_real = (FixReal) (ci.real() * SDR_TX_SCALEF);
    sample.m_imag = (FixReal) (ci.imag() * SDR_TX_SCALEF);
}

void DMRModSource::prefetch(unsigned int nbSamples)
{
    if (!m_settings.m_micEnable) {
        return;
    }

    unsigned int nbSamplesAudio = (unsigned int) (nbSamples * ((Real) m_audioSampleRate / (Real) m_channelSampleRate));

    if (nbSamplesAudio == 0) {
        nbSamplesAudio = 1;
    }

    pullAudio(nbSamplesAudio);
    feedAmbeFromAudio(nbSamplesAudio);
}

void DMRModSource::pullAudio(unsigned int nbSamplesAudio)
{
    QMutexLocker mlock(&m_audioMutex);

    if (nbSamplesAudio > m_audioBuffer.size()) {
        m_audioBuffer.resize(nbSamplesAudio);
    }

    if (m_audioReadBufferFill >= nbSamplesAudio)
    {
        std::copy(&m_audioReadBuffer[0], &m_audioReadBuffer[nbSamplesAudio], &m_audioBuffer[0]);
        m_audioBufferFill = 0;

        if (m_audioReadBufferFill > nbSamplesAudio)
        {
            std::copy(&m_audioReadBuffer[nbSamplesAudio], &m_audioReadBuffer[m_audioReadBufferFill], &m_audioReadBuffer[0]);
            m_audioReadBufferFill = m_audioReadBufferFill - nbSamplesAudio;
        }
        else
        {
            m_audioReadBufferFill = 0;
        }
    }
    else
    {
        std::fill(m_audioBuffer.begin(), m_audioBuffer.begin() + nbSamplesAudio, AudioSample{0, 0});
        m_audioBufferFill = 0;
    }
}

void DMRModSource::feedAmbeFromAudio(unsigned int nbAudioSamples)
{
    if (!m_settings.m_micEnable || m_downsampleRatio < 1) {
        return;
    }

    // Integer-ratio boxcar downsample to AMBE's fixed 8 kHz (e.g. 48k/6).
    // SDRangel Interpolator::decimate only supports factors < 2.
    for (unsigned int i = 0; i < nbAudioSamples; i++)
    {
        Real af = ((Real) m_audioBuffer[i].l + (Real) m_audioBuffer[i].r) * 0.5f * m_settings.m_micVolume;
        m_downsampleAcc += af;
        m_downsampleCount++;

        if (m_downsampleCount >= m_downsampleRatio)
        {
            Real avg = m_downsampleAcc / (Real) m_downsampleRatio;
            qint16 pcm = (qint16) std::max(-32768.0f, std::min(32767.0f, avg));
            m_ambeEncoder.pushPcm8k(&pcm, 1);
            m_downsampleAcc = 0.0f;
            m_downsampleCount = 0;
        }
    }
}

void DMRModSource::handleAudio()
{
    unsigned int nbRead;

    QMutexLocker mlock(&m_audioMutex);

    while ((nbRead = m_audioFifo.read(
                reinterpret_cast<quint8*>(&m_audioReadBuffer[m_audioReadBufferFill]),
                4096)) != 0)
    {
        if (m_audioReadBufferFill + nbRead + 4096 < m_audioReadBuffer.size()) {
            m_audioReadBufferFill += nbRead;
        } else {
            break;
        }
    }
}

void DMRModSource::modulateSample()
{
    if (m_frameByteIdx >= 33) {
        m_frameBuilder.nextFrame(m_frame);
        m_frameByteIdx = 0;
        m_dibitIdx = 0;
    }

    Real t = m_modem.nextSample(m_frame[m_frameByteIdx], m_dibitIdx);
    m_dibitIdx++;
    if (m_dibitIdx >= 4) {
        m_dibitIdx = 0;
        m_frameByteIdx++;
    }

    calculateLevel(t);

    m_modPhasor += (2.0f * M_PI * 648.0f / (float) m_channelSampleRate) * t * 3.0f;
    if (m_modPhasor > M_PI) {
        m_modPhasor -= 2.0f * M_PI;
    }

    Real scale = m_linearGain * 0.891235351562f * SDR_TX_SCALEF;
    m_modSample.real(cos(m_modPhasor) * scale);
    m_modSample.imag(sin(m_modPhasor) * scale);
}

void DMRModSource::calculateLevel(Real sample)
{
    if (m_levelCalcCount < m_levelNbSamples) {
        m_levelSum += sample * sample;
        m_peakLevel = std::max(std::fabs(m_peakLevel), std::fabs(sample));
        m_levelCalcCount++;
    } else {
        m_rmsLevel = sqrt(m_levelSum / m_levelNbSamples);
        m_peakLevelOut = m_peakLevel;
        m_levelSum = 0.0f;
        m_peakLevel = 0.0f;
        m_levelCalcCount = 0;
    }
}

void DMRModSource::startVoice()
{
    m_ambeEncoder.reset();
    m_frameBuilder.setVoiceActive(true);
    m_frameByteIdx = 33;
}

void DMRModSource::stopVoice()
{
    m_frameBuilder.setVoiceActive(false);
}

void DMRModSource::applyAudioSampleRate(int sampleRate)
{
    if (sampleRate < 0) {
        qWarning("DMRModSource::applyAudioSampleRate: invalid sample rate %d", sampleRate);
        return;
    }

    m_audioSampleRate = sampleRate;
    m_downsampleRatio = std::max(1, sampleRate / 8000);
    m_downsampleCount = 0;
    m_downsampleAcc = 0.0f;
}

void DMRModSource::applySettings(const QStringList& settingsKeys, const DMRModSettings& settings, bool force)
{
    if (settingsKeys.contains("gain") || force) {
        m_linearGain = Db::powerDb2mag(settings.m_gain);
    }

    if (settingsKeys.contains("colorCode")
        || settingsKeys.contains("srcId")
        || settingsKeys.contains("dstId")
        || settingsKeys.contains("groupCall")
        || settingsKeys.contains("slot")
        || settingsKeys.contains("duplex")
        || settingsKeys.contains("mode")
        || force)
    {
        DMRFrameBuilder::Mode mode = DMRFrameBuilder::ModeIdle;
        if (settings.m_mode == 1) {
            mode = DMRFrameBuilder::ModeVoice;
        } else if (settings.m_mode == 2) {
            mode = DMRFrameBuilder::ModeCal;
        }

        m_frameBuilder.configure(
            settings.m_colorCode,
            settings.m_srcId,
            settings.m_dstId,
            settings.m_groupCall,
            settings.m_slot,
            settings.m_duplex,
            mode);
        m_frameByteIdx = 33;
    }

    if (settingsKeys.contains("ambeGainDb") || force) {
        m_ambeEncoder.setGainDb(settings.m_ambeGainDb);
    }

    if (settingsKeys.contains("micEnable") || force)
    {
        if (settings.m_micEnable) {
            connect(&m_audioFifo, SIGNAL(dataReady()), this, SLOT(handleAudio()), Qt::UniqueConnection);
        } else {
            disconnect(&m_audioFifo, SIGNAL(dataReady()), this, SLOT(handleAudio()));
        }
    }

    m_settings = settings;
}

void DMRModSource::applyChannelSettings(int channelSampleRate, int channelFrequencyOffset, bool force)
{
    if ((channelSampleRate != m_channelSampleRate) || force) {
        m_carrierNco.setFreq(channelFrequencyOffset, channelSampleRate);
        m_modem.configure(channelSampleRate, Db::powerDb2mag(m_settings.m_gain));
        m_frameByteIdx = 33;
    } else if (channelFrequencyOffset != m_channelFrequencyOffset) {
        m_carrierNco.setFreq(channelFrequencyOffset, channelSampleRate);
    }

    m_channelSampleRate = channelSampleRate;
    m_channelFrequencyOffset = channelFrequencyOffset;
}
