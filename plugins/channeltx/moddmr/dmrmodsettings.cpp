///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2026 SDRangel-DMR-tx contributors                             //
///////////////////////////////////////////////////////////////////////////////////

#include <QDebug>
#include "util/simpleserializer.h"
#include "settings/serializable.h"
#include "audio/audiodevicemanager.h"
#include "dmrmodsettings.h"

DMRModSettings::DMRModSettings() :
    m_channelMarker(nullptr),
    m_rollupState(nullptr)
{
    resetToDefaults();
}

void DMRModSettings::resetToDefaults()
{
    m_inputFrequencyOffset = 0;
    m_gain = 0.0f;
    m_channelMute = false;
    m_colorCode = 1;
    m_srcId = 1234567;
    m_dstId = 9;
    m_groupCall = true;
    m_slot = 1;
    m_duplex = false;
    m_mode = 0;
    m_micEnable = true;
    m_micVolume = 1.0f;
    m_ambeGainDb = 0;
    m_audioDeviceName = AudioDeviceManager::m_defaultDeviceName;
    m_rgbColor = QColor(220, 120, 40).rgb();
    m_title = "DMR Modulator";
    m_streamIndex = 0;
    m_useReverseAPI = false;
    m_reverseAPIAddress = "127.0.0.1";
    m_reverseAPIPort = 8888;
    m_reverseAPIDeviceIndex = 0;
    m_reverseAPIChannelIndex = 0;
    m_workspaceIndex = 0;
    m_hidden = false;
}

QByteArray DMRModSettings::serialize() const
{
    SimpleSerializer s(1);
    s.writeS32(1, m_inputFrequencyOffset);
    s.writeReal(2, m_gain);
    s.writeBool(3, m_channelMute);
    s.writeU32(4, m_colorCode);
    s.writeU32(5, m_srcId);
    s.writeU32(6, m_dstId);
    s.writeBool(7, m_groupCall);
    s.writeU32(8, m_slot);
    s.writeBool(9, m_duplex);
    s.writeS32(10, m_mode);
    s.writeU32(11, m_rgbColor);
    s.writeString(12, m_title);
    if (m_channelMarker) {
        s.writeBlob(13, m_channelMarker->serialize());
    }
    s.writeS32(14, m_streamIndex);
    s.writeBool(15, m_useReverseAPI);
    s.writeString(16, m_reverseAPIAddress);
    s.writeU32(17, m_reverseAPIPort);
    s.writeU32(18, m_reverseAPIDeviceIndex);
    s.writeU32(19, m_reverseAPIChannelIndex);
    if (m_rollupState) {
        s.writeBlob(20, m_rollupState->serialize());
    }
    s.writeS32(21, m_workspaceIndex);
    s.writeBlob(22, m_geometryBytes);
    s.writeBool(23, m_hidden);
    s.writeBool(24, m_micEnable);
    s.writeReal(25, m_micVolume);
    s.writeS32(26, m_ambeGainDb);
    s.writeString(27, m_audioDeviceName);
    return s.final();
}

bool DMRModSettings::deserialize(const QByteArray& data)
{
    SimpleDeserializer d(data);

    if (!d.isValid())
    {
        resetToDefaults();
        return false;
    }

    if (d.getVersion() == 1)
    {
        QByteArray bytetmp;
        qint32 tmp;
        quint32 uintval;

        d.readS32(1, &tmp, 0);
        m_inputFrequencyOffset = tmp;
        d.readReal(2, &m_gain, 0.0f);
        d.readBool(3, &m_channelMute, false);
        d.readU32(4, &uintval, 1);
        m_colorCode = uintval;
        d.readU32(5, &uintval, 1234567);
        m_srcId = uintval;
        d.readU32(6, &uintval, 9);
        m_dstId = uintval;
        d.readBool(7, &m_groupCall, true);
        d.readU32(8, &uintval, 1);
        m_slot = uintval;
        d.readBool(9, &m_duplex, false);
        d.readS32(10, &m_mode, 0);
        d.readU32(11, &m_rgbColor, QColor(220, 120, 40).rgb());
        d.readString(12, &m_title, "DMR Modulator");
        if (m_channelMarker) {
            d.readBlob(13, &bytetmp);
            m_channelMarker->deserialize(bytetmp);
        }
        d.readS32(14, &m_streamIndex, 0);
        d.readBool(15, &m_useReverseAPI, false);
        d.readString(16, &m_reverseAPIAddress, "127.0.0.1");
        d.readU32(17, &uintval, 8888);
        m_reverseAPIPort = uintval;
        d.readU32(18, &uintval, 0);
        m_reverseAPIDeviceIndex = uintval;
        d.readU32(19, &uintval, 0);
        m_reverseAPIChannelIndex = uintval;
        if (m_rollupState) {
            d.readBlob(20, &bytetmp);
            m_rollupState->deserialize(bytetmp);
        }
        d.readS32(21, &m_workspaceIndex, 0);
        d.readBlob(22, &m_geometryBytes);
        d.readBool(23, &m_hidden, false);
        d.readBool(24, &m_micEnable, true);
        d.readReal(25, &m_micVolume, 1.0f);
        d.readS32(26, &m_ambeGainDb, 0);
        d.readString(27, &m_audioDeviceName, AudioDeviceManager::m_defaultDeviceName);
        return true;
    }

    resetToDefaults();
    return false;
}

QString DMRModSettings::getDebugString(const QStringList& settingsKeys, bool force) const
{
    (void) settingsKeys;
    (void) force;
    return QString("DMRModSettings");
}
