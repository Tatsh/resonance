#pragma once

#include "math/plane.h"

/**
 * Six-plane view volume.
 *
 * The class is not polymorphic and emits no RTTI descriptor, so the name is inferred. The member
 * order and the member titles both come from the dump routine at `0x0054f798`, which walks six
 * consecutive quadwords and writes them under "\n\tfront:", "\n\tback:", "\n\tleft:",
 * "\n\tright:", "\n\ttop:", and "\n\tbottom:". The order is therefore recovered rather than
 * assumed, and it is not the order the per-vertex clip flags of Rnd::DrawVert use.
 */
struct Frustum {
    Plane mFront;  // +0x00
    Plane mBack;   // +0x10
    Plane mLeft;   // +0x20
    Plane mRight;  // +0x30
    Plane mTop;    // +0x40
    Plane mBottom; // +0x50
};

/**
 * Build the six planes of a perspective view volume.
 *
 * Every plane is expressed in camera space. A caller that needs world space therefore transforms
 * the result afterwards. Rnd::Cam::UpdateProjection() is the only caller recovered.
 *
 * @param frustum Receives the six planes.
 * @param flNear Distance to the near plane.
 * @param flFar Distance to the far plane.
 * @param flFov Field of view in radians.
 * @param flAspect Vertical extent divided by the horizontal extent.
 * @ghidraAddress 0x00550b78
 */
void BuildFrustum(Frustum &frustum, float flNear, float flFar, float flFov, float flAspect);
