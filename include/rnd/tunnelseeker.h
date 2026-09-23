#pragma once

#include "math/color.h"
#include "math/transform.h"
#include "rnd/tunnelseekstrip.h"

namespace Rnd {

class Mat;
class Mesh;
class Object;
class Transformable;
class Tunnel;

/**
 * Marker that travels around the tunnel rings towards a target ring.
 *
 * The record is not polymorphic and has no RTTI. The name is inferred from the "_seek" titles of
 * the meshes its strip generates and from UpdateLane(), which moves the lane a bounded step
 * towards mTargetRing on every frame.
 *
 * The record is 0x80 bytes, the element size of the seeker vector of Rnd::Tunnel. Its destructor is
 * the implicit one at 0x00477cc0, and the copy constructors at 0x004720c0, 0x00471e90, and
 * 0x00471e28 are compiler-generated. The tunnel holds every reference the record takes, so each
 * reference is released on behalf of mTunnel.
 *
 * Every frame Rnd::Tunnel::SetFrameSelf() evaluates the tunnel path at three offsets from the
 * current frame. The position and look offsets orient the transform handed to SetTransXfm(), and
 * the mesh offset places the transform handed to SetMeshXfm().
 */
struct TunnelSeeker {
    /**
     * Construct a seeker with no tunnel.
     *
     * mTransFrameOffset starts at -500. mUnknown54 is left unset.
     *
     * @ghidraAddress 0x00477768
     */
    TunnelSeeker();

    /**
     * Release the transformable and mesh references and clear the strip.
     *
     * The two pointers are not cleared.
     *
     * @ghidraAddress 0x004777c0
     */
    void ReleaseRefs();

    /**
     * Attach the seeker to a tunnel.
     *
     * Takes the tunnel's filtered frame as mFrame, takes references on mTrans and mMesh on behalf
     * of the tunnel, and builds the strip.
     *
     * @param pTunnel The tunnel.
     * @param nIndex The index of the seeker in the tunnel.
     * @ghidraAddress 0x00477830
     */
    void SetTunnel(Tunnel *pTunnel, int nIndex);

    /**
     * Replace the transformable the seeker drives.
     *
     * @param pTrans The transformable, or null.
     * @ghidraAddress 0x00477a48
     */
    void SetTrans(Transformable *pTrans);

    /**
     * Set the ring the lane moves towards.
     *
     * @param nRing The ring.
     * @ghidraAddress 0x00477ab8
     */
    void SetTargetRing(int nRing);

    /**
     * Replace the mesh the seeker draws.
     *
     * @param pMesh The mesh, or null.
     * @ghidraAddress 0x00477ac0
     */
    void SetMesh(Mesh *pMesh);

    /**
     * Set the path frame offset used to place the mesh.
     *
     * @param flOffset The offset from the tunnel frame.
     * @ghidraAddress 0x00477b30
     */
    void SetMeshFrameOffset(float flOffset);

    /**
     * Set the path frame offset used to place the transformable.
     *
     * @param flOffset The offset from the tunnel frame.
     * @ghidraAddress 0x00477b38
     */
    void SetTransFrameOffset(float flOffset);

    /**
     * Set the path frame offset the transformable is oriented towards.
     *
     * @param flOffset The offset from the tunnel frame.
     * @ghidraAddress 0x00477b40
     */
    void SetLookFrameOffset(float flOffset);

    /**
     * Place the strip on a run of slices.
     *
     * @param nFirstSlice The first slice of the run.
     * @param nSliceCount The number of slices.
     * @param nRing The ring.
     * @ghidraAddress 0x00477b48
     */
    void SetRange(int nFirstSlice, int nSliceCount, int nRing);

    /**
     * Set the colour of the strip.
     *
     * @param color The colour.
     * @ghidraAddress 0x00477b68
     */
    void SetColor(const Color &color);

    /**
     * Set the material of the strip.
     *
     * @param pMat The material, or null.
     * @ghidraAddress 0x00477b88
     */
    void SetMat(Mat *pMat);

    // The five accessors below are inline. Each has an out-of-line copy without callers.

    /**
     * Report the first slice of the strip.
     *
     * @return The first slice.
     * @ghidraAddress 0x00477ba8
     */
    int GetFirstSlice() const {
        return mStrip.mFirstSlice;
    }

    /**
     * Report the slice count of the strip.
     *
     * @return The number of slices.
     * @ghidraAddress 0x00477bb0
     */
    int GetSliceCount() const {
        return mStrip.mSliceCount;
    }

    /**
     * Report the ring of the strip.
     *
     * @return The ring.
     * @ghidraAddress 0x00477bb8
     */
    int GetRing() const {
        return mStrip.mRing;
    }

    /**
     * Report the colour of the strip.
     *
     * @return The colour, by value.
     * @ghidraAddress 0x00477bc0
     */
    Color GetColor() const {
        return mStrip.mColor;
    }

    /**
     * Report the material of the strip.
     *
     * @return The material, or null.
     * @ghidraAddress 0x00477bd0
     */
    Mat *GetMat() const {
        return mStrip.mMat;
    }

    /**
     * Draw the strip section placed on one slice.
     *
     * @param nSlice The slice being drawn.
     * @param flScreenSize The screen size used to pick the level of detail.
     * @ghidraAddress 0x00477bd8
     */
    void DrawSection(int nSlice, float flScreenSize);

    /**
     * Draw the mesh, when there is one.
     *
     * @ghidraAddress 0x00477bf8
     */
    void DrawMesh();

    /**
     * Move the lane towards the target ring and return it.
     *
     * The ring distance is wrapped into half the tunnel ring count either way. The step is the
     * number of tunnel frames since the last call, divided by the tunnel member at `+0x64` and
     * scaled by the ring distance when the distance exceeds one. A lane within one step of the
     * target snaps onto it.
     *
     * @return The new lane.
     * @ghidraAddress 0x0046e6a0
     */
    float UpdateLane();

    /**
     * Install a transform as the local transform of mTrans and mark it dirty.
     *
     * @param xfm The transform.
     * @ghidraAddress 0x00477c20
     */
    void SetTransXfm(const Transform &xfm);

    /**
     * Install a transform as the local transform of mMesh and recompose its world transform.
     *
     * @param xfm The transform.
     * @ghidraAddress 0x00477c58
     */
    void SetMeshXfm(const Transform &xfm);

    /**
     * Replace the transformable, mesh, and material references.
     *
     * @param pFrom The object being replaced.
     * @param pTo The replacement.
     * @param pReferrer The object the references are held on behalf of.
     * @ghidraAddress 0x004778c0
     */
    void Replace(Object *pFrom, Object *pTo, Object *pReferrer);

    TunnelSeekStrip mStrip;  /*!< The highlighted run of slices. */
    Transformable *mTrans;   /*!< The transformable the seeker drives. */
    int mUnknown54;          /*!< Unrecovered. +0x54 */
    int mTargetRing;         /*!< The ring the lane moves towards. */
    float mMeshFrameOffset;  /*!< The path frame offset of the mesh. */
    float mTransFrameOffset; /*!< The path frame offset of the transformable. */
    float mLookFrameOffset;  /*!< The path frame offset the transformable faces. */
    Mesh *mMesh;             /*!< The mesh the seeker draws. */
    Tunnel *mTunnel;         /*!< The tunnel, which holds the references. */
    float mLane;             /*!< The current ring position, fractional while moving. */
    float mFrame;            /*!< The tunnel frame of the last UpdateLane(). */
};

} // namespace Rnd
