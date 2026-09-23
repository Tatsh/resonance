#pragma once

#include "rndartt/astretchspan.h"

/**
 * One stretched copy of a source bitmap into a destination rectangle, walked a row at a time.
 *
 * The record is not polymorphic and has no RTTI, and the image retains no title for it. The name
 * here is inferred from the routines that build and consume it, ACanvas::SetupStretchBlit() and the
 * ACanvas::StretchBlit() family.
 *
 * The record is 0x28 bytes. Its first 0x1c bytes are an AStretchSpan, which every consumer passes
 * as is to a stretch row slot, and the vertical walk follows. ACanvas::SetupStretchBlit() writes
 * every member except AStretchSpan::mY, which the row loop owns.
 */
struct AStretchBlit : AStretchSpan {
    short mTop;           /*!< The first destination row, clamped to the clip rectangle. +0x1c */
    short mBottom;        /*!< One past the last destination row, clamped likewise. +0x1e */
    int mSourcePositionY; /*!< The source row in 24.8 fixed point. +0x20 */
    int mSourceStepY;     /*!< The per row advance in 24.8 fixed point. +0x24 */
};
