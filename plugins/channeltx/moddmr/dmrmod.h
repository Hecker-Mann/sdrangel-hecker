///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2026 SDRangel-DMR-tx contributors                             //
///////////////////////////////////////////////////////////////////////////////////

#ifndef PLUGINS_CHANNELTX_MODDMR_DMRMOD_H_
#define PLUGINS_CHANNELTX_MODDMR_DMRMOD_H_

#include <QRecursiveMutex>
#include <QNetworkRequest>
#include "dsp/basebandsamplesource.h"
#include "channel/channelapi.h"
#include "util/message.h"
#include "dmrmodsettings.h"

class QNetworkAccessManager;
class QNetworkReply;
class QThread;
class DeviceAPI;
class DMRModBaseband;

class DMRMod : public BasebandSampleSource, public ChannelAPI {
public:
    class MsgConfigureDMRMod : public Message {
        MESSAGE_CLASS_DECLARATION
    public:
        const DMRModSettings& getSettings() const { return m_settings; }
        const QStringList& getSettingsKeys() const { return m_settingsKeys; }
        bool getForce() const { return m_force; }
        static MsgConfigureDMRMod* create(const QStringList& settingsKeys, const DMRModSettings& settings, bool force)
        {
            return new MsgConfigureDMRMod(settingsKeys, settings, force);
        }
    private:
        DMRModSettings m_settings;
        QStringList m_settingsKeys;
        bool m_force;
        MsgConfigureDMRMod(const QStringList& settingsKeys, const DMRModSettings& settings, bool force) :
            Message(), m_settings(settings), m_settingsKeys(settingsKeys), m_force(force) {}
    };

    class MsgStartVoice : public Message {
        MESSAGE_CLASS_DECLARATION
    public:
        static MsgStartVoice* create() { return new MsgStartVoice(); }
    private:
        MsgStartVoice() : Message() {}
    };

    class MsgStopVoice : public Message {
        MESSAGE_CLASS_DECLARATION
    public:
        static MsgStopVoice* create() { return new MsgStopVoice(); }
    private:
        MsgStopVoice() : Message() {}
    };

    DMRMod(DeviceAPI *deviceAPI);
    virtual ~DMRMod();
    virtual void destroy() { delete this; }
    virtual void setDeviceAPI(DeviceAPI *deviceAPI);
    virtual DeviceAPI *getDeviceAPI() { return m_deviceAPI; }
    virtual void start();
    virtual void stop();
    virtual void pull(SampleVector::iterator& begin, unsigned int nbSamples);
    virtual void pushMessage(Message *msg) { m_inputMessageQueue.push(msg); }
    virtual QString getSourceName() { return objectName(); }
    virtual void getIdentifier(QString& id) { id = objectName(); }
    virtual QString getIdentifier() const { return objectName(); }
    virtual void getTitle(QString& title) { title = m_settings.m_title; }
    virtual qint64 getCenterFrequency() const { return m_settings.m_inputFrequencyOffset; }
    virtual void setCenterFrequency(qint64 frequency);
    virtual QByteArray serialize() const;
    virtual bool deserialize(const QByteArray& data);
    virtual int getNbSinkStreams() const { return 1; }
    virtual int getNbSourceStreams() const { return 0; }
    virtual int getStreamIndex() const { return m_settings.m_streamIndex; }
    virtual qint64 getStreamCenterFrequency(int streamIndex, bool sinkElseSource) const
    {
        (void) streamIndex;
        (void) sinkElseSource;
        return m_settings.m_inputFrequencyOffset;
    }

    virtual int webapiSettingsGet(SWGSDRangel::SWGChannelSettings& response, QString& errorMessage);
    virtual int webapiWorkspaceGet(SWGSDRangel::SWGWorkspaceInfo& response, QString& errorMessage);
    virtual int webapiSettingsPutPatch(bool force, const QStringList& channelSettingsKeys,
            SWGSDRangel::SWGChannelSettings& response, QString& errorMessage);
    virtual int webapiReportGet(SWGSDRangel::SWGChannelReport& response, QString& errorMessage);

    double getMagSq() const;

    static const char* const m_channelIdURI;
    static const char* const m_channelId;

private:
    DeviceAPI* m_deviceAPI;
    QThread *m_thread;
    DMRModBaseband* m_basebandSource;
    DMRModSettings m_settings;
    QRecursiveMutex m_settingsMutex;
    QNetworkAccessManager *m_networkManager;
    QNetworkRequest m_networkRequest;

    virtual bool handleMessage(const Message& cmd);
    void applySettings(const QStringList& settingsKeys, const DMRModSettings& settings, bool force = false);
};

#endif
