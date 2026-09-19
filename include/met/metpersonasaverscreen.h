#pragma once

#include <vector>

#include "memcard/memcarduser.h"
#include "met/metkbuser.h"
#include "met/metpersonadata.h"
#include "met/metscreen.h"
#include "os/hxstr.h"

/**
 * Dialogue that writes a persona to a memory card.
 *
 * `21MetPersonaSaverScreen` in the RTTI descriptor at `0x008f0060`, with three public non-virtual
 * bases at fixed offsets, MetScreen at `+0x00`, MemcardUser at `+140`, and MetKBUser at `+144`.
 *
 * The 39-entry primary vtable is at `0x00805600`, the same length as the MetScreen table, so the
 * class declares no virtual of its own.
 *
 * The twenty-one-entry MemcardUser table at `0x00805550` adjusts `this` by `-140` in every entry.
 *
 * The three-entry MetKBUser table at `0x00805530` adjusts `this` by `-144` in every entry.
 *
 * The constructor at `0x0032ece0` takes only the renderer and the load priority, and supplies
 * `dlg` for the screen name, `metagame/Shared` for the directory, and `dialogue` for the
 * container. It writes `+0x8c` and `+0x90`, which are the two secondary vptrs, then zeroes
 * mUnknown98 and mUnknown9c, default-constructs mPersonas and mUnknownac, zeroes mUnknownbc, sets
 * mUnknownc0 to -1, constructs mUnknownc4 from the empty literal at `0x00805160`, and finishes
 * with mUnknownd0 at -1, mUnknownd4 at zero, and mUnknowncc at -1. The last three are stored out
 * of offset order, mUnknowncc last of all.
 *
 * The object is at least 0xd8 bytes. Nothing derives from the class, so no base offset in any
 * descriptor pins the total, and the figure is the lower bound the constructor's highest store
 * gives.
 *
 * The destructor at `0x0032f020` calls ClearPersonas() and then runs the compiler-generated
 * teardown of mUnknownc4, mUnknownac, and mPersonas in reverse declaration order, so the call is
 * the whole of its reconstructed body.
 *
 * A diff of the primary table against the MetScreen table at `0x0080b6a0` reads nine overrides,
 * slots 1, 5, 7, 9, 15, 23, 24, and 38 apart from the type function.
 *
 * Five addresses that the memory-card band worklist assigned to primary slots 2, 5, 7, and 13 are
 * in the two secondary tables instead. `0x0032f5a0`, `0x00331358`, `0x00333210`, and `0x00332428`
 * are MemcardUser slots 2, 5, 7, and 13, and `0x003391a8` is MetKBUser slot 2. Two of those, the
 * MemcardUser slots 5 and 7, carried the same slot numbers as the genuine primary overrides at
 * `0x003390c0` and `0x003390f0`, and the table diff is what separates the two pairs.
 *
 * `0x00338f98` is the out-of-line emission of the `new` expression that builds one, which is
 * compiler-generated glue rather than a member and is therefore not declared.
 *
 * Neither overridden MemcardUser virtual is declared here, because MemcardUser declares none of
 * those slots by a recovered name. The MetKBUser override is declared, because that base declares
 * its one pure virtual.
 *
 * Six bodies are not written, every one of them declared below with its address.
 */
class MetPersonaSaverScreen : public MetScreen, public MemcardUser, public MetKBUser {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x0032ece0
     */
    MetPersonaSaverScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x0032f020
     */
    virtual ~MetPersonaSaverScreen();

    /**
     * Populate the dialogue and enter.
     *
     * Slot 5, not the MemcardUser slot 5 at `0x00331358`. The body is not written.
     *
     * @ghidraAddress 0x003390c0
     */
    virtual void EnterAndShow();

    /**
     * Unrecovered. Slot 7, not the MemcardUser slot 7 at `0x00333210`.
     *
     * The MetScreen body is empty and reveals no parameter list, so the declaration follows the
     * base and is provisional. The body is not written.
     *
     * @ghidraAddress 0x003390f0
     */
    virtual void OnUnknownSlot7();

    /**
     * Begin the exit.
     *
     * Slot 9. The body is not written.
     *
     * @ghidraAddress 0x00339110
     */
    virtual void BeginExit();

    /**
     * Respond to a message screen being dismissed.
     *
     * Slot 15. The body is not written.
     *
     * @param name The message screen that was dismissed.
     * @param nChoice The response.
     * @ghidraAddress 0x003346c8
     */
    virtual void OnMsgScreenDismissed(const HxStr &name, int nChoice);

    /**
     * Resolve the container views.
     *
     * Slot 38. The body is not written.
     *
     * @ghidraAddress 0x003390a0
     */
    virtual void ResolveContainerViews();

    /**
     * Receive the text the keyboard committed.
     *
     * MetKBUser slot 2, in the secondary table at `0x00805530` with a `-144` adjustment. It
     * supplies the one MetKBUser pure virtual, which is what makes this class concrete. The
     * parameter comes from the base declaration. The body is not written.
     *
     * @param text The text the user entered.
     * @ghidraAddress 0x003391a8
     */
    virtual void OnUnknownSlot2(const HxStr &text);

    /**
     * Silence the cycle-left sound.
     *
     * Both overrides are two-instruction stubs, so each was written inline with an empty body.
     *
     * @ghidraAddress 0x00338f88
     */
    virtual void PlayCycleLeftSound(int) {
    }

    /**
     * Silence the cycle-right sound.
     *
     * @ghidraAddress 0x00338f90
     */
    virtual void PlayCycleRightSound(int) {
    }

private:
    /**
     * Delete every persona the screen built and empty mPersonas.
     *
     * Each element is released through slot 1 of its own table at `+0x168`, which is where
     * MetPersonaData places its vptr, with the deleting `__in_chrg` value. The title is inferred.
     *
     * @ghidraAddress 0x0032f4d0
     */
    void ClearPersonas();

    int mUnknown98; // +0x98
    int mUnknown9c; // +0x9c
    // The personas the screen offers. ClearPersonas() deletes every element. +0xa0
    std::vector<MetPersonaData *> mPersonas;
    std::vector<HxStr> mUnknownac; // +0xac
    int mUnknownb8;                // +0xb8, not written by the constructor
    int mUnknownbc;                // +0xbc
    int mUnknownc0;                // +0xc0, starts at -1
    HxStr mUnknownc4;              // +0xc4, constructed from the empty literal
    int mUnknowncc;                // +0xcc, starts at -1
    int mUnknownd0;                // +0xd0, starts at -1
    int mUnknownd4;                // +0xd4
};
