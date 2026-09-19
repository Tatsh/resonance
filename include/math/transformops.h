#pragma once

/**
 * Invert a rigid transform into a separate destination.
 *
 * The three basis rows are transposed with the parallel pack instructions and the translation row
 * becomes the negated transposed basis applied to the original translation, which makes the result
 * the inverse rather than a plain transpose. An input whose basis is not orthonormal therefore does
 * not invert correctly, and no caller supplies one.
 *
 * @param pDst Receives the inverse, four rows of four floats.
 * @param pSrc The transform to invert, four rows of four floats.
 * @ghidraAddress 0x005e7b08
 */
void XfmInvertRigid(float *pDst, const float *pSrc);

/**
 * Compose two transforms on VU0.
 *
 * Each row of the second transform is applied to the first, which makes the result the transform
 * that applies pA and then pB.
 *
 * @param pA The transform applied first, four rows of four floats.
 * @param pB The transform applied second, four rows of four floats.
 * @param pOut Receives the composition and may not alias either input.
 * @ghidraAddress 0x0045dae8
 */
void XfmConcat(const float *pA, const float *pB, float *pOut);
