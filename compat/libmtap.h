#ifndef LIBMTAP_H
#define LIBMTAP_H

// The game was built against Sony's official SDK, whose multitap API uses the sceMtap prefix. The
// open-source ps2sdk declares the same entry points under mtap* with no aliases. This shim forwards
// the names rather than redeclaring anything.
//
// Add ps2sdk/ee/rpc/multitap/include to the include path after this directory. The include below
// then resolves to the real header. This file is build support and is not part of the reconstructed
// source.

#include_next <libmtap.h>

#define sceMtapGetConnection mtapGetConnection
#define sceMtapInit mtapInit
#define sceMtapPortClose mtapPortClose
#define sceMtapPortOpen mtapPortOpen

#endif
