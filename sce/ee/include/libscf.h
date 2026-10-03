#ifndef LIBSCF_H
#define LIBSCF_H

#include <libcdvd.h>

#ifdef __cplusplus
extern "C" {
#endif

/** System configuration library: the console's language and time settings. */

/** Language codes sceScfGetLanguage() reports. */
#define SCE_JAPANESE_LANGUAGE 0 /*!< Japanese. */
#define SCE_ENGLISH_LANGUAGE 1  /*!< English. */
#define SCE_FRENCH_LANGUAGE 2   /*!< French. */
#define SCE_SPANISH_LANGUAGE 3  /*!< Spanish. */
#define SCE_GERMAN_LANGUAGE 4   /*!< German. */
#define SCE_ITALIAN_LANGUAGE 5  /*!< Italian. */

/**
 * Report the console's language setting.
 *
 * A tool console reports a fixed default instead.
 *
 * @return One of the language codes.
 * @ghidraAddress PAL: 0x005a59f0
 */
int sceScfGetLanguage(void);

/**
 * Convert a clock from the console's Japan-time RTC to local time, in place.
 *
 * Applies the configured time-zone offset and summer-time setting. The binary does not include the
 * name, and its null-argument assertion records only the file, "libscf.c".
 *
 * @param pClock A clock sceCdReadClock() read.
 * @ghidraAddress NTSC-U/C: 0x005f3650
 * @ghidraAddress PAL: 0x005a6258
 */
void sceScfGetLocalTimefromRTC(sceCdCLOCK *pClock);

#ifdef __cplusplus
}
#endif

#endif
