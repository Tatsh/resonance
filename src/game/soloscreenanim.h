#pragma once

#include <vector>

#include "game/screenanim.h"
#include "game/tnlarena.h"

namespace Rnd {
class Mat;
} // namespace Rnd

/**
 * Arena screens for a solo game, driven by the player's juice.
 *
 * `14SoloScreenAnim` in the RTTI descriptor at `0x008ef510`, deriving publicly from ScreenAnim at
 * offset 0. Its type function is at `0x0040c6c0` and its table at `0x008172a8`. The object is 0x1c
 * bytes. It inherits UpdateLeaders().
 *
 * Level 0 shows `noise.mat` on every screen, level 1 the screens' own materials, and level 2
 * alternates the player's material with the screens' own materials every mPeriod ticks.
 */
class SoloScreenAnim : public ScreenAnim {
public:
    /**
     * Record the screens and the player's material, and resolve `noise.mat`.
     *
     * Starts at level 1 with a period of 960 ticks.
     *
     * @param pScreens TnlArena::mScreenMeshes.
     * @param pPlayerMat The material of the one player.
     * @ghidraAddress 0x004066d0
     */
    SoloScreenAnim(const std::vector<TnlArena::ScreenMesh> *pScreens, Rnd::Mat *pPlayerMat);

    /** @ghidraAddress 0x0040c690 */
    virtual ~SoloScreenAnim();

    /**
     * Alternate the materials at level 2.
     *
     * At the start of each period the screens take the player's material in an even period and
     * their own materials in an odd one. Other levels do nothing.
     *
     * @param flFrame The song position, in MIDI ticks.
     * @ghidraAddress 0x0040c7f8
     */
    virtual void SetFrame(float flFrame);

    /**
     * Clamp the level to 0 through 2 and apply a changed one.
     *
     * A change puts `noise.mat`, the screens' own materials, or the player's material on every
     * screen, and then runs script template 1019 with the new level.
     *
     * @param nLevel The level.
     * @ghidraAddress 0x0040c738
     */
    virtual void SetLevel(int nLevel);

private:
    // Ticks each material of level 2 stays on the screens.
    float mPeriod;
    // The period SetFrame() last applied. Starts at -10000.
    int mPhase;
    int mLevel;
    Rnd::Mat *mPlayerMat;
    // `noise.mat`, or null.
    Rnd::Mat *mNoiseMat;
    const std::vector<TnlArena::ScreenMesh> *mScreens;
};
