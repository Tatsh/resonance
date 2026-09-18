#pragma once

#include <list>

#include "rnd/object.h"

namespace Rnd {

/** Rows in one transform. Three store the basis and the fourth stores the translation. */
constexpr int kXfmRowCount = 4;

/** Floats in one transform row. Three store x, y, and z, and the fourth is padding. */
constexpr int kXfmRowFloatCount = 4;

/**
 * Mix-in for an object with a local and a world transform.
 *
 * `Q23Rnd13Transformable` in the RTTI descriptor at `0x008ef770`, with `Rnd::Object` as a public
 * virtual base at offset 0. The subobject is 0xac bytes: the virtual-base pointer at `+0x00`,
 * mTransList at `+0x04`, two unrecovered fields at `+0x08` and `+0x0c`, the two transforms, the
 * origin, mDirty at `+0xa0`, mBillboard at `+0xa4`, and the vptr at `+0xa8`.
 * `Rnd::View` confirms the vptr offset by placing its Transformable subobject at `+0x30` and
 * writing that vptr to View + 0xd8.
 *
 * Each transform is four consecutive 16-byte rows of an x, a y, and a z float followed by
 * four bytes of padding, which is the layout the VU units read. The member titles come from the
 * text DumpText() writes, "localXfm:", "worldXfm:", "transList:", "billboard:", and " origin:".
 *
 * The class declares two virtuals of its own beyond the compiler-generated slot 0, SetBillboard
 * at slot 1 and UpdateWorldXfm at slot 2. Its remaining members are DumpText at `0x004f12e0`,
 * Save at `0x004f1a58`, Load at `0x004f1f18`, Copy at `0x004fce30`, AddTrans at `0x004f0838`, and
 * ClearTransList at `0x004f0a80`, none of which is reconstructed yet.
 */
class Transformable : public virtual Object {
public:
    virtual ~Transformable();

    /**
     * Report the transformable that positions this one.
     *
     * Walks the referrer list of the `Rnd::Object` subobject, casts each referrer to Transformable
     * and returns the first whose own mTransList includes this object, which makes the result the
     * parent in the transform hierarchy rather than a plain cast.
     *
     * @return The parent transformable, or null when no referrer positions this one.
     * @ghidraAddress 0x004f0770
     */
    Transformable *Parent();

    /**
     * Compose mWorldXfm from mLocalXfm and the parent's world transform.
     *
     * Vtable slot 2. Returns at once when nForce is clear, mDirty is clear, and the parent is
     * either absent or clean. Recurses over mTransList with the same call once it has recomposed.
     *
     * @param pParent The transformable this one hangs off, or null for a root.
     * @param nForce Non-zero to recompose even when nothing is marked dirty.
     * @ghidraAddress 0x004f0b18
     */
    virtual void UpdateWorldXfm(Transformable *pParent, int nForce);

    /**
     * Set the billboard mode and mark the transform dirty.
     *
     * Vtable slot 1.
     *
     * @param nBillboard The billboard mode.
     * @ghidraAddress 0x004fce08
     */
    virtual void SetBillboard(int nBillboard);

    /**
     * Set the origin row and mark the transform dirty.
     *
     * The row is copied as one 128-bit quadword, so pOrigin has to be 16-byte aligned.
     *
     * @param pOrigin Four floats, of which the first three are x, y, and z.
     * @ghidraAddress 0x004fce18
     */
    void SetOrigin(const float *pOrigin);

protected:
    /**
     * Adopt the transform of an owner this object is being detached from.
     *
     * Takes the owner's composed world transform as this object's local transform and recomposes,
     * which preserves the world position the owner was placing this object at, then takes the
     * owner's own local transform, billboard mode, and origin and recomposes again.
     *
     * The compiler inlined this at `0x00482c20` through `0x00482cd4` inside `Rnd::Mesh::Replace`,
     * its only call site, so no out-of-line body exists and the title is inferred. The alternative
     * reading is that the five transform fields were public and Mesh performed the sequence
     * itself; the image cannot separate the two, and this one preserves the narrower access.
     *
     * @param owner The transformable being detached from.
     */
    void AdoptXfmFrom(const Transformable &owner);

    // Declared in recovered offset order, with the access specifiers interleaved. Every member
    // below is protected because Rnd::Mesh reads and writes it directly: Mesh::Replace adopts a
    // departing owner's transform and Mesh::Load reads mBillboard and mOrigin for files below
    // version 3.
    float mLocalXfm[kXfmRowCount][kXfmRowFloatCount]; // +0x10
    float mWorldXfm[kXfmRowCount][kXfmRowFloatCount]; // +0x50
    float mOrigin[kXfmRowFloatCount];                 // +0x90
    // Set by every writer of a transform field and cleared once UpdateWorldXfm has recomposed.
    int mDirty;     // +0xa0
    int mBillboard; // +0xa4

private:
    std::list<Transformable *> mTransList; // +0x04
    int mUnknown08;                        // +0x08
    int mUnknown0c;                        // +0x0c
};

} // namespace Rnd
