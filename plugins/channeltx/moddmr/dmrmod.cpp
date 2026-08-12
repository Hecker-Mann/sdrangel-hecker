///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2026 SDRangel-DMR-tx contributors                             //
///////////////////////////////////////////////////////////////////////////////////

#include <QDebug>
#include <QThread>
#include <QNetworkAccessManager>

#include "SWGChannelSettings.h"
#include "SWGWorkspaceInfo.h"
#include "SWGChannelReport.h"

#include "device/deviceapi.h"
#include "dmrmodbaseband.h"
#include "dmrmod.h"

MESSAGE_CLASS_DEFINITION(DMRMod::MsgConfigureDMRMod, Message)
MESSAGE_CLASS_DEFINITION(DMRMod::MsgStartVoice, Message)
MESSAGE_CLASS_DEFINITION(DMRMod::MsgStopVoice, Message)

const char* const DMRMod::m_channelIdURI = "sdrangel.channeltx.moddmr";
const char* const DMRMod::m_channelId = "DMRMod";

DMRMod::DMRMod(DeviceAPI *deviceAPI) :
    ChannelAPI(m_channelIdURI, ChannelAPI::StreamSingleSource),
    m_deviceAPI(deviceAPI),
    m_thread(nullptr),
    m_basebandSource(nullptr),
    m_networkManager(nullptr)
{
    setObjectName(m_channelId);
    m_thread = new QThread(this);
    m_basebandSource = new DMRModBaseband();
    m_basebandSource->moveToThread(m_thread);

    applySettings(QStringList(), m_settings, true);
    m_deviceAPI->addChannelSource(this);
    m_deviceAPI->addChannelSourceAPI(this);
    m_networkManager = new QNetworkAccessManager();
}

DMRMod::~DMRMod()
{
    m_deviceAPI->removeChannelSourceAPI(this);
    m_deviceAPI->removeChannelSource(this, true);
    stop();
    delete m_basebandSource;
    delete m_networkManager;
    delete m_thread;
}

void DMRMod::setDeviceAPI(DeviceAPI *deviceAPI)
{
    if (deviceAPI != m_deviceAPI)
    {
        m_deviceAPI->removeChannelSourceAPI(this);
        m_deviceAPI->removeChannelSource(this, false);
        m_deviceAPI = deviceAPI;
        m_deviceAPI->addChannelSource(this);
        m_deviceAPI->addChannelSourceAPI(this);
    }
}

void DMRMod::start()
{
    m_basebandSource->reset();
    m_thread->start();
}

void DMRMod::stop()
{
    m_thread->exit();
    m_thread->wait();
}

void DMRMod::pull(SampleVector::iterator& begin, unsigned int nbSamples)
{
    m_basebandSource->pull(begin, nbSamples);
}

bool DMRMod::handleMessage(const Message& cmd)
{
    if (MsgConfigureDMRMod::match(cmd))
    {
        MsgConfigureDMRMod& cfg = (MsgConfigureDMRMod&) cmd;
        applySettings(cfg.getSettingsKeys(), cfg.getSettings(), cfg.getForce());
        return true;
    }
  else if (MsgStartVoice::match(cmd))
    {
        m_basebandSource->getInputMessageQueue()->push(new MsgStartVoice());
        return true;
    }
    else if (MsgStopVoice::match(cmd))
    {
        m_basebandSource->getInputMessageQueue()->push(new MsgStopVoice());
        return true;
    }
    else if (DSPSignalNotification::match(cmd))
    {
        DSPSignalNotification& notif = (DSPSignalNotification&) cmd;
        DSPSignalNotification* rep = new DSPSignalNotification(notif);
        m_basebandSource->getInputMessageQueue()->push(rep);
        if (getMessageQueueToGUI()) {
            getMessageQueueToGUI()->push(new DSPSignalNotification(notif));
        }
        return true;
    }
    return false;
}

void DMRMod::setCenterFrequency(qint64 frequency)
{
    DMRModSettings settings = m_settings;
    settings.m_inputFrequencyOffset = frequency;
    applySettings(QStringList("inputFrequencyOffset"), settings, false);
}

void DMRMod::applySettings(const QStringList& settingsKeys, const DMRModSettings& settings, bool force)
{
    qDebug() << "DMRMod::applySettings";

    DMRModBaseband::MsgConfigureDMRModBaseband *msg = DMRModBaseband::MsgConfigureDMRModBaseband::create(settingsKeys, settings, force);
    m_basebandSource->getInputMessageQueue()->push(msg);

    if (force) {
        m_settings = settings;
    } else {
        m_settings = settings;
    }
}

QByteArray DMRMod::serialize() const
{
    return m_settings.serialize();
}

bool DMRMod::deserialize(const QByteArray& data)
{
    return m_settings.deserialize(data);
}

double DMRMod::getMagSq() const
{
    return m_basebandSource->getMagSq();
}

int DMRMod::webapiSettingsGet(SWGSDRangel::SWGChannelSettings& response, QString& errorMessage)
{
    (void) response;
    errorMessage = "DMR modulator Web API settings not implemented";
    return 501;
}

int DMRMod::webapiWorkspaceGet(SWGSDRangel::SWGWorkspaceInfo& response, QString& errorMessage)
{
    (void) response;
    errorMessage = "DMR modulator Web API workspace not implemented";
    return 501;
}

int DMRMod::webapiSettingsPutPatch(bool force, const QStringList& channelSettingsKeys,
        SWGSDRangel::SWGChannelSettings& response, QString& errorMessage)
{
    (void) force;
    (void) channelSettingsKeys;
    (void) response;
    errorMessage = "DMR modulator Web API settings not implemented";
    return 501;
}

int DMRMod::webapiReportGet(SWGSDRangel::SWGChannelReport& response, QString& errorMessage)
{
    (void) response;
    errorMessage = "DMR modulator Web API report not implemented";
    return 501;
}
