///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2026 SDRangel-DMR-tx contributors                             //
///////////////////////////////////////////////////////////////////////////////////

#include "SWGChannelSettings.h"
#include "dmrmodwebapiadapter.h"

DMRModWebAPIAdapter::DMRModWebAPIAdapter()
{}

DMRModWebAPIAdapter::~DMRModWebAPIAdapter()
{}

int DMRModWebAPIAdapter::webapiSettingsGet(SWGSDRangel::SWGChannelSettings& response, QString& errorMessage)
{
    (void) response;
    errorMessage = "DMR modulator Web API settings not implemented";
    return 501;
}

int DMRModWebAPIAdapter::webapiSettingsPutPatch(bool force, const QStringList& channelSettingsKeys,
        SWGSDRangel::SWGChannelSettings& response, QString& errorMessage)
{
    (void) force;
    (void) channelSettingsKeys;
    (void) response;
    errorMessage = "DMR modulator Web API settings not implemented";
    return 501;
}
