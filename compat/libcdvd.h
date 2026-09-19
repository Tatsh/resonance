#ifndef LIBCDVD_H
#define LIBCDVD_H

// The game calls Sony's official libcdvd. The open-source ps2sdk provides most of the same
// declarations under the same sce* names. This shim defers to it with #include_next and then adds
// only the blocking-mode constants ps2sdk does not define. Sony's SDK declares those two and
// ps2sdk instead takes a bare int for the mode argument.
//
// Nothing in the reconstruction depends on the numeric values below. Every use passes the constant
// straight back to the SDK by name. Treat the values as unverified.
// This file is build support for the open-source SDK, not part of the reconstructed source.

#include_next <libcdvd.h>

enum SceCdBlockingMode {
    SCECdBlock = 0,
    SCECdNonblock = 1
};

#endif
