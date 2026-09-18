#pragma once

#include <list>

#include "rnd/object.h"

namespace Rnd {

/**
 * Mix-in for an object that can draw itself and a list of children.
 *
 * `Q23Rnd8Drawable` in the RTTI descriptor at `0x008ef320`, with `Rnd::Object` as a public
 * virtual base at offset 0. The subobject is 0x14 bytes. The compiler places the virtual-base
 * pointer at `+0x00` and, following the g++ 2.x layout for a class with no non-virtual base, the
 * vptr at `+0x10`, so the declared members occupy `+0x04` through `+0x0f`.
 *
 * Recovery is incomplete. One user vtable slot ahead of DrawSelf() has not been identified.
 */
class Drawable : public virtual Object {
public:
    virtual ~Drawable();

    /**
     * Draw this object followed by each of its children.
     *
     * Returns without drawing when the object is hidden or when DrawSelf() reports that the
     * subtree is not visible.
     *
     * @ghidraAddress 0x00506920
     */
    void Draw();

    /**
     * Draw this object alone.
     *
     * @return True when the children should be drawn as well.
     */
    virtual bool DrawSelf() = 0;

    int mShowing;                       // +0x04
    int mUnknown08;                     // +0x08
    std::list<Drawable *> *mChildren;   // +0x0c
};

} // namespace Rnd
