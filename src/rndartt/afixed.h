#pragma once

/**
 * One half in 24.8 fixed point, 0x80.
 *
 * The polygon fill routines of ACanvas add it to an edge position before truncating, which rounds
 * the edge to the nearest column.
 *
 * The five globals in this header are initialised at run time by the static initialiser at
 * 0x006209b0, which computes this one as 0x100 divided by 2 and stores the other four as
 * immediates. That translation unit defines nothing else, so its name is inferred from the
 * values it defines.
 *
 * @ghidraAddress NTSC-U/C: 0x007a82c0
 * @ghidraAddress PAL: 0x007ebfc0
 */
extern int g_nFixedHalf;

/**
 * Euler's number in 24.8 fixed point, 695.
 *
 * No reader was located.
 *
 * @ghidraAddress NTSC-U/C: 0x007a82c8
 * @ghidraAddress PAL: 0x007ebfc8
 */
extern int g_nFixedE;

/**
 * Pi in 24.8 fixed point, 804.
 *
 * No reader was located.
 *
 * @ghidraAddress NTSC-U/C: 0x007a82d0
 * @ghidraAddress PAL: 0x007ebfd0
 */
extern int g_nFixedPi;

/**
 * The smallest positive 24.8 fixed point value, 1.
 *
 * ACanvas::ClipLineToRect() subtracts it from the exclusive right and bottom clip edges, which
 * places a clipped endpoint on the last column or row that remains inside.
 *
 * @ghidraAddress NTSC-U/C: 0x007a82d8
 * @ghidraAddress PAL: 0x007ebfd8
 */
extern int g_nFixedEpsilon;

/**
 * The largest 24.8 fixed point value whose whole part fits a signed halfword, 0x7fffff.
 *
 * No reader was located.
 *
 * @ghidraAddress NTSC-U/C: 0x007a82e0
 * @ghidraAddress PAL: 0x007ebfe0
 */
extern int g_nFixedMax;
