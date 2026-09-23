#pragma once

#include <vector>

#include "memcard/memcardconnectstate.h"
#include "met/metmemcardpickeruser.h"
#include "met/metmemdetectscreen.h"
#include "rnd/object.h"

namespace Rnd {
class Button;
class Text;
class View;
} // namespace Rnd

/**
 * Screen that picks a memory card to load from.
 *
 * `20MetMemCardLoadScreen` in the RTTI descriptor at `0x00901f20`, with two public non-virtual
 * bases at fixed offsets, MetMemDetectScreen at `+0x00` and MetMemCardPickerUser at `+160`. New()
 * allocates 0xe8 bytes.
 *
 * The class emits **two** vtables, the 44-entry primary at `0x007fbde8` and the 21-entry
 * MemcardUser table at `0x007fbd38` that adjusts `this` by `-140`. It emits none for
 * MetMemCardPickerUser, and its constructor writes vptrs at `+0x00` and `+0x8c` and nothing at
 * `+0xa0`. That is a further confirmation that MetMemCardPickerUser declares no virtual function.
 *
 * The primary table is the same length as the MetMemDetectScreen table, and the class declares no
 * new virtual. Fifteen inherited slots differ, the most of any class in the subsystem.
 *
 * The screen lists up to two cards from GlobalSettings::mCardSlots, the card in port 1 and the
 * card in port 2 or in multitap slot `1-B`. The left and right commands step through them, the
 * select command hands the chosen card to MetMemCardTypeScreen, and command 7 probes again.
 *
 * The translation unit spans `0x002cb8c8` to `0x002d23b8`. Besides the members below, it has the
 * type function at `0x002d1d20`, per-unit copies of MsgSink and MetMemCardPickerUser routines,
 * and template library emissions.
 */
class MetMemCardLoadScreen : public MetMemDetectScreen, public MetMemCardPickerUser {
public:
    /**
     * Construct the screen.
     *
     * Supplies `mcl` for the screen name, `metagame/Shared` for the directory, and `memcard_load`
     * for the container, and records the help text `mcl_card`.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x002cb8c8
     */
    MetMemCardLoadScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x002cbcc8
     */
    virtual ~MetMemCardLoadScreen();

    /**
     * Build the screen on the heap.
     *
     * @param pRenderer The front-end renderer the screen registers on.
     * @param nPriority The load priority.
     * @return The new screen.
     * @ghidraAddress 0x002d1e28
     */
    static MetMemCardLoadScreen *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Bring up the registered card picker on behalf of a requester.
     *
     * Inline. `0x002d1d88` is its uncalled out-of-line copy. The title is inferred.
     *
     * @param pUser The requester.
     */
    static void OpenPicker(MetMemCardPickerUser *pUser);

    /**
     * Record the requester, then either bring the screen up at once or hide it and probe.
     *
     * The title is inferred.
     *
     * @param pUser The requester, recorded in mPickerUser.
     * @param bShowNow True to push and activate the screen, false to hide it while it has a
     *                 container view and run StartDetect().
     * @ghidraAddress 0x002ce160
     */
    void Present(MetMemCardPickerUser *pUser, bool bShowNow);

    /**
     * Hide the screen, then probe again after MetConfigOptionsButtonsScreen or list the cards.
     *
     * Slot 5.
     *
     * @ghidraAddress 0x002cc9c0
     */
    virtual void EnterAndShow();

    /**
     * Start an arrow's alternation, choose the card, back out, or probe again.
     *
     * Slot 19. The two arrows respond only while at least two cards are listed, and the select
     * command only while at least one is.
     *
     * @param pCommand The command.
     * @ghidraAddress 0x002cc328
     */
    virtual void HandleCommand(const MetScreenCommand *pCommand);

    /**
     * Step the selection once an arrow's alternation has finished, and show it.
     *
     * Slot 30. The selection wraps at both ends.
     *
     * @param pObject The arrow that finished alternating.
     * @ghidraAddress 0x002ccfb8
     */
    virtual void OnUnknownSlot30(Rnd::Object *pObject);

    /**
     * Show the help text and select the `mc_opt` preset once the enter animation has finished.
     *
     * Slot 33.
     *
     * @ghidraAddress 0x002d2018
     */
    virtual void OnUnknownSlot33();

    /**
     * Bring up the next screen once this one has exited.
     *
     * Slot 36. A back command returns to MetConfigOptionsButtonsScreen, command 7 probes again
     * through Present(), and a chosen card goes to MetMemCardTypeScreen.
     *
     * @ghidraAddress 0x002cd0e8
     */
    virtual void OnUnknownSlot36();

    /**
     * Resolve the base views, the two arrows, the two card texts, and the information view, and
     * set the panel title.
     *
     * Slot 38.
     *
     * @ghidraAddress 0x002cbeb8
     */
    virtual void ResolveContainerViews();

    /**
     * Play the slide sound while at least one card is listed.
     *
     * Slot 20.
     *
     * @param nSelector Passed through to MetScreen unchanged.
     * @ghidraAddress 0x002d1f40
     */
    virtual void PlaySlideSound(int nSelector);

    /**
     * Play nothing.
     *
     * Slot 22. The body is empty.
     *
     * @ghidraAddress 0x002d1d80
     */
    virtual void PlayHighSound(int) {
    }

    /**
     * Play the cycle-left sound while at least two cards are listed.
     *
     * Slot 23.
     *
     * @param nSelector Passed through to MetScreen unchanged.
     * @ghidraAddress 0x002d1eb0
     */
    virtual void PlayCycleLeftSound(int nSelector);

    /**
     * Play the cycle-right sound under the same condition as PlayCycleLeftSound().
     *
     * Slot 24.
     *
     * @param nSelector Passed through to MetScreen unchanged.
     * @ghidraAddress 0x002d1ef8
     */
    virtual void PlayCycleRightSound(int nSelector);

    /**
     * Show the detection notice, register the screen with the renderer, and run the base probe.
     *
     * Slot 39.
     *
     * @ghidraAddress 0x002ce2c0
     */
    virtual void StartDetect();

    /**
     * Report that the card in port 1 is missing.
     *
     * Slot 41. When the first card listed is in port 2 or slot `1-B`, the `mem_detect_special`
     * text names it together with NextCardSlot(). Otherwise the `mem_check12` text appears.
     *
     * @ghidraAddress 0x002cd4e0
     */
    virtual void OnNoCard();

    /**
     * Refresh the card list and bring the screen up.
     *
     * Slot 42.
     *
     * @ghidraAddress 0x002cdce0
     */
    virtual void OnDetectFinished();

private:
    // 0x002ccab8
    // Titles the panel, refreshes the card list, the arrows, and the selection, and enters. The
    // title is inferred.
    void ShowCards();

    // 0x002ccc48
    // Shows the selected card's name and free space, or the no-card instructions. The title is
    // inferred.
    void ShowSelection();

    // 0x002cde00
    // Rebuilds mCards from GlobalSettings::mCardSlots and clamps mSelected. The title is inferred.
    void RefreshCards();

    // 0x002d1f88
    // Shows both arrows while at least two cards are listed, and hides them otherwise. The title
    // is inferred.
    void UpdateArrows();

    Rnd::Button *mLeftArrow;    // +0xa4
    Rnd::Button *mRightArrow;   // +0xa8
    Rnd::Text *mAvailableText;  // +0xac
    Rnd::Text *mSlotNumberText; // +0xb0
    Rnd::View *mInfoView;       // +0xb4
    // Neither vector is read by a recovered routine, and the element type is not recovered.
    std::vector<Rnd::Object *> mUnknownb8;   // +0xb8
    std::vector<Rnd::Object *> mUnknownc4;   // +0xc4
    int mSelected;                           // +0xd0, the index into mCards
    int mUnknownd4;                          // +0xd4
    std::vector<MemcardConnectState> mCards; // +0xd8
    MetMemCardPickerUser *mPickerUser;       // +0xe4
};

inline void MetMemCardLoadScreen::OpenPicker(MetMemCardPickerUser *pUser) {
    static_cast<MetMemCardLoadScreen *>(FindScreenByName(HxStr("MetMemCardLoadScreen")))
        ->Present(pUser, true);
}
