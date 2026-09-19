#ifndef EEKERNEL_H
#define EEKERNEL_H

// The game was built against Sony's official SDK, whose EE kernel header is <eekernel.h>. The
// open-source ps2sdk provides the same declarations under <kernel.h>. This shim forwards to it
// rather than redeclaring anything. Add ps2sdk/ee/kernel/include to the include path to use it.
// This file is build support for the open-source SDK, not part of the reconstructed source.

#include <kernel.h>

#endif
