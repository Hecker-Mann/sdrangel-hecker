/*
 *   Copyright (C) 2015-2025 Jonathan Naylor G4KLX
 *   DMR sync helpers (extracted for moddmr plugin)
 */

#include "Sync.h"
#include "DMRDefines.h"

#include <cassert>

void CSync::addDMRDataSync(unsigned char* data, bool duplex)
{
	assert(data != nullptr);

	if (duplex) {
		for (unsigned int i = 0U; i < 7U; i++)
			data[i + 13U] = (data[i + 13U] & ~SYNC_MASK[i]) | BS_SOURCED_DATA_SYNC[i];
	} else {
		for (unsigned int i = 0U; i < 7U; i++)
			data[i + 13U] = (data[i + 13U] & ~SYNC_MASK[i]) | MS_SOURCED_DATA_SYNC[i];
	}
}

void CSync::addDMRAudioSync(unsigned char* data, bool duplex)
{
	assert(data != nullptr);

	if (duplex) {
		for (unsigned int i = 0U; i < 7U; i++)
			data[i + 13U] = (data[i + 13U] & ~SYNC_MASK[i]) | BS_SOURCED_AUDIO_SYNC[i];
	} else {
		for (unsigned int i = 0U; i < 7U; i++)
			data[i + 13U] = (data[i + 13U] & ~SYNC_MASK[i]) | MS_SOURCED_AUDIO_SYNC[i];
	}
}
