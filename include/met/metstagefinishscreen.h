#pragma once

#include "met/metscreen.h"

/**
 * Congratulation screen shown when a stage is finished.
 *
 * `20MetStageFinishScreen` in the RTTI descriptor at `0x008ef8e0`, with MetScreen as its one public
 * non-virtual base at offset 0.
 *
 * The 39-entry primary vtable is at `0x0080f9e0`, the same length as the MetScreen table, so the
 * class declares no virtual of its own.
 *
 * The constructor at `0x003bdbb8` takes only the renderer and the load priority, and supplies
 * `egc` for the screen name, `metagame/_Solo` for the directory, and `end_game_congrats` for the
 * container. It writes two vectors at `+0x8c` and `+0x98`, then `+0xa4`, `+0xa8`, `+0xac`,
 * `+0xb0`, and `+0xb4`.
 *
 * The object is at least 0xb8 bytes. Nothing derives from the class, so no base offset in any
 * descriptor pins the total, and the figure is the lower bound the constructor's highest store
 * gives.
 *
 * The destructor is at `0x003bded8`.
 *
 * Apart from the type function and the destructor, the slots that differ from the MetScreen table
 * are 5 `0x003be2a8`, 19 `0x003bf788`, 21 `0x003c4228`, 22 `0x003c4230`, 23 `0x003c4238`, 24
 * `0x003c4240`, 25 `0x003c4248`, 26 `0x003bf5e8`, 30 `0x003c4390`, 33 `0x003c42d8`, 36
 * `0x003c0530`, 38 `0x003be068`.
 */
class MetStageFinishScreen : public MetScreen {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x003bdbb8
     */
    MetStageFinishScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x003bded8
     */
    virtual ~MetStageFinishScreen();

    /**
     * @ghidraAddress 0x003c4228
     */
    virtual void PlayLeaveSound();

    /**
     * @ghidraAddress 0x003c4230
     */
    virtual void PlayHighSound();

    /**
     * @ghidraAddress 0x003c4238
     */
    virtual void PlayCycleLeftSound();

    /**
     * @ghidraAddress 0x003c4240
     */
    virtual void PlayCycleRightSound();

    /**
     * @ghidraAddress 0x003c4248
     */
    virtual void PlayErrorSound();
};
