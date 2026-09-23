#pragma once

#include <vector>

#include "met/metremixsaver.h"
#include "met/metscreen.h"
#include "rnd/object.h"

/**
 * Dialogue that writes a multiplayer remix to a memory card.
 *
 * `23MetMultiSaveRemixScreen` in the RTTI descriptor at `0x008ef530`, with two public non-virtual
 * bases at fixed offsets, MetScreen at `+0x00` and MetRemixSaver at `+140`. The 39-entry primary
 * vtable is at `0x007ffb90`, the same length as the MetScreen table, so the class declares no
 * virtual of its own, and the five-entry MetRemixSaver table at `0x007ffb60` adjusts `this` by
 * `-140` in every entry. That table is where this screen supplies the three MetRemixSaver pure
 * virtuals.
 *
 * The constructor at `0x002fa0b0` takes only the renderer and the load priority, and supplies
 * `dlg` for the screen name, `metagame/Shared` for the directory, and `dialogue` for the
 * container, the same container MetGlobalSettingsSaverScreen and MetRemixManager load.
 *
 * The constructor default-constructs the four vectors from `+0x90` through `+0xbf`, zeroes
 * mSaveCount, and default-constructs a fifth vector at `+0xd8`. It writes nothing from `+0xc0`
 * through `+0xd3` and nothing at `+0xe4` or `+0xe8`, which EnterAndShow() and slot 36 fill. The
 * object is 0xec bytes, the size New() allocates.
 *
 * The destructor at `0x002fa280` tears down the fifth vector first and then the other four in
 * reverse declaration order, restores the MetRemixSaver vptr to `0x007ffcd0`, runs the MetScreen
 * destructor, and releases the object with the tag `MsgSink`. Every part of that teardown is
 * compiler-generated member destruction, so the destructor body is empty.
 *
 * The screen saves the remix once for each player whose memory card is ready, one after another.
 * EnterAndShow() records which players have a formatted card, slot 36 opens MetSaveRemixScreen for
 * the first, and slot 2, which MetSaveRemixScreen runs when a save ends, opens it for the next.
 * After the last save the screen returns to the remix type screen.
 *
 * Eight slots differ from the MetScreen table. Slots 21 through 24 sit eight bytes apart at
 * `0x002feda0` through `0x002fedb8`, and the declaration order there puts the cycle sounds ahead
 * of high and leave, so the addresses do not ascend with the slot numbers. The others are 5
 * `0x002fa4e8` and 36 `0x002fa7c0`, and the MetRemixSaver table supplies slots 2 `0x002facc0`, 3
 * `0x002fee48`, and 4 `0x002fb248`.
 */
class MetMultiSaveRemixScreen : public MetScreen, public MetRemixSaver {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x002fa0b0
     */
    MetMultiSaveRemixScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x002fa280
     */
    virtual ~MetMultiSaveRemixScreen();

    /**
     * Build the screen on the heap.
     *
     * @param pRenderer The front-end renderer the screen registers on.
     * @param nPriority The load priority.
     * @return The new screen.
     * @ghidraAddress 0x002fedc0
     */
    static MetMultiSaveRemixScreen *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Hide the screen, push the multiplayer end-of-remix screen, and record which players have a
     * formatted memory card.
     *
     * Slot 5. A card at a port-0 slot below the player count marks that player, and a card at the
     * first slot of port 1 marks player 1. When no card is ready the end-of-remix screen is exited
     * again. The screen then begins its exit.
     *
     * @ghidraAddress 0x002fa4e8
     */
    virtual void EnterAndShow();

    /**
     * Start the first save once the exit finishes.
     *
     * Slot 36. With no ready card it goes straight to ReturnToRemixType(). Otherwise it records the
     * ready players in mReadyPlayers, gathers every player's appearance, and opens
     * MetSaveRemixScreen for the first ready player on the first card slot, clearing the entered
     * name, then resets mSaveIndex and mUnknowne8.
     *
     * @ghidraAddress 0x002fa7c0
     */
    virtual void OnUnknownSlot36();

    /**
     * Go on to the next save, or finish.
     *
     * Slot 2 of the MetRemixSaver table. After the last save the end-of-remix screen is exited
     * unless mUnknowne8 is set, and ReturnToRemixType() runs. Otherwise the end-of-remix screen is
     * pushed again when mUnknowne8 is set, and MetSaveRemixScreen opens for the next ready player
     * on the card slot the save index selects, keeping the entered name.
     *
     * @param nUnknown Not read.
     * @ghidraAddress 0x002facc0
     */
    virtual void OnUnknownSlot2(int nUnknown);

    /**
     * Set mUnknowne8 and exit the end-of-remix screen.
     *
     * Slot 3 of the MetRemixSaver table.
     *
     * @ghidraAddress 0x002fee48
     */
    virtual void OnUnknownSlot3();

    /**
     * Store the negation of a flag in mUnknowne8, and push or exit the end-of-remix screen on it.
     *
     * Slot 4 of the MetRemixSaver table.
     *
     * @param nFlag Non-zero to push the end-of-remix screen.
     * @ghidraAddress 0x002fb248
     */
    virtual void OnUnknownSlot4(int nFlag);

    /**
     * Silence the leave sound.
     *
     * All four overrides are two-instruction stubs, so each was written inline with an empty body.
     *
     * @ghidraAddress 0x002fedb8
     */
    virtual void PlayLeaveSound(int) {
    }

    /**
     * Silence the high sound.
     *
     * @ghidraAddress 0x002fedb0
     */
    virtual void PlayHighSound(int) {
    }

    /**
     * Silence the cycle-left sound.
     *
     * @ghidraAddress 0x002feda0
     */
    virtual void PlayCycleLeftSound(int) {
    }

    /**
     * Silence the cycle-right sound.
     *
     * @ghidraAddress 0x002feda8
     */
    virtual void PlayCycleRightSound(int) {
    }

private:
    // 0x002fb350
    // Resolves the arena view, runs the renderer's two hooks, and pushes and activates
    // `MetRemixTypeScreen`. Slots 2 and 36 are its callers, and the title is inferred.
    void ReturnToRemixType();

    // The players MetFrontEndState holds, at most this many.
    static constexpr int kMaxPlayers = 4;

    std::vector<Rnd::Object *> mUnknown90; // +0x90
    std::vector<Rnd::Object *> mUnknown9c; // +0x9c
    std::vector<Rnd::Object *> mUnknowna8; // +0xa8
    std::vector<Rnd::Object *> mUnknownb4; // +0xb4
    // The number of players, from MetFrontEndState. +0xc0
    int mPlayerCount;
    // 1 for each player whose memory card is formatted. +0xc4
    int mCardReady[kMaxPlayers];
    // The number of ready cards, which the constructor zeroes. +0xd4
    int mSaveCount;
    // The ready players, in order. +0xd8
    std::vector<int> mReadyPlayers;
    // The index into mReadyPlayers of the save in progress. +0xe4
    int mSaveIndex;
    // Set by slot 3 and by the negation slot 4 stores, and read by slot 2. +0xe8
    int mUnknowne8;
};
