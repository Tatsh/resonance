#pragma once

/**
 * Half open rectangle of pixel coordinates.
 *
 * The record is not polymorphic and has no RTTI, and the image retains no title for it. The name
 * here follows the `A` prefix every other art library type uses, and is therefore inferred rather
 * than recovered.
 *
 * The record is eight bytes, four signed halfwords in left, top, right, bottom order. The layout
 * was recovered from three independent places. ACanvas stores its clip rectangle in exactly this
 * form at offset 0x18. ACanvas::FillRectNoClip() reads the four halfwords at 0x00, 0x02, 0x04, and
 * 0x06 of its argument and treats the first pair as the origin and the second as the limit. And
 * Intersection() takes the maximum of the two origins with the minimum of the two limits.
 *
 * The right and bottom edges are exclusive. ACanvas::FillRowNoClip() fills the column range from
 * mLeft up to but excluding mRight, and ACanvas::FrameRectNoClip() draws its bottom edge at mBottom
 * minus one.
 *
 * An eight byte class passed by value arrives through an invisible reference to a copy the caller
 * allocated. ACanvas::FillRect() proves the convention. The routine writes the clipped result back
 * through the pointer it received and then copies the result into a fresh stack record before
 * passing it on.
 */
struct ARect {
    /**
     * Intersect this rectangle with another.
     *
     * Takes the maximum of the two origins and the minimum of the two limits. An empty result is
     * reported through mLeft at or beyond mRight, or mTop at or beyond mBottom, rather than being
     * normalised.
     *
     * @param other The rectangle to intersect with.
     * @return The intersection.
     * @ghidraAddress 0x00613b58
     */
    ARect Intersection(const ARect &other) const;

    short mLeft;   /*!< The first column the rectangle admits. +0x00 */
    short mTop;    /*!< The first row the rectangle admits. +0x02 */
    short mRight;  /*!< One past the last column the rectangle admits. +0x04 */
    short mBottom; /*!< One past the last row the rectangle admits. +0x06 */
};
