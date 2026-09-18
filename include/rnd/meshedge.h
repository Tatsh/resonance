#pragma once

namespace Rnd {

/**
 * One drawn edge of a Rnd::Mesh, as two indices into its vertex vector.
 *
 * The structure is not polymorphic and emits no RTTI descriptor. Its member names come from the
 * labels its own text dump writes, "(v1:" and " v2:", each printed with "%hu". An edge is 4 bytes,
 * which the vector stride in every loop over the edge vector confirms.
 *
 * Edges appeared in serial version 5. A file written before that version has no edge vector, and
 * the loader clears the vector when the legacy flag that accompanied edges in versions 5 through 7
 * is false.
 */
struct MeshEdge {
    unsigned short mV1; // +0x00
    unsigned short mV2; // +0x02
};

} // namespace Rnd
