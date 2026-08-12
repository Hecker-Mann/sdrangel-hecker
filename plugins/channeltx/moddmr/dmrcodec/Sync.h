/*
 *   Copyright (C) 2015-2025 Jonathan Naylor G4KLX
 */

#if !defined(SYNC_H)
#define SYNC_H

class CSync
{
public:
	static void addDMRDataSync(unsigned char* data, bool duplex);
	static void addDMRAudioSync(unsigned char* data, bool duplex);
};

#endif
