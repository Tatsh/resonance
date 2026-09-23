#pragma once

#include <list>

#include "math/quaternion.h"
#include "rnd/animatable.h"
#include "rnd/drawable.h"
// Included for kXfmRowFloatCount, which this header uses by value, rather than for Transformable.
#include "rnd/transformable.h"

class FailSink;
namespace Rnd {
class Object;
class Stream;
} // namespace Rnd

namespace Rnd {

/**
 * Bit of the Copy() flags word that makes the copy share the source's keyframes.
 *
 * Only Rnd::TransAnim::Copy() tests it among the routines recovered so far. With the bit clear a
 * copy duplicates the three keyframe lists and owns its own frames; with it set the copy points
 * mFramesOwner at whatever the source's owner is and empties its own lists.
 */
constexpr unsigned kCopyShareFrames = 0x100;

/**
 * Animation that drives one transformable from three keyframe channels.
 *
 * `Q23Rnd9TransAnim` in the RTTI descriptor at `0x008ef240`, with `Rnd::Animatable` as a public
 * non-virtual base at offset 0 and `Rnd::Drawable` as a public non-virtual base at offset 0x18.
 * Those two offsets pin the Animatable subobject at 0x18 bytes and place this class's own members
 * from `+0x2c`, Drawable occupying 0x14 bytes from `+0x18`. `Rnd::View` repeats both offsets, so
 * the Animatable size is fixed from outside the class rather than from a constructor's stores.
 *
 * The three channels are independent. mTransKeys and mScaleKeys store vector keyframes and
 * mRotKeys quaternion keyframes, and each channel has its own interpolation mode. The member
 * titles come from the text DumpText() writes, "trans:", " framesOwner:", "rotKeys:",
 * "transKeys:", "scaleKeys:", "rotInterp:", " transInterp:", " scaleInterp:", "repeatTrans:", and
 * " followPath:".
 *
 * mFramesOwner points at the object whose keyframes this one animates from, which is normally
 * this object itself. Replace() and Copy() both use `mFramesOwner == this` as the mark of an
 * object that owns its own frames, and Replace() copies the departing owner's three lists in
 * before taking ownership.
 */
class TransAnim : public Animatable, public Drawable {
public:
    /** Interpolation a channel applies between two keyframes. */
    enum Interp {
        kInterpLinear = 0, /*!< Straight line between the two values. */
        kInterpTCB = 1     /*!< Kochanek-Bartels spline through the stored tangents. */
    };

    /** Floats of a keyframe's shape triple, titled from the dumpers at `0x004f9160`. */
    enum ShapeComponent {
        kShapeTension = 0,    /*!< Kochanek-Bartels tension, dumped as "t:". */
        kShapeContinuity = 1, /*!< Kochanek-Bartels continuity, dumped as "c:". */
        kShapeBias = 2        /*!< Kochanek-Bartels bias, dumped as "b:". */
    };

    /**
     * One keyframe of the translation or the scale channel.
     *
     * The keyframe is 0x50 bytes, and the enclosing `std::list` node is 0x60 with the value at
     * `+0x10`, both of which follow from the 16-byte alignment the padded vectors impose. mFrame
     * lands at `+0x40` rather than at `+0x3c` for the same reason, which is what makes the shape
     * triple a padded vector rather than three loose floats.
     *
     * The assignment of the shape triple is settled by the arithmetic of the tangent builder at
     * `0x00552748`, which forms `1 - tension`, `(1 +- continuity) / 2`, and `1 +- bias`. That is
     * the Kochanek-Bartels weighting, and it agrees with the "t:", "c:", and "b:" titles the
     * dumper writes in that order.
     */
    struct TransKey {
        float mValue[kXfmRowFloatCount];      /*!< x, y, and z, then one padding float. +0x00 */
        float mTangentIn[kXfmRowFloatCount];  /*!< Tangent entering the key. +0x10 */
        float mTangentOut[kXfmRowFloatCount]; /*!< Tangent leaving the key. +0x20 */
        float mShape[kXfmRowFloatCount];      /*!< See ShapeComponent, then padding. +0x30 */
        float mFrame;                         /*!< Frame this key lands on. +0x40 */
    };

    /**
     * One keyframe of the rotation channel.
     *
     * The same 0x50 bytes as TransKey, and the same node placement. The value is a quaternion
     * rather than a vector, so all four floats are read, written, and dumped. The dumper at
     * `0x004f9160` titles the value "q:" and its components "x:", "y:", "z:", and "w:".
     */
    struct RotKey {
        /**
         * Build the two Kochanek-Bartels tangents of this keyframe from its neighbours.
         *
         * Every blend is a QuatSlerp(). With both neighbours present, `toPrev` is the slerp of
         * this key towards pPrev at `-(bias + 1) / 3` and `toNext` the slerp towards pNext at
         * `(1 - bias) / 3`. mTangentOut is then the slerp of this key towards the slerp of that
         * pair at `(1 - continuity) / 2`, weighted `1 - tension`, and mTangentIn the same with
         * `(1 + continuity) / 2` and `tension - 1`. The two end weights differ only in sign, which
         * is what opposes the two tangents. A keyframe with no previous neighbour writes only
         * mTangentOut, as the slerp towards pNext at
         * `(1 - tension) * (continuity * bias + 1) / 3`, and one with no next neighbour writes
         * only mTangentIn, as the slerp towards pPrev at
         * `(1 - tension) * (1 - continuity * bias) / 3`. A keyframe with neither neighbour is
         * unchanged.
         *
         * @param pPrev The preceding keyframe, or null at the start of the channel.
         * @param pNext The following keyframe, or null at the end of the channel.
         * @ghidraAddress 0x00552588
         */
        void ComputeSplineTangents(const RotKey *pPrev, const RotKey *pNext);

        Quat mQuat;                      /*!< The rotation. +0x00 */
        Quat mTangentIn;                 /*!< Tangent entering the key. +0x10 */
        Quat mTangentOut;                /*!< Tangent leaving the key. +0x20 */
        float mShape[kXfmRowFloatCount]; /*!< See ShapeComponent, then padding. +0x30 */
        float mFrame;                    /*!< Frame this key lands on. +0x40 */
    };

    /**
     * Report the last frame the frames owner's three channels animate to.
     *
     * Animatable vtable slot 1. Every channel is read off mFramesOwner rather than off this
     * object, so a borrowed set of frames reports the lender's last frame. An empty channel
     * contributes zero.
     *
     * @return The largest last-key frame across the three channels, never below zero.
     * @ghidraAddress 0x004f4020
     */
    virtual float EndFrame();

    /**
     * Report the first frame the frames owner's three channels animate from.
     *
     * Animatable vtable slot 4, the one virtual this class adds. Every channel is read off
     * mFramesOwner, and an empty channel contributes zero. Rnd::Generator's path setter at
     * `0x0045e920` calls it.
     *
     * @return The smallest first-key frame across the three channels.
     * @ghidraAddress 0x004f4188
     */
    virtual float StartFrame();

    /**
     * Write a description of this object to sink.
     *
     * @param sink The diagnostic sink to write to.
     * @ghidraAddress 0x004f2ab0
     */
    virtual void DumpText(FailSink &sink);

    /**
     * Write the revision, both bases, both target names, the three channels, and the flags.
     *
     * The two targets are written as the names of the objects they address, so a reader has to
     * resolve them through Rnd::g_manager.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x004f2d50
     */
    virtual void Save(Stream &stream);

    /**
     * Replace this object's state from stream.
     *
     * A revision above the one this build writes produces the report "Can't load new TransAnim".
     * Not reconstructed yet.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x004f2f68
     */
    virtual void Load(Stream &stream);

    /**
     * Retarget mTrans and mFramesOwner when either addresses pFrom.
     *
     * A null pTo against mFramesOwner is the case worth noting. Rather than dropping the pointer,
     * this object copies the departing owner's three keyframe lists into its own and then makes
     * itself the owner, so the animation survives the loss of the object it borrowed its timing
     * from.
     *
     * @param pFrom The object being replaced.
     * @param pTo The replacement, or null.
     * @ghidraAddress 0x004f28c8
     */
    virtual void Replace(Object *pFrom, Object *pTo);

    /**
     * Copy the targets, the interpolation modes, the flags, and the channels from pSource.
     *
     * @param pSource The object to copy from.
     * @param nFlags The set of fields to copy; see kCopyShareFrames.
     * @ghidraAddress 0x004f3e90
     */
    virtual void Copy(const Object *pSource, unsigned nFlags);

    /**
     * Evaluate the three channels at a frame into a transform.
     *
     * Not reconstructed yet. The signature is established by the one call site that is fully
     * recovered, SetFrameSelf() at `0x004fd2c8`, which passes a sixteen-float scratch transform in
     * a1 and zero in a2 while the frame arrives in f12.
     *
     * @param flFrame The frame to evaluate at.
     * @param pXfm Four rows of four floats, read for the starting value and overwritten.
     * @param nApplyOffset Non-zero to extrapolate rather than clamp outside the key range.
     * @ghidraAddress 0x004f42f0
     */
    void EvalFrame(float flFrame, float *pXfm, int nApplyOffset);

    /**
     * Make a transformable the target this animation drives.
     *
     * Drops this object's reference on the previous target, records the new one, and takes a
     * reference on it. A null target clears mTrans.
     *
     * @param pTrans The new target, or null.
     * @ghidraAddress 0x004fd000
     */
    void SetTrans(Transformable *pTrans);

    /**
     * Report the object whose keyframes drive this one.
     *
     * The out-of-line copy has no callers. TnlBumpFX::Start() inlines it.
     *
     * @return mFramesOwner, which is this object when it owns its frames.
     * @ghidraAddress 0x004fbf58
     */
    TransAnim *GetFramesOwner() const {
        return mFramesOwner;
    }

protected:
    /**
     * Write the transform this frame evaluates to into mTrans.
     *
     * Animatable vtable slot 3. Returns at once with no target. Reads mTrans->mLocalXfm out,
     * evaluates over it, writes it back, and marks the target dirty, so the animation drives the
     * local transform rather than the composed one.
     *
     * @param flFrame The filtered frame to animate to.
     * @ghidraAddress 0x004fd2c8
     */
    virtual void SetFrameSelf(float flFrame);

private:
    // Declared in recovered offset order. The transformable this animation drives.
    Transformable *mTrans; // +0x2c
    // Interpolation mode per channel, one of the Interp values.
    int mRotInterp;   // +0x30
    int mTransInterp; // +0x34
    int mScaleInterp; // +0x38

public:
    /**
     * The rotation keyframes, kept in frame order.
     *
     * Public because TnlBumpFX::Start() at `0x0043dea8` rewrites the first key of its path's frame
     * owner, sorts the list, and rebuilds the tangents, and the image has no accessor. +0x3c
     */
    std::list<RotKey> mRotKeys;

private:
    std::list<TransKey> mTransKeys; // +0x40
    std::list<TransKey> mScaleKeys; // +0x44
    // The object whose keyframes drive this one, which is this object when it owns its frames.
    TransAnim *mFramesOwner; // +0x48
    int mRepeatTrans;        // +0x4c
    int mFollowPath;         // +0x50
};

} // namespace Rnd
