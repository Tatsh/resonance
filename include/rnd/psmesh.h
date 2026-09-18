#pragma once

#include "rnd/mesh.h"

namespace Rnd {

/**
 * PlayStation 2 mesh, which draws through VU1 and the GS.
 *
 * `Q23Rnd6PsMesh` in the RTTI descriptor at `0x008efd60`, with `Rnd::Mesh` as its one public base
 * at offset 0. The shared Rnd::Object subobject moves to `+0x160` and the factory allocates 0x180
 * bytes, so the two lists below occupy `+0x150` through `+0x15f`.
 *
 * The subclass supplies the drawing the base class omits. Its Sync() converts the face vector into
 * triangle strips, and Refresh() runs the base fix-up and then normalises every vertex colour.
 * GfxDevice::Init() installs the creator at `0x00606928` over the mesh creator hook, so a mesh
 * loaded from a file is a PsMesh.
 *
 * Sync() rebuilds the cache on whichever mesh owns the faces rather than on this one. It reads
 * mFacesOwner and works through that mesh's two words at `+0x150` and `+0x154`, iterating to the
 * owner's edge vector end at `+0x104`, so a mesh that shares its geometry also shares the strips
 * built from it. The cache is a bit array allocated in 16-byte groups, one group per eight
 * halfwords of index. Sync() makes no contact with the VU1 microcode; it emits no VIF code and
 * calls no microprogram, so the geometry half of the draw path is recoverable from the MIPS text
 * alone.
 *
 * The VU1 paths upload geometry rather than register state. Both walk the 16-bit index list,
 * scaling each index by 0x40 to reach an Rnd::MeshVert, and copy the vertex into an UNPACK V4-32
 * batch whose VIFcode header they reserve ahead of the data and patch once the count is known. The
 * face path closes and reopens that batch every 252 quadwords. Each index list travels separately
 * in its packed form, UNPACK V3-16 for faces and V2-16 for edges, to VU address 0. Edges enter the
 * microprogram through MSCAL 0x1c2, always, and MSCNT continues an already-running program.
 *
 * How much of a vertex reaches VU1 depends on g_nStageTextureBound at `0x0076d668`, which
 * RndMat::BindStageTexture sets to 1 when a stage has a texture and 0 when it does not. With a
 * texture, DrawFacesVU1 uploads all four quadwords per vertex under STCYCL 1:1 and calls
 * microprogram address 0; without one it uploads three under STCYCL 4:3, leaving every fourth
 * destination quadword untouched, and calls 0x4ce. The write cycle keeps the destination stride at
 * four either way, so the fourth quadword of Rnd::MeshVert is the texture coordinate and the
 * untextured path simply omits it.
 *
 * Neither path builds a GIFtag. The microprogram cannot hold one: the whole `.vutext` section is
 * 1140 instruction pairs carrying exactly three literals, and all three are 255.0f, the vertex
 * colour scale. Nor can it assemble one integer-side, because a VU integer register is 16 bits and
 * a tag's PRIM field starts at bit 47. The primitive type therefore arrives in the parameter
 * quadwords the EE unpacks to VU addresses 8 and 9 for edges and 0x12 for faces, and where those
 * get their contents is not yet established.
 *
 * Recovery is partial. Sync() at `0x00600590` is 793 instructions of mostly inlined container
 * work and is not reconstructed yet, and neither is the GIF packet building. The other routines
 * are the vertex transform and lighting pass at `0x00584040`, the software face and edge paths at
 * `0x00601410` and `0x006017c0`, and the VU1 face and edge paths at `0x006019e0` and `0x00601de0`.
 *
 * The routines between `0x00604da0` and `0x00606678`, along with `0x00607170` and `0x00607198`,
 * are `std::vector`, `std::list`, `std::fill_n`, and `std::find` instantiations over
 * `unsigned short` and over the cache record. All of them are toolchain code, so none is
 * reconstructed here. Two of them, `0x00604da0` and `0x00605340`, operate on a different 20-byte
 * record altogether, a word, a word, an HxStr, and a word, which Sync() never touches; the earlier
 * names attributing them to a strip record were wrong.
 */
class PsMesh : public Mesh {
public:
    /**
     * Construct an empty PlayStation 2 mesh.
     *
     * @param name The object name, passed to the Rnd::Object constructor.
     * @ghidraAddress 0x00602600
     */
    PsMesh(const HxStr &name);

    /** @ghidraAddress 0x00605f48 */
    virtual ~PsMesh();

protected:
    /**
     * Draw the mesh.
     *
     * Picks the software or the VU1 path for the faces and the edges, then submits the packets.
     *
     * @return Non-zero when the children are to be drawn as well.
     * @ghidraAddress 0x00602128
     */
    virtual int DrawSelf();

    /**
     * Rebuild the triangle strips from the face vector.
     *
     * @ghidraAddress 0x00600590
     */
    virtual void Sync();

    /**
     * Restore the references, resynchronise, and renormalise every vertex colour.
     *
     * @ghidraAddress 0x00606a00
     */
    virtual void Refresh();

private:
    // Each word is the sentinel of a std::list whose node the constructor allocates at 0x10 bytes,
    // which puts an 8-byte element at the node's value offset. Neither element type is recovered.
    // No class derives from Rnd::PsMesh, so both are private.
    int mUnknown150; // +0x150
    int mUnknown154; // +0x154
};

/**
 * Allocate and construct a PlayStation 2 mesh.
 *
 * GfxDevice::Init() stores this creator in the mesh creator hook at `0x006eed60`.
 *
 * @param name The object name.
 * @return The new mesh.
 * @ghidraAddress 0x00606928
 */
Mesh *NewPsMesh(const HxStr &name);

} // namespace Rnd
