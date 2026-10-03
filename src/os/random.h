#pragma once

/**
 * Seed of NextRandomValue(), 1 until the first draw.
 *
 * NextRandomValue() is the only reader and the only writer.
 *
 * @ghidraAddress NTSC-U/C: 0x0072407c
 * @ghidraAddress PAL: 0x00767c6c
 */
extern int g_nRandomSeed;

/**
 * Advance the shared seed and return it.
 *
 * The step is the RANDU generator, the seed times 65539 masked to 31 bits, so every result is
 * non-negative and odd.
 *
 * @return The new seed.
 * @ghidraAddress NTSC-U/C: 0x0054f770
 * @ghidraAddress PAL: 0x0058fdb0
 */
int NextRandomValue();
