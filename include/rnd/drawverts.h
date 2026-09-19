#pragma once

#include "math/sphere.h"
#include "rnd/meshvert.h"

namespace Rnd {

/** Vertices a single draw may submit, from the overflow guard in Rnd::PsMesh::DrawSelf(). */
constexpr int kDrawVertCapacity = 2500;

/** Plane equations in the draw frustum. */
constexpr int kFrustumPlaneCount = 6;

/** Floats per plane equation, a quadword each. */
constexpr int kFrustumPlaneFloatCount = 4;

/**
 * Buffer every software draw path transforms its vertices into.
 *
 * One buffer serves the mesh paths and the particle system alike, so it is shared rather than
 * owned by any one of them. Exceeding the capacity is reported rather than clamped. The element
 * type is inferred from what the producers write, not measured.
 *
 * @ghidraAddress 0x00784960
 */
extern MeshVert g_aDrawVerts[kDrawVertCapacity];

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

} // namespace Rnd
