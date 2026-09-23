#pragma once

#include <vector>

#include "met/metscreen.h"
#include "rnd/object.h"

class MetButtonList;
class TexturePairRecord;

namespace Rnd {
class Font;
class Mat;
class View;
} // namespace Rnd

/**
 * Screen that picks an arena.
 *
 * `15MetArenasScreen` in the RTTI descriptor at `0x00901e80`, with MetScreen as its one public
 * non-virtual base at offset 0.
 *
 * The 39-entry primary vtable is at `0x007e8ba0`, the same length as the MetScreen table, so the
 * class declares no virtual of its own.
 *
 * The constructor at `0x001f65a0` takes only the renderer and the load priority, and supplies `as`
 * for the screen name, `metagame/_Solo` for the directory, and `arena_sel` for the container. It
 * pushes the object name `arenas` into the container object-name vector that MetScreen owns, and
 * allocates a MetButtonList into mUnknown8c.
 *
 * The object is 0xd4 bytes, which the allocation in New() fixes rather than the constructor's
 * highest store.
 *
 * Eleven entries of the primary table differ from the MetScreen table, which a diff of the two
 * settles rather than the title each routine carries. Apart from the type function they are 1
 * `0x001f70f0` the destructor, 5 `0x001f7750`, 19 `0x001f7310`, 20 `0x001fc718`, 23 `0x001fc680`,
 * 24 `0x001fc688`, 26 `0x001f8af0`, 30 `0x001f8378`, 33 `0x001fc758`, 36 `0x001f8658`, and 38
 * `0x001f6a08`. All ten behaviour slots are declared below.
 *
 * Slots 23 and 24 are two-instruction empty bodies at distinct addresses, and each is a genuine
 * override rather than an inherited empty body, because MetScreen's own slot 23 and 24 both play
 * a named sound. The class therefore silences both cycle sounds while overriding the slide sound
 * with a conditional one.
 *
 * An index below mUnknown98 is an unlocked arena, and mUnknown94 is the last button, the entry
 * for no arena. Both are selectable, and every other button is shown disabled.
 */
class MetArenasScreen : public MetScreen {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x001f65a0
     */
    MetArenasScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * Allocate and construct the screen.
     *
     * The allocation is 0xd4 bytes under MsgSink's tag. The screen factory at `0x00385180` is the
     * caller.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @return The screen.
     * @ghidraAddress 0x001fc690
     */
    static MetArenasScreen *New(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x001f70f0
     */
    virtual ~MetArenasScreen();

    /**
     * Title the screen from the session's mode and play mode, and show it. Slot 5.
     *
     * The title is the configuration string 0x269 for `solo` or `multi`, a space, the one for
     * `game` or `remix`, and the one for `arenas`. The screenshot pair is invalidated, the buttons
     * are set up through SetupArenaButtons(), the prompt layout `standard_title` is selected, and
     * MetScreen::EnterAndShow() runs.
     *
     * @ghidraAddress 0x001f7750
     */
    virtual void EnterAndShow();

    /**
     * Route one command to the button ring, the selection, or the departure. Slot 19.
     *
     * Commands 1 and 2 step the ring and then run UpdateScreenshot(). Command 5 is accepted only
     * for a selectable index, and it then clears the prompt, activates the empty panel name, and
     * starts the selected button alternating with an interval of 30.0f and two cycles. Command 6
     * clears the prompt, clears MetScreen::mUnknown18, exits `MetScreenTitleScreen`, and starts the
     * exit animation.
     *
     * @param pCommand The command to route.
     * @ghidraAddress 0x001f7310
     */
    virtual void HandleCommand(const MetScreenCommand *pCommand);

    /**
     * Play the slide sound only for a selectable index. Slot 20.
     *
     * The base implementation is called directly rather than dispatched.
     *
     * @param nSelector The controller index.
     * @ghidraAddress 0x001fc718
     */
    virtual void PlaySlideSound(int nSelector);

    /**
     * Play no left cycle sound. Slot 23.
     *
     * @param nSelector The controller index, which the body does not read.
     * @ghidraAddress 0x001fc680
     */
    virtual void PlayCycleLeftSound(int nSelector);

    /**
     * Play no right cycle sound. Slot 24.
     *
     * @param nSelector The controller index, which the body does not read.
     * @ghidraAddress 0x001fc688
     */
    virtual void PlayCycleRightSound(int nSelector);

    /**
     * Advance the arena screenshot pair and hand the current texture to the panel mesh. Slot 26.
     *
     * The mesh `as_arena_panel.mesh` is hidden before the read. A read that reports a texture shows
     * the mesh again and hands the texture to the first stage of mUnknown9c. A read that reports
     * nothing returns with the mesh hidden, which is how the screen looks while the screenshot is
     * still loading.
     *
     * @param flTime The current renderer frame position, which the body does not read.
     * @ghidraAddress 0x001f8af0
     */
    virtual void OnUnknownSlot26(float flTime);

    /**
     * Commit the chosen arena and depart, or activate the panel for a disabled entry. Slot 30.
     *
     * MetScreen slot 29 runs the slot once the selection has finished alternating. A selectable
     * index copies the game manager's GameParams, sets GameParams::mArenaName from the arena table,
     * hands the copy back, sets MetScreen::mUnknown18 to 2, exits `MetHelpScreen` and
     * `MetScreenTitleScreen`, and starts the exit animation. Any other index activates the panel
     * named `MetArenasScreen`.
     *
     * @param pObject The object slot 29 finished with, which the body does not read.
     * @ghidraAddress 0x001f8378
     */
    virtual void OnUnknownSlot30(Rnd::Object *pObject);

    /**
     * Post the first prompt. Slot 33.
     *
     * MetScreen slot 32 runs the slot once the enter animation has finished. The prompt is
     * MetScreen::mUnknown38's first element rather than the selected one, which is faithful.
     *
     * @ghidraAddress 0x001fc758
     */
    virtual void OnUnknownSlot33();

    /**
     * Go on to whichever screen the departure was for. Slot 36.
     *
     * MetScreen slot 35 runs the slot once the exit animation has finished. With
     * MetScreen::mUnknown18 clear the screen came from the remix load screen or from the stages,
     * and it goes back there. Otherwise it records `MetArenasScreen` as the return screen and goes
     * on to `MetLoadGameScreen`. Every path ends by selecting -1 on the ring.
     *
     * @ghidraAddress 0x001f8658
     */
    virtual void OnUnknownSlot36();

    /**
     * Resolve the arena buttons, the screenshot material, and the button view. Slot 38.
     *
     * The four palette vectors take the materials and fonts of `as_01.but` (the unlocked look) and
     * `as_02.but` (the locked look). Buttons `as_01.but` through `as_09.but` join the ring. The
     * routine ends by allocating the TexturePairRecord at mUnknown90 from `gArena1.tex` and
     * `gArena2.tex`.
     *
     * @ghidraAddress 0x001f6a08
     */
    virtual void ResolveContainerViews();

private:
    /**
     * Dim the screenshot for a disabled entry and load the selected arena's screenshot.
     *
     * The title is inferred.
     *
     * @ghidraAddress 0x001f75c8
     */
    void UpdateScreenshot();

    /**
     * Lay out the buttons for the arena table and the persona's unlock level.
     *
     * The view `arena_%d_buts.view` for the table's size replaces the contents of mUnknowna0. Each
     * button takes the arena's display name and the unlocked or locked palette, and the last one
     * reads `arena_none`. The title is inferred.
     *
     * @param bUnlockAll Non-zero to unlock every arena. EnterAndShow() passes
     * MetFrontEndState::mUnknown14.
     * @ghidraAddress 0x001f7d40
     */
    void SetupArenaButtons(int bUnlockAll);

    // The arena button ring. The constructor allocates it and the destructor releases it. +0x8c
    MetButtonList *mUnknown8c;
    // The two arena screenshot textures. Slot 38 allocates it and the destructor releases it.
    // +0x90
    TexturePairRecord *mUnknown90;
    // The index of the last button, the entry for no arena. +0x94
    int mUnknown94;
    // The number of unlocked arenas. Every index below it is a selectable arena. +0x98
    int mUnknown98;
    // The screenshot material, `as_screenshot.mat`. +0x9c
    Rnd::Mat *mUnknown9c;
    // The view the arena buttons are drawn through, `arena_butts.view`. +0xa0
    Rnd::View *mUnknowna0;
    // The unlocked palette, from `as_01.but`. +0xa4, +0xbc
    std::vector<Rnd::Mat *> mUnknowna4;
    // The locked palette, from `as_02.but`. +0xb0, +0xc8
    std::vector<Rnd::Mat *> mUnknownb0;
    std::vector<Rnd::Font *> mUnknownbc;
    std::vector<Rnd::Font *> mUnknownc8;
};
