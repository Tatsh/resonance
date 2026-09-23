#pragma once

#include <vector>

#include "memcard/memcarduser.h"
#include "met/listdataprovider.h"
#include "met/metmemcardpickeruser.h"
#include "met/metscreen.h"
#include "met/metsonglists.h"

class MetPersonaData;

/**
 * Screen that lists the saved FreQs on a memory card.
 *
 * The class name and the container name disagree. The RTTI titles the class for deletion and the
 * constructor loads `memcard_freq_load`, and both are reproduced as the image records them rather
 * than reconciled.
 *
 * `18MetMCFreqDelScreen` in the RTTI descriptor at `0x00901ea0`, with four public non-virtual bases
 * at fixed offsets, MetScreen at `+0x00`, MemcardUser at `+140`, ListDataProvider at `+144`, and
 * MetMemCardPickerUser at `+148`.
 *
 * The 39-entry primary vtable is at `0x007fabe0`, the same length as the MetScreen table, so the
 * class declares no virtual of its own.
 *
 * The twenty-one-entry MemcardUser table at `0x007fab30` adjusts `this` by `-140` in every entry.
 *
 * The four-entry ListDataProvider table at `0x007fab08` adjusts `this` by `-144` in every entry.
 *
 * The constructor at `0x002be968` takes only the renderer and the load priority, and supplies
 * `mcfl` for the screen name, `metagame/Shared` for the directory, and `memcard_freq_load` for the
 * container. It writes `+0x8c` and `+0x90`, which are the MemcardUser and ListDataProvider vptrs,
 * then `+0xb4` and `+0xb8`, the CardSlot at `+0xbc`, `+0xd4`, mPersonas at `+0xd8`, and `+0xe4`.
 *
 * It pushes the object name `del_freq` into the container object-name vector that MetScreen owns.
 * It emits two secondary vtables and none for MetMemCardPickerUser, and writes no vptr at `+0x94`
 * where that subobject sits, which is one of the five observations that prove the class declares
 * no virtual function.
 *
 * The object is at least 0xe8 bytes. Nothing derives from the class, so no base offset in any
 * descriptor pins the total, and the figure is the lower bound the constructor's highest store
 * gives.
 *
 * The destructor is at `0x002beca8`.
 *
 * A diff of the primary table against the MetScreen table at `0x0080b6a0` reads eleven overrides,
 * slots 1, 5, 7, 15, 19, 20, 23, 24, 36, and 38 apart from the type function. Every one of the nine
 * below is declared with its address and none has a written body. The three sound overrides sit
 * eight bytes apart at `0x002c5b48`, `0x002c5b50`, and `0x002c5b58`, which bounds each at two
 * instructions.
 *
 * Four addresses that the memory-card band worklist assigned to primary slots 2, 3, and 13 are in
 * the two secondary tables instead. `0x002c1d10` is MemcardUser slot 2 and `0x002c1510` is
 * MemcardUser slot 13, and `0x002c5c28` and `0x002c5b40` are ListDataProvider slots 2 and 3. The
 * primary table leaves slot 2 as the inherited MsgSink::Handle and slot 3 as the MetScreen override
 * of MsgSink::HandleMessage, and no derived table in the family fills primary slot 13 at all. All
 * four are now titled for the table they occupy.
 *
 * `0x002c5b60` is the out-of-line emission of the `new` expression that builds one, which is
 * compiler-generated glue rather than a member and is therefore not declared.
 *
 * Neither overridden MemcardUser virtual is declared here, because MemcardUser does not declare
 * either slot by a recovered name. The two ListDataProvider overrides are declared below.
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
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x002be968
     */
    MetMCFreqDelScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x002beca8
     */
    virtual ~MetMCFreqDelScreen();

    /**
     * Populate the FreQ list and enter.
     *
     * Slot 5. The body is not written.
     *
     * @ghidraAddress 0x002c5be8
     */
    virtual void EnterAndShow();

    /**
     * Unrecovered. Slot 7.
     *
     * The MetScreen body is empty and reveals no parameter list, so the declaration follows the
     * base and is provisional. The body is not written.
     *
     * @ghidraAddress 0x002bf750
     */
    virtual void OnUnknownSlot7();

    /**
     * Respond to a message screen being dismissed.
     *
     * Slot 15. The body is not written.
     *
     * @param name The message screen that was dismissed.
     * @param nChoice The response.
     * @ghidraAddress 0x002c0ad8
     */
    virtual void OnMsgScreenDismissed(const HxStr &name, int nChoice);

    /**
     * Act on one navigation command.
     *
     * Slot 19. The body is not written.
     *
     * @param pCommand The command the renderer translated from an input message.
     * @ghidraAddress 0x002bf3a8
     */
    virtual void HandleCommand(const MetScreenCommand *pCommand);

    /**
     * Play the slide sound.
     *
     * Slot 20. The body is not written.
     *
     * @param nSelector The controller the command came from.
     * @ghidraAddress 0x002c5b58
     */
    virtual void PlaySlideSound(int nSelector);

    /**
     * Play the cycle-left sound.
     *
     * Slot 23. The body is not written.
     *
     * @param nSelector The controller the command came from.
     * @ghidraAddress 0x002c5b48
     */
    virtual void PlayCycleLeftSound(int nSelector);

    /**
     * Play the cycle-right sound.
     *
     * Slot 24. The body is not written.
     *
     * @param nSelector The controller the command came from.
     * @ghidraAddress 0x002c5b50
     */
    virtual void PlayCycleRightSound(int nSelector);

    /**
     * Respond to the exit animation finishing.
     *
     * Slot 36. The body is not written.
     *
     * @ghidraAddress 0x002c01c0
     */
    virtual void OnUnknownSlot36();

    /**
     * Resolve the container views.
     *
     * Slot 38. The body is not written.
     *
     * @ghidraAddress 0x002bee90
     */
    virtual void ResolveContainerViews();

    /**
     * Show the username of one persona of mPersonas.
     *
     * An index past the end empties the text instead.
     *
     * @param nItem The entry.
     * @param nColumn The cell index, which the body does not read.
     * @param pText The cell.
     * @param nContext The list context, which the body does not read.
     * @return Always 1.
     * @ghidraAddress 0x002c5c28
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
     * @ghidraAddress 0x002c5b40
     */
    virtual int ProvideMesh(int nItem, int nColumn, Rnd::Mesh *pMesh, int nContext);

private:
    // The seven words from +0x98 through +0xb3 are not written by the constructor and no reader
    // is recovered.
    int mUnknown98[7];
    int mUnknownb4;      // +0xb4
    int mUnknownb8;      // +0xb8
    CardSlot mUnknownbc; // +0xbc
    int mUnknownd4;      // +0xd4
    // The personas the screen lists. The destructor deletes each one. +0xd8
    std::vector<MetPersonaData *> mPersonas;
    int mUnknowne4; // +0xe4
};
