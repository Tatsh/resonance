#pragma once

#include <list>
#include <vector>

#include "os/hxstr.h"
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
 * triangle strips, and Refresh() runs the base fix-up and then clamps every vertex colour.
 * GfxDevice::Init() installs the creator at `0x00606928` over the mesh creator hook, so a mesh
 * loaded from a file is a PsMesh.
 *
 * Both the constructor body and the destructor body are empty. The constructor at `0x00602600` is
 * the virtual-base and vptr setup, the Rnd::Mesh constructor, and the two list members, and the
 * destructor at `0x00605f48` is the two list members followed by the whole inlined Rnd::Mesh
 * destructor.
 *
 * Sync() rebuilds the cache on whichever mesh owns the faces rather than on this one. It reads
 * mFacesOwner and works through that mesh's two lists, iterating to the owner's edge vector end at
 * `+0x104`, so a mesh that shares its geometry also shares the strips built from it. The cache is a
 * bit array allocated in 16-byte groups, one group per eight halfwords of index. Sync() makes no
 * contact with the VU1 microcode; it emits no VIF code and calls no microprogram, so the geometry
 * half of the draw path is recoverable from the MIPS text alone.
 *
 * The VU1 paths upload geometry rather than register state. Both walk the 16-bit index list,
 * scaling each index by 0x40 to reach an Rnd::MeshVert, and copy the vertex into an UNPACK V4-32
 * batch whose VIFcode header they reserve ahead of the data and patch once the count is known. The
 * face path closes and reopens that batch every 252 quadwords. Each index list travels separately
 * in its packed form, UNPACK V3-16 for faces and V2-16 for edges, addressed immediately after the
 * vertex data rather than to VU address 0. A face batch enters the microprogram through MSCAL on
 * the first run of the list alone and MSCNT on every run after it, and an edge batch enters
 * through MSCAL 0x1c2 every time.
 *
 * How much of a vertex reaches VU1 depends on Rnd::g_nStageTextureBound, which
 * Rnd::PsMat::BindStageTexture() sets to 1 when a stage has a texture and 0 when it does not. With
 * a texture, DrawFacesVU1() uploads all four quadwords per vertex under STCYCL 1:1 and calls
 * microprogram address 0; without one it uploads three under STCYCL 4:3, leaving every fourth
 * destination quadword untouched, and calls 0x4ce. The write cycle retains the destination stride
 * of four either way, so the fourth quadword of Rnd::MeshVert is the texture coordinate and the
 * untextured path simply omits it.
 *
 * Neither path builds a GIFtag. The microprogram cannot store one: the whole `.vutext` section is
 * 1140 instruction pairs with exactly three literals, and all three are 255.0f, the vertex colour
 * scale. Nor can it assemble one integer-side, because a VU integer register is 16 bits and a
 * tag's PRIM field starts at bit 47. The primitive type therefore arrives in the parameter
 * quadword the EE unpacks to VU address 8 for edges and 0x12 for faces. Its four words are the
 * vertex count, the primitive count, a render-state word, and the clipping flag, and the
 * render-state word comes from `0x00583358` for faces and `0x005837d0` for edges. Both of those
 * read the whole cached material state, which is where the primitive type enters.
 *
 * Every member of the class is reconstructed. Sync() at `0x00600590` is 793 instructions, of
 * which the strip builder is the part that belongs here and the rest is inlined container work.
 * Two routines the VU1 paths depend on are not reconstructed and are not part of this class, the
 * per-pass setup emitters at `0x00583358` and `0x005837d0`, and what their returned word means is
 * undetermined.
 *
 * The routines between `0x00604da0` and `0x00606678`, along with `0x00607170` and `0x00607198`,
 * are `std::vector`, `std::list`, `std::fill_n`, and `std::find` instantiations over
 * `unsigned short` and over the list element below. All of them are toolchain code, so none is
 * reconstructed here.
 */
class PsMesh : public Mesh {
public:
    /**
     * Construct an empty PlayStation 2 mesh.
     *
     * The body is empty. Rnd::NewPsMesh() is the only construction site.
     *
     * @param name The object name, passed to the Rnd::Object constructor.
     * @ghidraAddress 0x00602600
     */
    PsMesh(const HxStr &name);

    /** @ghidraAddress 0x00605f48 */
    virtual ~PsMesh();

    /**
     * Write the GS depth mask and depth test a mesh needs for one material pass.
     *
     * Does nothing while the current camera renders to a texture, and nothing from the third pass
     * onward. DrawSelf() expands the same decision inline for its own passes, and this out-of-line
     * copy exists because Rnd::PsMultiMesh::DrawSelf() in another translation unit is its only
     * caller. Static rather than an instance method because the mesh arrives as an argument, and
     * the body reads only the two Rnd::Mesh depth fields.
     *
     * @param mesh The mesh whose depth mode and depth function decide the registers.
     * @param nPass The material pass index, counted from zero.
     * @ghidraAddress 0x00606d38
     */
    static void SelectDepthRegsForPass(const Mesh &mesh, int nPass);

protected:
    /**
     * Draw the mesh.
     *
     * Rejects the mesh through Mesh::CheckDrawVisible(), which also yields the world bounding
     * sphere, then tests that sphere against the view frustum to decide whether the draw needs
     * clipping. The face pass repeats once per material pass: it reserves GIF space, programs
     * ZBUF_1.ZMSK and TEST_1.ZTST from mZMode and mZFunc, selects the material, and submits either
     * DrawFacesVU1() or the software path fronted by TransformAndLightMeshVerts(). The edge pass
     * repeats the same shape once, always with depth writes masked off and with the material
     * forced to source-alpha blending.
     *
     * @return Non-zero when the children are to be drawn as well.
     * @ghidraAddress 0x00602128
     */
    virtual int DrawSelf();

    /**
     * Rebuild the draw runs from the face and the edge vectors.
     *
     * Each run is filled until the next primitive would not fit in the VU1 data memory a run may
     * occupy, then closed and a fresh one started. A vertex already in the run is reused rather
     * than sent twice, which is what makes a run a strip rather than a list of separate triangles.
     * The budget is expressed in VU1 destination quadwords, four for a face vertex and two for an
     * edge vertex, which agrees with the write cycle each VU1 batch programs, plus one per
     * primitive for its indices. A primitive that does not fit is un-done and reprocessed at the
     * head of the next run.
     *
     * The runs are built on mFacesOwner, so a mesh that shares its geometry shares them.
     *
     * @ghidraAddress 0x00600590
     */
    virtual void Sync();

    /**
     * Restore the references, resynchronise, and clamp every vertex colour to the unit range.
     *
     * @ghidraAddress 0x00606a00
     */
    virtual void Refresh();

private:
    /**
     * One batch of geometry the VU1 and software paths submit in a single go.
     *
     * The layout comes from the two VU1 paths, which read every field. mIndices is a packed VIF
     * data block copied to the GIF stream a quadword at a time, `(mIndexCount + 7) / 8` of them,
     * which is what identifies mIndexCount as a count of halfwords. Dividing it by three yields
     * the UNPACK V3-16 element count for a face run, and by two the UNPACK V2-16 count for an edge
     * run. mVertIndices selects which vertices of mVertsOwner travel with the batch, one 16-bit
     * index each, scaled by 0x40 to reach a Rnd::MeshVert.
     *
     * The structure is 0x14 bytes inside a 0x1c-byte list node, which the constructor draws from
     * the 0x20-byte allocator bucket. The node destructor at `0x006056f8` releases mIndices
     * through MemFree() and then the vector, and it zeroes mIndices and mIndexCount between the
     * two.
     */
    struct DrawRun {
        /** Packed VIF index data, in halfwords. +0x00 */
        unsigned short *mIndices;
        /** Halfwords of index data at mIndices. +0x04 */
        int mIndexCount;
        /** Vertices of mVertsOwner this batch uploads. +0x08 */
        std::vector<unsigned short> mVertIndices;
    };

    // Sync() builds both. Every reader takes them off mFacesOwner rather than off this mesh, so a
    // mesh that shares its geometry shares the batches built from it. No class derives from
    // Rnd::PsMesh, so both are private.
    std::list<DrawRun> mFaceRuns; // +0x150
    std::list<DrawRun> mEdgeRuns; // +0x154

    // Close one run, moving the two scratch index lists into a new node of runs. The compiler
    // inlined this at both of its call sites in Sync(), which is the only reason no address
    // belongs to it.
    static void AppendRun(std::list<DrawRun> &runs,
                          const std::vector<unsigned short> &vertIndices,
                          const std::vector<unsigned short> &primIndices);

    // Submit every face run of mFacesOwner through VU1. 0x006019e0.
    //
    // Advances the triangle counter by the face count, opens a VIF DMA chain, and builds the
    // render-state word from pXfm and mSphere. Then, once per run: the parameter quadword goes to
    // VU address 0x12 as {mVertIndices count, mIndexCount / 3, render state, nClip}, the selected
    // vertices follow it contiguously, the packed index data follows those, and the run enters the
    // microprogram. With a stage texture bound each vertex contributes all four of its quadwords
    // under the default write cycle and the program entry is address 0; without one each vertex
    // contributes three under STCYCL CL=4 WL=3, which retains the destination stride of four, and
    // the entry is 0x4ce. The textured path also closes and reopens the UNPACK every 252
    // destination quadwords, because the VIFcode NUM field is eight bits.
    void DrawFacesVU1(const float *pXfm, int nClip);

    // Submit every edge run of mFacesOwner through VU1. 0x00601de0.
    //
    // Advances the line counter by the edge count and builds the render-state word from pXfm and
    // the material specular colour, or from white when the mesh has no material. Each run then
    // sends its parameter quadword to VU address 8, the vertex positions to address 9 as one
    // quadword each under STCYCL CL=2 WL=1, and the packed index data after those. Only the first
    // quadword of each vertex travels, which is why an edge needs no normal, no colour, and no
    // texture coordinate. As with faces, the first run of the list enters through MSCAL, at
    // 0x1c2, and every run after it through MSCNT.
    void DrawEdgesVU1(const float *pXfm);

    // Submit the faces of mFacesOwner from the already transformed vertex buffer. 0x00601410.
    void DrawFacesSoftware(int nClip);

    // Submit the edges of mFacesOwner from the already transformed vertex buffer. 0x006017c0.
    void DrawEdgesSoftware(int nClip);
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
