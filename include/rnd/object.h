#pragma once

namespace Rnd {

/**
 * Base of every object the renderer can load, name, and serialise.
 *
 * `Q23Rnd6Object` in the RTTI descriptor at `0x0086f678`. Every renderer mix-in (Animatable,
 * Collideable, Drawable, Transformable) derives from this class virtually, which places the
 * subobject at the end of the most derived object. Its data members have not been recovered.
 */
class Object {
public:
    virtual ~Object();
};

} // namespace Rnd
