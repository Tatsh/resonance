#pragma once

#include "rnd/tex.h"

namespace Rnd {

/**
 * PlayStation 2 texture, which owns the GS residency of its mip levels.
 *
 * `Q23Rnd5PsTex` in the RTTI descriptor at `0x008efec0`, with `Rnd::Tex` as its one public base at
 * offset 0. The class factory allocates 0x4b0 bytes against the tag at `0x00831590`, so the
 * subclass occupies `+0x58` through `+0x4af`.
 *
 * The constructor at `0x00596f80` initialises only four of those fields, a vector at `+0x58` whose
 * elements are 8 bytes, and three words at `+0x4a0`, `+0x4a4`, and `+0x4a8`. Everything between
 * `+0x64` and `+0x49f` is left indeterminate, which makes that region a fixed array the upload path
 * fills rather than state the constructor owns. The cached TEX0 register value at `+0x68` that
 * BindToGsSlot() merges the sampler index into sits inside that region, so the array is a run of
 * per-mip GS register values. The destructor at `0x0059a558` releases `+0x4a0` through the scalar
 * path and walks the vector at `+0x58` in 8-byte steps.
 *
 * The class registration at `0x0059a888` installs the creator at `0x0059a770` over the texture
 * creator hook and then initialises a pool of 0x14 entries of 0x30 bytes, which is the GS slot
 * table the whole texture layer shares rather than per-texture state.
 *
 * No member is declared here, because only the four the constructor writes are pinned and a
 * partial member list would misstate the offsets of everything after the indeterminate region.
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
