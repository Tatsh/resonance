#pragma once

#include <vector>

#include "met/metscreen.h"

class MetButtonList;
class MetPersonaData;
namespace Rnd {
class Button;
class Tex;
} // namespace Rnd

/**
 * Screen that creates a new FreQ, either from a pre-fab identity or from scratch.
 *
 * Its RTTI descriptor is at `0x00901aa0`. It has MetScreen as its one public non-virtual base at
 * offset 0.
 *
 * The 39-entry primary vtable at `0x007f7808` is the same length as the MetScreen table, and the
 * class declares no new virtual.
 *
 * New() allocates 0xa4 bytes. The screen shows a carousel of the pre-fab identities beside two
 * buttons, `cf_prefab.but` and `cf_create.but`. While the first button is selected, the left and
 * right commands step the carousel and the selected identity is burned into `cf_char.mat`. Its
 * layout and its behaviour follow MetLoadFreqBaseScreen closely, although it does not derive from
 * that class.
 *
 * The translation unit spans `0x0029c130` to `0x002a0e30`. Besides the members below, it has the
 * type function at `0x002a0a30`, per-unit copies of MsgSink routines, and template library
 * emissions.
 *
 * Apart from the type function and the destructor, the slots that differ from the MetScreen table
 * are 5, 19, 23, 24, 30, 36, and 38.
 */
class MetFreqCreateScreen : public MetScreen {
public:
    /**
     * Construct the screen.
     *
     * Supplies `cf` for the screen name, `metagame/_Solo` for the directory, and `create_freq` for
     * the container, allocates the button list, waits for the FreQ maker assets, and resolves the
     * first persona burn texture.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress NTSC-U/C: 0x0029c130
     * @ghidraAddress PAL: 0x002b9950
     */
    MetFreqCreateScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * Delete the button list.
     *
     * @ghidraAddress NTSC-U/C: 0x002a0b08
     * @ghidraAddress PAL: 0x002be8a8
     */
    virtual ~MetFreqCreateScreen();

    /**
     * Build the screen on the heap.
     *
     * The routine at `0x00385180` that creates every front-end screen is the caller.
     *
     * @param pRenderer The front-end renderer the screen registers on.
     * @param nPriority The load priority.
     * @return The new screen.
     * @ghidraAddress NTSC-U/C: 0x002a0a80
     * @ghidraAddress PAL: 0x002be820
     */
    static MetFreqCreateScreen *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Take the pre-fab list, select the first button, refresh the preview, set the title, bring up
     * the left gizmo, and show the screen.
     *
     * Slot 5.
     *
     * @ghidraAddress NTSC-U/C: 0x0029ccb8
     * @ghidraAddress PAL: 0x002ba6c0
     */
    virtual void EnterAndShow();

    /**
     * Step the buttons or the carousel, start the chosen button's alternation, or back out.
     *
     * Slot 19. The left and right commands step the carousel only while the first button is
     * selected, and each starts its arrow's alternation first.
     *
     * @param pCommand The command.
     * @ghidraAddress NTSC-U/C: 0x0029c7a0
     * @ghidraAddress PAL: 0x002ba0f0
     */
    virtual void HandleCommand(const MetScreenCommand *pCommand);

    /**
     * Play the base cycle-left sound while the first button is selected.
     *
     * Slot 23.
     *
     * @param nSelector Passed through to MetScreen::PlayCycleLeftSound().
     * @ghidraAddress NTSC-U/C: 0x002a0b88
     * @ghidraAddress PAL: 0x002be928
     */
    virtual void PlayCycleLeftSound(int nSelector);

    /**
     * Play the base cycle-right sound while the first button is selected.
     *
     * Slot 24.
     *
     * @param nSelector Passed through to MetScreen::PlayCycleRightSound().
     * @ghidraAddress NTSC-U/C: 0x002a0bb8
     * @ghidraAddress PAL: 0x002be958
     */
    virtual void PlayCycleRightSound(int nSelector);

    /**
     * Exit forwards once the chosen button's alternation has finished.
     *
     * Slot 30. An arrow's alternation is passed over.
     *
     * @param pButton The button whose alternation finished.
     * @ghidraAddress NTSC-U/C: 0x0029ce58
     * @ghidraAddress PAL: 0x002ba8b8
     */
    virtual void OnRepeatingSoundFinished(Rnd::Button *pButton);

    /**
     * Bring up the next screen once this one has exited.
     *
     * Slot 36. After a back command MetLoadFreqScreen returns. Otherwise `MetFreqCreateScreen` is
     * recorded in MetFrontEndState::mReturnScreen, the persona is marked new, the canvas takes the
     * selected pre-fab for the first button or an empty persona for the second, the game manager's
     * persona list is cleared, and the four FreQ maker screens come up with the buttons screen
     * active. The button selection is cleared on both paths. After a back command the European
     * release returns to MetLoadNewFreqScreen instead when the load list is empty.
     *
     * @ghidraAddress NTSC-U/C: 0x0029cfa0
     * @ghidraAddress PAL: 0x002baa40
     */
    virtual void OnExitFinished();

    /**
     * Resolve the base views and the two arrows, and add the two buttons with their help texts.
     *
     * Slot 38.
     *
     * @ghidraAddress NTSC-U/C: 0x0029c330
     * @ghidraAddress PAL: 0x002b9bb8
     */
    virtual void ResolveContainerViews();

private:
    /**
     * Step the carousel one identity, wrapping at both ends, and refresh the preview.
     *
     * @ghidraAddress NTSC-U/C: 0x0029cb30
     * @ghidraAddress PAL: 0x002ba518
     */
    void StepSelection(const MetScreenCommand *pCommand);

    /**
     * Burn the selected identity into `cf_char.mat`.
     *
     * @ghidraAddress NTSC-U/C: 0x0029cbb8
     * @ghidraAddress PAL: 0x002ba5a0
     */
    void RefreshSelection();

    std::vector<MetPersonaData *> *mIdentities; // +0x8c, the pre-fab list slot 5 takes
    MetButtonList *mButtonList;                 // +0x90
    int mSelectedIdentity;                      // +0x94
    Rnd::Button *mLeftArrow;                    // +0x98, `cf_left.but`
    Rnd::Button *mRightArrow;                   // +0x9c, `cf_right.but`
    Rnd::Tex *mBurnTexture;                     // +0xa0
};
