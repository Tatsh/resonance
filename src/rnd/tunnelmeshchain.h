#pragma once

#include <vector>

#include "rnd/collideable.h"
#include "rnd/raytest.h"

class HxStr;

namespace Rnd {

class Mesh;

/**
 * Chain of generated meshes, one per level of detail, that deletes its meshes with itself.
 *
 * The class is not polymorphic and has no RTTI, and the image retains no title for it. The name is
 * inferred from its use. Rnd::Tunnel keeps one per grid cell and one per slice, and
 * Rnd::TunnelSeekSection keeps one per section. Its routines sit inside the Rnd::Tunnel
 * translation unit.
 *
 * The class is a `std::vector<Mesh *>` with a destructor. That destructor, at 0x00476a80, is what
 * the vector of chains runs for each element it destroys. It calls DeleteMeshes() before the
 * vector storage is released. Emptying Rnd::Tunnel::mCellChains therefore deletes every mesh.
 *
 * Element zero is the finest level. Build() links each mesh to the next coarser one through
 * Mesh::mNext and points every level at element zero for its vertices and its transform.
 */
class TunnelMeshChain : public std::vector<Mesh *> {
public:
    /**
     * Delete every mesh of the chain.
     *
     * @ghidraAddress NTSC-U/C: 0x00476a80
     * @ghidraAddress PAL: 0x004b46f8
     */
    ~TunnelMeshChain() {
        DeleteMeshes();
    }

    /**
     * Delete every mesh through its virtual destructor and empty the chain.
     *
     * A null entry is skipped.
     *
     * @ghidraAddress NTSC-U/C: 0x00476b28
     * @ghidraAddress PAL: 0x004b47a0
     */
    void DeleteMeshes();

    /**
     * Replace the chain with nCount newly created meshes.
     *
     * The existing meshes are deleted first. The meshes are created from the last level down,
     * each through g_pfnNewMesh(), called "[<name>.<level>]" when bInternal is set and
     * "<name>.<level>" otherwise, and each one takes the previously created mesh as its mNext.
     * Every mesh draws with kZModeZReadWrite and kZFuncLess, and every mesh with a coarser link
     * has its screen size threshold cleared.
     * Every level after the first then takes element zero as its vertex owner, and every level
     * takes element zero as its transform owner.
     *
     * @param name The base title.
     * @param nCount The number of levels.
     * @param bInternal Whether the meshes are marked Rnd::Object::mInternal.
     * @ghidraAddress NTSC-U/C: 0x004694a0
     * @ghidraAddress PAL: 0x004a6f60
     */
    void Build(const HxStr &name, int nCount, bool bInternal);

    /**
     * Point the transform owner of every mesh at one mesh.
     *
     * @param pOwner The mesh whose transform every level draws with, which may be null.
     * @ghidraAddress NTSC-U/C: 0x00476e48
     * @ghidraAddress PAL: 0x004b4ac0
     */
    void SetTransOwner(Mesh *pOwner);

    /**
     * Copy the screen size threshold of each level from another chain.
     *
     * Walks this chain and reads the same index of source, which has to be at least as long. Each
     * level passes its own mNext back to Mesh::SetNext() with the copied threshold, which drops and
     * retakes the same reference.
     *
     * @param source The chain to copy from.
     * @ghidraAddress NTSC-U/C: 0x00469820
     * @ghidraAddress PAL: 0x004a7318
     */
    void CopyScreenSizes(const TunnelMeshChain &source);

    /**
     * Give each level the screen size threshold of the same index.
     *
     * A level past the end of screenSizes is skipped. Each level passes its own mNext back to
     * Mesh::SetNext() with its threshold. The out-of-line copy has no callers, and Rnd::Tunnel
     * inlines the body.
     *
     * @param screenSizes The thresholds, finest level first.
     * @ghidraAddress NTSC-U/C: 0x00476c50
     * @ghidraAddress PAL: 0x004b48c8
     */
    void SetScreenSizes(const std::vector<float> &screenSizes);

    /**
     * Point each level at the triangles of the same level of another chain, then Sync() it.
     *
     * The out-of-line copy has no callers, and Rnd::TunnelSeekSection::Build() inlines the body.
     *
     * @param templates The chain whose triangles to share, at least as long as this one.
     * @ghidraAddress NTSC-U/C: 0x00476d28
     * @ghidraAddress PAL: 0x004b49a0
     */
    void ShareFaces(const TunnelMeshChain &templates);

    /**
     * Call Mesh::Sync() on every level.
     *
     * The out-of-line copy has no callers.
     *
     * @ghidraAddress NTSC-U/C: 0x00476de8
     * @ghidraAddress PAL: 0x004b4a60
     */
    void Sync();

    /**
     * Test a ray against every level of the chain.
     *
     * @param ray The segment to test along.
     * @param sink The collector to append intersections to.
     * @ghidraAddress NTSC-U/C: 0x00476ec0
     * @ghidraAddress PAL: 0x004b4b38
     */
    void Collide(const Ray &ray, Collideable::HitSink &sink);

    /**
     * Resize the vertices of the finest level, which every level draws.
     *
     * A new vertex sits at the origin with a zero normal, a white colour, and zero texture
     * coordinates.
     *
     * @param nCount The vertex count.
     * @ghidraAddress NTSC-U/C: 0x004698e8
     * @ghidraAddress PAL: 0x004a73e0
     */
    void SetVertexCount(unsigned nCount);

    /**
     * Draw the level of detail a screen size selects.
     *
     * Draws nothing when the chain is empty or its finest mesh is not showing. Otherwise the walk
     * starts at the finest level and moves to the next while that level is not the last and its
     * Mesh::mMinScreen is below flScreenSize, and the level it stops on is drawn.
     *
     * @param flScreenSize The projected screen size.
     * @ghidraAddress NTSC-U/C: 0x00476bc8
     * @ghidraAddress PAL: 0x004b4840
     */
    void Draw(float flScreenSize);
};

} // namespace Rnd
