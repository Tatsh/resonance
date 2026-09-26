#pragma once

#include "gfx/gfxdevice.h"

struct Color;
struct Frustum;
namespace Rnd {
struct MeshVert;
struct Particle;
} // namespace Rnd
struct Sphere;

namespace Rnd {

/** Vertices a single draw may submit, from the overflow guard in Rnd::PsMesh::DrawSelf(). */
constexpr int kDrawVertCapacity = 2500;

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
    float mS; // +0x00
    float mT; // +0x04
    /**
     * Q, which the PACKED ST write passes on. TransformMeshVertsNoLight() multiplies S and T by
     * it. +0x08
     */
    float mQ;
    /** Which frustum planes and scissor edges this vertex falls outside. +0x0c */
    int mClipFlags;
    /** RGBAQ as four integers, already scaled to the range the GS takes. +0x10 */
    int mColor[4];
    /** XYZF2 as four integers, indexed by DrawVertPosLane. +0x20 */
    int mPos[4];
};

/** Lanes of DrawVert::mPos. */
enum DrawVertPosLane {
    kDrawVertPosX = 0,   /*!< Horizontal GS coordinate with four fractional bits. */
    kDrawVertPosY = 1,   /*!< Vertical GS coordinate with four fractional bits. */
    kDrawVertPosZ = 2,   /*!< Depth. */
    kDrawVertPosFog = 3, /*!< Fog term with four fractional bits. */
};

/** Integer lanes in DrawVert::mColor and DrawVert::mPos. */
constexpr int kDrawVertLanes = 4;

/** Every plane of the six-plane frustum. */
constexpr int kDrawVertClipAnyPlane = 0x3f;

/**
 * Far plane, the vclipw judgement z > w, which drops a triangle outright rather than clipping it.
 *
 * ClipTriangleToFrustum() clips against the opposite judgement at the camera near distance, which
 * identifies this one as the far plane.
 */
constexpr int kDrawVertClipFarPlane = 0x10;

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
 * World-space frustum IsSphereInsideFrustum() tests against.
 *
 * Rnd::PsCam::DrawSelf() rebuilds it for each camera, with the side planes widened by the guard
 * band. Rnd::Cam retains a second, per-camera set that Rnd::Mesh::PrepareDraw() uses instead. The
 * two tests do not share a plane set.
 *
 * @ghidraAddress 0x00768420
 */
extern Frustum g_drawFrustum;

/**
 * Report whether a sphere lies wholly inside a frustum.
 *
 * Each plane is dotted with the centre and the radius subtracted, on VU0 in macro mode, and the
 * accumulated sign bit decides the result. A sphere that straddles a plane therefore reports false,
 * which is what lets a caller skip clipping only on a true.
 *
 * @param sphere The sphere, in the space the planes are expressed in.
 * @param frustum The view volume.
 * @return Non-zero when no plane gives a negative distance.
 * @ghidraAddress 0x005514a0
 */
int IsSphereInsideFrustum(const Sphere &sphere, const Frustum &frustum);

/**
 * Report whether a sphere lies wholly inside the draw frustum, g_drawFrustum.
 *
 * The only copy in the image is an out-of-line emission at the end of the Rnd::PsCam unit with no
 * caller, which marks the routine as an inline of a header that unit includes. The name is
 * inferred.
 *
 * @param sphere The sphere, in world space.
 * @return Non-zero when no plane gives a negative distance.
 * @ghidraAddress 0x005884e0
 */
inline int IsSphereInDrawFrustum(const Sphere &sphere) {
    return IsSphereInsideFrustum(sphere, g_drawFrustum);
}

/**
 * Transform and light a run of vertices into the shared draw buffer.
 *
 * With lighting on, the lights are first brought into object space and culled against the sphere
 * through TransformLightRecords(). Each vertex colour then starts from the material emissive
 * colour and the environment ambient light, unless the material takes a term from the vertex
 * colour, and gains every directional light by the clamped cosine and every point light not culled
 * by the cosine and a linear falloff over its range, before the sum is clamped to the unit range.
 * Texture coordinates come from the vertex or from sphere mapping as in
 * TransformMeshVertsNoLight(), and positions are projected, scaled to the viewport, and given the
 * fog term. The vertices are processed 400 at a time with the long operation poll between
 * batches.
 *
 * The sphere arrives in the sixth argument register and passes through to
 * TransformLightRecords() untouched.
 *
 * @param pOutVerts Destination, normally g_aDrawVerts.
 * @param pXfm The transform, four rows of four floats.
 * @param pVerts The source vertices.
 * @param nCount How many vertices to transform.
 * @param bWriteClipFlags Non-zero to record per-vertex clip flags.
 * @param sphere The bounds the point lights are culled against.
 * @ghidraAddress 0x00584040
 */
void TransformAndLightMeshVerts(DrawVert *pOutVerts,
                                const float *pXfm,
                                MeshVert *pVerts,
                                int nCount,
                                int bWriteClipFlags,
                                const Sphere &sphere);

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
void TransformMeshVertsNoLight(DrawVert *pOutVerts,
                               MeshVert *pVerts,
                               int nCount,
                               const float *pXfm);

/**
 * Clip one triangle against the near plane and the scissor rectangle and append the polygon to the
 * vertex buffer.
 *
 * The three vertices first gain the scissor edge flags (0x40 left, 0x80 right, 0x100 top, 0x200
 * bottom) against g_nScissorX0 through g_nScissorY1, and a triangle whose vertices share a flag is
 * dropped. Otherwise g_renderStats.mnSplitTriangles is incremented and the polygon is clipped in a
 * nine-vertex scratch vector, one plane at a time and only against the planes some vertex falls
 * outside. The near plane is clipped where w equals g_flCamNear, with the texture coordinates and
 * colours interpolated by the perspective-corrected fraction, and the scissor edges linearly in
 * screen space. A flat-shaded material first gives the second and third vertices the colour of the
 * first.
 *
 * The polygon vertices go above the index the fifth argument points at, and that index is advanced
 * in place. The routine returns nothing: the value left in the return register at the single exit
 * is the advanced index, but it is there as a by-product of storing it through the pointer, and
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
 * Emit the VU1 parameter block for a face pass and return the microprogram entry to run.
 *
 * One UNPACK of 18 quadwords to VU address 0 carries the inverse guard band scale, the draw
 * transform composed with the unscaled projection, the colour scale, the unscaled viewport scale
 * and offset with the two fog terms, the selected texture coordinate transform, a triangle GIFtag
 * and a fan GIFtag built from the material state, and a five-quadword lighting block. With
 * lighting on the block is the light SelectLightForVertex() chooses, the material's emissive,
 * ambient, and diffuse colours, and its four vertex colour flags. With lighting off the block is
 * skipped, left as whatever the packet buffer held. Rnd::PsMesh::DrawFacesVU1() stores the result
 * in the third slot of each run's parameter quadword.
 *
 * @param pXfm The draw transform.
 * @param sphere The mesh bounding sphere.
 * @return The VU1 program entry, 0x2ee for an unlit pass.
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
 * Walks the linked live set from pFirst rather than the pool. The line mode passes over each
 * particle twice, first from Particle::mPos and then from the quadword nLineLength past it, with
 * the colour alpha scaled by 0.1 on the second pass.
 *
 * @param pOutVerts Destination, normally g_aDrawVerts.
 * @param nMode The point, line, or sprite mode of the emitting system.
 * @param pFirst Head of the live particle list.
 * @param nLineLength Rnd::ParticleSys::mLineLength of the emitting system.
 * @return Vertices packed, one per point particle and two per line or sprite particle.
 * @ghidraAddress 0x00584980
 */
int PackParticleQuads(DrawVert *pOutVerts, int nMode, const Particle *pFirst, int nLineLength);

/**
 * Non-zero while fog is enabled.
 *
 * Both software draw paths shift it into the GS PRIM fog-enable bit, which is what identifies it
 * as fog rather than a general flag. Rnd::PsEnviron::DrawSelf() is the one writer.
 *
 * @ghidraAddress 0x00776118
 */
extern int g_nFogEnabled;

} // namespace Rnd
