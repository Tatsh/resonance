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
