#pragma once

#include <list>

#include "rnd/object.h"

namespace Rnd {
class Dbg;
class Stream;
} // namespace Rnd

namespace Rnd {

/**
 * Mix-in for an object that draws itself and then a list of other drawables.
 *
 * Its RTTI descriptor is at `0x008ef320`. It has `Rnd::Object` as a public virtual base at offset
 * 0. The subobject is 0x14 bytes. The compiler places the virtual-base pointer at `+0x00` and,
 * following the g++ 2.x layout for a class with no non-virtual base, the vptr at `+0x10`. The
 * declared members therefore occupy `+0x04` through `+0x0f`. For a standalone Drawable the
 * `Rnd::Object` subobject sits at `+0x14`.
 *
 * Two vtables belong to the class. The four-entry table at `0x00825498` is addressed by
 * the vptr at `+0x10` and stores the three virtuals declared here. The nine-entry table at
 * `0x008254c0` is addressed by the `Rnd::Object` subobject vptr and stores the overrides of the
 * `Rnd::Object` virtuals, each with a `-0x14` adjustment back to the Drawable subobject.
 *
 * Every entry of mDraws registers this object as a referrer through Rnd::Object::AddRef(), so a
 * drawable that goes away is removed from its parents' lists by Replace() rather than leaving a
 * stale pointer.
 */
class Drawable : public virtual Object {
public:
    /**
     * Construct a visible drawable with no children.
     *
     * @ghidraAddress NTSC-U/C: 0x005066f8
     * @ghidraAddress PAL: 0x005455b8
     */
    Drawable();

    /**
     * Drop this object's references on its children.
     *
     * @ghidraAddress NTSC-U/C: 0x00506590
     * @ghidraAddress PAL: 0x00545450
     */
    virtual ~Drawable();

    /**
     * Draw this object followed by each of its children.
     *
     * Draws nothing when mShowing is clear, and skips the children when DrawShowing() reports that
     * the subtree is invisible.
     *
     * @ghidraAddress NTSC-U/C: 0x00506920
     * @ghidraAddress PAL: 0x005457e8
     */
    void Draw();

    /**
     * Report the drawable that draws this one.
     *
     * Walks the referrer list of the `Rnd::Object` subobject, casts each referrer to Drawable and
     * returns the first whose own mDraws list includes this object, which makes the result the
     * parent in the draw hierarchy rather than a plain cast.
     *
     * @return The parent drawable, or null when no referrer draws this one.
     * @ghidraAddress NTSC-U/C: 0x00502d78
     * @ghidraAddress PAL: 0x00541ba0
     */
    Drawable *Parent();

    /**
     * Insert pDraw into mDraws in front of pBefore.
     *
     * Registers this object as a referrer of pDraw. A pBefore that is absent from mDraws appends
     * instead. A pDraw already in mDraws produces the report "%s already in %s" and no insertion.
     *
     * @param pDraw The drawable to add.
     * @param pBefore The drawable to insert in front of.
     * @ghidraAddress NTSC-U/C: 0x00503188
     * @ghidraAddress PAL: 0x00541fb0
     */
    void AddDraw(Drawable *pDraw, Drawable *pBefore);

    /**
     * Insert pDraw at the front of mDraws.
     *
     * @param pDraw The drawable to add.
     * @ghidraAddress NTSC-U/C: 0x005064e0
     * @ghidraAddress PAL: 0x005453a0
     */
    void AddDraw(Drawable *pDraw);

    /**
     * Erase pDraw from mDraws.
     *
     * Drops this object's reference on pDraw first. A pDraw absent from mDraws does nothing.
     *
     * @param pDraw The drawable to remove.
     * @ghidraAddress NTSC-U/C: 0x00503360
     * @ghidraAddress PAL: 0x00542188
     */
    void RemoveDraw(Drawable *pDraw);

    /**
     * Empty mDraws, dropping this object's reference on every entry.
     *
     * @ghidraAddress NTSC-U/C: 0x00503420
     * @ghidraAddress PAL: 0x00542248
     */
    void RemoveAllDraws();

    /**
     * Move pDraw nSteps places later in mDraws, or earlier for a negative count.
     *
     * The move stops at either end of the list, and a pDraw absent from mDraws does nothing. No
     * reference changes hands. FreqAppearanceDetail reorders the parts of an avatar with it. The
     * title is inferred.
     *
     * @param pDraw The drawable to move.
     * @param nSteps The number of places to move it.
     * @ghidraAddress NTSC-U/C: 0x005034b8
     * @ghidraAddress PAL: 0x005422e0
     */
    void MoveDraw(Drawable *pDraw, int nSteps);

    /**
     * Set whether this object draws at all.
     *
     * Drawable vtable slot 1.
     *
     * @param nShowing Non-zero to draw.
     * @ghidraAddress NTSC-U/C: 0x005066d8
     * @ghidraAddress PAL: 0x00545598
     */
    virtual void SetShowing(int nShowing);

    /**
     * Set whether this object and every drawable beneath it in mDraws draw at all.
     *
     * Runs SetShowing() on this object and then recurses over mDraws. Defined in the header. The
     * one out-of-line copy is emitted in FreqAppearance's translation unit, where
     * FreqAppearance::AttachToBurnSlot() expands the first level of the recursion.
     *
     * @param nShowing Non-zero to draw.
     * @ghidraAddress NTSC-U/C: 0x00174480
     * @ghidraAddress PAL: 0x00176e58
     */
    void SetShowingRecursive(int nShowing) {
        SetShowing(nShowing);
        for (std::list<Drawable *>::iterator it = mDraws.begin(); it != mDraws.end(); ++it) {
            (*it)->SetShowingRecursive(nShowing);
        }
    }

    /**
     * Set whether this object draws with its highlight treatment.
     *
     * Drawable vtable slot 2.
     *
     * @param nHighlight Non-zero to highlight.
     * @ghidraAddress NTSC-U/C: 0x00506c88
     * @ghidraAddress PAL: 0x00545b50
     */
    virtual void SetHighlight(int nHighlight);

    /**
     * Report the highlight flag.
     *
     * @return Non-zero when this object draws with its highlight treatment.
     * @ghidraAddress NTSC-U/C: 0x005066e0
     * @ghidraAddress PAL: 0x005455a0
     */
    int GetHighlight() const {
        return mHighlight;
    }

    /**
     * Report the showing flag.
     *
     * The out-of-line copy has no callers. Rnd::LodMesh::Draw() inlines it.
     *
     * @return Non-zero when this object draws at all.
     * @ghidraAddress NTSC-U/C: 0x00506520
     * @ghidraAddress PAL: 0x005453e0
     */
    int GetShowing() const {
        return mShowing;
    }

    /**
     * Resolve a registry key to a drawable.
     *
     * A key that resolves to an object of another class produces null. The image has no
     * caller.
     *
     * @param name The registry key to resolve.
     * @return The drawable, or null.
     * @ghidraAddress NTSC-U/C: 0x00506528
     * @ghidraAddress PAL: 0x005453e8
     */
    static Drawable *Find(const HxStr &name);

    /**
     * Report the child list.
     *
     * @return The list of drawables this object draws after itself.
     * @ghidraAddress NTSC-U/C: 0x005066e8
     * @ghidraAddress PAL: 0x005455a8
     */
    std::list<Drawable *> &GetDraws() {
        return mDraws;
    }

    /**
     * Write a description of this object to sink.
     *
     * Writes mShowing, mHighlight, and the mDraws list, and produces nothing at all when the dump
     * level of sink is not positive. Dbg::Print() discards its text in the shipped build. The
     * routine therefore produces no output on this target either way.
     *
     * @param sink The diagnostic sink to write to.
     * @ghidraAddress NTSC-U/C: 0x005069a8
     * @ghidraAddress PAL: 0x00545870
     */
    virtual void DumpText(Dbg &sink);

    /**
     * Write the revision, the showing flag as one byte, and the draw list.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x00506b28
     * @ghidraAddress PAL: 0x005459f0
     */
    virtual void Save(Stream &stream);

    /**
     * Point every mDraws entry that is pFrom at pTo instead, moving the reference, and drop the
     * entries left null.
     *
     * An entry already equal to pTo is reported through the debug sink.
     *
     * @param pFrom The object to replace.
     * @param pTo The replacement, which only counts when it is a Drawable.
     * @ghidraAddress NTSC-U/C: 0x00502e40
     * @ghidraAddress PAL: 0x00541c68
     */
    virtual void Replace(Object *pFrom, Object *pTo);

    /**
     * Take the showing flag, and with kCopyChildLists the draw list, from another drawable.
     *
     * The references on the old and new draw lists are released and taken around the copy.
     *
     * @param pSource The drawable to copy from.
     * @param nFlags The copy flags.
     * @ghidraAddress NTSC-U/C: 0x00506a88
     * @ghidraAddress PAL: 0x00545950
     */
    virtual void Copy(const Object *pSource, unsigned nFlags);

    /**
     * Read what Save() writes, replacing the draw list and moving its references.
     *
     * A revision newer than this build supports is reported and aborts the load.
     *
     * @param stream The stream to read from.
     * @ghidraAddress NTSC-U/C: 0x00503020
     * @ghidraAddress PAL: 0x00541e48
     */
    virtual void Load(Stream &stream);

    // Declared in recovered offset order. Protected because Rnd::Mesh derives from this class and
    // Mesh::FindCollisions tests this flag at its top.
    // Public because the drawable show commands read it with no accessor in the image.
    int mShowing; // +0x04

protected:
    /**
     * Draw this object alone.
     *
     * Drawable vtable slot 3. The base implementation draws nothing and reports that the children
     * are still to be drawn. Only Draw() invokes it.
     *
     * @return Non-zero when the children are to be drawn as well.
     * @ghidraAddress NTSC-U/C: 0x005066f0
     * @ghidraAddress PAL: 0x005455b0
     */
    virtual int DrawShowing();

    /**
     * Drop this object's reference on every mDraws entry without emptying the list.
     *
     * Every derived destructor invokes this before its own teardown.
     *
     * @ghidraAddress NTSC-U/C: 0x00506ba8
     * @ghidraAddress PAL: 0x00545a70
     */
    void ReleaseDrawsRefs();

private:
    /**
     * Only Copy() and Load() invoke this.
     *
     * @ghidraAddress NTSC-U/C: 0x00506c18
     * @ghidraAddress PAL: 0x00545ae0
     */
    void AcquireDrawsRefs();

    int mHighlight;               // +0x08
    std::list<Drawable *> mDraws; // +0x0c
};

} // namespace Rnd
