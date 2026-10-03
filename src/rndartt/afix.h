#pragma once

/**
 * Fixed-point number in 24.8 format.
 *
 * Its name comes from the debugging symbols of the North American demo release. The routines of
 * ACanvas pass 24.8 values as plain words. Only the five constants are modelled. The static
 * initialiser at 0x006209b0 computes onehalf as 0x100 divided by 2 and stores the other four as
 * immediates.
 */
class AFix {
public:
    /**
     * One half, 0x80.
     *
     * The polygon fill routines of ACanvas add it to an edge position before truncating. The
     * addition rounds the edge to the nearest column.
     *
     * @ghidraAddress NTSC-U/C: 0x007a82c0
     * @ghidraAddress PAL: 0x007ebfc0
     */
    static int onehalf;

    /**
     * Euler's number, 695.
     *
     * No reader was located.
     *
     * @ghidraAddress NTSC-U/C: 0x007a82c8
     * @ghidraAddress PAL: 0x007ebfc8
     */
    static int e;

    /**
     * Pi, 804.
     *
     * No reader was located.
     *
     * @ghidraAddress NTSC-U/C: 0x007a82d0
     * @ghidraAddress PAL: 0x007ebfd0
     */
    static int pi;

    /**
     * The smallest positive value, 1.
     *
     * ACanvas::ClipLine() subtracts it from the exclusive right and bottom clip edges. The
     * subtraction places a clipped endpoint on the last column or row that remains inside.
     *
     * @ghidraAddress NTSC-U/C: 0x007a82d8
     * @ghidraAddress PAL: 0x007ebfd8
     */
    static int epsilon;

    /**
     * The largest value whose whole part fits a signed halfword, 0x7fffff.
     *
     * No reader was located.
     *
     * @ghidraAddress NTSC-U/C: 0x007a82e0
     * @ghidraAddress PAL: 0x007ebfe0
     */
    static int max;
};
