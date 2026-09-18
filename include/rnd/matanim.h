#pragma once

#include "os/failsink.h"
#include "rnd/animatable.h"
#include "rnd/mat.h"
#include "rnd/stream.h"

namespace Rnd {

/**
 * Animation of the colours and the stage transforms of one material.
 *
 * `Q23Rnd7MatAnim` in the RTTI descriptor at `0x008ef4b0`, with `Rnd::Animatable` as its one
 * public base at offset 0. The Animatable subobject is 0xc bytes and the whole object is 0x40
 * bytes.
 *
 * Five key channels animate the four material colours and the alpha, and a sixth channel animates
 * the stages. The text dump titles the stage block " stages:".
 *
 * Recovery is partial. No member type below the channel sentinels is reconstructed, and the
 * members between the base subobject and `+0x18` are undetermined. The routines recovered so far
 * are the frame apply at `0x004d4820`, the text dump at `0x004d33b8`, the save at `0x004d35d0`,
 * the load at `0x004d38e8`, and the end frame query at `0x004d42f0`.
 */
class MatAnim : public Animatable {
public:
    /** @ghidraAddress 0x004d33b8 */
    virtual void DumpText(FailSink &sink);

    /** @ghidraAddress 0x004d35d0 */
    virtual void Save(Stream &stream);

    /** @ghidraAddress 0x004d38e8 */
    virtual void Load(Stream &stream);

    /**
     * Apply the animation at a frame.
     *
     * @param flFrame The frame to apply.
     * @ghidraAddress 0x004d4820
     */
    void Animate(float flFrame);

    /**
     * Return the last frame any channel has a key on.
     *
     * @return The end frame.
     * @ghidraAddress 0x004d42f0
     */
    float GetEndFrame();

private:
    // No class derives from Rnd::MatAnim and no access from outside it is recovered, so every
    // member is private. The order below is the recovered offset order.
    int mUnknown0c;         // +0x0c
    int mUnknown10;         // +0x10
    int mUnknown14;         // +0x14
    Mat *mMat;              // +0x18 The material the animation drives.
    float mBeatOffset;      // +0x1c
    int mStageChannelBegin; // +0x20
    int mStageChannelEnd;   // +0x24
    Mat *mSecondMat;        // +0x28
    int mEmissiveKeys;      // +0x2c
    int mAmbientKeys;       // +0x30
    int mDiffuseKeys;       // +0x34
    int mSpecularKeys;      // +0x38
    int mAlphaKeys;         // +0x3c
};

} // namespace Rnd
