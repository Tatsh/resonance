#pragma once

#include "os/failsink.h"
#include "os/hxstr.h"
#include "rnd/animatable.h"
#include "rnd/collideable.h"
#include "rnd/drawable.h"
#include "rnd/object.h"
#include "rnd/stream.h"
#include "rnd/transformable.h"

namespace Rnd {

/**
 * Root of a loadable scene.
 *
 * `Q23Rnd4View` in the RTTI descriptor at `0x008ef120`. The descriptor lists four public
 * non-virtual bases and fixes their subobject offsets, `Rnd::Animatable` at `+0x00`,
 * `Rnd::Drawable` at `+0x18`, `Rnd::Transformable` at `+0x30`, and `Rnd::Collideable` at `+0xe0`.
 * The shared `Rnd::Object` virtual base sits at `+0x100`, and the constructor writes that address
 * into the virtual-base pointer of all four subobjects. View's own four fields occupy `+0xf0`
 * through `+0xff` and start zeroed.
 *
 * Five vtables belong to the class, one per subobject plus the one the `Rnd::Object` subobject
 * vptr addresses. They are `0x00823628` (Animatable), `0x00823600` (Drawable), `0x008235e0`
 * (Transformable), `0x008235c0` (Collideable), and `0x00823650` (Object). Every entry of the last
 * four adjusts `this` back to the start of the View. The Drawable table repeats the base
 * implementations of SetShowing, SetHighlight, and DrawSelf, so View overrides none of the three
 * and draws nothing of its own.
 *
 * A `.rnd` file exposes its scene root under the name "view", which start-up resolves through
 * Rnd::Manager::Find().
 *
 * Recovery is partial. The constructor is at `0x004e2740`, the factory at `0x004e2088`, the
 * sub-object reader at `0x004e7c50`, and the five display-mode routines at `0x004e32a0` through
 * `0x004e3618`.
 */
class View : public Animatable, public Drawable, public Transformable, public Collideable {
public:
    /** @ghidraAddress 0x004e21b8 */
    virtual ~View();

    /** @ghidraAddress 0x004e3768 */
    virtual void DumpText(FailSink &sink);

    /** @ghidraAddress 0x004e37d0 */
    virtual void Save(Stream &stream);

    /** @ghidraAddress 0x004e36f8 */
    virtual void Replace(Object *pFrom, Object *pTo);

    /** @ghidraAddress 0x004e2720 */
    virtual const HxStr &ClassName() const;

    /** @ghidraAddress 0x004e3850 */
    virtual void Copy(const Object *pSource, unsigned nFlags);

    /** @ghidraAddress 0x004e0128 */
    virtual void Load(Stream &stream);
};

} // namespace Rnd
