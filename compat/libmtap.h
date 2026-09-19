// The game was built against Sony's official SDK, whose multitap API is spelled sceMtap*. The
// open-source ps2sdk declares the same entry points as mtap*, with no aliases, so this shim
// forwards the names rather than redeclaring anything.
//
// Add ps2sdk/ee/rpc/multitap/include to the include path, after this directory, so that the include
// below resolves to the real header. This file is build support and is not
// part of the reconstructed source.
#pragma once

#include_next <libmtap.h>

#define sceMtapGetConnection mtapGetConnection
#define sceMtapInit mtapInit
#define sceMtapPortClose mtapPortClose
#define sceMtapPortOpen mtapPortOpen
