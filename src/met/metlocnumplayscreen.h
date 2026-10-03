#pragma once

#include "met/metscreen.h"

class MetButtonList;

/**
 * Screen that picks the number of local players.
 *
 * Its RTTI descriptor is at `0x008f08e0`. It has MetScreen as its one public non-virtual base at
 * offset 0.
 *
 * The 39-entry primary vtable is at `0x007f9228`, the same length as the MetScreen table, so the
 * class declares no virtual of its own.
 *
 * The constructor at `0x002ad7e8` takes only the renderer and the load priority, and supplies
 * `mnp` for the screen name, `metagame/_Local` for the directory, and `num_players` for the
 * container. It writes only `+0x8c`, into which it allocates a MetButtonList.
 *
 * It pushes four object names into the container object-name vector that MetScreen owns, `loc_2p`,
 * `loc_3p`, `loc_4p`, and `multi_tips`. Its own registry key is the literal
 * `MetLocNumPlayersScreen`, which differs from the class name by three letters, and the five
 * MetMultiTips screens use that literal at both ends of their ring.
 *
 * The object is 0x90 bytes, which the allocation in New() fixes.
 *
 * The destructor is at `0x002b1008`.
 *
 * Apart from the type function and the destructor, the slots that differ from the MetScreen table
 * are 5 `0x002ae218`, 19 `0x002adef0`, 23 `0x002b0f70`, 24 `0x002b0f78`, 30 `0x002ae350`, 36
 * `0x002ae4f0`, 38 `0x002adbf0`.
 */
class MetLocNumPlayScreen : public MetScreen {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress NTSC-U/C: 0x002ad7e8
     * @ghidraAddress PAL: 0x002cc120
     */
    MetLocNumPlayScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * Build the screen on the heap.
     *
     * The object is allocated with the tag `MsgSink`. MetScreen::CreateFrontEndScreens() is the
     * one caller.
     *
     * @param pRenderer The front-end renderer the screen registers on.
     * @param nPriority The load priority.
     * @return The new screen.
     * @ghidraAddress NTSC-U/C: 0x002b0f80
     * @ghidraAddress PAL: 0x002cfd48
     */
    static MetLocNumPlayScreen *New(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress NTSC-U/C: 0x002b1008
     * @ghidraAddress PAL: 0x002cfdd0
     */
    virtual ~MetLocNumPlayScreen();

    /**
     * Show the choice last made and start the enter animation.
     *
     * Slot 5. The title is `m_num_p`, the renderer accepts four controllers, and the button for
     * MetFrontEndState::mPlayerCount players is selected, the first when the recorded count is
     * outside two through four. The prompt layout is `standard_title` and the help text follows
     * the selection.
     *
     * @ghidraAddress NTSC-U/C: 0x002ae218
     * @ghidraAddress PAL: 0x002ccd70
     */
    virtual void EnterAndShow();

    /**
     * Act on one navigation command.
     *
     * Slot 19. Previous and next move the selection and its help text. Select plays the button
     * alternation. Back departs to the main menu with the left gizmo and title screens.
     *
     * @param pCommand The command.
     * @ghidraAddress NTSC-U/C: 0x002adef0
     * @ghidraAddress PAL: 0x002cc9b0
     */
    virtual void HandleCommand(const MetScreenCommand *pCommand);

    /**
     * Silence the cycle-left sound.
     *
     * @param nSelector Not read. The body is empty.
     * @ghidraAddress NTSC-U/C: 0x002b0f70
     * @ghidraAddress PAL: 0x002cfd38
     */
    virtual void PlayCycleLeftSound(int nSelector);

    /**
     * Silence the cycle-right sound.
     *
     * @param nSelector Not read. The body is empty.
     * @ghidraAddress NTSC-U/C: 0x002b0f78
     * @ghidraAddress PAL: 0x002cfd40
     */
    virtual void PlayCycleRightSound(int nSelector);

    /**
     * Depart once the selected button has finished alternating.
     *
     * Slot 30. Records the exit as a choice and departs with the left gizmo, title, and help
     * screens.
     *
     * @param pButton The button that alternated, ignored.
     * @ghidraAddress NTSC-U/C: 0x002ae350
     * @ghidraAddress PAL: 0x002ccee0
     */
    virtual void OnRepeatingSoundFinished(Rnd::Button *pButton);

    /**
     * Go on to the screen the exit chose.
     *
     * Slot 36. Back returns to the main menu. A player count records itself in
     * MetFrontEndState::mPlayerCount and the renderer's controller limit and goes on to
     * MetLocPickCharScreen. The tips button goes on to MetMultiTips1Screen.
     *
     * @ghidraAddress NTSC-U/C: 0x002ae4f0
     * @ghidraAddress PAL: 0x002cd0e0
     */
    virtual void OnExitFinished();

    /**
     * Build the four buttons.
     *
     * Slot 38. Runs MetScreen::ResolveContainerViews() first, then appends `2player.but`,
     * `3player.but`, `4player.but`, and `mnp_info.but` with the labels `loc_2p`, `loc_3p`,
     * `loc_4p`, and `multi_tips`.
     *
     * @ghidraAddress NTSC-U/C: 0x002adbf0
     * @ghidraAddress PAL: 0x002cc5f0
     */
    virtual void ResolveContainerViews();

private:
    // The four buttons. The constructor allocates it and the destructor deletes it. +0x8c
    MetButtonList *mButtonList;
};
