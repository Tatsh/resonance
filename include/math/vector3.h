#pragma once

/**
 * Three-component vector, padded to a PlayStation 2 quadword.
 *
 * The class is not polymorphic and emits no RTTI descriptor, so the name is inferred. Its text
 * dump writes three components under the labels "(x:", " y:", and " z:", and every vector unit
 * access in the renderer loads and stores all four words at once. The fourth word is therefore
 * padding rather than a homogeneous coordinate. Construction sets the padding word to 1.0, which
 * makes the quadword usable as a row of a transform.
 */
struct Vector3 {
    float x;
    float y;
    float z;
    float w; // +0x0c Padding for quadword access, set to 1.0 on construction.
};
