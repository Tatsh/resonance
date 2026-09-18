#pragma once

namespace Rnd {

/**
 * One triangle of a Rnd::Mesh, as three indices into its vertex vector.
 *
 * The structure is not polymorphic and emits no RTTI descriptor. Its member names come from the
 * labels its own text dump writes, "(v1:", " v2:", and " v3:", each printed with "%hu". A face is
 * 6 bytes, which the vector stride in every loop over the face vector confirms.
 */
struct MeshFace {
    unsigned short mV1; // +0x00
    unsigned short mV2; // +0x02
    unsigned short mV3; // +0x04
};

} // namespace Rnd
