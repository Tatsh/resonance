#pragma once

#include "math/vector3.h"

/**
 * Plane equation, padded to a PlayStation 2 quadword.
 *
 * The class is not polymorphic and emits no RTTI descriptor, so the name is inferred. Its text
 * dump writes four components under the labels "(a:", " b:", " c:", and " d:", and every vector
 * unit access loads and stores all four words at once. The first three components are the normal
 * and the fourth is the signed distance along it, which is what the plane transform at
 * `0x00550fa8` proves by rotating the first three and adjusting the fourth by the translation.
 */
struct Plane {
    float a;
    float b;
    float c;
    float d;
};

/**
 * Move a plane out of the space a transform is expressed in.
 *
 * The normal is rotated by the three basis rows on VU0 in macro mode, and the distance becomes the
 * original distance less the dot of the translation row with the rotated normal.
 *
 * @param plane The plane, in the space the transform maps from.
 * @param pXfm The transform, four rows of four floats.
 * @return The plane in the space the transform maps to.
 * @ghidraAddress 0x00550fa8
 */
Plane TransformPlaneToWorld(const Plane &plane, const float *pXfm);

/**
 * Find where a segment crosses a plane.
 *
 * The parameter is the signed distance of the start divided by the difference between the signed
 * distances of the start and the end. It is stored whether or not the segment crosses the plane.
 *
 * @param segment The start and the end of the segment.
 * @param plane The plane.
 * @param pT Receives the crossing as a fraction of the way from the start to the end.
 * @return Whether the fraction lies in `[0, 1]`.
 * @ghidraAddress 0x00551218
 */
bool IntersectSegmentWithPlane(const Vector3 segment[2], const Plane &plane, float *pT);
