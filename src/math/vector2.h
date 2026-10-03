#pragma once

/**
 * Two-component vector, used for texture coordinates.
 *
 * The class is not polymorphic and emits no RTTI descriptor. A mesh vertex stores two vectors
 * back to back at its end, and the binary loads and writes exactly two floats for each. The type
 * is therefore not padded to a quadword.
 */
struct Vector2 {
    float x;
    float y;
};

namespace Rnd {

/**
 * Add two two-component vectors.
 *
 * The destination may alias either source because both loads precede both stores. The routine
 * reads y before x and stores y before x, with the x store in the return delay slot.
 *
 * @param a The first vector.
 * @param b The second vector.
 * @param out Receives a plus b, and may alias either input.
 * @ghidraAddress NTSC-U/C: 0x00169818
 * @ghidraAddress PAL: 0x0016ba00
 */
void Add(const Vector2 &a, const Vector2 &b, Vector2 &out);

/**
 * Scale a two-component vector.
 *
 * The destination may alias the source because both loads precede both stores. The scale arrives
 * in the first floating-point argument register.
 *
 * @param v The vector.
 * @param flScale The scale.
 * @param out Receives v times flScale.
 * @ghidraAddress NTSC-U/C: 0x00169840
 * @ghidraAddress PAL: 0x0016ba28
 */
void Multiply(const Vector2 &v, float flScale, Vector2 &out);

/**
 * Negate a two-component vector.
 *
 * The destination may alias the source because both loads precede both stores.
 *
 * @param v The vector.
 * @param out Receives the negated vector.
 * @ghidraAddress NTSC-U/C: 0x004bec88
 * @ghidraAddress PAL: 0x004fcd10
 */
void Negate(const Vector2 &v, Vector2 &out);

/**
 * Scale a two-component vector to unit length.
 *
 * The zero vector yields the zero vector rather than a division by zero.
 *
 * @param v The vector.
 * @param out Receives the unit vector.
 * @ghidraAddress NTSC-U/C: 0x004beca8
 * @ghidraAddress PAL: 0x004fcd30
 */
void Normalize(const Vector2 &v, Vector2 &out);

/**
 * Report the length of a two-component vector.
 *
 * @param v The vector.
 * @return The length.
 * @ghidraAddress NTSC-U/C: 0x004bfcc0
 * @ghidraAddress PAL: 0x004fdd60
 */
float Length(const Vector2 &v);

} // namespace Rnd

/**
 * Subtract one two-component vector from another.
 *
 * The destination may alias either source because both loads precede both stores. The routine
 * reads y before x and stores y before x, with the x store in the return delay slot, matching
 * Rnd::Add().
 *
 * @param pA The vector subtracted from.
 * @param pB The vector subtracted.
 * @param pOut Receives pA minus pB, and may alias either input.
 * @ghidraAddress NTSC-U/C: 0x004bec40
 * @ghidraAddress PAL: 0x004fccc8
 */
void SubVec2(const float *pA, const float *pB, float *pOut);
