#pragma once

#include <vector>

#include "game/screenanim.h"
#include "game/tnlarena.h"

namespace Rnd {
class Mat;
} // namespace Rnd

/**
 * Arena screens for a game of several players, cycling through the leaders' materials.
 *
 * `15MultiScreenAnim` in the RTTI descriptor at `0x00901bb0`, deriving publicly from ScreenAnim at
 * offset 0. Its type function is at `0x0040c618` and its table at `0x008172d8`. The object is 0x20
 * bytes. It inherits SetLevel().
 *
 * The screens cycle in steps of mPeriod ticks, eight steps to a cycle. Each even step shows the
 * material of one leader in turn, and every other step shows the screens' own materials. A single
 * leader appears in the first two even steps only.
 */
class MultiScreenAnim : public ScreenAnim {
public:
    /**
     * Record the screens and the players' materials.
     *
     * Reserves room for four leader materials, starts with a period of 480 ticks, and runs script
     * template 1019 with 1.
     *
     * @param pScreens TnlArena::mScreenMeshes.
     * @param pPlayerMaterials TnlArena::mPlayerMaterials.
     * @ghidraAddress 0x00406200
     */
    MultiScreenAnim(const std::vector<TnlArena::ScreenMesh> *pScreens,
                    const std::vector<TnlArena::PlayerMaterial *> *pPlayerMaterials);

    /** @ghidraAddress 0x0040c568 */
    virtual ~MultiScreenAnim();

    /**
     * Show the material of the current step on every screen.
     *
     * Does nothing without a leader or within the step SetFrame() last applied.
     *
     * @param flFrame The song position, in MIDI ticks.
     * @ghidraAddress 0x00406570
     */
    virtual void SetFrame(float flFrame);

    /**
     * Rebuild mLeaderMats from the players' scores.
     *
     * A leader is every player whose Player::GetScore() is less than 4 below the highest score,
     * with the highest starting at 0.
     *
     * @ghidraAddress 0x00406428
     */
    virtual void UpdateLeaders();

private:
    const std::vector<TnlArena::ScreenMesh> *mScreens;
    const std::vector<TnlArena::PlayerMaterial *> *mPlayerMaterials;
    // The materials of the leaders, in world order.
    std::vector<Rnd::Mat *> mLeaderMats;
    // Ticks each step of the cycle lasts.
    float mPeriod;
    // The step SetFrame() last applied. Starts at -1000.
    int mPhase;
};
