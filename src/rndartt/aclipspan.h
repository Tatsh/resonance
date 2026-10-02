#pragma once

/**
 * Columns and rows of a run length encoded copy that remain inside a clip rectangle.
 *
 * The record is not polymorphic and has no RTTI, and the image retains no title for it. The name
 * here follows the `A` prefix every other art library type uses and the routine that fills it,
 * ACanvas::ClipBlitSpan(), and is therefore inferred rather than recovered.
 *
 * The record is six bytes, three signed halfwords at 0x00, 0x02, and 0x04, which is the layout
 * ACanvas::ClipBlitSpan() writes through its last argument. The column pair is relative to the
 * decoded source row, and the stop row is a destination row.
 */
struct AClipSpan {
    short mSkipLeft;   /*!< Decoded columns that fall left of the clip rectangle. +0x00 */
    short mStopColumn; /*!< One past the last decoded column that remains. +0x02 */
    short mStopRow;    /*!< One past the last destination row that remains. +0x04 */
};
