#pragma once

#include "math/vector3.h"

/**
 * Bounding sphere.
 *
 * The class is not polymorphic and emits no RTTI descriptor, so the name is inferred. Its text
 * dump writes the two members under the labels "\n\tcenter:" and " radius:".
 *
 * The centre is quadword aligned on the PlayStation 2, which gives the whole structure 16-byte
 * alignment and a size of 0x20 bytes. Rnd::Mesh::CopyFrom() relies on that size and moves the
 * sphere as two quadwords.
 */
struct Sphere {
    /**
     * Grow this sphere to the smallest sphere that encloses both it and other.
     *
     * A zero-radius other changes nothing, and neither does an other already inside this sphere.
     * This sphere becomes other when other encloses it. A distinct centre otherwise places the new
     * centre halfway between the two far points along the line of the centres. The shipped program
     * does not call it, and the name is inferred.
     *
     * @param other The sphere to enclose.
     * @return This sphere.
     * @ghidraAddress 0x00550330
     */
    Sphere &GrowToContain(const Sphere &other);

    /**
     * Build the sphere whose diameter is the segment between two points.
     *
     * The shipped program does not call it, and the name is inferred.
     *
     * @param first One end of the diameter.
     * @param second The other end.
     * @return The sphere.
     * @ghidraAddress 0x005512b8
     */
    static Sphere Circumscribe(const Vector3 &first, const Vector3 &second);

    /**
     * Build the sphere through three points, as the binary computes it.
     *
     * The centre is where the plane of the triangle, the bisector plane of the first two points,
     * and a third plane meet. The third plane is the same bisector again rather than the bisector
     * of the first and third points, and the radius is the distance from the first point. The
     * result is the circumscribed sphere only by accident. The shipped program does not call it,
     * and the name is inferred.
     *
     * @param first The first point.
     * @param second The second point.
     * @param third The third point.
     * @return The sphere.
     * @ghidraAddress 0x00550510
     */
    static Sphere Circumscribe(const Vector3 &first, const Vector3 &second, const Vector3 &third);

    /**
     * Build the sphere through four points.
     *
     * The centre is where the bisector planes of the first point with each of the other three
     * meet, and the radius is the distance from the first point. The shipped program does not call
     * it, and the name is inferred.
     *
     * @param first The first point.
     * @param second The second point.
     * @param third The third point.
     * @param fourth The fourth point.
     * @return The sphere.
     * @ghidraAddress 0x00550850
     */
    static Sphere Circumscribe(const Vector3 &first,
                               const Vector3 &second,
                               const Vector3 &third,
                               const Vector3 &fourth);

    Vector3 mCenter; // +0x00
    float mRadius;   // +0x10
};
