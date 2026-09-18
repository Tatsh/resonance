#pragma once

#include <list>

#include "rnd/object.h"

namespace Rnd {

/**
 * Mix-in for an object that can be hit-tested.
 *
 * `Q23Rnd11Collideable` in the RTTI descriptor at `0x008ef440`, with `Rnd::Object` as a public
 * virtual base at offset 0. The subobject is 0xc bytes: the virtual-base pointer at `+0x00`,
 * mCollides at `+0x04`, and the vptr at `+0x08`. `Rnd::View` confirms the vptr offset by placing
 * its Collideable subobject at `+0xe0` and writing that vptr to View + 0xe8.
 *
 * The class declares two virtuals of its own beyond the compiler-generated slot 0, at vtable slots
 * 1 and 2, which for `Rnd::View` are `0x00502a28` and `0x00502ab8`. Both operate on mCollides and
 * neither is reconstructed yet. Its own members are the constructor at `0x00502668`, the
 * destructor at `0x00502518`, DumpText at `0x00502880`, Save at `0x005028f0`, and the list
 * teardown at `0x00502948`.
 */
class Collideable : public virtual Object {
public:
    virtual ~Collideable();

    /**
     * Report the collideable that hit-tests this one.
     *
     * Walks the referrer list of the `Rnd::Object` subobject, casts each referrer to Collideable
     * and returns the first whose own mCollides list includes this object, which makes the result
     * the parent in the collision hierarchy rather than a plain cast.
     *
     * @return The parent collideable, or null when no referrer hit-tests this one.
     * @ghidraAddress 0x00500348
     */
    Collideable *Parent();

private:
    std::list<Collideable *> mCollides; // +0x04
};

} // namespace Rnd
