#pragma once

#include <vector>

#include "met/metscreen.h"

class MetButtonList;

namespace Rnd {
class Button;
} // namespace Rnd

/**
 * End-of-game button row for a multiplayer session.
 *
 * Its RTTI descriptor is at `0x008ef170`. It has MetScreen as its one public non-virtual base at
 * offset 0.
 *
 * The 39-entry primary vtable is at `0x007ff4c8`, the same length as the MetScreen table, so the
 * class declares no virtual of its own.
 *
 * The constructor at `0x002f58d0` takes only the renderer and the load priority, and supplies
 * `egb` for the screen name, `metagame/_Solo` for the directory, and `end_game_butts` for the
 * container. It writes `+0x8c` and the five vectors at `+0x94`, `+0xa0`, `+0xac`, `+0xb8`, and
 * `+0xc4`.
 *
 * It loads the same container as MetSoloLoseScreen, `metagame/_Solo/end_game_butts`, under the
 * same screen name, and its bodies follow that class's closely. The two buttons play again or pick
 * new stages, and the multiplayer stats screen stands in for the solo one.
 *
 * The object is 0xd0 bytes, the size New() allocates.
 *
 * Apart from the type function and the destructor, the slots that differ from the MetScreen table
 * are 5 `0x002f6158`, 19 `0x002f5f90`, 21 `0x002f9da8`, 23 `0x002f9d98`, 24 `0x002f9da0`, 30
 * `0x002f6640`, 36 `0x002f67d8`.
 */
class MetMultiEndScreen : public MetScreen {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress NTSC-U/C: 0x002f58d0
     * @ghidraAddress PAL: 0x003196a8
     */
    MetMultiEndScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * Delete the button list and release the five vectors.
     *
     * @ghidraAddress NTSC-U/C: 0x002f5d20
     * @ghidraAddress PAL: 0x00319b60
     */
    virtual ~MetMultiEndScreen();

    /**
     * Build the screen on the heap.
     *
     * @param pRenderer The front-end renderer the screen registers on.
     * @param nPriority The load priority.
     * @return The new screen.
     * @ghidraAddress NTSC-U/C: 0x002f9db0
     * @ghidraAddress PAL: 0x0031dee8
     */
    static MetMultiEndScreen *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Rebuild the two buttons, set the `multi_game_over` title and the `no_back_title` layout, push
     * the help and multiplayer stats screens, select the first button, and enter the screen.
     *
     * Slot 5.
     *
     * @ghidraAddress NTSC-U/C: 0x002f6158
     * @ghidraAddress PAL: 0x00319fd8
     */
    virtual void EnterAndShow();

    /**
     * Move along the button ring, or act on the selection.
     *
     * Slot 19. Unlike MetSoloLoseScreen's, the select path does not record anything in
     * mExitChoice.
     *
     * @param pCommand The command.
     * @ghidraAddress NTSC-U/C: 0x002f5f90
     * @ghidraAddress PAL: 0x00319dd0
     */
    virtual void HandleCommand(const MetScreenCommand *pCommand);

    /**
     * Play nothing. Slot 21.
     *
     * @param nSelector Not read.
     * @ghidraAddress NTSC-U/C: 0x002f9da8
     * @ghidraAddress PAL: 0x0031dee0
     */
    virtual void PlayLeaveSound(int nSelector);

    /**
     * Play nothing. Slot 23.
     *
     * @param nSelector Not read.
     * @ghidraAddress NTSC-U/C: 0x002f9d98
     * @ghidraAddress PAL: 0x0031ded0
     */
    virtual void PlayCycleLeftSound(int nSelector);

    /**
     * Play nothing. Slot 24.
     *
     * @param nSelector Not read.
     * @ghidraAddress NTSC-U/C: 0x002f9da0
     * @ghidraAddress PAL: 0x0031ded8
     */
    virtual void PlayCycleRightSound(int nSelector);

    /**
     * Exit the multiplayer stats, title, and help screens and start this screen's exit.
     *
     * Slot 30.
     *
     * @param pButton Not read.
     * @ghidraAddress NTSC-U/C: 0x002f6640
     * @ghidraAddress PAL: 0x0031a5c0
     */
    virtual void OnRepeatingSoundFinished(Rnd::Button *pButton);

    /**
     * Leave for the load screen or the stage select once the exit has finished.
     *
     * Slot 36. With the first button selected, MetFrontEndState's return screen becomes this
     * screen and `MetLoadGameScreen` is pushed and activated. Otherwise the renderer resolves its
     * arena view, runs its two hooks, and `MetSoloStagesScreen` is pushed and activated. The
     * selection is then cleared.
     *
     * @ghidraAddress NTSC-U/C: 0x002f67d8
     * @ghidraAddress PAL: 0x0031a7b8
     */
    virtual void OnExitFinished();

private:
    MetButtonList *mButtonList; // +0x8c
    int mReserved;              // +0x90, never written or read by the recovered routines.
    // The constructor empties the five vectors and the destructor releases them. No recovered
    // routine reads them, and int stands in for the unrecovered four-byte element. The names follow
    // the vectors at the same offsets in MetMultiStatsScreen, the layout this class matches.
    std::vector<int> mScoreTexts;       // +0x94
    std::vector<int> mNameTexts;        // +0xa0
    std::vector<int> mPictureMaterials; // +0xac
    std::vector<int> mPlayerMeshes;     // +0xb8
    std::vector<int> mPlayerOrder;      // +0xc4
};
