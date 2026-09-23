#pragma once

#include "os/hxstr.h"

namespace Rnd {
class Animatable;
class Mat;
class Mesh;
class Tex;
class View;
} // namespace Rnd

/**
 * Three-target catcher of one player's activator, with its multiplier texture and movie.
 *
 * The class is not polymorphic and emits no RTTI, and no allocation tag or file path identifies
 * it. The name is inferred from its view, "catcher_<c>", where `<c>` is the first letter of the
 * player's colour name.
 *
 * Hit() swaps one target to the "tar dn" material. Ten updates later every target returns to the
 * "tar up" material, and the view is driven to `mTarget * 100` plus twice the updates since the
 * hit, capped at 100.
 *
 * TnlActivator embeds one, 0x3c bytes, at `+0x5c`.
 */
class TnlCatcher {
public:
    /** Number of targets. */
    enum { kTargetCount = 3 };

    /**
     * Resolve the view, the targets, the materials, the textures, and the movie.
     *
     * The multiplier starts off, every target starts on the "tar up" material, and both counts
     * start at 100000000.
     *
     * @param colorName The player's colour name, whose first letter selects every object.
     * @ghidraAddress 0x00439958
     */
    explicit TnlCatcher(const HxStr &colorName);

    /**
     * Switch the material between the plain and the multiplier texture.
     *
     * @param nMultiplied Non-zero for "act_mult_<c>.tex", zero for "act_<c>.bmp".
     * @ghidraAddress 0x00455268
     */
    void SetMultiplied(int nMultiplied);

    /**
     * Put every target on the "tar up" material.
     *
     * @ghidraAddress 0x004553c0
     */
    void ResetTargets();

    /**
     * Put one target on the "tar dn" material and restart the hit count.
     *
     * Does nothing while the view is hidden. The program lists no caller.
     *
     * @param nTarget The target, 0 through 2.
     * @ghidraAddress 0x00455330
     */
    void Hit(int nTarget);

    /**
     * Set the alpha of the "act_<c>.mat" material.
     *
     * @param flAlpha The alpha.
     * @ghidraAddress 0x00455238
     */
    void SetAlpha(float flAlpha);

    /**
     * Parent the view to another view for both transform and drawing.
     *
     * The program lists no caller.
     *
     * @param pParent The parent view.
     * @ghidraAddress 0x004552d8
     */
    void AttachTo(Rnd::View *pParent);

    /**
     * Advance the multiplier movie, the hit count, and the view.
     *
     * The view frame is written only when flFrame is not negative. The program lists no caller.
     *
     * @param flFrame The scaled song position TnlActivator::Update() receives.
     * @ghidraAddress 0x00455418
     */
    void Update(float flFrame);

    /**
     * Catcher view, "catcher_<c>".
     *
     * Public because TnlActivator shows and hides it directly and the image has no accessor.
     */
    Rnd::View *mView;

private:
    int mTarget;
    Rnd::Mesh *mTargets[kTargetCount]; // "act_<c>tar<n>.mesh".
    Rnd::Mat *mUpMat;                  // "act_<c> tar up.mat".
    Rnd::Mat *mDownMat;                // "act_<c> tar dn.mat".
    Rnd::Mat *mMat;                    // "act_<c>.mat".
    Rnd::Tex *mTex;                    // "act_<c>.bmp".
    Rnd::Tex *mMultTex;                // "act_mult_<c>.tex".
    Rnd::Animatable *mMultMovie;       // "act_mult_<c>.mov".
    int mMultiplied;
    int mUnknown30; // +0x30, zeroed and never read by the recovered routines.
    int mUpdateCount;
    int mHitCount; // Updates since the last hit.
};
