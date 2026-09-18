#pragma once

/**
 * Linear colour with four float components.
 *
 * The class is not polymorphic and emits no RTTI descriptor, so the name is inferred. The text
 * dump of a material colour and of a mesh vertex colour writes four components under the labels
 * "(r:", " g:", " b:", and " a:", and the renderer copies the four words as one quadword.
 */
struct Color {
    float r;
    float g;
    float b;
    float a;
};
