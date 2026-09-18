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
    Vector3 mCenter; // +0x00
    float mRadius;   // +0x10
};
