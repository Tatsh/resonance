#pragma once

#include "rnd/lodmesh.h"

class HxStr;
struct Color;

namespace Rnd {

class Tunnel;

/** Slice value of a TunnelSeekSection that is not placed. */
constexpr int kTunnelSeekNoSlice = -9999;

/**
 * One section of a Rnd::TunnelSeekStrip, a copy of the tunnel's cell geometry placed on one slice.
 *
 * The record is not polymorphic and has no RTTI, and the image retains no title for it. The name is
 * inferred from the format "[%s_seek%d.%d]" that labels its meshes with the strip index and then
 * this section's index.
 *
 * The record is 0x20 bytes, the element size of the section vector of Rnd::TunnelSeekStrip. Its
 * destructor is the implicit one, emitted at 0x00478018, which deletes the meshes through
 * Rnd::LodMesh. Its copy constructor is the implicit one, emitted at 0x00471e28. It copies
 * the three leading words, mMeshes, and the two trailing words.
 *
 * mDirty defers the geometry copy. Set() only records the slice, and Update() copies the vertices
 * of the matching tunnel cell the next time the section is drawn.
 */
class TunnelSeekSection {
public:
    /**
     * Construct an unplaced section.
     *
     * Sets mRing to -1 and mSlice to kTunnelSeekNoSlice. The other scalar members are left unset.
     *
     * @ghidraAddress NTSC-U/C: 0x0046ed38
     * @ghidraAddress PAL: 0x004ac938
     */
    TunnelSeekSection();

    /**
     * Place the section on a slice and ring, marking it dirty.
     *
     * @param nSlice The slice.
     * @param nRing The ring.
     * @param bEndCap Whether the section is the last of its strip.
     * @param bStartCap Whether the section is the first of its strip.
     * @ghidraAddress NTSC-U/C: 0x004780d0
     * @ghidraAddress PAL: 0x004b5d48
     */
    void Set(int nSlice, int nRing, int bEndCap, int bStartCap);

    /**
     * Remove the section from its slice, marking it dirty.
     *
     * @ghidraAddress NTSC-U/C: 0x004780f0
     * @ghidraAddress PAL: 0x004b5d68
     */
    void Invalidate();

    /**
     * Create one mesh per level of the tunnel's first cell, sharing that cell's triangles.
     *
     * Builds mMeshes with an internal title, points each level at the matching template level for
     * its faces, and copies the screen size thresholds. The finest level then draws with
     * kZModeZReadOnly and kZFuncEqual, which carries down the chain.
     *
     * @param name The base title of the meshes.
     * @param templates The chain whose triangles and thresholds to share.
     * @ghidraAddress NTSC-U/C: 0x0046ed78
     * @ghidraAddress PAL: 0x004ac978
     */
    void Build(const HxStr &name, const LodMesh &templates);

    /**
     * Copy the geometry of the tunnel cell the section is placed on and clear mDirty.
     *
     * The vertices are copied from the finest level of the cell into the finest level of this
     * section, recoloured with color, and given texture coordinates. The vertices are two rows. The
     * first row takes the horizontal coordinate 0 and the second 1, and every vertex takes the
     * vertical coordinate 0.5. A start cap overrides the vertical coordinate of the last vertex of
     * each row to 0, and an end cap that of the first vertex of each row to 1. The mesh is then
     * resynchronised whole.
     *
     * @param pTunnel The tunnel.
     * @param color The vertex colour.
     * @ghidraAddress NTSC-U/C: 0x0046ee80
     * @ghidraAddress PAL: 0x004aca80
     */
    void Update(Tunnel *pTunnel, const Color &color);

    int mDirty;      /*!< Set until Update() has copied the geometry. +0x00 */
    int mEndCap;     /*!< Set for the last section of the strip. +0x04 */
    int mStartCap;   /*!< Set for the first section of the strip. +0x08 */
    LodMesh mMeshes; /*!< The generated meshes, finest first. +0x0c */
    int mRing;       /*!< The ring the section sits on. +0x18 */
    int mSlice;      /*!< The slice, or kTunnelSeekNoSlice. +0x1c */
};

} // namespace Rnd
