#pragma once

/**
 * Owner of the art and sound assets the FreQ maker works from.
 *
 * `24MetFreqMakerAssetManager` in the RTTI descriptor at `0x0086f7a8`, a leaf class with no base.
 * Following the g++ 2.x layout for a class with no base, the vptr sits after the data members at
 * `+0x80`, so the object is 0x84 bytes. The two-entry vtable is at `0x007f0908`, which makes the
 * destructor the one virtual the class declares.
 *
 * This declaration is deliberately partial. The recovered layout is the shape of the teardown in
 * the destructor at `0x00250808`, which runs in this order.
 *
 *  - Two owned objects at `+0x04` and `+0x08` are released through slot 1 of each object's own
 *    vtable with an `__in_chrg` argument of 3, which is the deleting form.
 *  - A vector of 4-byte elements at `+0x74`, with its finish at `+0x78` and its end of storage at
 *    `+0x7c`, is deallocated.
 *  - A second vector of 4-byte elements at `+0x68` is deallocated.
 *  - A run of `std::list` members descending from `+0x60` through `+0x58` and below is cleared
 *    through the routine at `0x00255658`, and each dummy node is returned to the pool.
 *
 * The constructor at `0x00250b18` writes the vptr, zeroes `+0x00` through `+0x14`, and then runs
 * on for another 0x400 bytes of asset registration that is not recovered here. The classes of the
 * two owned objects and the element types of the two vectors and the lists are all undetermined,
 * so the whole span is recorded as reserved rather than typed.
 *
 * The one instance is created by the routine at `0x00255158`, which allocates exactly 0x84 bytes,
 * runs the constructor, and records the result in the global at `0x006a0f30` that shared() reads.
 * Creation and access are separate routines, so shared() does not construct on first use.
 *
 * The titles shared(), PollLoad(), and WaitForLoad() are inferred. No string in the image
 * identifies any of them.
 */
class MetFreqMakerAssetManager {
public:
    /**
     * Return the one instance, or null before the routine at `0x00255158` has created it.
     *
     * The compiler also emitted an out-of-line copy of this accessor at `0x00254950`.
     *
     * @return The instance.
     * @ghidraAddress 0x002551f0
     */
    static MetFreqMakerAssetManager *shared();

    /**
     * Release the owned assets, the two vectors, and the list run.
     *
     * @ghidraAddress 0x00250808
     */
    virtual ~MetFreqMakerAssetManager();

    /**
     * Advance the asset load by one step and report whether it has finished.
     *
     * A load already marked finished at `+0x10` reports success without doing work. Otherwise the
     * routine polls the loader at `+0x0c` and returns false while that loader is still running.
     *
     * @return True once every asset is resident.
     * @ghidraAddress 0x0024f798
     */
    bool PollLoad();

    /**
     * Block until PollLoad() reports the assets resident, waiting for vertical blank between
     * attempts.
     *
     * @ghidraAddress 0x00255200
     */
    void WaitForLoad();

private:
    // The 0x80-byte span the destructor walks. Its members are described in the class
    // documentation above and are not individually typed.
    unsigned char mUnknown00[0x80]; // +0x00
};
