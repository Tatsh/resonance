#pragma once

/**
 * Seed of NextRandomValue(), 1 until the first draw.
 *
 * NextRandomValue() is the only reader and the only writer.
 *
 * @ghidraAddress 0x0072407c
 */
extern int g_nRandomSeed;

/**
 * Advance the shared seed and return it.
 *
 * The step is the RANDU generator, the seed times 65539 masked to 31 bits, so every result is
 * non-negative and odd.
 *
 * @return The new seed.
 * @ghidraAddress 0x0054f770
 */
int NextRandomValue();
