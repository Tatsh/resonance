#pragma once

#include "rnd/object.h"

namespace Rnd {

/**
 * Mix-in for an object driven by a frame number.
 *
 * `Q23Rnd10Animatable` in the RTTI descriptor at `0x008eed68`, with `Rnd::Object` as a public
 * virtual base at offset 0. `Rnd::View` places this subobject at offset 0 and its `Rnd::Drawable`
 * subobject at `+0x18`, so the Animatable subobject occupies 0x18 bytes. Its members have not
 * been recovered.
 */
class Animatable : public virtual Object {
public:
    virtual ~Animatable();
};

} // namespace Rnd
