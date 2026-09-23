#ifndef LIBSCF_H
#define LIBSCF_H

// The game calls Sony's system-configuration library, libscf. The open-source ps2sdk does not
// provide an equivalent header. Only the declaration the reconstruction uses is shimmed here.
// This file is build support for the open-source SDK, not part of the reconstructed source.

#include <libcdvd.h>

#ifdef __cplusplus
extern "C" {
#endif

// Converts a clock read by sceCdReadClock() from the console's Japan-time RTC to local time, in
// place, by the configured time-zone offset and summer-time setting. The name is the one Sony's
// SDK documents. The binary does not include it, and its null-argument assert records only the
// file, "libscf.c".
void sceScfGetLocalTimefromRTC(sceCdCLOCK *pClock);

#ifdef __cplusplus
}
#endif

#endif
