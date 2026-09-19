#pragma once

/**
 * Rotation quaternion, stored as the vector part followed by the scalar part.
 *
 * The class is not polymorphic and emits no RTTI descriptor, and no text dump writes component
 * labels for it. The name is therefore inferred. The component order is recovered rather than
 * assumed, and three routines agree on it. Mat33ToQuat() writes half the square root of one plus
 * the matrix trace to the fourth word and the three antisymmetric differences of the off-diagonal
 * terms to the first three. AxisAngleToQuat() writes the cosine of the half angle to the fourth
 * word and the axis scaled by the sine of the half angle to the first three. QuatMultiply()
 * forms the Hamilton product with the fourth word as the real part.
 *
 * The type is a full quadword of meaningful data rather than a padded triple. The four-term dot
 * product in QuatSlerp() proves that.
 */
struct Quat {
    float x;
    float y;
    float z;
    float w;
};

/**
 * Build a rotation quaternion from an axis and an angle.
 *
 * The axis is used as supplied and is not normalised.
 *
 * @param pAxis The rotation axis, three floats.
 * @param flAngle The rotation angle in radians.
 * @return The quaternion.
 * @ghidraAddress 0x004efd80
 */
Quat AxisAngleToQuat(const float *pAxis, float flAngle);

/**
 * Recover the axis and angle of a rotation quaternion.
 *
 * A scalar part above one yields a zero angle rather than a domain error from the arc cosine. A
 * zero angle yields the Z axis, because the axis is otherwise indeterminate.
 *
 * @param quat The quaternion, treated as unit length.
 * @param pAxis Receives the axis, three floats.
 * @param pflAngle Receives the angle in radians.
 * @ghidraAddress 0x004f0350
 */
void QuatDecomposeAxisAngle(const Quat &quat, float *pAxis, float *pflAngle);

/**
 * Build a rotation quaternion from three Euler angles.
 *
 * The composition is the same one EulerAnglesToMatrix3x3() produces. That is, the product is
 * qz * qx * qy, with each factor a rotation about one axis by the matching component.
 *
 * @param pAngles The three angles in radians, ordered X, Y, and Z.
 * @return The quaternion.
 * @ghidraAddress 0x004f0230
 */
Quat EulerAnglesToQuat(const float *pAngles);

/**
 * Convert a rotation matrix to a quaternion.
 *
 * A positive trace takes the direct route. Otherwise the largest diagonal term selects which
 * component of the vector part is recovered first, and the other two components and the scalar
 * part follow from it. The routine is the Shoemake construction, including the cyclic successor
 * table.
 *
 * @param pMat3Rows The rotation, three rows of four floats, treated as orthonormal.
 * @return The quaternion.
 * @ghidraAddress 0x004ee9c0
 */
Quat Mat33ToQuat(const float *pMat3Rows);

/**
 * Compose two rotation quaternions.
 *
 * The product is the Hamilton product of a and b in that order.
 *
 * @param a The left factor.
 * @param b The right factor.
 * @param out Receives the product. Every component of both factors is loaded before the first
 *            store, which permits the destination to alias either factor.
 * @ghidraAddress 0x004f06a0
 */
void QuatMultiply(const Quat &a, const Quat &b, Quat &out);

/**
 * Compose a rotation quaternion with a rotation vector.
 *
 * The rotation vector encodes its axis by direction and its angle by length. The result is the
 * Hamilton product of the quaternion with the vector's own quaternion, in that order. A
 * zero-length vector divides by zero and is not guarded against.
 *
 * @param quat The rotation to start from.
 * @param pRotVec The rotation vector, three floats.
 * @return The composed rotation.
 * @ghidraAddress 0x004f0178
 */
Quat QuatRotateByVector(const Quat &quat, const float *pRotVec);

/**
 * Interpolate between two rotation quaternions along the shorter arc.
 *
 * A negative dot product flips the second quaternion. The interpolation therefore never takes
 * the long way round. Both endpoint values of the parameter are special cased and copy an
 * endpoint verbatim. Two nearly parallel inputs fall back to a component-wise linear blend,
 * because the sine of the half angle underflows there.
 *
 * The scale factors and the accumulation are double precision. This target has no double
 * precision unit, and the compiler expands them into software calls. The arithmetic is
 * genuinely slower than the surrounding single precision code. The trigonometry is single
 * precision throughout.
 *
 * @param from The rotation at a parameter of zero.
 * @param to The rotation at a parameter of one.
 * @param out Receives the interpolation.
 * @param flT The interpolation parameter.
 * @ghidraAddress 0x004eec20
 */
void QuatSlerp(const Quat &from, const Quat &to, Quat &out, float flT);

/**
 * Convert a rotation quaternion to a rotation matrix.
 *
 * The result is the transpose of the usual column-vector rotation, because this engine transforms
 * row vectors. Only nine of the twelve words are written, and the fourth word of each row is
 * untouched.
 *
 * @param quat The rotation, treated as unit length.
 * @param pMat3Rows Receives the rotation, three rows of four floats.
 * @ghidraAddress 0x004f0600
 */
void QuatToMat33(const Quat &quat, float *pMat3Rows);
