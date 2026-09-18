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
 * Recovery is partial. Sync() at `0x00600590` is 793 instructions of mostly inlined container
 * work and is not reconstructed yet, and neither is the GIF packet building. The other routines
 * are the vertex transform and lighting pass at `0x00584040`, the software face and edge paths at
 * `0x00601410` and `0x006017c0`, and the VU1 face and edge paths at `0x006019e0` and `0x00601de0`.
 *
 * The seven routines between `0x00604da0` and `0x00606678` are `std::vector` and `std::list`
 * instantiations over the strip record and over `unsigned short`, and the two at `0x00607170` and
 * `0x00607198` are `std::fill_n` and `std::find` over `unsigned short`. All nine are toolchain
 * code, so none of them is reconstructed here.
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
