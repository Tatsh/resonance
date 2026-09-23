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
 * Blend four consecutive floats linearly.
 *
 * Each result is `from + (to - from) * flT`. TnlCameraRig::SetFrame() blends a camera screen
 * rectangle with it. The title is inferred.
 *
 * @param pFrom The four values at flT of 0.
 * @param pTo The four values at flT of 1.
 * @param pOut Receives the four blended values.
 * @param flT The blend weight.
 * @ghidraAddress 0x00551078
 */
void InterpolateFourFloats(const float *pFrom, const float *pTo, float *pOut, float flT);

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

/**
 * Two points that fix a line or a segment.
 *
 * The routines that return one move it as two quadwords through a hidden result pointer. The name
 * is inferred.
 */
struct Segment {
    Vector3 mEnds[2]; /*!< The start, then the end. */
};

/**
 * Find the line two planes meet along.
 *
 * The direction is the cross product of the two normals, and the result is a point and that point
 * plus the direction. The point comes from a segment test against pSecond whose start is the
 * first normal scaled by the negated first distance and whose end is the cross product of the
 * direction with the first normal. That end is a direction treated as a position. The point
 * therefore lies on the second plane, and on the first only when the first distance is zero. The
 * title is inferred.
 *
 * @param first The first plane.
 * @param second The second plane.
 * @return The point, then the point plus the direction.
 * @ghidraAddress 0x0054fcf8
 */
Segment IntersectPlanes(const Plane &first, const Plane &second);

/**
 * Find the point three planes meet at.
 *
 * The line of the first two planes from IntersectPlanes() is tested against the third, and the
 * point is the one the segment test places at its fraction along that line. The shipped program
 * does not call it. The title is inferred.
 *
 * @param first The first plane.
 * @param second The second plane.
 * @param third The third plane.
 * @return The point.
 * @ghidraAddress 0x005510e0
 */
Vector3 IntersectPlanes(const Plane &first, const Plane &second, const Plane &third);
