#pragma once

extern "C" {

/**
 * Invert a rigid transform.
 *
 * Sony libvu0, linked as shipped. The basis rows are transposed and the translation row becomes the
 * negated transposed basis applied to the original translation. An input whose basis is not
 * orthonormal therefore does not invert correctly.
 *
 * @param pDst Receives the inverse, four rows of four floats.
 * @param pSrc The transform to invert, four rows of four floats.
 * @ghidraAddress NTSC-U/C: 0x005e7b08
 * @ghidraAddress PAL: 0x00629cf0
 */
void InversMatrix(float *pDst, const float *pSrc);

/**
 * Multiply two affine transforms across all four words of every row.
 *
 * A member of the Sony libvu0 object, linked as shipped, whose published name is unknown. It is
 * sceVu0MulMatrix with the loop cut to three rows that ignore the fourth word of pB, and a fourth
 * row that adds the translation row of pA. Callers use it where the fourth word is consumed.
 *
 * @param pDst Receives the product, four rows of four floats. It may alias either factor.
 * @param pA The left factor, four rows of four floats.
 * @param pB The right factor, four rows of four floats.
 * @ghidraAddress NTSC-U/C: 0x005e7a58
 * @ghidraAddress PAL: 0x00629c40
 */
void sceVu0MulAffineMatrix(float *pDst, const float *pA, const float *pB);

/**
 * Multiply two affine transforms in the first three words of every row.
 *
 * A member of the Sony libvu0 object, linked as shipped, whose published name is unknown. It is
 * sceVu0MulAffineMatrix under an `xyz` destination mask. The fourth word of every destination row
 * therefore receives whatever the vector register already had.
 *
 * @param pDst Receives the product, four rows of four floats. It may alias either factor.
 * @param pA The left factor, four rows of four floats.
 * @param pB The right factor, four rows of four floats.
 * @ghidraAddress NTSC-U/C: 0x005e7ab0
 * @ghidraAddress PAL: 0x00629c98
 */
void sceVu0MulAffineMatrixXyz(float *pDst, const float *pA, const float *pB);

} // extern "C"

/**
 * Compose two transforms on VU0.
 *
 * Each row of the second transform is applied to the first, which makes the result the transform
 * that applies pA and then pB.
 *
 * @param pA The transform applied first, four rows of four floats.
 * @param pB The transform applied second, four rows of four floats.
 * @param pOut Receives the composition and may alias either input.
 * @ghidraAddress NTSC-U/C: 0x0045dae8
 * @ghidraAddress PAL: 0x0049b190
 */
void XfmConcat(const float *pA, const float *pB, float *pOut);

namespace Rnd {

/**
 * Build a rotation matrix from three Euler angles.
 *
 * The product is Ry * Rx * Rz, with each factor a rotation about one axis by the matching
 * component and every factor written for row vectors. Mat34DecomposeEulerScale() inverts the same
 * composition and EulerAnglesToQuat() reproduces it as a quaternion. Only nine of the twelve words
 * are written, and the fourth word of each row is untouched.
 *
 * @param pAngles The three angles in radians, ordered X, Y, and Z.
 * @param pMat3Rows Receives the rotation, three rows of four floats.
 * @ghidraAddress NTSC-U/C: 0x004f0430
 * @ghidraAddress PAL: 0x0052f020
 */
void MakeRotMatrix(const float *pAngles, float *pMat3Rows);

} // namespace Rnd

/**
 * Build an orthonormal basis around one axis and a reference direction.
 *
 * The Y row is the supplied axis normalised. The X row is that axis crossed with the reference
 * direction and normalised. The Z row is X crossed with Y, and needs no normalising. A reference
 * direction parallel to the axis yields a zero cross product and then a division by zero.
 *
 * Each row is written as a whole quadword, and the two cross products propagate the fourth word of
 * the supplied axis into the other two rows. All three rows therefore end with pAxisY[3]. The 1.0
 * that the inlined vector construction writes into the scratch quadword is overwritten before
 * anything reads it.
 *
 * @param pAxisY The direction the Y row takes, four floats.
 * @param pReference The reference direction, three floats.
 * @param pMat3Rows Receives the basis, three rows of four floats.
 * @ghidraAddress NTSC-U/C: 0x004f0538
 * @ghidraAddress PAL: 0x0052f128
 */
void Mat33BuildOrthonormal(const float *pAxisY, const float *pReference, float *pMat3Rows);

/**
 * Rebuild a basis as orthonormal around its Y row.
 *
 * The Y row is normalised. The X row becomes that Y row crossed with the source Z row, normalised,
 * and the Z row becomes X crossed with Y. Each row is written as a whole quadword, and the cross
 * products propagate the fourth word of the source Y row into the other two rows. pDst may be pSrc.
 * The source Z row is read before the destination Z row is written.
 * Rnd::Transformable::GetDrawXfm() rebuilds its camera-facing basis in place with it, and the
 * routines at `0x00254b30` and `0x00254c20` also call it.
 *
 * @param pSrc The basis to rebuild, three rows of four floats.
 * @param pDst Receives the orthonormal basis, three rows of four floats.
 * @ghidraAddress NTSC-U/C: 0x002556c8
 * @ghidraAddress PAL: 0x0026adc0
 */
void Mat33OrthonormalizeAroundY(const float *pSrc, float *pDst);

/**
 * Split a basis into Euler angles and per-axis scales.
 *
 * Each scale is the length of the matching row. The Z scale is negated when the three rows are
 * left handed. Negating it is the only way the remaining rotation can stay a pure rotation.
 * The angles are then extracted from the rows divided by their scales, and the extraction
 * inverts Rnd::MakeRotMatrix() from three angles. A basis at the gimbal lock limit yields a Y
 * angle of zero and folds the whole remaining rotation into the Z angle.
 *
 * @param pMat3Rows The basis, three rows of four floats.
 * @param pAngles Receives the three angles in radians, ordered X, Y, and Z.
 * @param pScale Receives the three scales.
 * @ghidraAddress NTSC-U/C: 0x004ee750
 * @ghidraAddress PAL: 0x0052d2f8
 */
void Mat34DecomposeEulerScale(const float *pMat3Rows, float *pAngles, float *pScale);

/**
 * Multiply two rotation matrices on VU0.
 *
 * Every row of pMatA is pushed through pMatB. A destination that is the right factor is routed
 * through three scratch quadwords. A destination that is the left factor needs no special
 * handling and gets none, because each row of pMatA is consumed before that row is written.
 * Each row is stored as a whole quadword, and the fourth word of the result comes from the
 * fourth word of the matching row of pMatA.
 *
 * @param pMatA The left factor, three rows of four floats.
 * @param pMatB The right factor, three rows of four floats.
 * @param pOut Receives the product, and may alias either factor.
 * @ghidraAddress NTSC-U/C: 0x00453ec8
 * @ghidraAddress PAL: 0x004913e8
 */
void MultiplyMat3VU0(const float *pMatA, const float *pMatB, float *pOut);

namespace Rnd {

/**
 * Scale each row of a rotation matrix by one component of a vector.
 *
 * Row zero takes the first component, row one the second, and row two the third. This is how a
 * non-uniform scale is folded into a basis. Only nine of the twelve words are written, and the
 * fourth word of each row is untouched.
 *
 * @param pScale The three scales.
 * @param pMat3Rows The basis to scale, three rows of four floats.
 * @param pOut Receives the scaled basis, and may alias pMat3Rows.
 * @ghidraAddress NTSC-U/C: 0x0045da58
 * @ghidraAddress PAL: 0x0049b100
 */
void Scale(const float *pScale, const float *pMat3Rows, float *pOut);

} // namespace Rnd

/**
 * Transform a three-component vector by a rotation matrix on VU0.
 *
 * The vector is treated as a row vector, and each component selects a row of the matrix. The
 * store moves a whole quadword while the accumulate stage writes only three components. The
 * fourth word of the destination therefore receives the fourth word of the source.
 *
 * @param pVec The vector to transform, four floats.
 * @param pMat3Rows The rotation, three rows of four floats.
 * @param pOut Receives the transformed vector, and may alias pVec.
 * @ghidraAddress NTSC-U/C: 0x00453ea0
 * @ghidraAddress PAL: 0x004913c0
 */
void TransformVec3ByMat3VU0(const float *pVec, const float *pMat3Rows, float *pOut);
