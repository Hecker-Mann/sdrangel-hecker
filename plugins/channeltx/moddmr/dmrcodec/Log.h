/*
 * Stub logging for DMR codec (no-op in plugin context)
 */

#if !defined(DMRMOD_LOG_H)
#define DMRMOD_LOG_H

#include <cstdarg>

static inline void Log(int, const char*, ...) {}
static inline void LogDebug(const char*, ...) {}
static inline void LogMessage(const char*, ...) {}
static inline void LogInfo(const char*, ...) {}
static inline void LogWarning(const char*, ...) {}
static inline void LogError(const char*, ...) {}

#endif
