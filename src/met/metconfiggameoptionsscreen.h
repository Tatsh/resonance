#pragma once

#include <vector>

#include "met/gameoptions.h"
#include "met/metbuttonlist.h"
#include "met/metscreenmultisoundbank.h"

namespace Rnd {
class Button;
} // namespace Rnd

/**
 * Screen that edits the in-game options.
 *
 * `26MetConfigGameOptionsScreen` in the RTTI descriptor at `0x00901ee0`, with
 * MetScreenMultiSoundBank as its one public non-virtual base at offset 0. New() allocates 0xb4
 * bytes, and the 39-entry vtable is at `0x007eaea8`, the same length as the MetScreen table, so
 * the class declares no virtual of its own.
 *
 * Two rows edit a working copy of GameOptions. The audio row switches GameOptions::mStereo
 * between `STEREO` and `MONO`, and the force-feedback row switches GameOptions::mForceFeedback
 * between `ON` and `OFF`.
 *
 * Six slots differ from the MetScreenMultiSoundBank table.
 *
 *  - 1 `0x0020ce20` the destructor.
 *  - 5 `0x0020d310` EnterAndShow().
 *  - 19 `0x0020cf70` HandleCommand().
 *  - 36 `0x0020d5a8` OnExitFinished().
 *  - 38 `0x0020c800` ResolveContainerViews().
 *
 * The five sounds the base swapped for the multiplayer bank stay as MetScreenMultiSoundBank
 * defined them.
 */
class MetConfigGameOptionsScreen : public MetScreenMultiSoundBank {
public:
    /**
     * Construct the in-game options screen.
     *
     * The screen name is `nop`, the directory `metagame/shared`, and the container
     * `net_options_pangame`. The two row keys are appended to MetScreen::mHelpKeys.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x0020c3e0
     */
    MetConfigGameOptionsScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x0020ce20
     */
    virtual ~MetConfigGameOptionsScreen();

    /**
     * Build the screen on the heap.
     *
     * The routine at `0x00385180` that creates every front-end screen is the caller.
     *
     * @param pRenderer The front-end renderer the screen registers on.
     * @param nPriority The load priority.
     * @return The new screen.
     * @ghidraAddress 0x002114f0
     */
    static MetConfigGameOptionsScreen *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Select the first row, copy the stored options, and enter.
     *
     * @ghidraAddress 0x0020d310
     */
    virtual void EnterAndShow();

    /**
     * Act on a command.
     *
     * Previous and next step the rows and post the new row's prompt. Left and right flash the
     * row's arrow and switch its setting. Select applies the options and exits. Back exits
     * without applying them.
     *
     * @param pCommand The command.
     * @ghidraAddress 0x0020cf70
     */
    virtual void HandleCommand(const MetScreenCommand *pCommand);

    /**
     * Push the screens that follow this one.
     *
     * Entered from the pause menu, the pause screen recorded in MetFrontEndState::mReturnScreen is
     * pushed and activated again, after the help screen exits for the remix pause screen, and
     * the record is cleared. Otherwise a cancel returns to `MetConfigOptionsButtonsScreen`, and
     * any other exit stores the options into GlobalSettings and saves them through
     * MetGlobalSettingsSaverScreen.
     *
     * @ghidraAddress 0x0020d5a8
     */
    virtual void OnExitFinished();

    /**
     * Resolve the two labels, the two rows, and the four arrow buttons.
     *
     * The labels are not tested for null.
     *
     * @ghidraAddress 0x0020c800
     */
    virtual void ResolveContainerViews();

private:
    // 0x0020d448
    // Shows the working copy's two settings on the two rows.
    void UpdateOptionLabels();

    // 0x00211578
    // Switches one row's setting and shows it. Any other row only refreshes the labels.
    void ToggleOption(int nRow);

    // 0x002115c8
    // Stores the working copy into GlobalSettings, applies the audio mode to the synthesiser, and
    // applies the force-feedback setting to the world when one exists.
    void ApplyOptions();

    MetButtonList *mRows;                    // +0x8c
    std::vector<Rnd::Button *> mLeftArrows;  // +0x90
    std::vector<Rnd::Button *> mRightArrows; // +0x9c
    GameOptions mOptions;                    // +0xa8, the working copy
};
