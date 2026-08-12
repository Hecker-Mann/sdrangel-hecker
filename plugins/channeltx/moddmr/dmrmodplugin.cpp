///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2026 SDRangel-DMR-tx contributors                             //
///////////////////////////////////////////////////////////////////////////////////

#include <QtPlugin>
#include "plugin/pluginapi.h"

#ifndef SERVER_MODE
#include "dmrmodgui.h"
#endif
#include "dmrmod.h"
#include "dmrmodwebapiadapter.h"
#include "dmrmodplugin.h"

const PluginDescriptor DMRModPlugin::m_pluginDescriptor = {
    DMRMod::m_channelId,
    QStringLiteral("DMR Modulator"),
    QStringLiteral("1.0.0"),
    QStringLiteral("SDRangel-DMR-tx contributors"),
    QStringLiteral("https://github.com/SDRangel-DMR-tx"),
    true,
    QStringLiteral("https://github.com/SDRangel-DMR-tx")
};

DMRModPlugin::DMRModPlugin(QObject* parent) :
    QObject(parent),
    m_pluginAPI(nullptr)
{
}

const PluginDescriptor& DMRModPlugin::getPluginDescriptor() const
{
    return m_pluginDescriptor;
}

void DMRModPlugin::initPlugin(PluginAPI* pluginAPI)
{
    m_pluginAPI = pluginAPI;
    m_pluginAPI->registerTxChannel(DMRMod::m_channelIdURI, DMRMod::m_channelId, this);
}

void DMRModPlugin::createTxChannel(DeviceAPI *deviceAPI, BasebandSampleSource **bs, ChannelAPI **cs) const
{
    if (bs || cs)
    {
        DMRMod *instance = new DMRMod(deviceAPI);
        if (bs) {
            *bs = instance;
        }
        if (cs) {
            *cs = instance;
        }
    }
}

#ifdef SERVER_MODE
ChannelGUI* DMRModPlugin::createTxChannelGUI(DeviceUISet *deviceUISet, BasebandSampleSource *txChannel) const
{
    (void) deviceUISet;
    (void) txChannel;
    return nullptr;
}
#else
ChannelGUI* DMRModPlugin::createTxChannelGUI(DeviceUISet *deviceUISet, BasebandSampleSource *txChannel) const
{
    return DMRModGUI::create(m_pluginAPI, deviceUISet, txChannel);
}
#endif

ChannelWebAPIAdapter* DMRModPlugin::createChannelWebAPIAdapter() const
{
    return new DMRModWebAPIAdapter();
}
