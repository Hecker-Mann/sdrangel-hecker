///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2026 SDRangel-DMR-tx contributors                             //
///////////////////////////////////////////////////////////////////////////////////

#ifndef PLUGINS_CHANNELTX_MODDMR_DMRMODBASEBAND_H_
#define PLUGINS_CHANNELTX_MODDMR_DMRMODBASEBAND_H_

#include <QObject>
#include <QRecursiveMutex>
#include "dsp/samplesourcefifo.h"
#include "util/message.h"
#include "util/messagequeue.h"
#include "audio/audiofifo.h"
#include "dmrmodsource.h"

class UpChannelizer;
class ChannelAPI;

class DMRModBaseband : public QObject
{
    Q_OBJECT
public:
    class MsgConfigureDMRModBaseband : public Message {
        MESSAGE_CLASS_DECLARATION
    public:
        const DMRModSettings& getSettings() const { return m_settings; }
        const QStringList& getSettingsKeys() const { return m_settingsKeys; }
        bool getForce() const { return m_force; }
        static MsgConfigureDMRModBaseband* create(const QStringList& settingsKeys, const DMRModSettings& settings, bool force)
        {
            return new MsgConfigureDMRModBaseband(settingsKeys, settings, force);
        }
    private:
        DMRModSettings m_settings;
        QStringList m_settingsKeys;
        bool m_force;
        MsgConfigureDMRModBaseband(const QStringList& settingsKeys, const DMRModSettings& settings, bool force) :
            Message(), m_settings(settings), m_settingsKeys(settingsKeys), m_force(force) {}
    };

    DMRModBaseband();
    ~DMRModBaseband();
    void reset();
    void pull(const SampleVector::iterator& begin, unsigned int nbSamples);
    MessageQueue *getInputMessageQueue() { return &m_inputMessageQueue; }
    double getMagSq() const { return m_source.getMagSq(); }
    void setChannel(ChannelAPI *channel);
    int getChannelSampleRate() const;
    AudioFifo *getAudioFifo() { return m_source.getAudioFifo(); }
    int getAudioSampleRate() const { return m_source.getAudioSampleRate(); }

signals:
    void levelChanged(qreal rmsLevel, qreal peakLevel, int numSamples);

private:
    SampleSourceFifo m_sampleFifo;
    UpChannelizer *m_channelizer;
    DMRModSource m_source;
    MessageQueue m_inputMessageQueue;
    DMRModSettings m_settings;
    QRecursiveMutex m_mutex;

    void processFifo(SampleVector& data, unsigned int iBegin, unsigned int iEnd);
    bool handleMessage(const Message& cmd);
    void applySettings(const QStringList& settingsKeys, const DMRModSettings& settings, bool force = false);

private slots:
    void handleInputMessages();
    void handleData();
};

#endif
