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
 * at slot 1 and UpdateWorldXfm at slot 2.
 *
 * `Rnd::Cam`, `Rnd::Mesh`, `Rnd::View`, and `Rnd::Arena` each place their next base immediately
 * after this subobject, at a distance of 0xb0 bytes from its start, which pins the size from
 * outside the class rather than from the highest store of a constructor. The 0xac the members
 * occupy rounds up to 0xb0 under the 16-byte alignment the transform rows impose.
 */
class Transformable : public virtual Object {
    // CollectChildren() in rnd/collectchildren.h walks mTransList directly, and the image has no
    // accessor for it.
    friend void CollectChildren(std::list<Object *> &objects, Transformable *pTransformable);

public:
    /**
     * Mode mBillboard selects, recovered from the name dumper at `0x004f26f0`.
     *
     * The low bits select an axis set and bit 0x80 adds the scaling variant of the same set, so
     * the fourteen values below are the whole set the dumper recognises. A value outside it
     * produces no text at all rather than a fallback.
     */
    enum Billboard {
        kBillboardNone = 0,            /*!< No billboarding. */
        kBillboardX = 1,               /*!< Face the camera about x. */
        kBillboardY = 2,               /*!< Face the camera about y. */
        kBillboardZ = 4,               /*!< Face the camera about z. */
        kBillboardXZ = 8,              /*!< Face the camera about x and z. */
        kBillboardXYZ = 16,            /*!< Face the camera about all three axes. */
        kBillboardSimpleXYZ = 32,      /*!< Face the camera with the cheaper orientation. */
        kBillboardLocalRotate = 64,    /*!< Rotate about the local axes instead. */
        kBillboardScaleX = 129,        /*!< kBillboardX with the scale preserved. */
        kBillboardScaleY = 130,        /*!< kBillboardY with the scale preserved. */
        kBillboardScaleZ = 132,        /*!< kBillboardZ with the scale preserved. */
        kBillboardScaleXZ = 136,       /*!< kBillboardXZ with the scale preserved. */
        kBillboardScaleXYZ = 144,      /*!< kBillboardXYZ with the scale preserved. */
        kBillboardScaleSimpleXYZ = 160 /*!< kBillboardSimpleXYZ with the scale preserved. */
    };

    virtual ~Transformable();

    /**
     * Append pTrans to mTransList and mark it dirty.
     *
     * Registers this object as a referrer of pTrans. A pTrans already in mTransList produces the
     * report "%s already in %s" and no insertion.
     *
     * @param pTrans The transformable to position from this one.
     * @ghidraAddress 0x004f0838
     */
    void AddTrans(Transformable *pTrans);

    /**
     * Erase pTrans from mTransList.
     *
     * Drops this object's reference on pTrans first. A pTrans absent from mTransList does nothing.
     *
     * @param pTrans The transformable to remove.
     * @ghidraAddress 0x004f09c0
     */
    void RemoveTrans(Transformable *pTrans);

    /**
     * Drop this object's reference on every mTransList entry and empty the list.
     *
     * @ghidraAddress 0x004f0a80
     */
    void ClearTransList();

    /**
     * Write a description of this object to sink.
     *
     * Writes both transforms a row at a time, then mTransList, mBillboard, and mOrigin. Produces
     * nothing at all when the dump level of sink is not positive. FailSink::Print() discards its
     * text in the shipped build, so the routine produces no output on this target.
     *
     * @param sink The diagnostic sink to write to.
     * @ghidraAddress 0x004f12e0
     */
    virtual void DumpText(FailSink &sink);

    /**
     * Write the revision, both transforms, mTransList, mBillboard, and mOrigin to stream.
     *
     * Each transform row writes only its first three floats, so the padding word is not stored.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x004f1a58
     */
    virtual void Save(Stream &stream);

    /**
     * Replace both transforms, mTransList, mBillboard, and mOrigin from stream.
     *
     * A revision above the one this build writes produces the report "Can't load new
     * Transformable". Revisions below 3 store a legacy billboard index the reader maps through a
     * table, and revisions below 1 store neither the billboard nor the origin.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x004f1f18
     */
    virtual void Load(Stream &stream);

    /**
     * Retarget every mTransList entry equal to pFrom at pTo.
     *
     * An entry that already equals pTo produces the report "%s already in %s" without stopping
     * the walk. An entry that ends up null is erased.
     *
     * @param pFrom The object being replaced.
     * @param pTo The replacement, or null.
     * @ghidraAddress 0x004f2510
     */
    virtual void Replace(Object *pFrom, Object *pTo);

    /**
     * Copy both transforms, mBillboard, and mOrigin from pSource.
     *
     * mDirty is not copied. AcquireTransRefs() sets it instead, so a copied object always
     * recomposes once.
     *
     * @param pSource The object to copy from.
     * @param nFlags The set of fields to copy; see kCopyChildLists.
     * @ghidraAddress 0x004fce30
     */
    virtual void Copy(const Object *pSource, unsigned nFlags);

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
     * Two independent arguments fix the return. The final exit at `0x004f0ca4` loads mDirty into
     * the return register and only then clears it, so the value handed back is the flag as it
     * stood on entry. Rnd::Cam's override at `0x004b1fa0` then calls this implementation, branches
     * on the result with `beq v0,zero`, runs its own projection rebuild only on the non-zero path,
     * and normalises its own answer to 1 or 0.
     *
     * @param pParent The transformable this one hangs off, or null for a root.
     * @param nForce Non-zero to recompose even when nothing is marked dirty.
     * @return Non-zero when the world transform was recomposed.
     * @ghidraAddress 0x004f0b18
     */
    virtual int UpdateWorldXfm(Transformable *pParent, int nForce);

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

    /**
     * Build the transform this object draws with and return it.
     *
     * With mBillboard clear the four mWorldXfm rows are copied out unchanged. With it set a
     * camera-facing orientation is built instead. Either way the result is the one shared scratch
     * buffer g_drawXfm, so it is valid only until the next call.
     *
     * @return The shared draw transform, four rows of four floats.
     * @ghidraAddress 0x004f0cc0
     */
    float *GetDrawXfm();

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

    /**
     * Drop this object's reference on every mTransList entry without emptying the list.
     *
     * Protected rather than private because every destructor of a derived class invokes it
     * directly, twenty call sites among them `Rnd::Mesh`, `Rnd::Cam`, `Rnd::View`, `Rnd::Light`,
     * and `Rnd::ParticleSys`.
     *
     * @ghidraAddress 0x004fcf18
     */
    void ReleaseTransRefs();

    // Declared in recovered offset order, with the access specifiers interleaved.

public:
    /**
     * Transform this object is positioned by, before the parent transform is applied.
     *
     * Public rather than protected because `Rnd::TransAnim::SetFrameSelf` at `0x004fd2c8` reads
     * all four rows out of its target and writes the interpolated rows back, and TransAnim derives
     * from Animatable and Drawable rather than from this class. A friend declaration would fit the
     * image equally well; public asserts the weaker of the two.
     *
     * +0x10
     */
    float mLocalXfm[kXfmRowCount][kXfmRowFloatCount];

    /**
     * Composed world transform.
     *
     * Public rather than protected because `Rnd::Mesh::Collide` reads it through a
     * `Transformable *` that need not be a mesh, and protected access cannot reach a member
     * through a pointer to the base type. The image exposes no accessor for it.
     *
     * +0x50
     */
    float mWorldXfm[kXfmRowCount][kXfmRowFloatCount];

protected:
    float mOrigin[kXfmRowFloatCount]; // +0x90

public:
    /**
     * Set by every writer of a transform field and cleared once UpdateWorldXfm has recomposed.
     *
     * Public for the same reason as mLocalXfm. `Rnd::TransAnim::SetFrameSelf` sets it on its
     * target after writing the interpolated transform, and `Rnd::Transformable::AddTrans` sets it
     * on the transformable being added.
     *
     * +0xa0
     */
    int mDirty;

protected:
    int mBillboard; // +0xa4

private:
    // 0x004fcf88
    // Marks this object dirty and registers it as a referrer of every mTransList
    // entry. Copy() invokes it and Load() inlines the same body.
    void AcquireTransRefs();

    std::list<Transformable *> mTransList; // +0x04
    int mUnknown08;                        // +0x08
    int mUnknown0c;                        // +0x0c
};

/**
 * Scratch transform GetDrawXfm() returns.
 *
 * One buffer serves every caller, which is why the value it returns survives only until the next
 * call.
 *
 * @ghidraAddress 0x007067e0
 */
extern float g_drawXfm[kXfmRowCount][kXfmRowFloatCount];

} // namespace Rnd
