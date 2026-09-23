#pragma once

#include <vector>

#include "math/color.h"
#include "math/vector3.h"
#include "rnd/environ.h"

class HxStr;
namespace Rnd {
class Light;
}

namespace Rnd {

/**
 * One directional light as the vertex lighting path consumes it.
 *
 * The record is not polymorphic and emits no RTTI descriptor. The name is the analysis program's
 * and is inferred. PsEnviron::DrawSelf() fills every member except mTransformedDirection.
 */
struct DirectionalLightRecord {
    Vector3 mDirection;            /*!< Negated second row of the light's world transform. */
    Vector3 mTransformedDirection; /*!< Untouched by PsEnviron::DrawSelf(). Inferred. +0x10 */
    Color mAmbient;                /*!< Copied from Rnd::Light::mAmbient. */
    Color mDiffuse;                /*!< Copied from Rnd::Light::mDiffuse. */
};

/**
 * One point light as the vertex lighting path consumes it.
 *
 * The record is not polymorphic and emits no RTTI descriptor. The name is the analysis program's
 * and is inferred. PsEnviron::DrawSelf() fills mPosition, mAmbient, and mDiffuse.
 */
struct PointLightRecord {
    /*!< Translation row of the light's world transform, with Rnd::Light::mRange in the padding
         word. */
    Vector3 mPosition;
    Vector3 mTransformedPosition; /*!< Untouched by PsEnviron::DrawSelf(). Inferred. +0x10 */
    Color mAmbient;               /*!< Copied from Rnd::Light::mAmbient. */
    Color mDiffuse;               /*!< Copied from Rnd::Light::mDiffuse. */
    unsigned mUnknown40;          /*!< Written by the routine at `0x005af0c8`. +0x40 */
    unsigned mUnknown44[3];       /*!< Pads the record to 0x50 bytes. +0x44 */
};

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
 * The routine at `0x005b2888` undoes what Init() installs. It destroys both default objects and
 * restores Rnd::g_pfnNewEnviron to the base Rnd::Environ factory, and the class-registration sweep
 * at `0x0049b0c4` is its one caller. Its title is undetermined.
 */
class PsEnviron : public Environ {
public:
    /**
     * Construct an environment with no lights and no fog.
     *
     * NewEnviron() and Init() both open-code the body. The out-of-line copy has no caller.
     *
     * @param name The registry key for this object.
     * @ghidraAddress 0x005b2648
     */
    explicit PsEnviron(const HxStr &name) : Object(name), Environ(name) {
    }

    /**
     * @ghidraAddress 0x005b2200
     */
    virtual ~PsEnviron();

    /**
     * Build a PlayStation 2 environment the class registry vends.
     *
     * The return type is the base class, which is what lets Init() store the routine in
     * Rnd::g_pfnNewEnviron.
     *
     * @param name The registry key for the new environment.
     * @return The new environment.
     * @ghidraAddress 0x005b27b0
     */
    static Environ *NewEnviron(const HxStr &name);

    /**
     * Install the PlayStation 2 environment factory and build the default scene fixtures.
     *
     * Rnd::g_pfnNewEnviron becomes NewEnviron(), which is how a `.rnd` file naming the unchanged
     * "Environ" key loads this subclass. The environment named "[default environ]" is then built
     * with a direct allocation rather than through the factory, marked internal, and added to the
     * draw list of Rnd::g_pDefaultCam. A light named "[default light]" follows, built through
     * Rnd::g_pfnNewLight, marked internal, added to the environment, and made a transform child of
     * Rnd::g_pDefaultCam.
     *
     * @ghidraAddress 0x005aea68
     */
    static void Init();

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
     * Both record vectors are then emptied and each entry of mLights appended to one of them, a
     * directional light contributing the negated second basis row of its world transform and a
     * point light its translation row and its range. A spot light is skipped. The routine ends by
     * making this environment Rnd::g_pCurrentEnviron.
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
 * Light the default environment applies when a scene supplies none.
 *
 * @ghidraAddress 0x00776140
 */
extern Light *g_pDefaultLight;

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

/**
 * Directional lights of the current environment, rebuilt by PsEnviron::DrawSelf().
 *
 * The program's label is `g_abDirectionalLightRecords`.
 *
 * @ghidraAddress 0x00776120
 */
extern std::vector<DirectionalLightRecord> g_directionalLightRecords;

/**
 * Point lights of the current environment, rebuilt by PsEnviron::DrawSelf().
 *
 * The program's label is `g_abPointLightRecords`.
 *
 * @ghidraAddress 0x00776130
 */
extern std::vector<PointLightRecord> g_pointLightRecords;

} // namespace Rnd
