///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2026 SDRangel-DMR-tx contributors                             //
///////////////////////////////////////////////////////////////////////////////////

#ifndef PLUGINS_CHANNELTX_MODDMR_DMRMODWEBAPIADAPTER_H_
#define PLUGINS_CHANNELTX_MODDMR_DMRMODWEBAPIADAPTER_H_

#include "channel/channelwebapiadapter.h"
#include "dmrmodsettings.h"

class DMRModWebAPIAdapter : public ChannelWebAPIAdapter {
public:
    DMRModWebAPIAdapter();
    virtual ~DMRModWebAPIAdapter();
    virtual int webapiSettingsGet(SWGSDRangel::SWGChannelSettings& response, QString& errorMessage);
    virtual int webapiSettingsPutPatch(bool force, const QStringList& channelSettingsKeys,
            SWGSDRangel::SWGChannelSettings& response, QString& errorMessage);

private:
    DMRModSettings m_settings;
};

#endif
