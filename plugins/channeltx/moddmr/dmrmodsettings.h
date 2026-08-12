///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2026 SDRangel-DMR-tx contributors                             //
///////////////////////////////////////////////////////////////////////////////////

#ifndef PLUGINS_CHANNELTX_MODDMR_DMRMODSETTINGS_H_
#define PLUGINS_CHANNELTX_MODDMR_DMRMODSETTINGS_H_

#include <QByteArray>
#include <QString>
#include <QColor>
#include "dsp/dsptypes.h"

class Serializable;

struct DMRModSettings
{
    qint64 m_inputFrequencyOffset;
    Real m_gain;
    bool m_channelMute;
    unsigned int m_colorCode;
    unsigned int m_srcId;
    unsigned int m_dstId;
    bool m_groupCall;
    unsigned int m_slot;
    bool m_duplex;
    int m_mode; // 0 idle, 1 voice, 2 cal

    bool m_micEnable;
    Real m_micVolume;
    int m_ambeGainDb;
    QString m_audioDeviceName;

    quint32 m_rgbColor;
    QString m_title;
    Serializable *m_channelMarker;
    int m_streamIndex;
    bool m_useReverseAPI;
    QString m_reverseAPIAddress;
    uint16_t m_reverseAPIPort;
    uint16_t m_reverseAPIDeviceIndex;
    uint16_t m_reverseAPIChannelIndex;
    Serializable *m_rollupState;
    int m_workspaceIndex;
    QByteArray m_geometryBytes;
    bool m_hidden;

    DMRModSettings();
    void resetToDefaults();
    void setChannelMarker(Serializable *channelMarker) { m_channelMarker = channelMarker; }
    void setRollupState(Serializable *rollupState) { m_rollupState = rollupState; }
    QByteArray serialize() const;
    bool deserialize(const QByteArray& data);
    QString getDebugString(const QStringList& settingsKeys, bool force=false) const;
};

#endif
