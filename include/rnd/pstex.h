#pragma once

#include "rnd/tex.h"

namespace Rnd {

/**
 * PlayStation 2 texture, which owns the GS residency of its mip levels.
 *
 * `Q23Rnd5PsTex` in the RTTI descriptor at `0x008efec0`, with `Rnd::Tex` as its one public base at
 * offset 0. The subclass extends the object well past the 0x58 bytes of the base; the recovered
 * accesses reach a vector at `+0x58`, a cached TEX0 register value at `+0x68`, and a GS slot table
 * at `+0x4a8`. The exact layout is not recovered, so no member is declared here.
 *
 * Binding waits for the asynchronous mip loads to finish. BindToGsSlot() polls PollAsyncMips() and
 * yields until every requested level has arrived, then merges the slot index into the cached TEX0
 * value and uploads whatever is not yet resident.
 *
 * Recovery is partial. The routines recovered so far are the bind at `0x00598000`, the mip upload
 * at `0x00597d50`, the upload that also builds the MIPTBP registers at `0x005982a0`, and the
 * pending-mip upload sweep at `0x00597e38`. None of their bodies is reconstructed.
 */
class PsTex : public Tex {
public:
    /**
     * Bind the texture to a sampler slot.
     *
     * @param nSlot The sampler slot, of which the low two bits select the GS sampler.
     * @return True once the texture is resident and bound.
     * @ghidraAddress 0x00598000
     */
    bool BindToGsSlot(int nSlot);

    /**
     * Upload one mip level to GS memory.
     *
     * @ghidraAddress 0x00597d50
     */
    void UploadBitmapMipToGs();

    /**
     * Upload a mip level and build the MIPTBP register values for it.
     *
     * @ghidraAddress 0x005982a0
     */
    void UploadMipAndBuildMipTbp();

    /**
     * Upload every mip level that has arrived but is not yet resident.
     *
     * @ghidraAddress 0x00597e38
     */
    void UploadPendingMips();
};

} // namespace Rnd
