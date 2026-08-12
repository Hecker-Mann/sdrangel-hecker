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
    SimpleSerializer s(1);
    if (!s.deserialize(data)) {
        return false;
    }

    int intval;
    quint32 uintval;

    s.readS32(1, &m_inputFrequencyOffset, 0);
    s.readReal(2, &m_gain, 0.0f);
    s.readBool(3, &m_channelMute, false);
    s.readU32(4, &uintval, 1);
    m_colorCode = uintval;
    s.readU32(5, &uintval, 1234567);
    m_srcId = uintval;
    s.readU32(6, &uintval, 9);
    m_dstId = uintval;
    s.readBool(7, &m_groupCall, true);
    s.readU32(8, &uintval, 1);
    m_slot = uintval;
    s.readBool(9, &m_duplex, false);
    s.readS32(10, &intval, 0);
    m_mode = intval;
    s.readU32(11, &m_rgbColor, QColor(220, 120, 40).rgb());
    s.readString(12, &m_title, "DMR Modulator");
    if (m_channelMarker) {
        QByteArray bytetmp;
        s.readBlob(13, &bytetmp);
        m_channelMarker->deserialize(bytetmp);
    }
    s.readS32(14, &m_streamIndex, 0);
    s.readBool(15, &m_useReverseAPI, false);
    s.readString(16, &m_reverseAPIAddress, "127.0.0.1");
    s.readU32(17, &m_reverseAPIPort, 8888);
    s.readU32(18, &m_reverseAPIDeviceIndex, 0);
    s.readU32(19, &m_reverseAPIChannelIndex, 0);
    if (m_rollupState) {
        QByteArray bytetmp;
        s.readBlob(20, &bytetmp);
        m_rollupState->deserialize(bytetmp);
    }
    s.readS32(21, &m_workspaceIndex, 0);
    s.readBlob(22, &m_geometryBytes);
    s.readBool(23, &m_hidden, false);
    s.readBool(24, &m_micEnable, true);
    s.readReal(25, &m_micVolume, 1.0f);
    s.readS32(26, &m_ambeGainDb, 0);
    s.readString(27, &m_audioDeviceName, AudioDeviceManager::m_defaultDeviceName);
    return true;
}

QString DMRModSettings::getDebugString(const QStringList& settingsKeys, bool force) const
{
    (void) settingsKeys;
    (void) force;
    return QString("DMRModSettings");
}
