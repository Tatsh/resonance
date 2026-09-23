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
 * `17MetMultiEndScreen` in the RTTI descriptor at `0x008ef170`, with MetScreen as its one public
 * non-virtual base at offset 0.
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
     * @ghidraAddress 0x002f58d0
     */
    MetMultiEndScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * Delete the button list and release the five vectors.
     *
     * @ghidraAddress 0x002f5d20
     */
    virtual ~MetMultiEndScreen();

    /**
     * Build the screen on the heap.
     *
     * @param pRenderer The front-end renderer the screen registers on.
     * @param nPriority The load priority.
     * @return The new screen.
     * @ghidraAddress 0x002f9db0
     */
    static MetMultiEndScreen *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Rebuild the two buttons, set the `multi_game_over` title and the `no_back_title` layout, push
     * the help and multiplayer stats screens, select the first button, and enter the screen.
     *
     * Slot 5.
     *
     * @ghidraAddress 0x002f6158
     */
    virtual void EnterAndShow();

    /**
     * Move along the button ring, or act on the selection.
     *
     * Slot 19. Unlike MetSoloLoseScreen's, the select path does not record anything in
     * mUnknown18.
     *
     * @param pCommand The command.
     * @ghidraAddress 0x002f5f90
     */
    virtual void HandleCommand(const MetScreenCommand *pCommand);

    /**
     * Play nothing. Slot 21.
     *
     * @param nSelector Not read.
     * @ghidraAddress 0x002f9da8
     */
    virtual void PlayLeaveSound(int nSelector);

    /**
     * Play nothing. Slot 23.
     *
     * @param nSelector Not read.
     * @ghidraAddress 0x002f9d98
     */
    virtual void PlayCycleLeftSound(int nSelector);

    /**
     * Play nothing. Slot 24.
     *
     * @param nSelector Not read.
     * @ghidraAddress 0x002f9da0
     */
    virtual void PlayCycleRightSound(int nSelector);

    /**
     * Exit the multiplayer stats, title, and help screens and start this screen's exit.
     *
     * Slot 30.
     *
     * @param pButton Not read.
     * @ghidraAddress 0x002f6640
     */
    virtual void OnUnknownSlot30(Rnd::Button *pButton);

    /**
     * Leave for the load screen or the stage select once the exit has finished.
     *
     * Slot 36. With the first button selected, MetFrontEndState's return screen becomes this
     * screen and `MetLoadGameScreen` is pushed and activated. Otherwise the renderer resolves its
     * arena view, runs its two hooks, and `MetSoloStagesScreen` is pushed and activated. The
     * selection is then cleared.
     *
     * @ghidraAddress 0x002f67d8
     */
    virtual void OnUnknownSlot36();

private:
    MetButtonList *mUnknown8c; // +0x8c
    // Never written by the constructor and not recovered.
    int mUnknown90; // +0x90
    // The constructor empties the five vectors and the destructor releases them. No recovered
    // routine reads them, and int stands in for the unrecovered four-byte element.
    std::vector<int> mUnknown94; // +0x94
    std::vector<int> mUnknowna0; // +0xa0
    std::vector<int> mUnknownac; // +0xac
    std::vector<int> mUnknownb8; // +0xb8
    std::vector<int> mUnknownc4; // +0xc4
};
