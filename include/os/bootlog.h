#ifndef OS_BOOTLOG_H
#define OS_BOOTLOG_H

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Start the boot log on the memory card in port 0, slot 0.
 *
 * Loads the memory card modules from the disc ahead of the other IOP modules, starts libmc, and
 * recreates `/RESONANCE/BOOT.TXT` with a header that identifies the build. The log stays off when
 * no formatted card is present. Call it once, after InitIop().
 */
void BootLogOpen(void);

/**
 * Record a start-up checkpoint and write the pending log text to the card.
 *
 * The line is prefixed with the milliseconds since BootLogOpen(). While the game is still using
 * the card, the text stays pending until the next checkpoint or heartbeat.
 *
 * @param pszFormat A printf() format.
 */
void BootLogCheckpoint(const char *pszFormat, ...) __attribute__((format(printf, 1, 2)));

/**
 * Add formatted text to the pending log text without writing to the card.
 *
 * @param pszFormat A printf() format.
 * @param args The arguments for the format.
 */
void BootLogAppendV(const char *pszFormat, va_list args);

/**
 * Write a heartbeat line every five seconds, for the first ten minutes.
 *
 * Call it once per main loop iteration. Heartbeats that continue to arrive show that the game runs
 * while the screen stays black. Heartbeats that stop show where it hung.
 */
void BootLogHeartbeat(void);

/**
 * Turn off both display circuits and show a solid red background.
 *
 * Fatal() calls it after recording its message. The colour shows on any display mode the CRTC
 * already drives.
 */
void BootLogShowFatalScreen(void);

#ifdef __cplusplus
}
#endif

#endif
