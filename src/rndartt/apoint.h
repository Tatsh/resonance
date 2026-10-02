#pragma once

/**
 * Pair of fixed point coordinates.
 *
 * The record is not polymorphic and has no RTTI, and the image retains no title for it. The name
 * here follows the `A` prefix every other art library type uses, and is therefore inferred rather
 * than recovered.
 *
 * The record is eight bytes, two signed words. ACanvas::TextureRowIndexed() reads the horizontal
 * component at 0x00 and the vertical one at 0x04, and advances both by a second record of the same
 * shape, which is the only evidence of the layout.
 *
 * Both components are 24.8 fixed point in every use recovered so far.
 */
struct APoint {
    int mX; /*!< The horizontal component. +0x00 */
    int mY; /*!< The vertical component. +0x04 */
};
