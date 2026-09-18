#pragma once

#include <list>

#include "rnd/object.h"

namespace Rnd {

/**
 * Mix-in for an object driven by a frame number.
 *
 * `Q23Rnd10Animatable` in the RTTI descriptor at `0x008eed68`, with `Rnd::Object` as a public
 * virtual base at offset 0. The subobject is 0x18 bytes, with the virtual-base pointer at `+0x00`
 * and the vptr at `+0x14`, which `Rnd::View` confirms by placing its Animatable subobject at
 * `+0x00` and writing that vptr to View + 0x14.
 *
 * Recovery is partial. Two `std::list` members sit at `+0x04` and `+0x08`, and the fields at
 * `+0x0c` and `+0x10` are unrecovered. The title mAnims is inferred from the use Parent() makes of
 * the list rather than attested by a string. The class declares three virtuals of its own beyond
 * the compiler-generated slot 0, at vtable slots 1 through 3; for `Rnd::View` those are
 * `0x00494b50`, `0x0049a3b8`, and `0x0049a100`. Its overrides of the `Rnd::Object` virtuals are
 * `0x0049a640` (DumpText), `0x0049a6e8` (Save), `0x004951e0` (Replace), `0x00494e70` (Copy), and
 * `0x00494d68` (Load).
 */
class Animatable : public virtual Object {
public:
    virtual ~Animatable();

    /**
     * Report the animatable that animates this one.
     *
     * Walks the referrer list of the `Rnd::Object` subobject, casts each referrer to Animatable
     * and returns the first whose own child list at `+0x04` includes this object, which makes the
     * result the parent in the animation hierarchy rather than a plain cast.
     *
     * @return The parent animatable, or null when no referrer animates this one.
     * @ghidraAddress 0x00494a88
     */
    Animatable *Parent();

private:
    std::list<Animatable *> mAnims;     // +0x04
    std::list<Animatable *> mUnknown08; // +0x08
};

} // namespace Rnd
