namespace Rnd {
struct Ray;
}
struct Sphere;

#pragma once

namespace Rnd {

/**
 * Triangle prepared for one ray test.
 *
 * The caller assembles this on the stack immediately before the test, as four consecutive
 * quadwords, which is why the call site writes four of them rather than passing three vertices.
 * Only the first three components of each quadword carry data.
 */
struct TriangleTest {
    float mVertex[4]; /*!< First vertex. +0x00 */
    float mEdge1[4];  /*!< Second vertex less the first. +0x10 */
    float mEdge2[4];  /*!< Third vertex less the first. +0x20 */
    float mNormal[4]; /*!< Cross product of the two edges. +0x30 */
};

/**
 * Test a ray against a bounding sphere.
 *
 * @param ray The ray, in the same space as the sphere.
 * @param sphere The sphere to test.
 * @param pflDistance Receives the distance to the strike.
 * @return Non-zero when the ray strikes the sphere.
 * @ghidraAddress 0x005501b0
 */
int TestRayAgainstSphere(const Ray &ray, const Sphere &sphere, float *pflDistance);

/**
 * Test a ray against one triangle.
 *
 * The facing test is skipped when nCull is Mat::kCullModeNone, which the routine recognises by
 * comparing the argument against 2 on entry.
 *
 * @param ray The ray, in the same space as the triangle.
 * @param tri The prepared triangle.
 * @param nCull The cull mode to apply.
 * @param pflDistance Receives the distance to the strike.
 * @return Non-zero when the ray strikes the triangle.
 * @ghidraAddress 0x0054fe98
 */
int TestRayAgainstTriangle(const Ray &ray, const TriangleTest &tri, int nCull, float *pflDistance);

} // namespace Rnd
