#pragma once

#include <vector>

#include "memcard/memcarduser.h"
#include "met/metbuttonlist.h"
#include "met/metscreenmultisoundbank.h"
#include "os/hxstr.h"

class ControllerConfig;

namespace Rnd {
class Mesh;
class Text;
} // namespace Rnd

/**
 * Screen that assigns the controller buttons.
 *
 * Its RTTI descriptor is at `0x008efbf0`. It has two public non-virtual bases at fixed offsets,
 * MetScreenMultiSoundBank at `+0x00` and MemcardUser at `+140`. New() allocates 0xd0 bytes. Its
 * primary 39-entry vtable is at `0x007e9cd8`, the same length as the MetScreen table, and the class
 * declares no new virtual. The 21-entry MemcardUser table at `0x007e9c28` adjusts `this` by `-140`
 * in every entry.
 *
 * The screen lists nine configuration rows, one per action, and each row shows one button code.
 * The codes `a` through `h` are square, triangle, circle, cross, L1, L2, R1, and R2, and `o`
 * marks a row with no button. The two analogue-stick rows show `left analog stick` or
 * `right analog stick` instead. A button index is the code less `a`, with 8 and 9 for the two
 * sticks, which is the index ControllerConfig::SetButton() takes.
 *
 * Fourteen slots differ from the MetScreenMultiSoundBank table.
 *
 *  - 1 `0x002008e8` the destructor.
 *  - 5 `0x00201478` EnterAndShow().
 *  - 9 `0x002069a0` BeginExit().
 *  - 15 `0x00206bb0` OnMsgScreenDismissed().
 *  - 19 `0x00200ba8` HandleCommand().
 *  - 20 through 24 at `0x002068b0`, `0x00206970`, `0x00206940`, `0x002068e0`, and `0x00206910`,
 *    which replace the five MetScreenMultiSoundBank sounds.
 *  - 33 `0x002069d0` OnEnterFinished().
 *  - 36 `0x00201790` OnExitFinished().
 *  - 38 `0x001fff40` ResolveContainerViews().
 *
 * The translation unit's static initialiser at `0x002065a0` constructs the three button-name
 * strings the rows compare against.
 */
class MetConfigControllerScreen : public MetScreenMultiSoundBank, public MemcardUser {
public:
    /**
     * Construct the controller configuration screen.
     *
     * The screen name is `psx`, the directory `metagame/shared`, and the container `psx_config`.
     * The nine row keys are appended to MetScreen::mHelpKeys, and the names of the ten button
     * meshes and nine value texts are recorded for ResolveContainerViews().
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress NTSC-U/C: 0x001ff1e0
     * @ghidraAddress PAL: 0x002066c8
     */
    MetConfigControllerScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress NTSC-U/C: 0x002008e8
     * @ghidraAddress PAL: 0x00208cb0
     */
    virtual ~MetConfigControllerScreen();

    /**
     * Build the screen on the heap.
     *
     * The routine at `0x00385180` that creates every front-end screen is the caller.
     *
     * @param pRenderer The front-end renderer the screen registers on.
     * @param nPriority The load priority.
     * @return The new screen.
     * @ghidraAddress NTSC-U/C: 0x00206828
     * @ghidraAddress PAL: 0x0020f6f0
     */
    static MetConfigControllerScreen *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Select the first row, show the configured buttons, and enter.
     *
     * The title reads the localised player label, the one-based controller number, and the
     * localised options label. The help screen takes the first row's prompt and the
     * `cc_save_back` layout when MetFrontEndState::mUsingMemcard is set, otherwise
     * `standard_title`. The European release instead takes `standard_title` when
     * MetFrontEndState::mReturnScreen is either solo pause screen, and `cc_save_back` otherwise.
     *
     * @ghidraAddress NTSC-U/C: 0x00201478
     * @ghidraAddress PAL: 0x00209bd0
     */
    virtual void EnterAndShow();

    /**
     * Hide every button highlight and begin the exit.
     *
     * @ghidraAddress NTSC-U/C: 0x002069a0
     * @ghidraAddress PAL: 0x0020f868
     */
    virtual void BeginExit();

    /**
     * Act on a dismissed dialogue.
     *
     * `missingconfigvals` makes this screen the active panel again. `nomemcard` records 0 in
     * MetScreen::mExitChoice and begins the exit. The choice is not read.
     *
     * @param name The dialogue name.
     * @param nChoice The chosen button.
     * @ghidraAddress NTSC-U/C: 0x00206bb0
     * @ghidraAddress PAL: 0x0020f9f8
     */
    virtual void OnMsgScreenDismissed(const HxStr &name, int nChoice);

    /**
     * Act on a command from the controller this screen configures.
     *
     * A command from another pad is ignored. Previous and next clear a duplicate of the row being
     * departed, move along the rows, and post the new row's prompt. Left and right cycle the
     * selected row's button, or swap the two sticks on a stick row. Select stores the rows into
     * GlobalSettings and exits, or shows `missingconfigvals` when a row has no button. Back exits
     * without storing. Command 7 plays the slide sound and shows the default mapping.
     *
     * @param pCommand The command.
     * @ghidraAddress NTSC-U/C: 0x00200ba8
     * @ghidraAddress PAL: 0x00208f98
     */
    virtual void HandleCommand(const MetScreenCommand *pCommand);

    /**
     * Play the slide sound for this screen's controller only.
     *
     * @param nSelector The pad index of the command.
     * @ghidraAddress NTSC-U/C: 0x002068b0
     * @ghidraAddress PAL: 0x0020f778
     */
    virtual void PlaySlideSound(int nSelector);

    /**
     * Play the leave sound for this screen's controller only.
     *
     * @param nSelector The pad index of the command.
     * @ghidraAddress NTSC-U/C: 0x00206970
     * @ghidraAddress PAL: 0x0020f838
     */
    virtual void PlayLeaveSound(int nSelector);

    /**
     * Play the emphasised-selection sound for this screen's controller only.
     *
     * @param nSelector The pad index of the command.
     * @ghidraAddress NTSC-U/C: 0x00206940
     * @ghidraAddress PAL: 0x0020f808
     */
    virtual void PlayHighSound(int nSelector);

    /**
     * Play the cycle-left sound for this screen's controller only.
     *
     * @param nSelector The pad index of the command.
     * @ghidraAddress NTSC-U/C: 0x002068e0
     * @ghidraAddress PAL: 0x0020f7a8
     */
    virtual void PlayCycleLeftSound(int nSelector);

    /**
     * Play the cycle-right sound for this screen's controller only.
     *
     * @param nSelector The pad index of the command.
     * @ghidraAddress NTSC-U/C: 0x00206910
     * @ghidraAddress PAL: 0x0020f7d8
     */
    virtual void PlayCycleRightSound(int nSelector);

    /**
     * Clear a duplicate of the selected row and highlight its button.
     *
     * @ghidraAddress NTSC-U/C: 0x002069d0
     * @ghidraAddress PAL: 0x0020f898
     */
    virtual void OnEnterFinished();

    /**
     * Push the screens that follow this one.
     *
     * Entered from the pause menu, the pause screen recorded in MetFrontEndState::mReturnScreen is
     * pushed and activated again, and the record is cleared. MetFrontEndState::mSettingsDirty
     * becomes 1 when MetFrontEndState::mUsingMemcard is 1 and the exit was not a cancel. Otherwise
     * a cancel returns to `MetConfigOptionsButtonsScreen` and a store saves the global settings
     * through MetGlobalSettingsSaverScreen.
     *
     * @ghidraAddress NTSC-U/C: 0x00201790
     * @ghidraAddress PAL: 0x00209fa0
     */
    virtual void OnExitFinished();

    /**
     * Resolve the rows, the two instruction texts, the button meshes, and the value texts.
     *
     * The instruction texts are not tested for null. The European release also fills the five
     * action labels ahead of the instructions, and after them the two panel titles and the two
     * stick rows' values.
     *
     * @ghidraAddress NTSC-U/C: 0x001fff40
     * @ghidraAddress PAL: 0x00207618
     */
    virtual void ResolveContainerViews();

private:
    /**
     * Clears every other row that shows the same button as nRow.
     *
     * On a stick row it places the other stick on the other stick row instead.
     *
     * @ghidraAddress NTSC-U/C: 0x00201eb8
     * @ghidraAddress PAL: 0x0020a818
     */
    void ClearDuplicateAssignment(int nRow);

    /**
     * Highlights the button mesh of the selected row's button.
     *
     * @ghidraAddress NTSC-U/C: 0x00202070
     * @ghidraAddress PAL: 0x0020ab70
     */
    void UpdateButtonHighlight();

    /**
     * Shows one mapping in the value texts.
     *
     * @ghidraAddress NTSC-U/C: 0x002022a0
     * @ghidraAddress PAL: 0x0020afa8
     */
    void ShowConfig(ControllerConfig &config);

    /**
     * Shows the highlight mesh of one button and hides the others. -1 hides every mesh.
     *
     * @ghidraAddress NTSC-U/C: 0x00206a08
     * @ghidraAddress PAL: 0x0020f8d0
     */
    void SetButtonHighlights(int nButton);

    /**
     * Stores the value texts into this controller's mapping.
     *
     * Always returns 1, and the one caller discards it.
     *
     * @ghidraAddress NTSC-U/C: 0x00206aa0
     * @ghidraAddress PAL: 0x0020f968
     */
    int StoreConfig();

    /**
     * Maps a value text to a button index, or -1.
     *
     * @ghidraAddress NTSC-U/C: 0x00206b30
     * @ghidraAddress PAL: 0x0020ae38
     */
    int ButtonIndexForText(const HxStr &text) const;

    /**
     * Reports whether every row has a button.
     *
     * @ghidraAddress NTSC-U/C: 0x00206ca8
     * @ghidraAddress PAL: 0x0020fb10
     */
    bool AllRowsAssigned() const;

    /**
     * The code before one code in a row's range.
     *
     * Returns 0 for a stick row. `this` is passed and not read.
     *
     * @ghidraAddress NTSC-U/C: 0x00206d88
     * @ghidraAddress PAL: 0x0020fb88
     */
    char PreviousButtonCode(int nRow, char code) const;

    /**
     * The code after one code in a row's range.
     *
     * Returns 0 for a stick row. `this` is passed and not read.
     *
     * @ghidraAddress NTSC-U/C: 0x00206e30
     * @ghidraAddress PAL: 0x0020fc30
     */
    char NextButtonCode(int nRow, char code) const;

    MetButtonList *mRows;                    // +0x90
    std::vector<Rnd::Mesh *> mButtonMeshes;  // +0x94, the button highlight meshes
    std::vector<HxStr> mButtonMeshNames;     // +0xa0, the names of mButtonMeshes
    std::vector<Rnd::Text *> mRowValueTexts; // +0xac, the row value texts
    std::vector<HxStr> mRowValueTextNames;   // +0xb8, the names of mRowValueTexts
    char mFirstButtonCode;                   // +0xc4, the first button code, `a`
    char mLastButtonCode;                    // +0xc5, the last button code, `h`, never read

public:
    /**
     * Index of the controller this screen configures, from 0.
     *
     * The constructor writes 0. MetConfigOptionsButtonsScreen::OnExitFinished() writes its
     * selected row here at `0x00208574` before pushing this screen, and the image has no setter
     * to route that write through. +0xc8
     */
    int mControllerIndex;

private:
    int mReserved; // +0xcc, never read or written
};
