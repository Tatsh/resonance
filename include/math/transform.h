#pragma once

#include "math/vector3.h"

/**
 * Affine transform stored as three basis rows and a translation row.
 *
 * The class is not polymorphic and emits no RTTI descriptor, so the name is inferred. The four
 * rows are quadwords the vector unit multiplies in place, which is why each row is a padded
 * Vector3 rather than a bare triple. Rnd::MatStage and Rnd::Transformable both store transforms
 * in this shape, and the identity every default constructor writes is the three unit basis rows
 * followed by a zero translation.
 */
struct Transform {
    Vector3 mBasisX;      // +0x00
    Vector3 mBasisY;      // +0x10
    Vector3 mBasisZ;      // +0x20
    Vector3 mTranslation; // +0x30
};
