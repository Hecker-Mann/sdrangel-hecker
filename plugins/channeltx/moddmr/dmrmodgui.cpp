///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2026 SDRangel-DMR-tx contributors                             //
///////////////////////////////////////////////////////////////////////////////////

#include <QDebug>
#include "device/deviceuiset.h"
#include "plugin/pluginapi.h"
#include "gui/basicchannelsettingsdialog.h"
#include "gui/dialpopup.h"
#include "gui/dialogpositioner.h"
#include "gui/colormapper.h"
#include "maincore.h"
#include "dsp/dspcommands.h"
#include "util/db.h"

#include "ui_dmrmodgui.h"
#include "dmrmodgui.h"

DMRModGUI* DMRModGUI::create(PluginAPI* pluginAPI, DeviceUISet *deviceUISet, BasebandSampleSource *channelTx)
{
    return new DMRModGUI(pluginAPI, deviceUISet, channelTx);
}

void DMRModGUI::destroy()
{
    delete this;
}

DMRModGUI::DMRModGUI(PluginAPI* pluginAPI, DeviceUISet *deviceUISet, BasebandSampleSource *channelTx, QWidget* parent) :
    ChannelGUI(parent),
    ui(new Ui::DMRModGUI),
    m_pluginAPI(pluginAPI),
    m_deviceUISet(deviceUISet),
    m_channelMarker(this),
    m_deviceCenterFrequency(0),
    m_basebandSampleRate(48000),
    m_doApplySettings(true),
    m_dmrMod((DMRMod*) channelTx)
{
    setAttribute(Qt::WA_DeleteOnClose, true);
    m_helpURL = "plugins/channeltx/moddmr/readme.md";
    RollupContents *rollupContents = getRollupContents();
    ui->setupUi(rollupContents);
    setSizePolicy(rollupContents->sizePolicy());
    rollupContents->arrangeRollups();
    connect(rollupContents, SIGNAL(widgetRolled(QWidget*,bool)), this, SLOT(onWidgetRolled(QWidget*,bool)));
    connect(this, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(onMenuDialogCalled(const QPoint &)));

    m_dmrMod->setMessageQueueToGUI(getInputMessageQueue());
    connect(&MainCore::instance()->getMasterTimer(), SIGNAL(timeout()), this, SLOT(tick()));

    ui->deltaFrequencyLabel->setText(QString("%1f").arg(QChar(0x94, 0x03)));
    ui->deltaFrequency->setColorMapper(ColorMapper(ColorMapper::GrayGold));
    ui->deltaFrequency->setValueRange(false, 7, -9999999, 9999999);

    m_channelMarker.blockSignals(true);
    m_channelMarker.setColor(m_settings.m_rgbColor);
    m_channelMarker.setBandwidth(12500);
    m_channelMarker.setCenterFrequency(0);
    m_channelMarker.setTitle("DMR Modulator");
    m_channelMarker.setSourceOrSinkStream(false);
    m_channelMarker.blockSignals(false);
    m_channelMarker.setVisible(true);

    m_deviceUISet->addChannelMarker(&m_channelMarker);
    connect(&m_channelMarker, SIGNAL(changedByCursor()), this, SLOT(channelMarkerChangedByCursor()));
    connect(getInputMessageQueue(), SIGNAL(messageEnqueued()), this, SLOT(handleSourceMessages()));

    m_settings.setChannelMarker(&m_channelMarker);
    m_settings.setRollupState(&m_rollupState);

    displaySettings();
    makeUIConnections();
    applySettings(QStringList(), true);
    DialPopup::addPopupsToChildDials(this);
    m_resizer.enableChildMouseTracking();
}

DMRModGUI::~DMRModGUI()
{
    delete ui;
}

void DMRModGUI::resetToDefaults()
{
    m_settings.resetToDefaults();
    displaySettings();
    applySettings(QStringList(), true);
}

QByteArray DMRModGUI::serialize() const
{
    return m_settings.serialize();
}

bool DMRModGUI::deserialize(const QByteArray& data)
{
    if (m_settings.deserialize(data)) {
        displaySettings();
        applySettings(QStringList(), true);
        return true;
    }
    resetToDefaults();
    return false;
}

void DMRModGUI::blockApplySettings(bool block)
{
    m_doApplySettings = !block;
}

void DMRModGUI::applySettings(const QStringList& settingsKeys, bool force)
{
    if (!m_doApplySettings) {
        return;
    }

    DMRMod::MsgConfigureDMRMod* msg = DMRMod::MsgConfigureDMRMod::create(settingsKeys, m_settings, force);
    m_dmrMod->getInputMessageQueue()->push(msg);
}

void DMRModGUI::displaySettings()
{
    m_channelMarker.blockSignals(true);
    m_channelMarker.setCenterFrequency(m_settings.m_inputFrequencyOffset);
    m_channelMarker.setTitle(m_settings.m_title);
    m_channelMarker.blockSignals(false);
    m_channelMarker.setColor(m_settings.m_rgbColor);

    setTitleColor(m_settings.m_rgbColor);
    setWindowTitle(m_channelMarker.getTitle());
    setTitle(m_channelMarker.getTitle());
    updateIndexLabel();

    blockApplySettings(true);
    ui->deltaFrequency->setValue(m_channelMarker.getCenterFrequency());
    ui->gain->setValue((int) (m_settings.m_gain * 10.0f));
    ui->gainText->setText(QString("%1 dB").arg(m_settings.m_gain, 0, 'f', 1));
    ui->colorCode->setValue(m_settings.m_colorCode);
    ui->srcId->setValue(m_settings.m_srcId);
    ui->dstId->setValue(m_settings.m_dstId);
    ui->groupCall->setChecked(m_settings.m_groupCall);
    ui->slot->setValue(m_settings.m_slot == 2 ? 2 : 1);
    ui->mode->setCurrentIndex(m_settings.m_mode);
    ui->duplex->setChecked(m_settings.m_duplex);
    ui->micEnable->setChecked(m_settings.m_micEnable);
    ui->micVolume->setValue((int) (m_settings.m_micVolume * 10.0f));
    ui->micVolumeText->setText(QString("%1x").arg(m_settings.m_micVolume, 0, 'f', 1));
    ui->ambeGain->setValue(m_settings.m_ambeGainDb);
    ui->channelMute->setChecked(m_settings.m_channelMute);
    getRollupContents()->restoreState(m_rollupState);
    updateAbsoluteCenterFrequency();
    blockApplySettings(false);
}

void DMRModGUI::updateAbsoluteCenterFrequency()
{
    m_channelMarker.setCenterFrequency(m_settings.m_inputFrequencyOffset);
}

bool DMRModGUI::handleMessage(const Message& message)
{
    if (DMRMod::MsgConfigureDMRMod::match(message))
    {
        const DMRMod::MsgConfigureDMRMod& cfg = (const DMRMod::MsgConfigureDMRMod&) message;
        m_settings = cfg.getSettings();
        blockApplySettings(true);
        m_channelMarker.updateSettings(static_cast<const ChannelMarker*>(m_settings.m_channelMarker));
        displaySettings();
        blockApplySettings(false);
        return true;
    }
    else if (DSPSignalNotification::match(message))
    {
        const DSPSignalNotification& notif = (const DSPSignalNotification&) message;
        m_deviceCenterFrequency = notif.getCenterFrequency();
        m_basebandSampleRate = notif.getSampleRate();
        ui->deltaFrequency->setValueRange(false, 7, -m_basebandSampleRate/2, m_basebandSampleRate/2);
        updateAbsoluteCenterFrequency();
        return true;
    }
    return false;
}

void DMRModGUI::handleSourceMessages()
{
    Message* message;
    while ((message = getInputMessageQueue()->pop()) != nullptr)
    {
        if (handleMessage(*message)) {
            delete message;
        }
    }
}

void DMRModGUI::channelMarkerChangedByCursor()
{
    ui->deltaFrequency->setValue(m_channelMarker.getCenterFrequency());
    m_settings.m_inputFrequencyOffset = m_channelMarker.getCenterFrequency();
    applySettings(QStringList("inputFrequencyOffset"));
}

void DMRModGUI::on_deltaFrequency_changed(qint64 value)
{
    m_channelMarker.setCenterFrequency(value);
    m_settings.m_inputFrequencyOffset = value;
    updateAbsoluteCenterFrequency();
    applySettings(QStringList("inputFrequencyOffset"));
}

void DMRModGUI::on_gain_valueChanged(int value)
{
    m_settings.m_gain = value / 10.0f;
    ui->gainText->setText(QString("%1 dB").arg(m_settings.m_gain, 0, 'f', 1));
    applySettings(QStringList("gain"));
}

void DMRModGUI::on_colorCode_valueChanged(int value)
{
    m_settings.m_colorCode = value;
    applySettings(QStringList("colorCode"));
}

void DMRModGUI::on_srcId_valueChanged(int value)
{
    m_settings.m_srcId = value;
    applySettings(QStringList("srcId"));
}

void DMRModGUI::on_dstId_valueChanged(int value)
{
    m_settings.m_dstId = value;
    applySettings(QStringList("dstId"));
}

void DMRModGUI::on_groupCall_toggled(bool checked)
{
    m_settings.m_groupCall = checked;
    applySettings(QStringList("groupCall"));
}

void DMRModGUI::on_slot_valueChanged(int value)
{
    m_settings.m_slot = value;
    applySettings(QStringList("slot"));
}

void DMRModGUI::on_channelMute_toggled(bool checked)
{
    m_settings.m_channelMute = checked;
    applySettings(QStringList("channelMute"));
}

void DMRModGUI::on_mode_currentIndexChanged(int index)
{
    m_settings.m_mode = index;
    applySettings(QStringList("mode"));
}

void DMRModGUI::on_duplex_toggled(bool checked)
{
    m_settings.m_duplex = checked;
    applySettings(QStringList("duplex"));
}

void DMRModGUI::on_micEnable_toggled(bool checked)
{
    m_settings.m_micEnable = checked;
    applySettings(QStringList("micEnable"));
}

void DMRModGUI::on_micVolume_valueChanged(int value)
{
    m_settings.m_micVolume = value / 10.0f;
    ui->micVolumeText->setText(QString("%1x").arg(m_settings.m_micVolume, 0, 'f', 1));
    applySettings(QStringList("micVolume"));
}

void DMRModGUI::on_ambeGain_valueChanged(int value)
{
    m_settings.m_ambeGainDb = value;
    applySettings(QStringList("ambeGainDb"));
}

void DMRModGUI::on_txButton_toggled(bool checked)
{
    if (checked) {
        m_dmrMod->getInputMessageQueue()->push(DMRMod::MsgStartVoice::create());
    } else {
        m_dmrMod->getInputMessageQueue()->push(DMRMod::MsgStopVoice::create());
    }
}

void DMRModGUI::onWidgetRolled(QWidget* widget, bool rollDown)
{
    (void) widget;
    (void) rollDown;
    getRollupContents()->saveState(m_rollupState);
    applySettings(QStringList(), false);
}

void DMRModGUI::onMenuDialogCalled(const QPoint& p)
{
    if (m_contextMenuType == ContextMenuType::ContextMenuChannelSettings)
    {
        BasicChannelSettingsDialog dialog(&m_channelMarker, this);
        dialog.setUseReverseAPI(m_settings.m_useReverseAPI);
        dialog.setReverseAPIAddress(m_settings.m_reverseAPIAddress);
        dialog.setReverseAPIPort(m_settings.m_reverseAPIPort);
        dialog.setReverseAPIDeviceIndex(m_settings.m_reverseAPIDeviceIndex);
        dialog.setReverseAPIChannelIndex(m_settings.m_reverseAPIChannelIndex);
        dialog.setDefaultTitle(m_displayedName);
        dialog.move(p);
        new DialogPositioner(&dialog, false);
        dialog.exec();

        m_settings.m_rgbColor = m_channelMarker.getColor().rgb();
        m_settings.m_title = m_channelMarker.getTitle();
        m_settings.m_useReverseAPI = dialog.useReverseAPI();
        m_settings.m_reverseAPIAddress = dialog.getReverseAPIAddress();
        m_settings.m_reverseAPIPort = dialog.getReverseAPIPort();
        m_settings.m_reverseAPIDeviceIndex = dialog.getReverseAPIDeviceIndex();
        m_settings.m_reverseAPIChannelIndex = dialog.getReverseAPIChannelIndex();

        setWindowTitle(m_settings.m_title);
        setTitle(m_channelMarker.getTitle());
        setTitleColor(m_settings.m_rgbColor);

        applySettings(QStringList({"rgbColor", "title", "useReverseAPI", "reverseAPIAddress",
            "reverseAPIPort", "reverseAPIDeviceIndex", "reverseAPIChannelIndex"}));
    }

    resetContextMenuType();
}

void DMRModGUI::leaveEvent(QEvent* event)
{
    m_channelMarker.setHighlighted(false);
    ChannelGUI::leaveEvent(event);
}

void DMRModGUI::enterEvent(EnterEventType* event)
{
    m_channelMarker.setHighlighted(true);
    ChannelGUI::enterEvent(event);
}

void DMRModGUI::tick()
{
    double powDb = CalcDb::dbPower(m_dmrMod->getMagSq());
    m_channelPowerDbAvg(powDb);
    ui->channelPower->setText(tr("%1 dB").arg(m_channelPowerDbAvg.asDouble(), 0, 'f', 1));
}

void DMRModGUI::makeUIConnections()
{
    QObject::connect(ui->deltaFrequency, &ValueDialZ::changed, this, &DMRModGUI::on_deltaFrequency_changed);
    QObject::connect(ui->gain, &QDial::valueChanged, this, &DMRModGUI::on_gain_valueChanged);
    QObject::connect(ui->colorCode, &QSpinBox::valueChanged, this, &DMRModGUI::on_colorCode_valueChanged);
    QObject::connect(ui->srcId, &QSpinBox::valueChanged, this, &DMRModGUI::on_srcId_valueChanged);
    QObject::connect(ui->dstId, &QSpinBox::valueChanged, this, &DMRModGUI::on_dstId_valueChanged);
    QObject::connect(ui->groupCall, &QCheckBox::toggled, this, &DMRModGUI::on_groupCall_toggled);
    QObject::connect(ui->slot, QOverload<int>::of(&QSpinBox::valueChanged), this, &DMRModGUI::on_slot_valueChanged);
    QObject::connect(ui->mode, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &DMRModGUI::on_mode_currentIndexChanged);
    QObject::connect(ui->duplex, &QCheckBox::toggled, this, &DMRModGUI::on_duplex_toggled);
    QObject::connect(ui->micEnable, &QCheckBox::toggled, this, &DMRModGUI::on_micEnable_toggled);
    QObject::connect(ui->micVolume, &QDial::valueChanged, this, &DMRModGUI::on_micVolume_valueChanged);
    QObject::connect(ui->ambeGain, &QSpinBox::valueChanged, this, &DMRModGUI::on_ambeGain_valueChanged);
    QObject::connect(ui->channelMute, &QToolButton::toggled, this, &DMRModGUI::on_channelMute_toggled);
    QObject::connect(ui->txButton, &QToolButton::toggled, this, &DMRModGUI::on_txButton_toggled);
}
