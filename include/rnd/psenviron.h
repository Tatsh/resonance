#pragma once

#include "os/hxstr.h"
#include "rnd/environ.h"

namespace Rnd {

/**
 * Lighting and fog a subtree is drawn under, PlayStation 2.
 *
 * `Q23Rnd9PsEnviron` in the RTTI descriptor at `0x008ef250`, with `Rnd::Environ` as its only
 * public non-virtual base at offset 0. The class adds no data member. Its allocator at
 * `0x005b27b0` requests the same 0x80 bytes the base does, under the same "Rnd::Environ" tag, and
 * its constructor writes nothing beyond the two vtable pointers.
 *
 * Two vtables belong to the class. The four-entry table at `0x00833968` is addressed by the
 * `Rnd::Drawable` vptr at `0x10`, and the eight-entry table at `0x00833920` by the `Rnd::Object`
 * subobject vptr. DrawSelf() is the only entry that is not the base implementation.
 *
 * The routine at `0x005aea68` builds the environment the renderer falls back on, named
 * "[default environ]", and stores it in g_pDefaultEnviron.
 */
class PsEnviron : public Environ {
public:
    /**
     * Construct an environment with no lights and no fog.
     *
     * @param name The registry key for this object.
     * @ghidraAddress 0x005b2648
     */
    explicit PsEnviron(const HxStr &name);

    /**
     * @ghidraAddress 0x005b2200
     */
    virtual ~PsEnviron();

    /**
     * Build a PlayStation 2 environment the class registry vends.
     *
     * @param name The registry key for the new environment.
     * @return The new environment.
     * @ghidraAddress 0x005b27b0
     */
    static PsEnviron *NewEnviron(const HxStr &name);

protected:
    /**
     * Make this environment the current one, program fog, and build the light records.
     *
     * Vtable slot 3 of the Rnd::Drawable table. Rnd::g_nFogEnabled becomes non-zero whenever the
     * fog mode is anything other than None, and the two fog terms are then set so that a camera
     * space depth maps onto the byte range the GS fog register takes. The scale is 255.0 divided
     * by the fog start less the fog end, and the offset is the negated fog end scaled by it. With
     * no fog the scale is zero and the offset 255.0, which fogs no pixel at all. The fog colour
     * is packed into the low three bytes of GS register 0x3d.
     *
     * Each entry of mLights is then appended to one of two record vectors, a directional light
     * contributing the negated second basis row of its world transform and a point light its
     * translation row and its range. A spot light is skipped. The body is not reconstructed,
     * because neither record vector has been recovered.
     *
     * @return Non-zero, which draws the children as well.
     * @ghidraAddress 0x005aecb8
     */
    virtual int DrawSelf();
};

/**
 * Environment the renderer falls back on when a scene selects none.
 *
 * @ghidraAddress 0x0077613c
 */
extern PsEnviron *g_pDefaultEnviron;

/**
 * Scale a camera space depth is multiplied by to arrive at the byte range of the GS fog
 * register.
 *
 * @ghidraAddress 0x00776110
 */
extern float g_flFogScale;

/**
 * Offset added after g_flFogScale has been applied.
 *
 * @ghidraAddress 0x00776114
 */
extern float g_flFogOffset;

} // namespace Rnd
