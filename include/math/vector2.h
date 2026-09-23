#pragma once

/**
 * Two-component vector, used for texture coordinates.
 *
 * The class is not polymorphic and emits no RTTI descriptor, so the name is inferred. A mesh
 * vertex stores two of these back to back at its end, and the binary loads and writes exactly two
 * floats for each, so the type is not padded to a quadword.
 */
struct Vector2 {
    float x;
    float y;
};

/**
 * Add two two-component vectors.
 *
 * Both sources and the destination are separate arguments, and the destination may alias either
 * source because both loads precede both stores. The routine reads y before x and stores y before
 * x, with the x store in the return delay slot. No argument is a receiver and the routine operates
 * on no single class, so it is a free function of the vector library rather than a member.
 *
 * @param pA The first vector.
 * @param pB The second vector.
 * @param pOut Receives pA plus pB, and may alias either input.
 * @ghidraAddress 0x00169818
 */
void AddVec2(const float *pA, const float *pB, float *pOut);

/**
 * Subtract one two-component vector from another.
 *
 * The destination may alias either source because both loads precede both stores. The routine
 * reads y before x and stores y before x, with the x store in the return delay slot, matching
 * AddVec2().
 *
 * @param pA The vector subtracted from.
 * @param pB The vector subtracted.
 * @param pOut Receives pA minus pB, and may alias either input.
 * @ghidraAddress 0x004bec40
 */
void SubVec2(const float *pA, const float *pB, float *pOut);
