#pragma once

#include <vector>

#include "memcard/memcardconnectstate.h"
#include "memcard/memcarduser.h"
#include "met/listdataprovider.h"
#include "met/metmemcardpickeruser.h"
#include "met/metscreen.h"

class MetPersonaData;
class ScrollingList;

namespace Rnd {
class Mat;
class Mesh;
class Tex;
class Text;
} // namespace Rnd

/**
 * Screen that lists the saved FreQs on a memory card, to copy one to the other card or delete one.
 *
 * `18MetMCFreqDelScreen` in the RTTI descriptor at `0x00901ea0`, with four public non-virtual bases
 * at fixed offsets, MetScreen at `+0x00`, MemcardUser at `+140`, ListDataProvider at `+144`, and
 * MetMemCardPickerUser at `+148`. New() allocates 0xe8 bytes.
 *
 * The 39-entry primary vtable is at `0x007fabe0`, the same length as the MetScreen table, and the
 * class declares no new virtual. The twenty-one-entry MemcardUser table at `0x007fab30` adjusts
 * `this` by `-140` in every entry, and the four-entry ListDataProvider table at `0x007fab08` by
 * `-144`. The class emits no vtable for MetMemCardPickerUser and writes no vptr at `+0x94`.
 *
 * MetMemCardTypeScreen hands the card over through SetCardSlot(). Activating the screen loads the
 * card's personas, and the list then shows each one's username, face, and birthday. Command 7
 * offers to copy the selected FreQ to NextCardSlot() through MetPersonaSaverScreen::StartSave(),
 * and command 8 offers to delete it through MetPersonaSaverScreen::StartDelete().
 *
 * The translation unit spans `0x002be968` to `0x002c6090`. Besides the members below, it has the
 * type function at `0x002c5aa0`, per-unit copies of MsgSink, MemcardUser, and ListDataProvider
 * routines, and template library emissions.
 */
class MetMCFreqDelScreen :
    public MetScreen,
    public MemcardUser,
    public ListDataProvider,
    public MetMemCardPickerUser {
public:
    /**
     * Construct the screen.
     *
     * Supplies `mcfl` for the screen name, `metagame/Shared` for the directory, and
     * `memcard_freq_load` for the container, records the help text `del_freq`, and clears
     * MetScreen::mUnknown60.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x002be968
     */
    MetMCFreqDelScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * Delete the list and every listed persona.
     *
     * @ghidraAddress 0x002beca8
     */
    virtual ~MetMCFreqDelScreen();

    /**
     * Build the screen on the heap.
     *
     * @param pRenderer The front-end renderer the screen registers on.
     * @param nPriority The load priority.
     * @return The new screen.
     * @ghidraAddress 0x002c5b60
     */
    static MetMCFreqDelScreen *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Hide the screen and mark the card's personas as due to load.
     *
     * Slot 5.
     *
     * @ghidraAddress 0x002c5be8
     */
    virtual void EnterAndShow();

    /**
     * Show the loading notice and query the card, when a load is due.
     *
     * Slot 7.
     *
     * @ghidraAddress 0x002bf750
     */
    virtual void OnUnknownSlot7();

    /**
     * Act on the player's response to one of the screen's dialogues.
     *
     * Slot 15. A confirmed `okDelete` or `okCopy` starts the deletion or the copy, and a refusal
     * brings the list back. `notifyloadfailed` and `no_freq_on_card` return to
     * MetMemCardTypeScreen. Any other dialogue shows the list.
     *
     * @param name The message screen that was dismissed.
     * @param nChoice The chosen button, counted from zero.
     * @ghidraAddress 0x002c0ad8
     */
    virtual void OnMsgScreenDismissed(const HxStr &name, int nChoice);

    /**
     * Scroll the list, back out, or start a copy or a deletion.
     *
     * Slot 19. The select command does nothing.
     *
     * @param pCommand The command.
     * @ghidraAddress 0x002bf3a8
     */
    virtual void HandleCommand(const MetScreenCommand *pCommand);

    /**
     * Play nothing.
     *
     * Slot 20. The body is empty.
     *
     * @ghidraAddress 0x002c5b58
     */
    virtual void PlaySlideSound(int) {
    }

    /**
     * Play nothing.
     *
     * Slot 23. The body is empty.
     *
     * @ghidraAddress 0x002c5b48
     */
    virtual void PlayCycleLeftSound(int) {
    }

    /**
     * Play nothing.
     *
     * Slot 24. The body is empty.
     *
     * @ghidraAddress 0x002c5b50
     */
    virtual void PlayCycleRightSound(int) {
    }

    /**
     * Confirm a copy or a deletion, or return to MetMemCardTypeScreen, once the screen has exited.
     *
     * Slot 36.
     *
     * @ghidraAddress 0x002c01c0
     */
    virtual void OnUnknownSlot36();

    /**
     * Resolve the base views, title the two panels, and resolve the persona detail objects.
     *
     * Slot 38.
     *
     * @ghidraAddress 0x002bee90
     */
    virtual void ResolveContainerViews();

    /**
     * Load the card's personas, or report that the card could not be read.
     *
     * MemcardUser slot 2.
     *
     * @param state The card that was queried. The body does not read it.
     * @param nStatus The result, 0 when the card is present.
     * @ghidraAddress 0x002c1d10
     */
    virtual void OnConnectState(MemcardConnectState state, int nStatus);

    /**
     * Close the loading notice, report that the card has no FreQ, or report a failed load.
     *
     * MemcardUser slot 13. Statuses 0, 3, and 11 count as loaded, and nothing happens unless a
     * load is due.
     *
     * @param nPortSlot The card the personas came from. The body does not read it.
     * @param nStatus The result.
     * @ghidraAddress 0x002c1510
     */
    virtual void OnPersonasLoaded(int nPortSlot, int nStatus);

    /**
     * Show the username of one persona of mPersonas.
     *
     * An index past the end empties the text instead.
     *
     * @param nItem The entry.
     * @param nColumn The cell index. The body does not read it.
     * @param pText The cell.
     * @param nContext The list context. The body does not read it.
     * @return Always 1.
     * @ghidraAddress 0x002c5c28
     */
    virtual int ProvideText(int nItem, int nColumn, Rnd::Text *pText, int nContext);

    /**
     * Leave the cell as it is.
     *
     * @param nItem The row. The body does not read it.
     * @param nColumn The cell index. The body does not read it.
     * @param pMesh The cell. The body does not read it.
     * @param nContext The list context. The body does not read it.
     * @return Always 1.
     * @ghidraAddress 0x002c5b40
     */
    virtual int ProvideMesh(int nItem, int nColumn, Rnd::Mesh *pMesh, int nContext);

    /**
     * Record the card the screen works on, and clear mLoadPending.
     *
     * MetMemCardTypeScreen's slot 36 is the caller. The title is inferred.
     *
     * @param slot The card.
     * @ghidraAddress 0x002c5d10
     */
    void SetCardSlot(MemcardConnectState slot);

private:
    // 0x002bfa88
    // Builds the list on first use, fills it from mPersonas, titles the panel with the card, and
    // enters. The title is inferred.
    void ShowList();

    // 0x002bff98
    // Shows the selected persona's username, face, and birthday, or hides the details when the
    // list is empty. The title is inferred.
    void ShowSelection();

    Rnd::Text *mNameText;     // +0x98
    Rnd::Mat *mFaceMat;       // +0x9c
    int mUnknowna0;           // +0xa0
    Rnd::Mesh *mFreqMesh;     // +0xa4
    Rnd::Text *mBirthdayText; // +0xa8
    Rnd::Tex *mBurnTexture;   // +0xac
    int mUnknownb0;           // +0xb0
    // Set while a confirmed deletion runs. +0xb4
    int mDeleting;
    ScrollingList *mList;          // +0xb8
    MemcardConnectState mCardSlot; // +0xbc
    // The persona a copy command chose, until the copy starts. +0xd4
    MetPersonaData *mCopyPersona;
    // The personas the screen lists. The destructor deletes each one. +0xd8
    std::vector<MetPersonaData *> mPersonas;
    // Set by EnterAndShow() and cleared once the list shows. +0xe4
    int mLoadPending;
};
