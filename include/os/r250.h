#pragma once

/**
 * Words in the R250 generator's state table. SeedR250() fills all of them.
 */
constexpr int kR250TableSize = 256;

/**
 * State table of the R250 generator.
 *
 * SeedR250() fills every word, and NextR250() replaces one word per draw with the exclusive or of
 * two others.
 *
 * @ghidraAddress 0x0071c8e8
 */
extern int g_aR250Table[kR250TableSize];

/**
 * Index of the word NextR250() replaces next. Zero at startup and after SeedR250().
 *
 * @ghidraAddress 0x0071c8e0
 */
extern int g_nR250Index;

/**
 * Index of the word NextR250() combines with the replaced one. 103 at startup and after SeedR250().
 *
 * @ghidraAddress 0x0071c8e4
 */
extern int g_nR250LagIndex;

/**
 * Fill the state table from a linear congruential sequence and reset both indices.
 *
 * Each word is the high half of one step of the sequence plus the next step masked to
 * 0x7fff0000.
 *
 * MetRenderer's constructor and the routine at `0x0030e2c8` are the recovered callers.
 *
 * @param nSeed The starting value of the sequence.
 * @ghidraAddress 0x0052d038
 */
void SeedR250(int nSeed);

/**
 * Draw the next value.
 *
 * Both indices wrap to zero on reaching 249, so the draws cycle through the first 249 words of the
 * table.
 *
 * @return The replaced word.
 * @ghidraAddress 0x0052cfd0
 */
int NextR250();

/**
 * Draw an integer in a half-open range.
 *
 * @param nLow The smallest value.
 * @param nHigh The value the range stops below.
 * @return nLow plus the remainder of a draw divided by the width of the range.
 * @ghidraAddress 0x0052d098
 */
int RandomInt(int nLow, int nHigh);

/**
 * Draw a float in [0, 1).
 *
 * @return The low sixteen bits of a draw divided by 65536.
 * @ghidraAddress 0x0052d0e0
 */
float RandomFloat();
