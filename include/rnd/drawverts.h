#pragma once

#include "gfx/gfxdevice.h"

struct Color;
namespace Rnd {
struct MeshVert;
struct Particle;
} // namespace Rnd
struct Sphere;

namespace Rnd {

/** Vertices a single draw may submit, from the overflow guard in Rnd::PsMesh::DrawSelf(). */
constexpr int kDrawVertCapacity = 2500;

/** Plane equations in the draw frustum. */
constexpr int kFrustumPlaneCount = 6;

/** Floats per plane equation, a quadword each. */
constexpr int kFrustumPlaneFloatCount = 4;

/**
 * Vertex as the software draw paths leave it, ready for the GS.
 *
 * The stride is measured rather than inferred: the clipper multiplies its vertex index by 0x30.
 * The three quadwords are pinned by the GIFtags that consume them rather than by the stride alone.
 * A textured triangle declares NREG 9 with REGS `0x412412412`, which is ST, RGBAQ, and XYZF2 per
 * vertex, and copies three quadwords from `+0x00`. An untextured one declares NREG 6 with REGS
 * `0x414141` and copies two from `+0x10`. A line declares NREG 3 with REGS `0x441`, supplies its
 * own colour, and copies one quadword from `+0x20` per endpoint. Register order and copy offset
 * agree in all three cases.
 *
 * Only the first two words of the ST quadword reach the GS, and the path stores the clip flags in
 * the fourth.
 */
struct DrawVert {
    float mS;       // +0x00
    float mT;       // +0x04
    int mUnknown08; // +0x08
    /** Which frustum planes this vertex falls outside. +0x0c */
    int mClipFlags;
    /** RGBAQ, already scaled to the range the GS takes. +0x10 */
    GifQuadword mColor;
    /** XYZF2. +0x20 */
    GifQuadword mPos;
};

/** Every plane of the six-plane frustum. */
constexpr int kDrawVertClipAnyPlane = 0x3f;

/** Near plane, which drops a triangle outright rather than clipping it. */
constexpr int kDrawVertClipNearPlane = 0x10;

/** The five planes a triangle is rejected against only when all its vertices fail one. */
constexpr int kDrawVertClipOtherPlanes = 0x2f;

/**
 * Buffer every software draw path transforms its vertices into.
 *
 * One buffer serves the mesh paths and the particle system alike, and is shared rather than owned
 * by any one of them. The bound above is the guard the mesh path applies to its own vertex count,
 * not the size of the allocation: the clipper appends beyond that index and the triangle fan reads
 * its results back from there.
 *
 * @ghidraAddress 0x00784960
 */
extern DrawVert g_aDrawVerts[];

/**
 * Plane equations of the frustum IsSphereInsideFrustum() tests against.
 *
 * Rnd::Cam retains a second, per-camera set that Rnd::Mesh::PrepareDraw() uses instead. The two
 * tests do not share a plane set.
 *
 * @ghidraAddress 0x00768420
 */
extern float g_afDrawFrustumPlanes[kFrustumPlaneCount * kFrustumPlaneFloatCount];

/**
 * Report whether a sphere lies wholly inside a frustum.
 *
 * Each plane is dotted with the centre and the radius subtracted, on VU0 in macro mode, and the
 * accumulated sign bit decides the result. A sphere that straddles a plane therefore reports false,
 * which is what lets a caller skip clipping only on a true.
 *
 * @param sphere The sphere, in the space the planes are expressed in.
 * @param pPlanes The plane equations, kFrustumPlaneCount quadwords.
 * @return Non-zero when no plane gives a negative distance.
 * @ghidraAddress 0x005514a0
 */
int IsSphereInsideFrustum(const Sphere &sphere, const float *pPlanes);

/**
 * Transform and light a run of vertices into the shared draw buffer.
 *
 * The final argument exists in the prototype and the body never reads it. Two independent call
 * sites materialise the mesh bounding sphere into its register, which the compiler emits only for
 * a declared parameter, and the register is caller-saved, so nothing downstream reads it either.
 *
 * @param pOutVerts Destination, normally g_aDrawVerts.
 * @param pXfm The transform, four rows of four floats.
 * @param pVerts The source vertices.
 * @param nCount How many vertices to transform.
 * @param bWriteClipFlags Non-zero to record per-vertex clip flags.
 * @param sphereUnused Passed by every caller and read by none.
 * @ghidraAddress 0x00584040
 */
void TransformAndLightMeshVerts(void *pOutVerts,
                                const float *pXfm,
                                MeshVert *pVerts,
                                int nCount,
                                int bWriteClipFlags,
                                const Sphere &sphereUnused);

/**
 * Transform a run of vertices without lighting them.
 *
 * Returns immediately when no texture coordinate transform is selected or the count is zero, and
 * branches on the selected texture coordinate generation mode.
 *
 * @param pOutVerts Destination, normally g_aDrawVerts.
 * @param pVerts The source vertices.
 * @param nCount How many vertices to transform.
 * @param pXfm The transform, four rows of four floats.
 * @ghidraAddress 0x00584700
 */
void TransformMeshVertsNoLight(void *pOutVerts, MeshVert *pVerts, int nCount, const float *pXfm);

/**
 * Clip one triangle against the frustum and append the pieces to the vertex buffer.
 *
 * The new vertices go above the index the fifth argument points at, and that index is advanced in
 * place. The routine returns nothing: the value left in the return register at the single exit is
 * the advanced index, but it is there as a by-product of storing it through the pointer, and
 * neither caller reads the register.
 *
 * @param nIdx0 First vertex of the triangle.
 * @param nIdx1 Second vertex.
 * @param nIdx2 Third vertex.
 * @param pVerts The vertex buffer, indexed by the three arguments above.
 * @param pnNextIndex Where the next vertex is written, advanced by this call.
 * @ghidraAddress 0x00584cc8
 */
void ClipTriangleToFrustum(
    unsigned nIdx0, unsigned nIdx1, unsigned nIdx2, DrawVert *pVerts, int *pnNextIndex);

/**
 * Emit the VU1 parameter quadwords for a face pass and return its third word.
 *
 * The routine writes quadwords into the packet buffer and advances the write pointer, making it an
 * emitter rather than a builder. It returns a word deliberately, one path yielding the literal
 * 0x2ee, and Rnd::PsMesh::DrawFacesVU1() stores the result in the third slot of the parameter
 * quadword. What the word means is undetermined.
 *
 * @param pXfm The draw transform.
 * @param sphere The mesh bounding sphere.
 * @return The third word of the parameter quadword.
 * @ghidraAddress 0x00583358
 */
int EmitFaceVu1Setup(const float *pXfm, const Sphere &sphere);

/**
 * Emit the VU1 parameter quadwords for an edge pass.
 *
 * The colour is the material specular colour, or opaque white when the mesh has no material.
 *
 * @param pXfm The draw transform.
 * @param color The line colour.
 * @ghidraAddress 0x005837d0
 */
void EmitEdgeVu1Setup(const float *pXfm, const Color &color);

/**
 * Emit the VU1 parameter quadwords for a particle pass.
 *
 * Takes no argument. The routine overwrites the first argument register with the packet write
 * pointer before reading it, and the value the one call site leaves there is incidental. It
 * returns nothing: the single exit leaves the advanced write pointer in the return register as a
 * by-product of storing it, and the caller discards it.
 *
 * @ghidraAddress 0x005839d0
 */
void EmitParticleVu1Setup();

/**
 * Pack a run of live particles into the shared draw buffer.
 *
 * Walks the linked live set from pFirst rather than the pool, and stops at nMaxParticles.
 *
 * @param pOutVerts Destination, normally g_aDrawVerts.
 * @param nMode The point, line, or sprite mode of the emitting system.
 * @param pFirst Head of the live particle list.
 * @param nMaxParticles Population ceiling of the emitting system.
 * @return Vertices packed, one per point particle and two per line or sprite particle.
 * @ghidraAddress 0x00584980
 */
int PackParticleQuads(DrawVert *pOutVerts, int nMode, const Particle *pFirst, int nMaxParticles);

/**
 * Non-zero while fog is enabled.
 *
 * Both software draw paths shift it into the GS PRIM fog-enable bit, which is what identifies it
 * as fog rather than a general flag. Rnd::Environ::Select() is the writer.
 *
 * @ghidraAddress 0x00776118
 */
extern int g_nFogEnabled;

} // namespace Rnd
