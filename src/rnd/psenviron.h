#pragma once

#include <vector>

#include "math/color.h"
#include "math/vector3.h"
#include "rnd/environ.h"

class HxStr;
struct GifQuadword;
class Sphere;
namespace Rnd {
class Light;
}

namespace Rnd {

/**
 * One directional light as the vertex lighting path consumes it.
 *
 * The record is not polymorphic and does not emit an RTTI descriptor. The name is inferred.
 * PsEnviron::DrawShowing() fills every member except mTransformedDirection.
 */
struct DirectionalLightRecord {
    Vector3 mDirection; /*!< Negated second row of the light's world transform. */
    /** mDirection in the space of the transform TransformLightRecords() last took. +0x10 */
    Vector3 mTransformedDirection;
    Color mAmbient; /*!< Copied from Rnd::Light::mAmbient. */
    Color mDiffuse; /*!< Copied from Rnd::Light::mDiffuse. */
};

/**
 * One point light as the vertex lighting path consumes it.
 *
 * The record is not polymorphic and does not emit an RTTI descriptor. The name is inferred.
 * PsEnviron::DrawShowing() fills mPosition, mAmbient, and mDiffuse.
 */
struct PointLightRecord {
    /*!< Translation row of the light's world transform, with Rnd::Light::mRange in the padding
         word. */
    Vector3 mPosition;
    /**
     * mPosition in the space of the transform TransformLightRecords() or SelectLightForVertex()
     * last took. The fourth word carries the range over unchanged, and the reach test reads it
     * from here. +0x10
     */
    Vector3 mTransformedPosition;
    Color mAmbient; /*!< Copied from Rnd::Light::mAmbient. */
    Color mDiffuse; /*!< Copied from Rnd::Light::mDiffuse. */
    /** 1 when TransformLightRecords() found the light out of reach of the bounding sphere. +0x40 */
    int mCulled;
    unsigned mReserved44[3]; /*!< Pads the record to 0x50 bytes. +0x44 */
};

/**
 * Bring every light record into an object's space and cull the point lights against its bounds.
 *
 * Each directional record receives its direction rotated by the inverse of pXfm, and each point
 * record its position transformed by it. A point light is culled when the distance from the sphere
 * centre exceeds its range plus the sphere radius. A null sphere or one of zero radius culls
 * nothing. TransformAndLightMeshVerts() is the one caller.
 *
 * The name is inferred.
 *
 * @param pDirectionalBegin Receives the first directional record.
 * @param pDirectionalEnd Receives the end of the directional records.
 * @param pPointBegin Receives the first point record.
 * @param pPointEnd Receives the end of the point records.
 * @param pXfm The object transform, four rows of four floats.
 * @param pSphere The object bounds, or null.
 * @return The directional lights plus the point lights not culled.
 * @ghidraAddress NTSC-U/C: 0x005af0c8
 * @ghidraAddress PAL: 0x005f1670
 */
int TransformLightRecords(DirectionalLightRecord *&pDirectionalBegin,
                          DirectionalLightRecord *&pDirectionalEnd,
                          PointLightRecord *&pPointBegin,
                          PointLightRecord *&pPointEnd,
                          const float *pXfm,
                          const Sphere *pSphere);

/**
 * Choose the one light the VU1 face program applies to an object and write its parameters.
 *
 * With lighting off the unlit program is chosen and nothing is written. Otherwise the first
 * directional light wins, and failing one the first point light that reaches the bounding sphere.
 * The light's direction or position in object space goes to pLight, and its ambient and diffuse
 * colours scale the material colours at pAmbient and pDiffuse, or replace them where the
 * material takes that term from the vertex colours.
 *
 * The name is inferred.
 *
 * @param pLight Receives the light direction or position, one packet quadword.
 * @param pAmbient The material ambient colour in the packet, updated in place.
 * @param pDiffuse The material diffuse colour in the packet, updated in place.
 * @param pXfm The object transform, four rows of four floats.
 * @param pSphere The object bounds, or null.
 * @return The VU1 program entry: 0x2ee unlit, 0x2f8 directional, 0x35c point, or 0x3d4 when lit
 *         by no light.
 * @ghidraAddress NTSC-U/C: 0x005af2c0
 * @ghidraAddress PAL: 0x005f1868
 */
int SelectLightForVertex(GifQuadword *pLight,
                         GifQuadword *pAmbient,
                         GifQuadword *pDiffuse,
                         const float *pXfm,
                         const Sphere *pSphere);

/**
 * Lighting and fog a subtree is drawn under, PlayStation 2.
 *
 * Its RTTI descriptor is at `0x008ef250`. It has `Rnd::Environ` as its only public non-virtual base
 * at offset 0. The class does not add a data member. Its allocator at `0x005b27b0` requests the
 * same 0x80 bytes the base does, under the same "Rnd::Environ" tag, and its constructor writes only
 * the two vtable pointers.
 *
 * Two vtables belong to the class. The four-entry table at `0x00833968` is addressed by the
 * `Rnd::Drawable` vptr at `0x10`, and the eight-entry table at `0x00833920` by the `Rnd::Object`
 * subobject vptr. DrawShowing() is the only entry that is not the base implementation.
 */
class PsEnviron : public Environ {
public:
    /**
     * Directional lights of the current environment, rebuilt by DrawShowing().
     *
     * @ghidraAddress NTSC-U/C: 0x00776120
     * @ghidraAddress PAL: 0x007b9ff8
     */
    static std::vector<DirectionalLightRecord> sDirLights;

    /**
     * Point lights of the current environment, rebuilt by DrawShowing().
     *
     * @ghidraAddress NTSC-U/C: 0x00776130
     * @ghidraAddress PAL: 0x007ba008
     */
    static std::vector<PointLightRecord> sPointLights;

    /**
     * Construct an environment with no lights and no fog.
     *
     * NewEnviron() and Init() both open-code the body. The out-of-line copy has no caller.
     *
     * @param name The registry key for this object.
     * @ghidraAddress NTSC-U/C: 0x005b2648
     * @ghidraAddress PAL: 0x005f4c18
     */
    explicit PsEnviron(const HxStr &name) : Object(name), Environ(name) {
    }

    /**
     * @ghidraAddress NTSC-U/C: 0x005b2200
     * @ghidraAddress PAL: 0x005f47d0
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
     * @ghidraAddress NTSC-U/C: 0x005b27b0
     * @ghidraAddress PAL: 0x005f4d80
     */
    static Environ *NewEnviron(const HxStr &name);

    /**
     * Install the PlayStation 2 environment factory and build the default scene fixtures.
     *
     * Rnd::g_pfnNewEnviron becomes NewEnviron(), which is how a `.rnd` file naming the unchanged
     * "Environ" key loads this subclass. The environment named "[default environ]" is then built
     * with a direct allocation rather than through the factory, marked internal, and added to the
     * draw list of Rnd::PsCam::sDefault. A light named "[default light]" follows, built through
     * Rnd::g_pfnNewLight, marked internal, added to the environment, and made a transform child of
     * Rnd::PsCam::sDefault.
     *
     * @ghidraAddress NTSC-U/C: 0x005aea68
     * @ghidraAddress PAL: 0x005f0fd0
     */
    static void Init();

    /**
     * Undo what Init() installs.
     *
     * Destroys g_pDefaultEnviron and then g_pDefaultLight through their virtual destructors,
     * without clearing either pointer, and then expands the "Environ" class registration, whose
     * out-of-line copy is at `0x00518e70`. GfxDevice::Terminate() is the one caller. The name is
     * inferred from GfxDevice::Terminate().
     *
     * @ghidraAddress NTSC-U/C: 0x005b2888
     * @ghidraAddress PAL: 0x005f4e58
     */
    static void Terminate();

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
     * making this environment Rnd::Environ::sCurrent.
     *
     * @return Non-zero, which draws the children as well.
     * @ghidraAddress NTSC-U/C: 0x005aecb8
     * @ghidraAddress PAL: 0x005f1260
     */
    virtual int DrawShowing();
};

/**
 * Environment the renderer falls back on when a scene selects none.
 *
 * @ghidraAddress NTSC-U/C: 0x0077613c
 * @ghidraAddress PAL: 0x007ba014
 */
extern PsEnviron *g_pDefaultEnviron;

/**
 * Light the default environment applies when a scene supplies none.
 *
 * @ghidraAddress NTSC-U/C: 0x00776140
 * @ghidraAddress PAL: 0x007ba018
 */
extern Light *g_pDefaultLight;

/**
 * Scale a camera space depth is multiplied by to arrive at the byte range of the GS fog
 * register.
 *
 * @ghidraAddress NTSC-U/C: 0x00776110
 * @ghidraAddress PAL: 0x007b9fe8
 */
extern float g_flFogScale;

/**
 * Offset added after g_flFogScale has been applied.
 *
 * @ghidraAddress NTSC-U/C: 0x00776114
 * @ghidraAddress PAL: 0x007b9fec
 */
extern float g_flFogOffset;

} // namespace Rnd
