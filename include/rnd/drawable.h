#pragma once

#include <list>

#include "os/failsink.h"
#include "rnd/object.h"
#include "rnd/stream.h"

namespace Rnd {

/**
 * Mix-in for an object that draws itself and then a list of other drawables.
 *
 * `Q23Rnd8Drawable` in the RTTI descriptor at `0x008ef320`, with `Rnd::Object` as a public virtual
 * base at offset 0. The subobject is 0x14 bytes. The compiler places the virtual-base pointer at
 * `+0x00` and, following the g++ 2.x layout for a class with no non-virtual base, the vptr at
 * `+0x10`, so the declared members occupy `+0x04` through `+0x0f`. For a standalone Drawable the
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
     * @ghidraAddress 0x005066f8
     */
    Drawable();

    /**
     * Drop this object's references on its children.
     *
     * @ghidraAddress 0x00506590
     */
    virtual ~Drawable();

    /**
     * Draw this object followed by each of its children.
     *
     * Draws nothing when mShowing is clear, and skips the children when DrawSelf() reports that
     * the subtree is invisible.
     *
     * @ghidraAddress 0x00506920
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
     * @ghidraAddress 0x00502d78
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
     * @ghidraAddress 0x00503188
     */
    void AddDraw(Drawable *pDraw, Drawable *pBefore);

    /**
     * Insert pDraw at the front of mDraws.
     *
     * @param pDraw The drawable to add.
     * @ghidraAddress 0x005064e0
     */
    void AddDraw(Drawable *pDraw);

    /**
     * Empty mDraws, dropping this object's reference on every entry.
     *
     * @ghidraAddress 0x00503420
     */
    void ClearDraws();

    /**
     * Set whether this object draws at all.
     *
     * Drawable vtable slot 1.
     *
     * @param nShowing Non-zero to draw.
     * @ghidraAddress 0x005066d8
     */
    virtual void SetShowing(int nShowing);

    /**
     * Set whether this object draws with its highlight treatment.
     *
     * Drawable vtable slot 2.
     *
     * @param nHighlight Non-zero to highlight.
     * @ghidraAddress 0x00506c88
     */
    virtual void SetHighlight(int nHighlight);

    /**
     * Report the highlight flag.
     *
     * @return Non-zero when this object draws with its highlight treatment.
     * @ghidraAddress 0x005066e0
     */
    int GetHighlight() const {
        return mHighlight;
    }

    /**
     * Report the child list.
     *
     * @return The list of drawables this object draws after itself.
     * @ghidraAddress 0x005066e8
     */
    std::list<Drawable *> &GetDraws() {
        return mDraws;
    }

    /**
     * Write a description of this object to sink.
     *
     * Writes mShowing, mHighlight, and the mDraws list, and produces nothing at all when the dump
     * level of sink is not positive. FailSink::Print() discards its text in the shipped build, so
     * the routine produces no output on this target either way.
     *
     * @param sink The diagnostic sink to write to.
     * @ghidraAddress 0x005069a8
     */
    virtual void DumpText(FailSink &sink);

    virtual void Save(Stream &stream);
    virtual void Replace(Object *pFrom, Object *pTo);
    virtual void Copy(const Object *pSource, unsigned nFlags);
    virtual void Load(Stream &stream);

protected:
    /**
     * Draw this object alone.
     *
     * Drawable vtable slot 3. The base implementation draws nothing and reports that the children
     * are still to be drawn. Only Draw() invokes it.
     *
     * @return Non-zero when the children are to be drawn as well.
     * @ghidraAddress 0x005066f0
     */
    virtual int DrawSelf();

    /**
     * Drop this object's reference on every mDraws entry without emptying the list.
     *
     * Every derived destructor invokes this before its own teardown.
     *
     * @ghidraAddress 0x00506ba8
     */
    void ReleaseDrawsRefs();

private:
    // 0x00506c18. Only Copy() and Load() invoke this.
    void AcquireDrawsRefs();

    int mShowing;                 // +0x04
    int mHighlight;               // +0x08
    std::list<Drawable *> mDraws; // +0x0c
};

} // namespace Rnd
