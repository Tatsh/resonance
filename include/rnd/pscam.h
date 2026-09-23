#pragma once

#include "math/transform.h"
#include "math/vector2.h"
#include "math/vector3.h"
#include "rnd/cam.h"

class HxStr;
namespace Rnd {
class Tex;
}

namespace Rnd {

/**
 * Camera, PlayStation 2.
 *
 * `Q23Rnd5PsCam` in the RTTI descriptor at `0x008efdc0`, with `Rnd::Cam` as its only public
 * non-virtual base at offset 0. The class adds no data member. Its allocator at `0x00588500`
 * requests the same 0x330 bytes the base does, under the same "Rnd::Cam" tag, and its constructor
 * writes nothing beyond the four vtable pointers.
 *
 * Four vtables belong to the class. The seven-entry table at `0x0082f2e0` is addressed by the
 * `Rnd::Drawable` vptr at `0x10`, the three-entry table at `0x0082f2c0` by the
 * `Rnd::Transformable` vptr at `0xc8`, the three-entry table at `0x0082f2a0` by the
 * `Rnd::Collideable` vptr at `0xd8`, and the eight-entry table at `0x0082f258` by the
 * `Rnd::Object` subobject vptr. Every entry beyond the ones below stores the implementation
 * `Rnd::Cam` supplies.
 *
 * The seventh entry of the first table proves SetTargetTex() is a virtual declared here rather
 * than an override. The base table has six entries and this one has seven.
 *
 * The routine at `0x00588428` is an unreferenced out-of-line emission of an inline routine whose
 * live copy is inlined into the class-registration sweep at `0x0049b01c`. That routine destroys
 * g_pDefaultCam, restores Rnd::g_pfnNewCam to the base Rnd::Cam factory, clears
 * Rnd::g_pCurrentCam, and re-registers the "Cam" key, which undoes what Init() installs. Its
 * title is undetermined, because the only live copy is inlined and no call site records one.
 */
class PsCam : public Cam {
public:
    /**
     * Construct a camera with the default projection.
     *
     * @param name The registry key for this object.
     * @ghidraAddress 0x00582558
     */
    explicit PsCam(const HxStr &name);

    /**
     * @ghidraAddress 0x005826e0
     */
    virtual ~PsCam();

    /**
     * Make this camera the one the frame is drawn through and submit its display registers.
     *
     * Vtable slot 3 of the Rnd::Drawable table. A render target is bound through
     * Rnd::PsTex::BindAsRenderTarget(), and a camera following one that drew into a texture points
     * the GS back at the display buffer. The routine then computes the guard band, widens the four
     * side planes of the local frustum by it, moves all six planes into world space as
     * Rnd::g_afDrawFrustumPlanes, builds the viewport and projection transforms, programs
     * SCISSOR_1 from the screen rectangle clamped to the unit square, and stores itself in
     * g_pCurrentCam. It does not call the base implementation.
     *
     * @return Non-zero, which draws the children as well.
     * @ghidraAddress 0x00582830
     */
    virtual int DrawSelf();

    /**
     * Place a point of the screen rectangle in render target pixels.
     *
     * Vtable slot 4 of the Rnd::Drawable table. The point is scaled by the screen rectangle
     * extents and offset by its origin, then scaled again by the size of the render target, or by
     * the display size of g_gfxDevice when there is no render target.
     *
     * @param ptScreen The point, in the coordinates the screen rectangle is expressed in.
     * @return The same point in render target pixels.
     * @ghidraAddress 0x005885b0
     */
    virtual Vector2 ScreenToPixels(const Vector2 &ptScreen);

    /**
     * Take the aspect ratio from the render target and rebuild the projection.
     *
     * Vtable slot 5 of the Rnd::Drawable table. With no render target the vertical ratio becomes
     * the 0.75 of a four by three display, which is the default the base implementation has no way
     * to supply. The base then runs.
     *
     * @ghidraAddress 0x00588578
     */
    virtual void UpdateTargetAspect();

    /**
     * Set the texture this camera draws into, or none to draw into the frame buffer.
     *
     * Vtable slot 6 of the Rnd::Drawable table, which is a slot of its own rather than an override
     * of the base routine of the same title. The base routine runs first, and clearing the render
     * target then restores the vertical ratio of a four by three display.
     *
     * @param pTex The render target, or null to draw into the frame buffer.
     * @ghidraAddress 0x00588498
     */
    virtual void SetTargetTex(Tex *pTex);

    /**
     * Build a camera the class registry vends.
     *
     * The return type is the base class, which is what lets Init() store the routine in
     * Rnd::g_pfnNewCam.
     *
     * @param name The registry key for the new camera.
     * @return The new camera.
     * @ghidraAddress 0x00588500
     */
    static Cam *NewCam(const HxStr &name);

    /**
     * Install the PlayStation 2 camera factory and build the default camera.
     *
     * Rnd::g_pfnNewCam becomes NewCam(), which is how a `.rnd` file naming the unchanged "Cam"
     * key loads this subclass. The camera named "[default cam]" is then built, marked internal,
     * and placed at the translation (0, -150, 0) with its dirty flag set, which puts the default
     * viewpoint 150 units back along the axis this engine looks down.
     *
     * @ghidraAddress 0x00582430
     */
    static void Init();
};

/**
 * Camera the renderer falls back on when a scene selects none.
 *
 * Rnd::GfxDevice::BeginFrame() reads it, and Rnd::PsEnviron::Init() makes the default environment
 * and the default light children of it.
 *
 * @ghidraAddress 0x00768410
 */
extern PsCam *g_pDefaultCam;

// The globals below are the draw state PsCam::DrawSelf() leaves for the software and VU1 paths.
// Every title is inferred from the computation that fills the global, because the image
// retains no name for any of them.

/**
 * Camera to clip transform, the local projection widened by the guard band and then preceded by
 * the camera's world to camera transform.
 *
 * @ghidraAddress 0x008e4020
 */
extern Transform g_viewProjectXfm;

/**
 * The same transform without the guard band widening.
 *
 * @ghidraAddress 0x008e4060
 */
extern Transform g_viewProjectUnscaledXfm;

/**
 * Clip to GS coordinate transform, scaled by the guard band.
 *
 * @ghidraAddress 0x008e40a0
 */
extern Transform g_viewportXfm;

/**
 * The same transform without the guard band scaling.
 *
 * @ghidraAddress 0x008e40e0
 */
extern Transform g_viewportUnscaledXfm;

/**
 * g_viewportXfm with a small depth bias added to the translation.
 *
 * @ghidraAddress 0x008e4120
 */
extern Transform g_viewportBiasedXfm;

/**
 * Screen scale of a particle's extent, the widened projection scale times the viewport scale.
 *
 * @ghidraAddress 0x008e4160
 */
extern Vector3 g_particleScreenScale;

/**
 * The widened projection scale on each axis, with y negated.
 *
 * @ghidraAddress 0x008e4170
 */
extern Vector3 g_particleProjectScale;

/**
 * Factor the view is widened by on each axis before clipping.
 *
 * Each axis is the room left in the 4096-unit GS coordinate space around the screen rectangle,
 * divided by the rectangle's extent with a 2 per cent margin. Primitives inside the widened view
 * need no clipping, because the GS scissors them.
 *
 * @ghidraAddress 0x008e4180
 */
extern Vector3 g_guardBandScale;

/**
 * Reciprocal of g_guardBandScale, left unchanged when either axis of it is zero.
 *
 * @ghidraAddress 0x008e4190
 */
extern Vector3 g_invGuardBandScale;

/**
 * Near plane distance of the camera last drawn through.
 *
 * @ghidraAddress 0x008e41a0
 */
extern float g_flCamNear;

/**
 * Scissor rectangle of the camera last drawn through, in GS primitive coordinates of 1/16 pixel.
 *
 * DrawSelf() first stores pixel bounds here to program SCISSOR_1 and then converts them.
 *
 * @ghidraAddress 0x008e41a4
 */
extern int g_nScissorX0;

/** @ghidraAddress 0x008e41a8 */
extern int g_nScissorX1;

/** @ghidraAddress 0x008e41ac */
extern int g_nScissorY0;

/** @ghidraAddress 0x008e41b0 */
extern int g_nScissorY1;

} // namespace Rnd
