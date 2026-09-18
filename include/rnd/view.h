#pragma once

#include "rnd/animatable.h"
#include "rnd/collideable.h"
#include "rnd/drawable.h"
#include "rnd/transformable.h"

namespace Rnd {

/**
 * Root of a loadable scene.
 *
 * `Q23Rnd4View` in the RTTI descriptor at `0x008ef120`. The descriptor lists four public
 * non-virtual bases and fixes their subobject offsets, `Rnd::Animatable` at `+0x00`,
 * `Rnd::Drawable` at `+0x18`, `Rnd::Transformable` at `+0x30`, and `Rnd::Collideable` at
 * `+0xe0`. A `.rnd` file exposes its scene root under the name "view", which start-up resolves
 * through `Rnd::Manager::Find()`. Its own members have not been recovered.
 */
class View : public Animatable, public Drawable, public Transformable, public Collideable {
public:
    virtual ~View();

    virtual bool DrawSelf();
};

} // namespace Rnd
