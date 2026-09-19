// The game calls Sony's official libcdvd. The open-source ps2sdk provides most of the same
// declarations under the same sce* names, so this shim defers to it with #include_next and then
// adds only the blocking-mode constants ps2sdk does not define. Sony's SDK declares those two and
// ps2sdk instead takes a bare int for the mode argument.
//
// The numeric values below are not what the syntax check tests, and nothing in the reconstruction
// depends on them: every use passes the constant straight back to the SDK by name. Treat the values
// as unverified.
// This file is build support for the open-source SDK, not part of the reconstructed source.
#pragma once

#include_next <libcdvd.h>

enum SceCdBlockingMode {
    SCECdBlock = 0,
    SCECdNonblock = 1
};
