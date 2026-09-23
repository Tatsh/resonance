#pragma once

#include "met/metpausebasescreen.h"

/**
 * Pause screen.
 *
 * `18MetPauseGameScreen` in the RTTI descriptor at `0x00902310`, with MetPauseBaseScreen as its one
 * public non-virtual base at offset 0. The 40-entry vtable at `0x00803280` is the same length as
 * the MetPauseBaseScreen table, so the class declares no virtual of its own. New() allocates 0xb0
 * bytes, the size of the base, so the class adds no member.
 *
 * The destructor at `0x0031fac8` is compiler-generated.
 */
class MetPauseGameScreen : public MetPauseBaseScreen {
public:
    /**
     * Construct the screen.
     *
     * The screen name is `pmgr`, the directory `metagame/Transition`, and the container
     * `pause_multi`. The panel a dismissed confirmation reactivates is `MetPauseGameScreen`.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x0031c368
     */
    MetPauseGameScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * Build the screen on the heap.
     *
     * @param pRenderer The front-end renderer the screen registers on.
     * @param nPriority The load priority.
     * @return The new screen.
     * @ghidraAddress 0x0031fa40
     */
    static MetPauseGameScreen *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Fill the paused heading and the option labels, then enter.
     *
     * Slot 5. The heading `pmgr_paused.txt` reads configuration code 0x258 under `pause_tutorial`
     * in front-end phase 5, `pause_remix` in jam mode, and `pause_game` otherwise. The labels come
     * from code 0x259 under `pause_multi_game` in phase 5 or in game mode, and `pause_multi_remix`
     * otherwise. The play mode is read from a copy of the game settings. The heading is not tested
     * for null.
     *
     * @ghidraAddress 0x0031c658
     */
    virtual void EnterAndShow();

    /**
     * Resolve the container views and the three option texts `pmgr_opt1.txt` to `pmgr_opt3.txt`.
     *
     * Slot 38. MetScreen::ResolveContainerViews() runs first. No text is tested for null.
     *
     * @ghidraAddress 0x0031c508
     */
    virtual void ResolveContainerViews();
};
