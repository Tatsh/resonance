#pragma once

#include "met/metscreen.h"
#include "os/hxstr.h"

/**
 * Base of the five loading-tip screens.
 *
 * Its RTTI descriptor is at `0x008ef920`. It has MetScreen as its one public non-virtual base at
 * offset 0. The 39-entry vtable is at `0x00801610`, the same length as the MetScreen table, and the
 * class declares no new virtual. The object is 0xa0 bytes, the size every child's New()
 * requests.
 *
 * Five classes derive from the class, MetMultiTips1Screen through MetMultiTips5Screen. Each child
 * has an accessor-sized GetTypeInfo of its own at `0x0030d880`, `0x0030da10`, `0x0030dba0`,
 * `0x0030dd30`, and `0x0030dec0`.
 *
 * The five children form a ring through the previous-page and next-page registry keys, and both
 * ends of the ring lead to `MetLocNumPlayersScreen`. Every page loads from `metagame/_Local` and
 * shows `multi_tip_help` as its help prompt.
 *
 * Nine slots differ from the MetScreen table. Slots 22, 23, and 24 are two-instruction `jr ra`
 * stubs, so this class silences three of the six MetScreen sounds by overriding them with an empty
 * body.
 *
 *  - 1 `0x0030d710` the destructor.
 *  - 5 `0x003070f0` EnterAndShow().
 *  - 9 `0x0030d7d0` BeginExit().
 *  - 19 `0x00306ee0` HandleCommand().
 *  - 22 `0x0030d7a0` PlayHighSound(), overridden empty.
 *  - 23 `0x0030d790` PlayCycleLeftSound(), overridden empty.
 *  - 24 `0x0030d798` PlayCycleRightSound(), overridden empty.
 *  - 33 `0x0030d7a8` OnEnterFinished().
 *  - 36 `0x003072a0` OnExitFinished().
 */
class MetMultiTipsBaseScreen : public MetScreen {
public:
    /**
     * @ghidraAddress NTSC-U/C: 0x0030d710
     * @ghidraAddress PAL: 0x00333240
     */
    virtual ~MetMultiTipsBaseScreen();

    /**
     * Show the numbered title, enter, and select the tips help layout.
     *
     * Slot 5. The title is the `multi_tips` title (code 0x269) followed by the page number. The
     * MetScreen body runs between setting the title and selecting `multi_tips_tab`.
     *
     * @ghidraAddress NTSC-U/C: 0x003070f0
     * @ghidraAddress PAL: 0x0032bbc8
     */
    virtual void EnterAndShow();

    /**
     * Exit the title screen and begin the exit.
     *
     * Slot 9.
     *
     * @ghidraAddress NTSC-U/C: 0x0030d7d0
     * @ghidraAddress PAL: 0x00333328
     */
    virtual void BeginExit();

    /**
     * Leave the page.
     *
     * Slot 19. Select moves to the next page, back to the previous one, and command 8 returns to
     * the player-count screen after playing the leave sound. Each clears the active panel,
     * records its choice in MetScreen::mExitChoice, and begins the exit.
     *
     * @param pCommand The command.
     * @ghidraAddress NTSC-U/C: 0x00306ee0
     * @ghidraAddress PAL: 0x0032b958
     */
    virtual void HandleCommand(const MetScreenCommand *pCommand);

    /**
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress NTSC-U/C: 0x0030d7a0
     * @ghidraAddress PAL: 0x003332f8
     */
    virtual void PlayHighSound(int nSelector);

    /**
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress NTSC-U/C: 0x0030d790
     * @ghidraAddress PAL: 0x003332e8
     */
    virtual void PlayCycleLeftSound(int nSelector);

    /**
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress NTSC-U/C: 0x0030d798
     * @ghidraAddress PAL: 0x003332f0
     */
    virtual void PlayCycleRightSound(int nSelector);

    /**
     * Post the first help prompt.
     *
     * Slot 33. Posts the first entry of MetScreen::mHelpKeys at the renderer's current time.
     *
     * @ghidraAddress NTSC-U/C: 0x0030d7a8
     * @ghidraAddress PAL: 0x00333300
     */
    virtual void OnEnterFinished();

    /**
     * Push the page the recorded choice leads to.
     *
     * Slot 36. A back pushes the previous page, a select the next page, and anything else returns
     * to the player-count screen.
     *
     * @ghidraAddress NTSC-U/C: 0x003072a0
     * @ghidraAddress PAL: 0x0032bde0
     */
    virtual void OnExitFinished();

protected:
    /** The exit a command recorded in MetScreen::mExitChoice. */
    enum Exit {
        kExitPrevious = 0, /*!< Back to the previous page. */
        kExitNext = 1,     /*!< On to the next page. */
        kExitQuit = 2,     /*!< Back to the player-count screen. */
    };

    /**
     * Construct a tip page.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @param name The screen name.
     * @param file The container name, without the `.rnd` suffix.
     * @param nPage The page number shown in the title, counted from 1.
     * @param previous The registry key of the previous page.
     * @param next The registry key of the next page.
     * @ghidraAddress NTSC-U/C: 0x00306cd8
     * @ghidraAddress PAL: 0x0032b700
     */
    MetMultiTipsBaseScreen(MetRenderer *pRenderer,
                           int nPriority,
                           const HxStr &name,
                           const HxStr &file,
                           int nPage,
                           const HxStr &previous,
                           const HxStr &next);

    /**
     * Return to the player-count screen.
     *
     * Inline. Slot 36 and the first and last pages' slot 36 overrides expand it. Pushes
     * `MetLeftGizmoScreen` and `MetLocNumPlayersScreen` and activates the second.
     */
    void ReturnToPlayerCount() {
        PushNamedScreen(HxStr("MetLeftGizmoScreen"));
        PushNamedScreen(HxStr("MetLocNumPlayersScreen"));
        ActivateNamedPanel(HxStr("MetLocNumPlayersScreen"));
    }

private:
    HxStr mPreviousScreen; // +0x8c, the previous page
    HxStr mNextScreen;     // +0x94, the next page
    int mPage;             // +0x9c, the page number
};
