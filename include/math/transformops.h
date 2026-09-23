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

/**
 * Multiply two affine transforms on VU0.
 *
 * Every row of pB is pushed through pA, and the translation row of pA is added to the translation
 * row of the result. The vector register the accumulator lands in is never given a fourth word.
 * The fourth word of every destination row therefore receives whatever that register already
 * held. This reconstruction does not write those four words at all.
 *
 * XfmConcat() is a wrapper that swaps the two factors.
 *
 * @param pDst Receives the product, four rows of four floats.
 * @param pA The left factor, four rows of four floats.
 * @param pB The right factor, four rows of four floats.
 * @ghidraAddress 0x005e7ab0
 */
void Mat44Multiply(float *pDst, const float *pA, const float *pB);

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
 * @ghidraAddress 0x004f0430
 */
void EulerAnglesToMatrix3x3(const float *pAngles, float *pMat3Rows);

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
 * @ghidraAddress 0x004f0538
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
 * @ghidraAddress 0x002556c8
 */
void Mat33OrthonormalizeAroundY(const float *pSrc, float *pDst);

/**
 * Split a basis into Euler angles and per-axis scales.
 *
 * Each scale is the length of the matching row. The Z scale is negated when the three rows are
 * left handed. Negating it is the only way the remaining rotation can stay a pure rotation.
 * The angles are then extracted from the rows divided by their scales, and the extraction
 * inverts EulerAnglesToMatrix3x3(). A basis at the gimbal lock limit yields a Y angle of zero
 * and folds the whole remaining rotation into the Z angle.
 *
 * @param pMat3Rows The basis, three rows of four floats.
 * @param pAngles Receives the three angles in radians, ordered X, Y, and Z.
 * @param pScale Receives the three scales.
 * @ghidraAddress 0x004ee750
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
 * @ghidraAddress 0x00453ec8
 */
void MultiplyMat3VU0(const float *pMatA, const float *pMatB, float *pOut);

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
 * @ghidraAddress 0x0045da58
 */
void ScaleRows3x3(const float *pScale, const float *pMat3Rows, float *pOut);

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
 * @ghidraAddress 0x00453ea0
 */
void TransformVec3ByMat3VU0(const float *pVec, const float *pMat3Rows, float *pOut);

/**
 * Multiply two affine transforms on VU0 across all four words of every row.
 *
 * The algorithm matches Mat44Multiply() exactly. The one difference is the destination field
 * mask, which is `xyzw` here and `xyz` there, so this variant also multiplies the fourth column
 * of the left factor through and adds the fourth word of the translation row. The two bodies sit
 * next to each other in the image, at `0x005e7a58` and `0x005e7ab0`, which is what two
 * instantiations of one inline routine under different masks produce. Every caller of this variant
 * is a PlayStation 2 render path that consumes the fourth word.
 *
 * @param pDst Receives the product, four rows of four floats.
 * @param pA The left factor, four rows of four floats.
 * @param pB The right factor, four rows of four floats.
 * @ghidraAddress 0x005e7a58
 */
void Mat44Concat(float *pDst, const float *pA, const float *pB);
