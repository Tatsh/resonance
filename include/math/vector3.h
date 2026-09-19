#pragma once

/**
 * Three-component vector, padded to a PlayStation 2 quadword.
 *
 * The class is not polymorphic and emits no RTTI descriptor, so the name is inferred. Its text
 * dump writes three components under the labels "(x:", " y:", and " z:", and every vector unit
 * access in the renderer loads and stores all four words at once. The fourth word is therefore
 * padding rather than a homogeneous coordinate. Construction sets the padding word to 1.0, which
 * makes the quadword usable as a row of a transform.
 */
struct Vector3 {
    float x;
    float y;
    float z;
    float w; // +0x0c Padding for quadword access, set to 1.0 on construction.
};

/**
 * Add two three-component vectors.
 *
 * Writes three components and does not touch the fourth word of the destination. The two inputs
 * arrive in $a0 and $a1 and the destination in $a2.
 *
 * @param pA The first vector.
 * @param pB The second vector.
 * @param pOut Receives pA plus pB, and may alias either input.
 * @ghidraAddress 0x0028c218
 */
void AddVec3(const float *pA, const float *pB, float *pOut);

/**
 * Subtract one three-component vector from another.
 *
 * Writes three components and does not touch the fourth word of the destination. The minuend
 * arrives in $a0, the subtrahend in $a1, and the destination in $a2.
 *
 * @param pA The vector subtracted from.
 * @param pB The vector to subtract.
 * @param pOut Receives pA less pB, and may alias either input.
 * @ghidraAddress 0x00317160
 */
void Vec3Sub(const float *pA, const float *pB, float *pOut);

/**
 * Multiply a three-component vector by a scalar.
 *
 * Writes three components and does not touch the fourth word of the destination. The source
 * arrives in $a0, the destination in $a1, and the factor in $f12. Integer and float argument
 * registers are allocated independently on this target. The declared position of the factor is
 * therefore an inference from the destination-last order the other helpers here use, and the
 * register evidence alone does not fix it.
 *
 * @param pSrc The vector to scale.
 * @param flScale The factor to apply.
 * @param pOut Receives the product, and may alias pSrc.
 * @ghidraAddress 0x0024ecd0
 */
void Vec3Scale(const float *pSrc, float flScale, float *pOut);

/**
 * Negate a three-component vector.
 *
 * Writes three components and does not touch the fourth word of the destination.
 *
 * @param pSrc The vector to negate.
 * @param pOut Receives the negation, and may alias pSrc.
 * @ghidraAddress 0x00492458
 */
void NegateVec3(const float *pSrc, float *pOut);

/**
 * Scale a three-component vector to unit length.
 *
 * The vector unit takes the reciprocal square root of the dot of the first three components with
 * themselves, then scales those three by it. Both the load and the store move a whole quadword.
 * The fourth word of the destination therefore receives the fourth word of the source unchanged.
 * A zero-length input is not guarded against.
 *
 * @param pSrc The vector to normalise.
 * @param pOut Receives the unit vector, and may alias pSrc.
 * @ghidraAddress 0x00476140
 */
void Vec3Normalize(const float *pSrc, float *pOut);
