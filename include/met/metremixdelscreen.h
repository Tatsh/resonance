#pragma once

#include <vector>

#include "memcard/memcardconnectstate.h"
#include "met/listdataprovider.h"
#include "met/metmemcardpickeruser.h"
#include "met/metsaveremix.h"

class HxStr;
class ScrollingList;
struct MetRemixRecord;

namespace Rnd {
class Font;
} // namespace Rnd

/**
 * Screen that deletes a remix from a memory card.
 *
 * `17MetRemixDelScreen` in the RTTI descriptor at `0x008ef690`, with three public non-virtual
 * bases at fixed offsets, MetSaveRemix at `+0x00`, ListDataProvider at `+232`, and
 * MetMemCardPickerUser at `+236`. The object is 0x140 bytes, which the factory at `0x00343f30`
 * pins by requesting exactly that many with the tag `MsgSink`. That factory is one inlined
 * `new MetRemixDelScreen(renderer, priority)` expression emitted out of line, and the tag is
 * MsgSink's rather than this class's, because MsgSink is the base that declares `operator new`.
 *
 * Four vtables belong to the class, the 43-entry primary at `0x008061d8`, the four-entry
 * ListDataProvider table at `0x008060e0` that adjusts `this` by `-232`, the three-entry MetKBUser
 * table at `0x00806108` that adjusts it by `-144`, and the 21-entry MemcardUser table at
 * `0x00806128` that adjusts it by `-140`. There is no fifth table, and the constructor writes no
 * vptr at `+0xec`, which is one of the four observations that prove MetMemCardPickerUser declares
 * no virtual function.
 *
 * The constructor at `0x003394a0` takes only the renderer and the load priority. It runs the
 * MetSaveRemix constructor at `0x00372120` with `mcrd` for the screen name, `metagame/Shared` for
 * the directory, and `memcard_remix_del` for the container, writes its four vptrs, zeroes
 * mUnknownf4, mUnknownfc, mUnknown100, and mUnknown104, default-constructs the two
 * MemcardConnectState records, zeroes mUnknown138 and mUnknown13c, clears MetScreen::mUnknown60,
 * and pushes `mem_del_remix` into the container object-name vector MetScreen declares at `+0x38`.
 *
 * An earlier reading recorded the span from `+0x10c` to `+0x137` as reserved, on the grounds that
 * the constructor addresses a nested object through a register it could not resolve. The two
 * registers are `+0x108` and `+0x120`, exactly 0x18 apart, and each receives the identical
 * five-store run from the same empty literal at `0x00805ca8`. Both are MemcardConnectState records.
 *
 * The destructor at `0x003397a8` restores the four vptrs, deletes mUnknownf4 through slot 1 of a
 * table at `+0x94` of the object itself, which is where ScrollingList places its vptr, releases
 * the two record names in reverse order as compiler-generated member teardown, restores the
 * ListDataProvider vptr to `0x007ec830`, runs the MetSaveRemix destructor, and releases the object
 * with the tag `MsgSink`.
 *
 * Sixteen entries of the primary table differ from the MetSaveRemix table, which a diff of the two
 * tables settles rather than the title each routine carries. They are 0 `0x00343e88`, the
 * compiler-generated GetTypeInfo, 1 `0x003397a8` the destructor, 5 `0x0033a098`, 7 `0x00344078`,
 * 15 `0x0033b280`, 16 `0x003441a0`, 19 `0x00339b30`, 20 `0x00343f28`, 23 `0x00343f18`,
 * 24 `0x00343f20`, 33 `0x00344058`, 36 `0x0033ce00`, 38 `0x00339880`, 40 `0x0033e5c0`,
 * 41 `0x0033e6d8`, and 42 `0x0033e7f0`. All fourteen behaviour slots are declared below.
 *
 * Slot 16 is otherwise empty in every class of the subsystem, and this screen is the one class that
 * fills it.
 *
 * Slots 40 and 41 have instruction-for-instruction identical bodies at two addresses, each pushing
 * and then activating this screen's own registry key. The vtable indices are what separates them,
 * so the original declared two members with the same body rather than one member the compiler
 * emitted twice.
 *
 * The MemcardUser table overrides two slots this class supplies itself, 12 at `0x0033dec0` and 16
 * at `0x0033d768`, and inherits the four MetSaveRemix supplies. The three-entry MetKBUser table
 * overrides its one slot at `0x00344240`. All three are declared below with the spelling their base
 * gives them.
 *
 * Both pure virtuals of the four-entry ListDataProvider table are supplied here, ProvideText() at
 * `0x0033cc78` and ProvideMesh() at `0x00343f10`.
 *
 * Slot 5 writes MetSaveRemix::mUnknowndc, which that class still declares private. The member
 * belongs in the protected section on the same reasoning that already moved mUnknowne0 there.
 */
class MetRemixDelScreen :
    public MetSaveRemix,
    public ListDataProvider,
    public MetMemCardPickerUser {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x003394a0
     */
    MetRemixDelScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x003397a8
     */
    virtual ~MetRemixDelScreen();

    /**
     * Build the screen on the heap.
     *
     * @param pRenderer The front-end renderer the screen registers on.
     * @param nPriority The load priority.
     * @return The new screen.
     * @ghidraAddress 0x00343f30
     */
    static MetRemixDelScreen *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Hide the screen and request the remix catalogue. Slot 5.
     *
     * It hides itself through slot 17 with a zero argument, clears MetSaveRemix::mUnknowne0 and
     * mUnknown104, writes one into MetSaveRemix::mUnknowndc, and points mUnknownf0 at the
     * MetRemixManager catalogue for mUnknown108.
     *
     * Returning from a confirmation, with mUnknownfc or mUnknown100 set, it reselects the first
     * row of the existing list. Otherwise it raises the one-button `no_remix` dialogue when the
     * card's listing status is 15 or the catalogue is empty, and creates a new ScrollingList when
     * it is not, without deleting any list an earlier entry left. It then clears both flags,
     * refills and redraws the list, pushes the help screen with the `only_back_title` preset and
     * the first prompt of MetScreen::mUnknown38, sets the title to `mem_del_type` formatted with
     * the slot name, pushes the data screen, and runs the MetScreen body.
     *
     * @ghidraAddress 0x0033a098
     */
    virtual void EnterAndShow();

    /**
     * Run the pending keyboard action once the panel has finished activating. Slot 7.
     *
     * MetScreen slot 6 is its one caller. A set MetSaveRemix::mUnknowne0 records that slot 42
     * requested the on-screen keyboard, and the flag is cleared before slot 40 runs so that the
     * request runs once.
     *
     * @ghidraAddress 0x00344078
     */
    virtual void OnUnknownSlot7();

    /**
     * Act on whichever of the screen's dialogues was dismissed. Slot 15.
     *
     * The name is compared against each dialogue in turn. YES on `del_remix_ask` and RETRY on
     * `del_fail_nocard` raise the button-less `del_remix` progress dialogue and queue the delete
     * of the selected row. YES on `remix_copy_ask` records the selected row in mUnknown104 and the
     * next card in mUnknown120 and queues the load of the row for a copy. `del_remix` refills the
     * list and asks MetRemixManager to list mUnknown108 again. `no_remix` removes this screen from
     * the renderer and returns to the card-type screen. Every other answer of this screen's
     * dialogues returns to the list, and a name this screen did not raise goes to the
     * MetSaveRemix body.
     *
     * @param name The dialogue the screen requested, which the message screen reports back.
     * @param nChoice Which of the dialogue's buttons the user chose, counted from zero.
     * @ghidraAddress 0x0033b280
     */
    virtual void OnMsgScreenDismissed(const HxStr &name, int nChoice);

    /**
     * Make the message screen the active panel. Slot 16.
     *
     * The argument is ignored. The panel is the one registered under `MetMsgScreen`.
     *
     * @param name The dialogue the message screen reports back, which the body does not read.
     * @ghidraAddress 0x003441a0
     */
    virtual void OnMsgScreenShown(const HxStr &name);

    /**
     * Act on a navigation command. Slot 19.
     *
     * An eight-entry jump table at `0x00805e00` indexed by the command code less one selects the
     * branch, and a code outside one through eight returns at once. Codes 3, 4, and 5 route to the
     * same return the out-of-range path uses, so the screen discards the two cycle commands and the
     * select command. Codes 7 and 8 are two of the codes above six that the input translator at
     * `0x002e3738` produces and that MetScreen::DeliverCommand() passes through unchanged, and this
     * screen is one of the few that acts on them.
     *
     * Codes 1 and 2 move along the list at mUnknownf4 and show the new row on the data screen,
     * without testing mUnknownf0 for null. Code 6 departs the screen. Codes 7 and 8 do nothing
     * with an empty catalogue, and otherwise play the toggle sound, set mUnknown100 or mUnknownfc
     * respectively, clear the help text, and depart the screen.
     *
     * @param pCommand The command the renderer translated from an input message.
     * @ghidraAddress 0x00339b30
     */
    virtual void HandleCommand(const MetScreenCommand *pCommand);

    /**
     * Silence the slide sound.
     *
     * All three overrides are two-instruction stubs, so each was written inline with an empty
     * body.
     *
     * @ghidraAddress 0x00343f28
     */
    virtual void PlaySlideSound(int) {
    }

    /**
     * Silence the cycle-left sound.
     *
     * @ghidraAddress 0x00343f18
     */
    virtual void PlayCycleLeftSound(int) {
    }

    /**
     * Silence the cycle-right sound.
     *
     * @ghidraAddress 0x00343f20
     */
    virtual void PlayCycleRightSound(int) {
    }

    /**
     * Show the first catalogue row on the data screen. Slot 33.
     *
     * MetScreen slot 32 runs this slot once the enter animation has finished.
     *
     * @ghidraAddress 0x00344058
     */
    virtual void OnUnknownSlot33();

    /**
     * Release the list and depart once the exit animation has finished. Slot 36.
     *
     * With neither mUnknownfc nor mUnknown100 set, it deletes the ScrollingList at mUnknownf4,
     * pushes `MetLeftGizmoScreen` and `MetMemCardTypeScreen`, and activates the latter. With
     * mUnknown100 set, it raises the `remix_copy_ask` confirmation, formatted with the slot name
     * NextCardSlot() gives for mUnknown108. With only mUnknownfc set, it raises the `del_remix_ask`
     * confirmation. Both confirmations offer `NO` and `YES`.
     *
     * @ghidraAddress 0x0033ce00
     */
    virtual void OnUnknownSlot36();

    /**
     * Resolve the list title and the rest of the container objects. Slot 38.
     *
     * It runs the MetSaveRemix slot 38 body, which is MetScreen's, resolves the Rnd::Text
     * `mcrd_listpan_title.txt`, sets it to the configuration string `mcrf_remix`, and shows it.
     * It then resolves the Rnd::Font objects `font1_pink_2` into mUnknown138 and
     * `font1_pinkgrey_2` into mUnknown13c. The title is not tested for null.
     *
     * @ghidraAddress 0x00339880
     */
    virtual void ResolveContainerViews();

    /**
     * Bring this screen back and make it the active panel again. Slot 40.
     *
     * Pushes and then activates this screen's own registry key, `MetRemixDelScreen`. Slot 7 and
     * MetSaveRemix slot 42 are the two callers.
     *
     * @ghidraAddress 0x0033e5c0
     */
    virtual void OnUnknownSlot40();

    /**
     * Bring this screen back and make it the active panel again. Slot 41.
     *
     * The body is identical to slot 40's, instruction for instruction, and the two occupy separate
     * vtable slots.
     *
     * @ghidraAddress 0x0033e6d8
     */
    virtual void OnUnknownSlot41();

    /**
     * Request a remix name from the on-screen keyboard. Slot 42.
     *
     * It sets MetSaveRemix::mUnknowne0 so that slot 7 runs slot 40 once
     * the keyboard has finished, and then builds a MetKeyboardRequest with this screen's own
     * registry key as the screen to return to, `Remix name` as the prompt,
     * MetSaveRemix::mUnknownb8 as the initial text, -1 for any controller, and this object's own
     * MetKBUser subobject as the receiver, and passes it to MetKeyboardScreen::Open().
     *
     * This override replaces the MetSaveRemix body, which is an alias for the empty slot 40 rather
     * than a stub.
     *
     * @ghidraAddress 0x0033e7f0
     */
    virtual void OnUnknownSlot42();

    /**
     * Fill the catalogue from the card. MemcardUser slot 12.
     *
     * A non-zero status raises the one-button `remix_copy_fail` dialogue with the
     * `copy_fail_general` text. A zero status makes this screen MemcardManager::mUser and hands the
     * row at mUnknown104 to MetSaveRemix::RecordPendingSave() with mUnknown120 as the target and
     * -1 as the selector, passing the row's name, its first string, its appearances, and its last
     * word. It also builds a two-entry list of this screen's registry key and `MetHelpScreen` that
     * nothing reads. The port and slot argument is not read.
     *
     * @param nPortSlot Which card port and slot reported, which the body does not read.
     * @param nStatus Zero on success.
     * @ghidraAddress 0x0033dec0
     */
    virtual void OnRemixLoaded(int nPortSlot, int nStatus);

    /**
     * Act on the delete the card reported. MemcardUser slot 16.
     *
     * A zero status exits `MetMsgScreen` and does nothing else. A status of 15 raises the
     * `del_fail_nocard` dialogue with `RETRY` and `CANCEL`, its text formatted with the slot name
     * of mUnknown108. Every other status raises the one-button `del_remix` dialogue with the
     * `del_fail` text. The port and slot argument is not read.
     *
     * @param nPortSlot Which card port and slot reported, which the body does not read.
     * @param nStatus Zero on success, and 15 for the failure the retry dialogue covers.
     * @ghidraAddress 0x0033d768
     */
    virtual void OnRemixDeleted(int nPortSlot, int nStatus);

    /**
     * Forward the entered name to the saver once. MetKBUser slot 2.
     *
     * A clear MetSaveRemix::mUnknowne0 discards the text, which is how a keyboard result that this
     * screen did not request is ignored.
     *
     * @param text The remix name the user entered.
     * @ghidraAddress 0x00344240
     */
    virtual void OnUnknownSlot2(const HxStr &text);

    /**
     * Show the second string of one row of mUnknownf0.
     *
     * An index past the end of mUnknownf0 empties the text instead. The column and the context are
     * not read.
     *
     * @param nItem The row of mUnknownf0.
     * @param nColumn The cell index, which the body does not read.
     * @param pText The cell.
     * @param nContext The list context, which the body does not read.
     * @return Always 1.
     * @ghidraAddress 0x0033cc78
     */
    virtual int ProvideText(int nItem, int nColumn, Rnd::Text *pText, int nContext);

    /**
     * Leave the cell as it is.
     *
     * @param nItem The row, which the body does not read.
     * @param nColumn The cell index, which the body does not read.
     * @param pMesh The cell, which the body does not read.
     * @param nContext The list context, which the body does not read.
     * @return Always 1.
     * @ghidraAddress 0x00343f10
     */
    virtual int ProvideMesh(int nItem, int nColumn, Rnd::Mesh *pMesh, int nContext);

    /**
     * Record the card the screen works on in mUnknown108.
     *
     * MetMemCardTypeScreen's slot 36 is the caller. The title is inferred.
     *
     * @param slot The card.
     * @ghidraAddress 0x00343fb8
     */
    void SetCardSlot(MemcardConnectState slot);

private:
    // Raise the delete progress dialogue and queue the delete of the selected row. Slot 15
    // expands this sequence twice, for the YES of `del_remix_ask` and the RETRY of
    // `del_fail_nocard`, and it has no address of its own.
    inline void StartDelete();

    // 0x003440b8
    // Shows one catalogue row on the data screen. Slot 33 and the shared tail of codes 1 and 2 in
    // slot 19 are its callers. An index past the end hides the record instead, and mUnknownf0 is
    // not tested for null.
    void ShowRowOnDataScreen(int nIndex);

    // The row catalogue the ListDataProvider override at `0x0033cc78` indexes. Never written by any
    // routine of this class, so it is filled from outside. +0xf0
    std::vector<MetRemixRecord> *mUnknownf0;
    // Deleted by the destructor and by slot 36. +0xf4
    ScrollingList *mUnknownf4;
    int mUnknownf8;  // +0xf8, not written by the constructor
    int mUnknownfc;  // +0xfc
    int mUnknown100; // +0x100
    // The catalogue row MemcardUser slot 12 hands to RecordPendingSave(). +0x104
    MetRemixRecord *mUnknown104;
    // Two memory-card locations the screen tracks. +0x108 and +0x120
    MemcardConnectState mUnknown108;
    MemcardConnectState mUnknown120;
    Rnd::Font *mUnknown138; // +0x138, `font1_pink_2`, resolved by slot 38
    Rnd::Font *mUnknown13c; // +0x13c, `font1_pinkgrey_2`, resolved by slot 38
};
