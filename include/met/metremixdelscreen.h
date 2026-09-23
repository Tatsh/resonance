#pragma once

#include <vector>

#include "met/listdataprovider.h"
#include "met/metmemcardpickeruser.h"
#include "met/metremixselection.h"
#include "met/metsaveremix.h"

class HxStr;
class ScrollingList;
struct MetRemixRecord;

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
 * MetRemixSelection records, zeroes mUnknown138 and mUnknown13c, clears MetScreen::mUnknown60,
 * and pushes `mem_del_remix` into the container object-name vector MetScreen declares at `+0x38`.
 *
 * An earlier reading recorded the span from `+0x10c` to `+0x137` as reserved, on the grounds that
 * the constructor addresses a nested object through a register it could not resolve. The two
 * registers are `+0x108` and `+0x120`, exactly 0x18 apart, and each receives the identical
 * five-store run from the same empty literal at `0x00805ca8`. Both are MetRemixSelection records.
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
     * Hide the screen and request the remix catalogue. Slot 5.
     *
     * The body is not written. It hides itself through slot 17 with a zero argument, clears
     * MetSaveRemix::mUnknowne0 and mUnknown104, writes one into MetSaveRemix::mUnknowndc, and then
     * resolves the MetRemixManager through the accessor at `0x00361000` and drives it for roughly
     * 0x400 further instructions. None of the manager routines it reaches is identified.
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
     * Act on the delete confirmation. Slot 15.
     *
     * The body is not written. It compares the dialogue name against `del_remix_ask` through
     * HxStr::MatchesLiteral and returns at once when it differs, then branches on the choice
     * against 1. The accepting branch builds a memcard delete request and runs for roughly 0x2e0
     * instructions through routines of the memcard layer, none of which is identified.
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
     * The body is not written. An eight-entry jump table at `0x00805e00` indexed by the command
     * code less one selects the branch, and a code outside one through eight returns at once. Codes
     * 3, 4, and 5 route to the same return the out-of-range path uses, so the screen discards the
     * two cycle commands and the select command. Codes 7 and 8 are two of the codes above six that
     * the input translator at `0x002e3738` produces and that MetScreen::DeliverCommand() passes
     * through unchanged, and this screen is one of the few that acts on them.
     *
     * Codes 1 and 2 walk the list at mUnknownf4 through the same ScrollingList accessor and move
     * methods MetRemixLoadScreen uses, at `0x00401160`, `0x00400ec8`, and `0x00400f78`. Code 6
     * departs the screen, code 7 runs from `0x00339ddc`, and code 8 from `0x00339c84`.
     *
     * What blocks the body is the three ScrollingList methods, none of which that class declares.
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
     * The body is not written. It returns at once while either mUnknownfc or mUnknown100 is set,
     * then deletes the ScrollingList at mUnknownf4 and runs for roughly 0x300 further instructions
     * choosing which screen to return to. Several of the routines it reaches belong to the memcard
     * layer and are not identified.
     *
     * @ghidraAddress 0x0033ce00
     */
    virtual void OnUnknownSlot36();

    /**
     * Resolve the list title and the rest of the container objects. Slot 38.
     *
     * The body is not written. It runs the MetSaveRemix slot 38 body, which is MetScreen's, and
     * then resolves `mcrd_listpan_title.txt` and several further objects by name through
     * Rnd::Manager::Find(), narrowing each with a dynamic_cast to Rnd::Text.
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
     * The body is not written. It sets MetSaveRemix::mUnknowne0 so that slot 7 runs slot 40 once
     * the keyboard has finished, and then runs the six-argument keyboard entry point at
     * `0x0028cf18` with this screen's own registry key as the screen to return to, `Remix name` as
     * the prompt, MetSaveRemix::mUnknownb8 as the initial text, -1, and this object's own MetKBUser
     * subobject as the receiver. That entry point belongs to MetKeyboardScreen and is neither
     * titled nor declared.
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
     * The body is not written. A non-zero status returns through a separate error path at
     * `0x0033e208`. The success path builds the row records, assigning each row's strings from the
     * container through `0x004b7dd8`, and runs for roughly 0x340 instructions. The port and slot
     * argument is not read.
     *
     * @param nPortSlot Which card port and slot reported, which the body does not read.
     * @param nStatus Zero on success.
     * @ghidraAddress 0x0033dec0
     */
    virtual void OnRemixLoaded(int nPortSlot, int nStatus);

    /**
     * Act on the delete the card reported. MemcardUser slot 16.
     *
     * The body is not written. A zero status exits `MetMsgScreen` and does nothing else. A status
     * of 15 raises a dialogue whose button set includes `RETRY`. Every other status returns
     * through a third path at `0x0033da58`. The port and slot argument is not read.
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

private:
    // 0x003440b8
    // Shows one catalogue row on the data screen. Slot 33 and four branches of slot 19
    // are its callers. The body is not written: it resolves the screen registered under
    // `MetRemixDataScreen`, and then runs the routine at `0x003455a8` on that screen with the
    // address of the indexed row, or the one at `0x00345ba0` with a null argument when mUnknownf0
    // is null or the index is out of range. Neither MetRemixDataScreen routine is declared.
    void ShowRowOnDataScreen(int nIndex);

    // The row catalogue the ListDataProvider override at `0x0033cc78` indexes. Never written by any
    // routine of this class, so it is filled from outside. +0xf0
    std::vector<MetRemixRecord> *mUnknownf0;
    // Deleted by the destructor and by slot 36. +0xf4
    ScrollingList *mUnknownf4;
    int mUnknownf8;  // +0xf8, not written by the constructor
    int mUnknownfc;  // +0xfc
    int mUnknown100; // +0x100
    int mUnknown104; // +0x104
    // The two remixes the screen tracks. +0x108 and +0x120
    MetRemixSelection mUnknown108;
    MetRemixSelection mUnknown120;
    int mUnknown138; // +0x138
    int mUnknown13c; // +0x13c
};
