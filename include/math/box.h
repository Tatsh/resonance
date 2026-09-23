#pragma once

#include "math/vector3.h"

/**
 * Axis-aligned bounding box.
 *
 * The class is not polymorphic and emits no RTTI descriptor, and no dump labels its members. The
 * name and the member titles are therefore inferred from GrowToContain(), the one routine that
 * treats the two quadwords as a lower and an upper corner.
 */
struct Box {
    /**
     * Widen this box on each axis where a point falls outside it.
     *
     * An axis whose value is below the lower corner moves only the lower corner, and the upper
     * corner is tested only when the lower one did not move. Two unidentified routines, at
     * `0x00483030` and `0x00493fb8`, call it. The name is inferred.
     *
     * @param point The point to enclose.
     * @ghidraAddress 0x00551020
     */
    void GrowToContain(const Vector3 &point);

    Vector3 mMin; /*!< Lower corner. */
    Vector3 mMax; /*!< Upper corner. */
};
