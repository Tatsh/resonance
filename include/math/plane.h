#pragma once

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
