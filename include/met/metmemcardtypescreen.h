#pragma once

#include "memcard/memcardconnectstate.h"
#include "met/metscreen.h"

class MetButtonList;

/**
 * Screen that picks what kind of saved data to delete from a memory card, remixes or FreQs.
 *
 * `20MetMemCardTypeScreen` in the RTTI descriptor at `0x00901f30`, with MetScreen as its one public
 * non-virtual base at offset 0.
 *
 * The 39-entry primary vtable at `0x007fc488` is the same length as the MetScreen table, and the
 * class declares no new virtual.
 *
 * New() allocates 0xa8 bytes. MetMemCardLoadScreen hands the chosen card to SetCardSlot() before
 * bringing the screen up, the title names that card, and the chosen button opens
 * MetRemixDelScreen through MetRemixManager or MetMCFreqDelScreen on the same card.
 *
 * The translation unit spans `0x002d23b8` to `0x002d89c8`. Besides the members below, it has the
 * type function at `0x002d83e8`, per-unit copies of MsgSink routines, and template library
 * emissions.
 *
 * Apart from the type function and the destructor, the slots that differ from the MetScreen table
 * are 5, 19, 23, 24, 30, 36, and 38.
 */
class MetMemCardTypeScreen : public MetScreen {
public:
    /**
     * Construct the screen.
     *
     * Supplies `mcrf` for the screen name, `metagame/Shared` for the directory, and `mcrf_load`
     * for the container, and records the help texts `mcrf_remix` and `mcrf_freq`.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x002d23b8
     */
    MetMemCardTypeScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * Delete the button list.
     *
     * @ghidraAddress 0x002d84d0
     */
    virtual ~MetMemCardTypeScreen();

    /**
     * Build the screen on the heap.
     *
     * @param pRenderer The front-end renderer the screen registers on.
     * @param nPriority The load priority.
     * @return The new screen.
     * @ghidraAddress 0x002d8448
     */
    static MetMemCardTypeScreen *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Select the first button when nothing is selected, title the screen with the card's name
     * through the `mem_del_type` format, and show the screen.
     *
     * Slot 5.
     *
     * @ghidraAddress 0x002d2b50
     */
    virtual void EnterAndShow();

    /**
     * Step the selection, start the chosen button's alternation, or back out.
     *
     * Slot 19.
     *
     * @param pCommand The command.
     * @ghidraAddress 0x002d2898
     */
    virtual void HandleCommand(const MetScreenCommand *pCommand);

    /**
     * Play nothing.
     *
     * Slot 23. The body is empty.
     *
     * @ghidraAddress 0x002d8438
     */
    virtual void PlayCycleLeftSound(int) {
    }

    /**
     * Play nothing.
     *
     * Slot 24. The body is empty.
     *
     * @ghidraAddress 0x002d8440
     */
    virtual void PlayCycleRightSound(int) {
    }

    /**
     * Exit forwards once the chosen button's alternation has finished.
     *
     * Slot 30.
     *
     * @param pButton The button whose alternation finished, which is not read.
     * @ghidraAddress 0x002d2cf0
     */
    virtual void OnUnknownSlot30(Rnd::Button *pButton);

    /**
     * Bring up the next screen once this one has exited.
     *
     * Slot 36. After a back command MetMemCardLoadScreen returns. The remix button hands the card
     * to MetRemixDelScreen and lists that card's remixes through MetRemixManager::ListRemixes(),
     * returning to MetRemixDelScreen. The FreQ button hands the card to MetMCFreqDelScreen and
     * brings that screen up.
     *
     * @ghidraAddress 0x002d2e90
     */
    virtual void OnUnknownSlot36();

    /**
     * Resolve the base views, allocate the button list, and add the two buttons.
     *
     * Slot 38.
     *
     * @ghidraAddress 0x002d26b0
     */
    virtual void ResolveContainerViews();

    /**
     * Record the card the screen works on.
     *
     * MetMemCardLoadScreen's slot 36 is the caller. The title is inferred.
     *
     * @param slot The card.
     * @ghidraAddress 0x002d8560
     */
    void SetCardSlot(MemcardConnectState slot);

private:
    MetButtonList *mButtonList;    // +0x8c
    MemcardConnectState mCardSlot; // +0x90
};
