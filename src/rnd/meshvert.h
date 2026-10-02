#pragma once

#include "math/color.h"
#include "math/vector2.h"
#include "math/vector3.h"

namespace Rnd {

/**
 * One vertex of a Rnd::Mesh.
 *
 * The structure is not polymorphic and emits no RTTI descriptor. Its member names come from the
 * labels its own text dump writes ("\n\tp:", "\n\tn:", "\n\tc:", "\n\tt1:", and " t2:") and from
 * the Rnd::MeshAnim dump, which titles the three animated channels "vertPointsKeys",
 * "vertTexsKeys", and "vertColorsKeys". A vertex is 0x40 bytes, and the two padded vectors at the
 * front make the whole record four quadwords for the vector unit.
 *
 * Serialisation writes fourteen floats per vertex in member order, the three components of each
 * padded vector and both components of each texture coordinate, so the padding words never reach
 * a file.
 */
struct MeshVert {
    Vector3 mPoint; // +0x00
    Vector3 mNorm;  // +0x10
    Color mColor;   // +0x20
    Vector2 mTex1;  // +0x30
    Vector2 mTex2;  // +0x38
};

} // namespace Rnd
