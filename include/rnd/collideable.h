#pragma once

#include "rnd/object.h"

namespace Rnd {

/**
 * Mix-in for an object that can be hit-tested.
 *
 * `Q23Rnd11Collideable` in the RTTI descriptor at `0x008ef440`, with `Rnd::Object` as a public
 * virtual base at offset 0. Its members have not been recovered.
 */
class Collideable : public virtual Object {
public:
    virtual ~Collideable();
};

} // namespace Rnd
