#pragma once

#include <vector>

#include "math/color.h"
#include "rnd/tunnelseeksection.h"

namespace Rnd {

class Mat;
class Object;
class Tunnel;
struct TunnelSeeker;

/**
 * Run of consecutive tunnel slices a Rnd::TunnelSeeker highlights on one ring.
 *
 * The record is not polymorphic and has no RTTI. The name is inferred from its owner, the record
 * the tunnel stores one of per seeker, and from the "_seek" titles of the meshes its sections
 * generate.
 *
 * The record is 0x50 bytes and is the first member of Rnd::TunnelSeeker. The sections form a ring
 * buffer of three, indexed by slice modulo the section count, so the strip never shows more than
 * three slices at once.
 *
 * The copy constructor is the implicit one, emitted at 0x00471e90. It copies the leading members
 * word by word, mColor as one quadword, and then mSections element by element through the implicit
 * copy constructor of Rnd::TunnelSeekSection.
 */
struct TunnelSeekStrip {
    /**
     * Construct an empty strip with a white colour.
     *
     * The step counters at `+0x30` and `+0x34` are left unset until Build().
     *
     * @ghidraAddress 0x0046e7d8
     */
    TunnelSeekStrip();

    /**
     * Release the material reference and destroy the strip.
     *
     * The image has no separate copy. The body is inlined into the implicit destructor of
     * Rnd::TunnelSeeker at 0x00477cc0, which calls Clear() and then frees the section storage.
     */
    ~TunnelSeekStrip() {
        Clear();
    }

    /**
     * Release the material reference and empty the sections.
     *
     * mTunnel is cleared as well, and mMat is not.
     *
     * @ghidraAddress 0x0046ea98
     */
    void Clear();

    /**
     * Place the strip on a run of slices.
     *
     * Every section is invalidated first. The count is clamped to the section count, and each slice
     * of the run is placed in the section its slice index selects, with the first flagged as the
     * start cap and the last as the end cap.
     *
     * @param nFirstSlice The first slice of the run.
     * @param nSliceCount The number of slices.
     * @param nRing The ring.
     * @ghidraAddress 0x0046eb48
     */
    void SetRange(int nFirstSlice, int nSliceCount, int nRing);

    /**
     * Place the strip again on the run it last received.
     *
     * @ghidraAddress 0x00477ff0
     */
    void Refresh();

    /**
     * Generate the sections for a tunnel.
     *
     * Creates three sections titled "[<tunnel>_seek<index>.<section>]" from the first cell of the
     * tunnel mesh grid, hands mMat to the finest mesh of each, copies the tunnel step counters,
     * takes a reference on mMat on behalf of the tunnel, and then calls Refresh().
     *
     * @param pTunnel The tunnel.
     * @param pOwner The seeker that owns the strip.
     * @param nIndex The index of the seeker, used in the mesh titles.
     * @ghidraAddress 0x0046e830
     */
    void Build(Tunnel *pTunnel, TunnelSeeker *pOwner, int nIndex);

    /**
     * Replace the material and hand it to the finest mesh of every section.
     *
     * @param pMat The material, or null.
     * @ghidraAddress 0x00477f18
     */
    void SetMat(Mat *pMat);

    /**
     * Store a colour and recolour every section already updated.
     *
     * A dirty section takes the colour when Update() next copies its geometry.
     *
     * @param color The colour.
     * @ghidraAddress 0x00477e68
     */
    void SetColor(const Color &color);

    /**
     * Draw the section placed on one slice.
     *
     * Does nothing unless the section the slice selects is placed on that slice. A dirty section
     * is updated first, and the level of detail is chosen against flScreenSize the way
     * Rnd::TunnelMeshChain chooses it.
     *
     * @param nSlice The slice being drawn.
     * @param flScreenSize The screen size used to pick the level of detail.
     * @ghidraAddress 0x0046ec40
     */
    void DrawSection(int nSlice, float flScreenSize);

    /**
     * Replace the material reference.
     *
     * @param pFrom The object being replaced.
     * @param pTo The replacement, which must be a Rnd::Mat or null.
     * @param pReferrer The object the reference is held on behalf of.
     * @ghidraAddress 0x00477db8
     */
    void Replace(Object *pFrom, Object *pTo, Object *pReferrer);

    int mFirstSlice;                          /*!< The first slice of the run. */
    int mSliceCount;                          /*!< The number of slices in the run. */
    int mRing;                                /*!< The ring of the run. */
    Color mColor;                             /*!< The vertex colour of the sections. */
    Mat *mMat;                                /*!< The material of the sections. */
    Tunnel *mTunnel;                          /*!< The tunnel, which holds the references. */
    TunnelSeeker *mOwner;                     /*!< The owning seeker. */
    int mStepNumerator;                       /*!< The tunnel step numerator, truncated. */
    int mStepSize;                            /*!< mStepNumerator divided by mStepCount. */
    int mStepCount;                           /*!< The tunnel step divisor. */
    std::vector<TunnelSeekSection> mSections; /*!< The section ring buffer. */
};

} // namespace Rnd
