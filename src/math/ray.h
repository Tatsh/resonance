#pragma once

#include "math/vector2.h"

namespace Rnd {

/**
 * Line in a plane, given as a point on it and its direction.
 *
 * Its name comes from the debugging symbols of the North American demo release. The type is a
 * plain record with no RTTI. Rnd::Intersect() reads the point from the first two floats and the
 * direction from the next two.
 */
struct Ray {
    Vector2 mPoint;     /*!< A point on the line. */
    Vector2 mDirection; /*!< The direction of the line. */
};

/**
 * Find where two lines in a plane cross.
 *
 * Parallel lines report the point of the first line. Rnd::String::EmitRibbonVerts() is the one
 * caller.
 *
 * @param first The first line.
 * @param second The second line.
 * @return The crossing point.
 * @ghidraAddress NTSC-U/C: 0x00551190
 * @ghidraAddress PAL: 0x005917d0
 */
Vector2 Intersect(const Ray &first, const Ray &second);

} // namespace Rnd
