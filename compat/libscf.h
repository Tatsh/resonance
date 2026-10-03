#ifndef LIBSCF_H
#define LIBSCF_H

// The game calls Sony's system-configuration library, libscf. The open-source ps2sdk does not
// provide an equivalent header. Only the declaration the reconstruction uses is shimmed here.
// This file is build support for the open-source SDK, not part of the reconstructed source.

#include <libcdvd.h>

#ifdef __cplusplus
extern "C" {
#endif

// System-configuration language codes, as sceScfGetLanguage() reports them.
#define SCE_JAPANESE_LANGUAGE 0
#define SCE_ENGLISH_LANGUAGE 1
#define SCE_FRENCH_LANGUAGE 2
#define SCE_SPANISH_LANGUAGE 3
#define SCE_GERMAN_LANGUAGE 4
#define SCE_ITALIAN_LANGUAGE 5

// Reports the console's language setting as one of the codes above. A tool console reports a
// fixed default instead.
int sceScfGetLanguage(void);

// Converts a clock read by sceCdReadClock() from the console's Japan-time RTC to local time, in
// place, by the configured time-zone offset and summer-time setting. The name is the one Sony's
// SDK documents. The binary does not include it, and its null-argument assert records only the
// file, "libscf.c".
void sceScfGetLocalTimefromRTC(sceCdCLOCK *pClock);

#ifdef __cplusplus
}
#endif

#endif
