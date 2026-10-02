#pragma once

#include <vector>

#include "game/freqappearance.h"
#include "memcard/memcardconnectstate.h"
#include "memcard/memcarduser.h"
#include "met/metkbuser.h"
#include "met/metremixrecord.h"
#include "met/metscreen.h"
#include "os/hxstr.h"

/**
 * Base of the two screens that write a remix to a memory card.
 *
 * `13MetSaveRemix` in the RTTI descriptor at `0x008ef660`, with three public non-virtual bases at
 * fixed offsets, MetScreen at `+0x00`, MemcardUser at `+140`, and MetKBUser at `+144`. The object
 * is 0xe8 bytes, which both children fix independently. MetSaveRemixScreen starts its own members
 * at `+0xe8` and MetRemixDelScreen places its ListDataProvider base at `+232`.
 *
 * Two classes derive from the class, MetRemixDelScreen and MetSaveRemixScreen.
 *
 * Three vtables belong to the class. The primary table at `0x00809c00` has 43 entries, four more
 * than the MetScreen table, so the class declares four virtuals of its own at slots 39 through 42
 * at `0x00372488`, `0x0037a618`, `0x0037a620`, and `0x0037a628`. Slots 40 and 41 are
 * two-instruction `jr ra` stubs. Slot 42 is not. Its body dispatches back through slot 40 of the
 * primary table with no argument. Slot 42 is therefore a public alias for the empty slot 40. The
 * 21-entry MemcardUser table at `0x00809b50` adjusts `this` by `-140` and overrides its slots 2, 5,
 * 8, and 11 at `0x00372c10`, `0x00373808`, `0x00374b58`, and `0x00374208`. The three-entry
 * MetKBUser table at `0x00809b30` adjusts `this` by `-144` and overrides its slot 2 at
 * `0x0037a650`.
 *
 * All four slots the class declares are declared below. Slot 39 takes six arguments in a1 through
 * t2, and two of their types come from other classes:
 * MemcardConnectState is the 0x18-byte record whose five fields mirror this class's own `+0x94`
 * through `+0xa8`, and FreqAppearance is the 0x14-byte element of the vector at `+0xac`, a name the
 * RTTI records. The 0x38-byte element of the vector at `+0xcc` is MetRemixRecord, whose name is
 * inferred.
 *
 * The constructor at `0x00372120` takes the renderer, the load priority, and the three names, and
 * forwards all five to MetScreen. Everything it does after the three vptr writes is member
 * initialisation, in this order. The port and slot of mTargetSlot start at -1. Its name is built
 * from the empty string at `0x00809840`, and the destructor frees its buffer for that reason. Its
 * words at `+0xa0` and `+0xa4` start at -1 and its word at `+0xa8` at zero. Both vectors and the
 * two strings at `+0xb8` and `+0xc0` start empty, mOwnerPad, mRefreshFirstCardSlot, mCopying, and
 * mAlbumNumber start at zero, and mKeyboardPending is never written. The three words at `+0xa0`,
 * `+0xa4`, and `+0xa8` are reached through a register set to `this + 0x94`, and all three receive
 * literals.
 *
 * Every one of those stores is a member initialiser, and the MemcardConnectState default
 * constructor produces the -1, empty-string, -1, -1, zero run at `+0x94` exactly, which is
 * independent confirmation of that record's layout.
 *
 * The destructor at `0x00372248` destroys the 0x38-byte elements of mCardRemixes and deallocates
 * its buffer, frees the two strings at `+0xc0` and `+0xb8`, destroys the 0x14-byte elements of
 * mAppearances and deallocates that buffer, frees the name of mTargetSlot, restores the MetKBUser
 * vptr to `0x007f16a0` and the MemcardUser vptr to `0x007daf78`, runs the MetScreen destructor, and
 * releases the object with the tag `MsgSink`. Every one of those steps is compiler-generated
 * member destruction or a vptr restore. The definition is therefore empty. The routine at
 * `0x00183fd0` is the element destructor rather than a statement of this destructor.
 *
 * One inherited slot differs from the MetScreen table, slot 15 at `0x00375590`. That override
 * compares an `HxStr` argument in a1 against a literal and then tests a second argument in a2
 * against one, so slot 15 takes two parameters rather than the one MetScreen records for it.
 */
class MetSaveRemix : public MetScreen, public MemcardUser, public MetKBUser {
public:
    /**
     * Construct the saver.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @param name The screen name.
     * @param directory The directory the container loads from.
     * @param file The container name, without the `.rnd` suffix.
     * @ghidraAddress 0x00372120
     */
    MetSaveRemix(MetRenderer *pRenderer,
                 int nPriority,
                 const HxStr &name,
                 const HxStr &directory,
                 const HxStr &file);

    /**
     * @ghidraAddress 0x00372248
     */
    virtual ~MetSaveRemix();

    /**
     * Record the remix a save is about to write and enquire about the target card. Slot 39.
     *
     * The six parameters are what the register reads prove: a1 is a MemcardConnectState whose
     * five fields are copied into mTargetSlot through the compiler-generated assignment, a2 becomes
     * mOwnerPad, a3 and t0 are assigned to mRemixName and mLevelName, t1 is assigned to
     * mAppearances, and t2 becomes mAlbumNumber. It also clears mRefreshFirstCardSlot, stores
     * itself in MemcardManager::mUser, and queues a connect-state enquiry for the selection's port
     * and slot, with OnConnectState() receiving the result.
     *
     * a3, t0, and t1 are by value rather than by reference, which the tail of the routine proves:
     * it frees the string buffer of each of the first two and destroys every element of the third
     * before returning. a1 is not destroyed there, so it is a reference. OnMsgScreenDismissed() is
     * the one recovered caller, and the title is inferred.
     *
     * @param selection The remix being saved.
     * @param nOwnerPad The controller the save belongs to, or -1 for any controller.
     * @param remixName Assigned to mRemixName.
     * @param levelName Assigned to mLevelName.
     * @param appearances Assigned to mAppearances.
     * @param nAlbumNumber Assigned to mAlbumNumber.
     * @ghidraAddress 0x00372488
     */
    virtual void RecordPendingSave(const MemcardConnectState &selection,
                                   int nOwnerPad,
                                   HxStr remixName,
                                   HxStr levelName,
                                   std::vector<FreqAppearance> appearances,
                                   int nAlbumNumber);

    /**
     * React to the user giving up on the save. Slot 40, empty.
     *
     * A two-instruction `jr ra` stub. OnMsgScreenDismissed() runs it for the giving-up choice of
     * `mem_check`, `mem_remix_2many`, and the two no-space dialogues, and OnDuplicateNameDeclined()
     * runs it as well.
     *
     * @ghidraAddress 0x0037a618
     */
    virtual void OnSaveAbandoned();

    /**
     * React to the closing of a save dialogue that OnMsgScreenDismissed() does not handle. Slot 41,
     * empty.
     *
     * A two-instruction `jr ra` stub.
     *
     * @ghidraAddress 0x0037a620
     */
    virtual void OnSaveDialogueClosed();

    /**
     * React to NO on `mem_remix_dupe`. Slot 42.
     *
     * Dispatches through OnSaveAbandoned() and does nothing else. It is therefore an alias for the
     * empty slot 40 rather than a separate stub. Both subclasses override it to request a new name
     * from the on-screen keyboard.
     *
     * @ghidraAddress 0x0037a628
     */
    virtual void OnDuplicateNameDeclined();

    /**
     * Record the entered name and start the save. MetKBUser slot 2.
     *
     * The override assigns its argument to mRemixName. That assignment settles the parameter of the
     * MetKBUser virtual as a `const HxStr &`. Both subclasses forward to this body.
     *
     * After the assignment it stores itself in MemcardManager::mUser, queues a connect-state
     * enquiry for mTargetSlot, raises the save dialogue through BeginSave(), and limits that
     * dialogue to the controller mOwnerPad identifies.
     *
     * @param text The remix name the user entered.
     * @ghidraAddress 0x0037a650
     */
    virtual void OnKeyboardTextEntered(const HxStr &text);

    /**
     * Act on the state of the card the save targets. MemcardUser slot 2.
     *
     * A failed enquiry raises `mem_check` with RETRY and CONTINUE, or RETRY and CANCEL when
     * mCopying marks a copy. An unformatted card raises `mem_format_check` with NO and YES. A
     * formatted card either records the state as the first GlobalSettings::mCardSlots entry and
     * exits MetMsgScreen when mRefreshFirstCardSlot is set, or empties mCardRemixes, queues a
     * listing of the target card into it, and raises the save dialogue. Every dialogue is limited
     * to the controller mOwnerPad identifies.
     *
     * @param state The card's state, passed by value and destroyed on return.
     * @param nStatus The enquiry status, kMemcardStatusOk on success.
     * @ghidraAddress 0x00372c10
     */
    virtual void OnConnectState(MemcardConnectState state, int nStatus);

    /**
     * Report a finished format. MemcardUser slot 5.
     *
     * Success raises `mem_format_done` and status 13 raises `mem_format_already`, each with one
     * CONTINUE button and made the active panel. Any other status raises `mem_check` with RETRY
     * and BACK. The port and slot are not read.
     *
     * @param nPortSlot The packed port and slot, which is not read.
     * @param nStatus The format status.
     * @ghidraAddress 0x00373808
     */
    virtual void OnCardFormatted(int nPortSlot, int nStatus);

    /**
     * Report a finished remix save. MemcardUser slot 8.
     *
     * Success on port 1 sets mRefreshFirstCardSlot and queues a connect-state enquiry.
     * OnConnectState() then records the result as the first GlobalSettings::mCardSlots entry.
     * Success anywhere else exits MetMsgScreen. A full card raises `save_fail_no_space`, or
     * `copy_fail_no_space` for a copy with the space GlobalSettings::mMinimumFreeClusters requires,
     * and any other status raises `save_fail_no_space` with the `save_fail_general` text.
     *
     * @param nPortSlot The packed port and slot, which is not read.
     * @param nStatus The save status.
     * @ghidraAddress 0x00374b58
     */
    virtual void OnRemixSaved(int nPortSlot, int nStatus);

    /**
     * Check the listing of the target card before saving. MemcardUser slot 11.
     *
     * A remix in mCardRemixes with the name mRemixName raises `mem_remix_dupe` with NO and YES.
     * Fifty or more remixes raise `mem_remix_2many` with RETRY and CONTINUE. Otherwise the remix is
     * saved through MemcardManager::CreateSaveRemixTask(). Neither argument is read.
     *
     * @param nPortSlot The packed port and slot, which is not read.
     * @param nStatus The listing status, which is not read.
     * @ghidraAddress 0x00374208
     */
    virtual void OnRemixesListed(int nPortSlot, int nStatus);

    /**
     * Act on the answer to one of the save dialogues. Slot 15.
     *
     * A retry answer queues a new connect-state enquiry, and a give-up answer runs
     * OnSaveAbandoned(). The YES answer to `mem_format_check` queues a format and raises
     * `mem_format_go`, and NO runs slot 39 with the recorded request. `mem_format_done` and the YES
     * answer to `mem_remix_dupe` save the remix and raise the save dialogue, and NO to
     * `mem_remix_dupe` runs OnDuplicateNameDeclined(). Any other dialogue runs
     * OnSaveDialogueClosed().
     *
     * @param name The dialogue name.
     * @param nChoice The index of the button chosen.
     * @ghidraAddress 0x00375590
     */
    virtual void OnMsgScreenDismissed(const HxStr &name, int nChoice);

private:
    // 0x00372760
    // Raises the `save_remix` dialogue, headed `save_title` with the `mem_save` text
    // for a save, or `copy_title` with the `mem_copy12` text naming the next card slot for a copy.
    void BeginSave();

protected:
    // The card location to save to. MetSaveRemixScreen::Open() assigns it directly, which is why
    // it is protected. +0x94
    MemcardConnectState mTargetSlot;
    // The players' appearances. MetSaveRemixScreen::SetAppearances() assigns it, which is why it
    // is protected. +0xac
    std::vector<FreqAppearance> mAppearances;

    // The remix name. MetSaveRemixScreen's slot 42 hands it to the keyboard, which is why it is
    // protected. +0xb8
    HxStr mRemixName;

private:
    // The level the remix was built over, passed to CreateSaveRemixTask(). +0xc0
    HxStr mLevelName;

protected:
    // The controller the save belongs to. Every dialogue is limited to it. Protected because the
    // MetSaveRemixScreen sound overrides compare their argument against it. The declarations
    // below return to private to preserve the recovered offset order.
    int mOwnerPad; // +0xc8

private:
    // The remixes already on the target card. OnRemixesListed() searches them for a duplicate.
    std::vector<MetRemixRecord> mCardRemixes; // +0xcc
    // Set by a successful save to port 1. OnConnectState() then records the next connect state as
    // the first GlobalSettings::mCardSlots entry. +0xd8
    int mRefreshFirstCardSlot;

protected:
    // Non-zero for a copy rather than a save. MetSaveRemixScreen::EnterAndShow() clears it, which
    // is why it is protected. +0xdc
    int mCopying;

    // Set while a keyboard request from slot 42 is outstanding. Protected because the
    // MetSaveRemixScreen constructor clears it. This class's constructor never writes it.
    int mKeyboardPending; // +0xe0

private:
    // The album number the remix index records, passed to CreateSaveRemixTask(). +0xe4
    int mAlbumNumber;
};
