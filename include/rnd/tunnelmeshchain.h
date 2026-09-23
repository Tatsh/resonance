#pragma once

#include <vector>

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
 * vector storage is released, which is why emptying Rnd::Tunnel::mUnknowna4 deletes every mesh.
 *
 * Element zero is the finest level. Build() links each mesh to the next coarser one through
 * Mesh::mNext and points every level at element zero for its vertices and its transform.
 */
class TunnelMeshChain : public std::vector<Mesh *> {
public:
    /**
     * Delete every mesh of the chain.
     *
     * @ghidraAddress 0x00476a80
     */
    ~TunnelMeshChain() {
        DeleteMeshes();
    }

    /**
     * Delete every mesh through its virtual destructor and empty the chain.
     *
     * A null entry is skipped.
     *
     * @ghidraAddress 0x00476b28
     */
    void DeleteMeshes();

    /**
     * Replace the chain with nCount newly created meshes.
     *
     * The existing meshes are deleted first. The meshes are created from the last level down,
     * each through g_pfnNewMesh(), titled "[<name>.<level>]" when bInternal is set and
     * "<name>.<level>" otherwise, and each one takes the previously created mesh as its mNext.
     * Every mesh draws with kZModeZReadWrite and kZFuncLess and has no screen size threshold.
     * Every level after the first then takes element zero as its vertex owner, and every level
     * takes element zero as its transform owner.
     *
     * @param name The base title.
     * @param nCount The number of levels.
     * @param bInternal Whether the meshes are marked Rnd::Object::mInternal.
     * @ghidraAddress 0x004694a0
     */
    void Build(const HxStr &name, int nCount, bool bInternal);

    /**
     * Point the transform owner of every mesh at one mesh.
     *
     * @param pOwner The mesh whose transform every level draws with, which may be null.
     * @ghidraAddress 0x00476e48
     */
    void SetTransOwner(Mesh *pOwner);

    /**
     * Copy the screen size threshold of each level from another chain.
     *
     * Walks this chain and reads the same index of source, which has to be at least as long. Each
     * level then passes its own mNext back to Mesh::SetNext(), which drops and retakes the same
     * reference.
     *
     * @param source The chain to copy from.
     * @ghidraAddress 0x00469820
     */
    void CopyScreenSizes(const TunnelMeshChain &source);
};

} // namespace Rnd
