#pragma once

#include "math/frustum.h"
#include "math/vector3.h"
#include "math/vector2.h"
#include "os/failsink.h"
#include "os/hxstr.h"
#include "rnd/collideable.h"
#include "rnd/drawable.h"
#include "rnd/object.h"
#include "rnd/stream.h"
#include "rnd/tex.h"
#include "rnd/transformable.h"

namespace Rnd {

/**
 * Camera.
 *
 * `Q23Rnd3Cam` in the RTTI descriptor at `0x008eeeb8`, with three public non-virtual bases:
 * `Rnd::Drawable` at offset 0, `Rnd::Transformable` at `0x20`, and `Rnd::Collideable` at `0xd0`.
 * The class is 0x330 bytes, which the factory at `0x004b2470` proves by allocating exactly that
 * much under the tag "Rnd::Cam". The virtual `Rnd::Object` subobject sits at `0x310`, which the
 * constructor proves by writing that address into all three virtual-base pointers.
 *
 * Each matrix and each frustum member is quadword aligned in the original. The four bytes between
 * the end of the `Rnd::Collideable` subobject at `0xdc` and the first matrix at `0xe0` are the
 * padding that alignment produces rather than a member.
 *
 * The member titles come from the text DumpText() writes: "nearPlane:", " farPlane:", " fov:",
 * "yRatio:", "screenRect:", "zRange:", " targetTex:", " localProject:", "worldProject:", and
 * "invWorldProject:". Both frustum members are titled from the same dump. The two matrices the
 * dump omits are titled from what builds them: `0xe0` is the inverse of the world transform, and
 * `0x160` is the inverse of the local projection.
 *
 * Four vtables belong to the class. The six-entry table at `0x00820958` is addressed by the
 * `Rnd::Drawable` vptr at `0x10` and stores the two virtuals declared here after the three
 * `Rnd::Drawable` ones. The three-entry table at `0x00820938` is addressed by the
 * `Rnd::Transformable` vptr at `0xc8`, the three-entry table at `0x00820918` by the
 * `Rnd::Collideable` vptr at `0xd8`, and the eight-entry table at `0x00820990` by the
 * `Rnd::Object` subobject vptr, each entry with the adjustment back to the Cam pointer.
 *
 * The routine at `0x004b1ff0` is an out-of-line copy of an inline accessor that returns
 * g_pCurrentCam. It has no caller: every reader in the image loads the global directly.
 *
 * One member is recovered and not declared here. Slot 2 of the `Rnd::Transformable` table holds
 * the override of UpdateWorldXfm() at `0x004b1fa0`, which chains to the base at `0x004f0b18` and
 * runs UpdateWorldProject() only when the base reports that it recomposed. It cannot be declared
 * until `Rnd::Transformable` declares that virtual as returning an int, which the base at
 * `0x004f0b18` does and `transformable.h` does not yet record.
 */
class Cam : public Drawable, public Transformable, public Collideable {
public:
    /**
     * Rectangle the projected image is placed in.
     *
     * Titled from the single dump label "screenRect:" the four components are written under, which
     * are "(x:", " y:", " w:", and " h:". The extents are fractions of the render target rather
     * than pixels, which Rnd::PsCam::ScreenToPixels() proves by scaling them by the target size.
     * The type is nested because no other class stores a rectangle in this shape.
     */
    struct Rect {
        float x; // +0x00
        float y; // +0x04
        float w; // +0x08
        float h; // +0x0c
    };

    /**
     * Construct a camera with the default projection.
     *
     * The near plane starts at 1.0, the far plane at 1000.0, the field of view at a right angle,
     * the vertical ratio at 0.75, the depth range at 0.0 to 1.0, and the screen rectangle at the
     * whole target. Every projection matrix is left with only its fourth column filled, and the
     * final call builds them all.
     *
     * @param name The registry key for this object.
     * @ghidraAddress 0x004aeb70
     */
    explicit Cam(const HxStr &name);

    /**
     * Clear g_pCurrentCam when it addresses this camera and release the render target.
     *
     * @ghidraAddress 0x004af668
     */
    virtual ~Cam();

    /**
     * Build the local frustum and both local projection matrices.
     *
     * The aspect ratio is the vertical ratio scaled by the height of the screen rectangle and
     * divided by its width. A field of view of zero selects an orthographic projection and any
     * other value a perspective one, the second scaling by the reciprocal tangent of the half
     * angle. Either way the matrices map camera space x to screen x, camera space y to depth, and
     * camera space z to negated screen y, which establishes that this engine looks down its y
     * axis with z upwards. UpdateWorldProject() runs afterwards.
     *
     * @ghidraAddress 0x004afac0
     */
    void UpdateProjection();

    /**
     * Compose the world projection, its inverse, and the world frustum.
     *
     * The inverse of the world transform becomes the world-to-camera matrix, the six local frustum
     * planes are moved into world space, the world projection is the world-to-camera matrix
     * followed by the local projection, and the inverse world projection is the inverse local
     * projection followed by the world transform.
     *
     * @ghidraAddress 0x004afc18
     */
    void UpdateWorldProject();

    /**
     * Set the texture this camera draws into, or none to draw into the frame buffer.
     *
     * The previous target loses its reference on this camera and the new one gains one. A new
     * target is then resized, through a call that takes its width, its height, and a depth capped
     * at 0x20, after which the render target is acquired and UpdateTargetAspect() runs.
     *
     * The routine is not virtual. `Rnd::PsCam` declares a virtual of the same title that chains
     * here, which is what places that virtual in a slot of its own rather than in one of these.
     *
     * @param pTex The render target, or null to draw into the frame buffer.
     * @ghidraAddress 0x004ad6f8
     */
    void SetTargetTex(Tex *pTex);

    /**
     * Place a point of the screen rectangle in render target pixels.
     *
     * Vtable slot 4 of the Rnd::Drawable table. This implementation writes nothing and returns its
     * result slot untouched, so the value a caller receives is indeterminate. Only
     * Rnd::PsCam::ScreenToPixels() produces a result.
     *
     * @param ptScreen The point, in the coordinates the screen rectangle is expressed in.
     * @return The same point in render target pixels.
     * @ghidraAddress 0x004b2000
     */
    virtual Vector2 ScreenToPixels(const Vector2 &ptScreen);

    /**
     * Take the aspect ratio from the render target and rebuild the projection.
     *
     * Vtable slot 5 of the Rnd::Drawable table. With a render target the vertical ratio becomes
     * the target height divided by its width. With none the ratio is left as it is, which is what
     * Rnd::PsCam overrides to supply a default.
     *
     * @ghidraAddress 0x004b2738
     */
    virtual void UpdateTargetAspect();

    /**
     * Report the class key a `.rnd` file writes for a camera.
     *
     * The returned string is the global at `0x006f9590`, which the class registration fills with
     * "Cam".
     *
     * @return The class key.
     * @ghidraAddress 0x004b23d0
     */
    virtual const HxStr &ClassName() const;

    /**
     * Write a description of this camera to sink.
     *
     * The three base descriptions come first. The near plane, the far plane, the field of view,
     * the screen rectangle, the depth range, and the render target name follow at any positive
     * dump level, and the vertical ratio, both local matrices, both frustums, and the inverse
     * world projection only from level two. A render target with no name produces "no object".
     *
     * @param sink The diagnostic sink to write to.
     * @ghidraAddress 0x004ad980
     */
    virtual void DumpText(FailSink &sink);

    /**
     * Repoint the render target when the object it addressed is replaced.
     *
     * The three base implementations run first. A replacement is cast to Rnd::Tex, so a
     * replacement that is not a texture clears the render target rather than storing a pointer of
     * the wrong type.
     *
     * @param pFrom The object going away.
     * @param pTo The object to store instead, or null.
     * @ghidraAddress 0x004b2608
     */
    virtual void Replace(Object *pFrom, Object *pTo);

    virtual void Save(Stream &stream);
    virtual void Copy(const Object *pSource, unsigned nFlags);
    virtual void Load(Stream &stream);

    /**
     * Build a camera the class registry vends.
     *
     * @param name The registry key for the new camera.
     * @return The new camera.
     * @ghidraAddress 0x004b2470
     */
    static Cam *NewCam(const HxStr &name);

    // Declared in recovered offset order, with the access specifiers interleaved. Each member that
    // remains public is read directly by a class outside this hierarchy, and the image exposes no
    // accessor for it.

    /**
     * Inverse of the world transform, which maps world space into camera space.
     *
     * Rnd::Mesh::PrepareDraw() reads the second component of the translation row to obtain the
     * depth of a bounding sphere, which is what proves the routine that fills it is an inverse
     * rather than a transpose. +0xe0
     */
    Vector3 mWorldToCam[kXfmRowCount];

    /** Projection from camera space onto the screen rectangle. +0x120 */
    Vector3 mLocalProject[kXfmRowCount];

private:
    // Inverse of mLocalProject. Only UpdateProjection() fills it and only UpdateWorldProject()
    // reads it.
    Vector3 mInvLocalProject[kXfmRowCount]; // +0x160

public:
    /** mWorldToCam followed by mLocalProject. +0x1a0 */
    Vector3 mWorldProject[kXfmRowCount];

    /** mInvLocalProject followed by the world transform. +0x1e0 */
    Vector3 mInvWorldProject[kXfmRowCount];

private:
    // The view volume in camera space, which UpdateProjection() builds from the four projection
    // parameters below.
    Frustum mLocalFrustum; // +0x220

public:
    /**
     * The view volume in world space.
     *
     * Rnd::Mesh::PrepareDraw() tests a bounding sphere against these planes. They are not the same
     * planes as Rnd::g_afDrawFrustumPlanes, which a separate test uses. +0x280
     */
    Frustum mWorldFrustum;

private:
    float mNearPlane; // +0x2e0
    float mFarPlane;  // +0x2e4
    float mFov;       // +0x2e8

protected:
    // Protected because Rnd::PsCam writes it when there is no render target.
    float mYRatio; // +0x2ec

private:
    // Depth the projected image is mapped into, as a low value in x and a high value in y.
    Vector2 mZRange; // +0x2f0

public:
    /**
     * Fraction of the render target the projected image is placed in.
     *
     * Rnd::Mesh::PrepareDraw() reads the width to scale its projected size estimate. +0x2f8
     */
    Rect mScreenRect;

    /**
     * Texture this camera draws into, or null to draw into the frame buffer.
     *
     * Rnd::PsMesh::DrawSelf() tests it and suppresses its depth register writes while it is set,
     * which is what makes a camera with a render target skip them. +0x308
     */
    Tex *mpTargetTex;

protected:
    /**
     * Report whether a ray starts inside the screen rectangle.
     *
     * Vtable slot 2 of the Rnd::Collideable table. The camera tests the start point of the ray
     * against mScreenRect rather than against geometry, which makes the second collision query of
     * Rnd::Collideable a screen space test here.
     *
     * @param ray The ray to test.
     * @param sink The collector to append an intersection to.
     * @ghidraAddress 0x004ad820
     */
    virtual void CollideUnknown(const Ray &ray, HitSink &sink);

    /**
     * Make this camera the one the frame is drawn through.
     *
     * Vtable slot 3 of the Rnd::Drawable table. Rnd::PsCam overrides it with the routine that also
     * submits the display registers.
     *
     * @return Non-zero, so the children are always drawn.
     * @ghidraAddress 0x004b1fe0
     */
    virtual int DrawSelf();

private:
    // 0x004b27a8. Registers this camera as a referrer of the render target and then runs
    // UpdateTargetAspect(). The constructor and SetTargetTex() are the callers.
    void AcquireTargetTex();

    // Neither written by the constructor nor read anywhere in the image. A reserved run records a
    // span that has not been recovered and is not a field. This one is either such a field or the
    // alignment the virtual base subobject at 0x310 is placed on.
    unsigned char mReserved30c[0x04];
};

/**
 * Camera the frame is being drawn through.
 *
 * Rnd::Cam::DrawSelf() stores itself here, the destructor clears it when it addresses the camera
 * going away, and Rnd::PsCam::DrawSelf() stores its own camera the same way.
 *
 * @ghidraAddress 0x006f9588
 */
extern Cam *g_pCurrentCam;

} // namespace Rnd
