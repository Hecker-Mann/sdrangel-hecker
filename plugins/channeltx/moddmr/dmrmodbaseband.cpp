///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2026 SDRangel-DMR-tx contributors                             //
///////////////////////////////////////////////////////////////////////////////////

#include <QDebug>
#include "dsp/upchannelizer.h"
#include "dsp/dspcommands.h"
#include "dsp/dspengine.h"
#include "audio/audiodevicemanager.h"
#include "dmrmodbaseband.h"
#include "dmrmod.h"

MESSAGE_CLASS_DEFINITION(DMRModBaseband::MsgConfigureDMRModBaseband, Message)

DMRModBaseband::DMRModBaseband()
{
    m_sampleFifo.resize(SampleSourceFifo::getSizePolicy(48000));
    m_channelizer = new UpChannelizer(&m_source);

    QObject::connect(&m_sampleFifo, &SampleSourceFifo::dataRead, this, &DMRModBaseband::handleData, Qt::QueuedConnection);
    connect(&m_inputMessageQueue, SIGNAL(messageEnqueued()), this, SLOT(handleInputMessages()));
}

DMRModBaseband::~DMRModBaseband()
{
    DSPEngine::instance()->getAudioDeviceManager()->removeAudioSource(m_source.getAudioFifo());
    delete m_channelizer;
}

void DMRModBaseband::reset()
{
    QMutexLocker mutexLocker(&m_mutex);
    m_sampleFifo.reset();
}

void DMRModBaseband::setChannel(ChannelAPI *channel)
{
    m_source.setChannel(channel);
}

void DMRModBaseband::pull(const SampleVector::iterator& begin, unsigned int nbSamples)
{
    unsigned int part1Begin, part1End, part2Begin, part2End;
    m_sampleFifo.read(nbSamples, part1Begin, part1End, part2Begin, part2End);
    SampleVector& data = m_sampleFifo.getData();

    if (part1Begin != part1End) {
        std::copy(data.begin() + part1Begin, data.begin() + part1End, begin);
    }

    unsigned int shift = part1End - part1Begin;
    if (part2Begin != part2End) {
        std::copy(data.begin() + part2Begin, data.begin() + part2End, begin + shift);
    }
}

void DMRModBaseband::handleData()
{
    QMutexLocker mutexLocker(&m_mutex);
    SampleVector& data = m_sampleFifo.getData();
    unsigned int ipart1begin, ipart1end, ipart2begin, ipart2end;
    qreal rmsLevel, peakLevel;
    int numSamples;
    unsigned int remainder = m_sampleFifo.remainder();

    while ((remainder > 0) && (m_inputMessageQueue.size() == 0))
    {
        m_sampleFifo.write(remainder, ipart1begin, ipart1end, ipart2begin, ipart2end);
        if (ipart1begin != ipart1end) {
            processFifo(data, ipart1begin, ipart1end);
        }
        if (ipart2begin != ipart2end) {
            processFifo(data, ipart2begin, ipart2end);
        }
        remainder = m_sampleFifo.remainder();
    }

    m_source.getLevels(rmsLevel, peakLevel, numSamples);
    emit levelChanged(rmsLevel, peakLevel, numSamples);
}

void DMRModBaseband::processFifo(SampleVector& data, unsigned int iBegin, unsigned int iEnd)
{
    m_channelizer->prefetch(iEnd - iBegin);
    m_channelizer->pull(data.begin() + iBegin, iEnd - iBegin);
}

void DMRModBaseband::handleInputMessages()
{
    Message* message;
    while ((message = m_inputMessageQueue.pop()) != nullptr)
    {
        if (handleMessage(*message)) {
            delete message;
        }
    }
}

bool DMRModBaseband::handleMessage(const Message& cmd)
{
    if (MsgConfigureDMRModBaseband::match(cmd))
    {
        QMutexLocker mutexLocker(&m_mutex);
        MsgConfigureDMRModBaseband& cfg = (MsgConfigureDMRModBaseband&) cmd;
        applySettings(cfg.getSettingsKeys(), cfg.getSettings(), cfg.getForce());
        return true;
    }
    else if (DMRMod::MsgStartVoice::match(cmd))
    {
        m_source.startVoice();
        return true;
    }
    else if (DMRMod::MsgStopVoice::match(cmd))
    {
        m_source.stopVoice();
        return true;
    }
    else if (DSPSignalNotification::match(cmd))
    {
        QMutexLocker mutexLocker(&m_mutex);
        DSPSignalNotification& notif = (DSPSignalNotification&) cmd;
        m_sampleFifo.resize(SampleSourceFifo::getSizePolicy(notif.getSampleRate()));
        m_channelizer->setBasebandSampleRate(notif.getSampleRate());
        m_source.applyChannelSettings(m_channelizer->getChannelSampleRate(), m_channelizer->getChannelFrequencyOffset());
        return true;
    }
    return false;
}

void DMRModBaseband::applySettings(const QStringList& settingsKeys, const DMRModSettings& settings, bool force)
{
    if ((settingsKeys.contains("inputFrequencyOffset") && (settings.m_inputFrequencyOffset != m_settings.m_inputFrequencyOffset)) || force)
    {
        m_channelizer->setChannelization(48000, settings.m_inputFrequencyOffset);
        m_source.applyChannelSettings(m_channelizer->getChannelSampleRate(), m_channelizer->getChannelFrequencyOffset());
    }

    if ((settingsKeys.contains("audioDeviceName") && (settings.m_audioDeviceName != m_settings.m_audioDeviceName)) || force)
    {
        AudioDeviceManager *audioDeviceManager = DSPEngine::instance()->getAudioDeviceManager();
        int audioDeviceIndex = audioDeviceManager->getInputDeviceIndex(settings.m_audioDeviceName);
        audioDeviceManager->removeAudioSource(getAudioFifo());
        int audioSampleRate = audioDeviceManager->getInputSampleRate(audioDeviceIndex);

        if (m_source.getAudioSampleRate() != audioSampleRate) {
            m_source.applyAudioSampleRate(audioSampleRate);
        }

        if (settings.m_micEnable) {
            audioDeviceManager->addAudioSource(getAudioFifo(), getInputMessageQueue(), audioDeviceIndex);
        }
    }
    else if ((settingsKeys.contains("micEnable") && (settings.m_micEnable != m_settings.m_micEnable)) || force)
    {
        AudioDeviceManager *audioDeviceManager = DSPEngine::instance()->getAudioDeviceManager();
        int audioDeviceIndex = audioDeviceManager->getInputDeviceIndex(settings.m_audioDeviceName);

        if (settings.m_micEnable) {
            audioDeviceManager->addAudioSource(getAudioFifo(), getInputMessageQueue(), audioDeviceIndex);
        } else {
            audioDeviceManager->removeAudioSource(getAudioFifo());
        }
    }

    m_source.applySettings(settingsKeys, settings, force);
    m_settings = settings;
}

int DMRModBaseband::getChannelSampleRate() const
{
    return m_channelizer->getChannelSampleRate();
}
