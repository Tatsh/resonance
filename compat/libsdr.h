#ifndef LIBSDR_H
#define LIBSDR_H

// The game was built against Sony's official SDK, whose sound-driver RPC header declares its
// entry points without C linkage guards. The open-source ps2sdk header does the same, so a C++
// caller would mangle the names while the image holds C-linkage bodies. This shim gives the real
// header C linkage rather than redeclaring anything.
//
// Add ps2sdk/ee/rpc/sdr/include to the include path after this directory. The include below then
// resolves to the real header. This file is build support and is not part of the reconstructed
// source.

#ifdef __cplusplus
extern "C" {
#endif

#include_next <libsdr.h>

#ifdef __cplusplus
}
#endif

#endif
