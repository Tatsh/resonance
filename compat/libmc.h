#ifndef LIBMC_H
#define LIBMC_H

// The game was built against Sony's official SDK, whose memory card API uses the sceMc prefix. The
// open-source ps2sdk declares the same entry points under mc* with no aliases. This shim forwards
// the names rather than redeclaring anything, and reconstructed source retains the official prefix
// the original wrote.
//
// Add ps2sdk/ee/rpc/memorycard/include to the include path after this directory. The include below
// then resolves to the real header. This file is build support and is not part of the reconstructed
// source.

#include_next <libmc.h>

#define sceMcClose mcClose
#define sceMcDelete mcDelete
#define sceMcFlush mcFlush
#define sceMcFormat mcFormat
#define sceMcGetDir mcGetDir
#define sceMcGetEntSpace mcGetEntSpace
#define sceMcGetInfo mcGetInfo
#define sceMcMkDir mcMkDir
#define sceMcOpen mcOpen
#define sceMcRead mcRead
#define sceMcRename mcRename
#define sceMcSeek mcSeek
#define sceMcSync mcSync
#define sceMcUnformat mcUnformat
#define sceMcWrite mcWrite

#ifdef __cplusplus
extern "C" {
#endif

// Every line above renames an entry point ps2sdk writes differently. This one is not a rename. The
// image lists the entry point at 0x005659e8 as sceMcInitLibrary. The routine takes no arguments,
// while ps2sdk's mcInit() takes a server-type argument. The declaration therefore covers the Sony
// routine directly.
int sceMcInitLibrary(void);

#ifdef __cplusplus
}
#endif

#endif
