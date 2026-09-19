#pragma once

#include "math/vector2.h"
#include "os/hxstr.h"
#include "rnd/cam.h"
#include "rnd/tex.h"

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
     * Vtable slot 3 of the Rnd::Drawable table. Beyond storing itself in g_pCurrentCam and
     * reporting that the children are to be drawn, the routine builds the scissor and offset
     * registers from the screen rectangle and the render target size, moves the six frustum planes
     * into the space the draw path tests against through the plane transform at `0x00550fa8`, and
     * records the previous display state for the next camera to restore. The body is not
     * reconstructed.
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
     * the display size of Rnd::g_gfxDevice when there is no render target. The body is not
     * reconstructed, because the display width and height of the device sit inside a reserved run
     * of `gfx/gfxdevice.h`.
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
     * @param name The registry key for the new camera.
     * @return The new camera.
     * @ghidraAddress 0x00588500
     */
    static PsCam *NewCam(const HxStr &name);

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

} // namespace Rnd
