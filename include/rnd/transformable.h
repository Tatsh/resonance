#pragma once

#include "rnd/object.h"

namespace Rnd {

/**
 * Mix-in for an object with a world and local transform.
 *
 * `Q23Rnd13Transformable` in the RTTI descriptor at `0x008ef770`, with `Rnd::Object` as a public
 * virtual base at offset 0. The subobject spans 0xa8 bytes and stores two 4x4 matrices followed
 * by a rest-pose row, a notify flag, and constraint flags. Its members have not been confirmed
 * against the disassembly yet.
 */
class Transformable : public virtual Object {
public:
    virtual ~Transformable();
};

} // namespace Rnd
