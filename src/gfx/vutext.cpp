#include "gfx/gfxdevice.h"

// DMA chains that upload the VU microcode, recovered as extents rather than contents. The VU1
// chain runs from the start of `.vutext` at `0x00664ab0` through its end tag, 563 quadwords, and
// the VU0 chain follows it at `0x00666de0` through its end tag, 7 quadwords. Both extents come
// from walking the DMA tags in the image. The microcode words themselves are not recovered yet,
// so both chains start cleared. The 16-byte alignment is the DMA requirement.

GifQuadword g_vu1MicrocodeChain[563] __attribute__((aligned(16)));
GifQuadword g_vu0MicrocodeChain[7] __attribute__((aligned(16)));
