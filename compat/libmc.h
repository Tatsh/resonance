// The game was built against Sony's official SDK, whose memory card API is spelled sceMc*. The
// open-source ps2sdk declares the same entry points as mc*, with no aliases, so this shim forwards
// the names rather than redeclaring anything. Reconstructed source keeps the official spelling,
// which is what the original wrote.
//
// Add ps2sdk/ee/rpc/memorycard/include to the include path, after this directory, so that the
// include below resolves to the real header. This file is build support and is not part of the
// reconstructed source.
#pragma once

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

// Every line above renames an entry point ps2sdk spells differently. This one is not a rename:
// ps2sdk models no slot-count entry point at all, so the declaration is the shim covering a Sony
// routine the open-source SDK lacks. The identification of 0x0053a920 is inferred from its port
// argument, its position in the libmc RPC neighbourhood, and every caller comparing the result
// against one.
extern "C" int sceMcGetSlotMax(int nPort);

// Nor is this one a rename. ps2sdk's mcInit() takes a server-type argument, and 0x005659e8 reads no
// argument register before writing a0, so the Sony entry point this forwards to takes none.
extern "C" int sceMcInit(void);
