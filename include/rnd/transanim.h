#pragma once

#include <list>

#include "math/quaternion.h"
#include "rnd/animatable.h"
#include "rnd/drawable.h"
// Included for kXfmRowFloatCount, which this header uses by value, rather than for Transformable.
#include "rnd/transformable.h"

class FailSink;
struct Vector3;
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
        /**
         * Build the two Kochanek-Bartels tangents of this keyframe from its neighbours.
         *
         * With both neighbours, `toPrev` is `(value - pPrev->value) * (bias + 1)` and `toNext` is
         * `(pNext->value - value) * (1 - bias)`. mTangentOut is then
         * `toPrev + (toNext - toPrev) * (0.5 - continuity / 2)` weighted `1 - tension`, and
         * mTangentIn the same with `0.5 + continuity / 2`. A keyframe with no previous neighbour
         * writes only mTangentOut, as
         * `((pNext->value - value) * 1.5 - pNext->mTangentIn * 0.5 * (bias + 1)) * (1 - tension)`,
         * and one with no next neighbour writes only mTangentIn, as
         * `((value - pPrev->value) * 1.5 - pPrev->mTangentOut * 0.5 * (bias + 1)) * (1 - tension)`.
         * Every tangent written ends in a padding float of 1.0. A keyframe with neither neighbour
         * is unchanged.
         *
         * @param pPrev The preceding keyframe, or null at the start of the channel.
         * @param pNext The following keyframe, or null at the end of the channel.
         * @ghidraAddress 0x00552748
         */
        void ComputeSplineTangents(const TransKey *pPrev, const TransKey *pNext);

        /**
         * Evaluate the cubic Hermite segment from this keyframe to pNext.
         *
         * The segment leaves mValue along mTangentOut and arrives at `pNext->mValue` along
         * `pNext->mTangentIn`. A parameter of exactly zero or exactly one copies the matching value
         * quadword, padding float included. Any other parameter writes a padding float of 1.0.
         *
         * @param pNext The keyframe that ends the segment.
         * @param pOut Receives the position, four floats.
         * @param flT The segment parameter, zero at this keyframe and one at pNext.
         * @ghidraAddress 0x00552af8
         */
        void EvaluateSpline(const TransKey *pNext, float *pOut, float flT) const;

        /**
         * Evaluate the derivative of the segment EvaluateSpline() traces.
         *
         * Neither end of the segment is special cased. The result returns through the hidden
         * pointer in a0, ahead of this object in a1, and its padding float is 1.0.
         *
         * @param pNext The keyframe that ends the segment.
         * @param flT The segment parameter, zero at this keyframe and one at pNext.
         * @return The derivative with respect to the segment parameter.
         * @ghidraAddress 0x00552cb8
         */
        Vector3 EvaluateSplineDerivative(const TransKey *pNext, float flT) const;

        /**
         * Approximate the arc length of the segment EvaluateSpline() traces.
         *
         * A left Riemann sum of the derivative's length in parameter steps of 0.005, stopping once
         * the accumulated parameter arrives at one.
         *
         * @param pNext The keyframe that ends the segment.
         * @return The approximate length.
         * @ghidraAddress 0x00554d90
         */
        float SplineLength(const TransKey *pNext) const;

        /**
         * Order keyframes by frame.
         *
         * The list sort the loader calls inlines it in its merge step.
         *
         * @param other The keyframe to compare against.
         * @return Whether this keyframe lands before other.
         */
        bool operator<(const TransKey &other) const {
            return mFrame < other.mFrame;
        }

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

        /**
         * Evaluate the spherical Bezier segment from this keyframe to pNext.
         *
         * The control points are mQuat, mTangentOut, `pNext->mTangentIn`, and `pNext->mQuat`. Three
         * rounds of QuatSlerp() at the one parameter reduce the four points to three, then two,
         * then the result. A parameter
         * of exactly zero or exactly one copies the matching end quaternion.
         *
         * @param pNext The keyframe that ends the segment.
         * @param out Receives the rotation.
         * @param flT The segment parameter, zero at this keyframe and one at pNext.
         * @ghidraAddress 0x00554c68
         */
        void EvaluateSpline(const RotKey *pNext, Quat &out, float flT) const;

        /**
         * Order keyframes by frame.
         *
         * The list sort at `0x00108a50`, which TnlBumpFX::Start() and the loader call, inlines it
         * in its merge step at `0x001088e0`.
         *
         * @param other The keyframe to compare against.
         * @return Whether this keyframe lands before other.
         */
        bool operator<(const RotKey &other) const {
            return mFrame < other.mFrame;
        }

        Quat mQuat;                      /*!< The rotation. +0x00 */
        Quat mTangentIn;                 /*!< Tangent entering the key. +0x10 */
        Quat mTangentOut;                /*!< Tangent leaving the key. +0x20 */
        float mShape[kXfmRowFloatCount]; /*!< See ShapeComponent, then padding. +0x30 */
        float mFrame;                    /*!< Frame this key lands on. +0x40 */
    };

    /**
     * Construct an animation with no target and three empty channels that it owns.
     *
     * The translation channel starts as kInterpTCB and the other two as kInterpLinear.
     *
     * @param name The object name, passed to the Rnd::Object constructor.
     * @ghidraAddress 0x004fc000
     */
    explicit TransAnim(const HxStr &name);

    /**
     * Drop this object's references and every reference held on it.
     *
     * @ghidraAddress 0x004fbb78
     */
    virtual ~TransAnim();

    /**
     * Report the registered class name, "TransAnim".
     *
     * @return The class name.
     * @ghidraAddress 0x004fbf60
     */
    virtual const HxStr &ClassName() const;

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
     * Below revision 2 the rotation and translation keys arrive in an older form, a value and a
     * frame each, and a channel whose interpolation reads as 0 is rebuilt from that form one key at
     * a time, sorting the channel and rebuilding its tangents after every key. The scale channel
     * arrives from revision 1, and the follow-path flag from revision 2. Below that the flag is
     * derived as whether the frames owner has no rotation keys and at least two translation keys.
     * An animation that does not own its frames then empties its own three channels.
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
     * The keys come from mFramesOwner and the interpolation modes and flags from this object.
     * The translation channel writes the fourth row. Outside the key range a channel clamps to
     * its end value. With mRepeatTrans set, the frame is wrapped by the span of the translation
     * keys and each wrap adds the chord from the first key to the last. A negative frame on a
     * spline translation channel instead extrapolates along the first key's outgoing tangent.
     * The wrapped frame is relative to the first key but is compared against absolute key
     * frames. A channel whose first key is not at frame zero therefore samples a shifted
     * segment. A repeating spline translation channel also copies the first key's outgoing
     * tangent over the last key's incoming tangent on every call.
     *
     * The basis rows come from the rotation channel, or with mFollowPath set from the direction
     * of the translation curve against a +z reference. A follow path of fewer than two keys
     * writes the identity basis. The scale channel then scales each basis row by one component.
     *
     * @param flFrame The frame to evaluate at.
     * @param pXfm Four rows of four floats, updated in place. A channel with no keys writes
     * nothing to its rows unless nResetEmpty is set.
     * @param nResetEmpty Non-zero to reset the rows of an empty translation or rotation channel to
     * the identity.
     * @ghidraAddress 0x004f42f0
     */
    void EvalFrame(float flFrame, float *pXfm, int nResetEmpty);

    /**
     * Redistribute the frames owner's translation keys to an even speed.
     *
     * A channel of fewer than three keys is not changed. A linear channel retains its values, and
     * each frame is set in proportion to the chord distance covered, between the first and last
     * frames.
     *
     * A spline channel is rebuilt. Its arc length is walked in parameter steps of 0.005, and a key
     * is emitted each time another total length divided by one less than the key count has been
     * covered. The emitted value is the straight-line blend of the two bracketing keys at the
     * step's parameter rather than the curve position, and its shape comes from the earlier key.
     * The emitted frames advance from one frame step rather than from the first key's frame. Each
     * insertion sorts the new channel again and rebuilds its tangents. A walk one key short
     * appends a copy of the last key, and a walk that arrives at the full count replaces its last
     * key with the original last key. Any other count reports "Couldn't normalize" and does not
     * change the channel. When an emission overshoots its target by more than 0.005 of the
     * spacing, the routine runs again on the result.
     *
     * No call site outside the routine itself survives in the shipped program. The name is
     * inferred from the report text.
     *
     * @ghidraAddress 0x004f4c48
     */
    void Normalize();

    /**
     * Set whether the translation channel repeats.
     *
     * Turning repetition off sorts the frames owner's translation keys and rebuilds their
     * tangents. The rebuild restores the incoming tangent of the last key. EvalFrame() overwrites
     * that tangent while repetition is on. No call site survives in the shipped program, and the
     * name is inferred from the member it sets.
     *
     * @param nRepeat Non-zero to repeat.
     * @ghidraAddress 0x004fb7e0
     */
    void SetRepeatTrans(int nRepeat);

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
     * Make another animation the one whose keyframes drive this one.
     *
     * Moves this object's reference from the previous owner to the new one, and then empties this
     * object's own channels unless it is its own owner. The routine has no caller in the shipped
     * build. The name is inferred.
     *
     * @param pOwner The new frames owner, or null.
     * @ghidraAddress 0x004fd0a0
     */
    void SetFramesOwner(TransAnim *pOwner);

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
    // Empty the three channels unless this object owns its frames. The out-of-line copy has no
    // caller, and SetFramesOwner() and Load() inline the same body. 0x004fd058.
    void ClearKeys();

    // Take a reference on the target and on the frames owner. Load() and Copy() inline the same
    // body. 0x004fd168.
    void AddObjectRefs();

    // Drop the references AddObjectRefs() took. The destructor is the one out-of-line caller.
    // 0x004fd118.
    void RemoveObjectRefs();

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

/**
 * Allocate and construct a transform animation, the base creator of the "TransAnim" class.
 *
 * The allocation is untagged and 0x70 bytes, the 0x54 of this class and the 0x1c of the shared
 * Rnd::Object subobject, and the constructor is inlined into this body.
 *
 * @param name The object name.
 * @return The new animation.
 * @ghidraAddress 0x004fc740
 */
TransAnim *NewTransAnim(const HxStr &name);

/**
 * Creator the registered "TransAnim" class builds through.
 *
 * RegisterTransAnimClass() points it at NewTransAnim().
 *
 * @ghidraAddress 0x00706820
 */
extern TransAnim *(*g_pfnNewTransAnim)(const HxStr &name);

/**
 * Build a transform animation through the creator hook.
 *
 * No call site survives in the shipped program. The name is inferred from the Rnd::Button
 * counterpart.
 *
 * @param name The object name.
 * @return The new animation.
 * @ghidraAddress 0x004fba90
 */
TransAnim *NewTransAnimThroughHook(const HxStr &name);

/**
 * Build a transform animation for the registered "TransAnim" class by calling through
 * g_pfnNewTransAnim.
 *
 * Rnd::Manager::Init() registers it as well.
 *
 * @param name The object name.
 * @return The new animation, as its Rnd::Object subobject.
 * @ghidraAddress 0x004fbf70
 */
Object *CreateRegisteredTransAnim(const HxStr &name);

/**
 * Point g_pfnNewTransAnim at NewTransAnim() and register the "TransAnim" class with Rnd::Manager.
 *
 * No call site survives in the shipped program. The name is inferred.
 *
 * @ghidraAddress 0x004fba50
 */
void RegisterTransAnimClass();

/**
 * Registered class name of Rnd::TransAnim, the string "TransAnim".
 *
 * @ghidraAddress 0x00706828
 */
extern HxStr g_transAnimClassName;

} // namespace Rnd
