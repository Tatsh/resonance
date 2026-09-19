#pragma once

#include <vector>

#include "met/metmemcardpickeruser.h"
#include "met/metmemdetectscreen.h"
#include "os/hxstr.h"
#include "rnd/object.h"

/**
 * One memory card the picker lists.
 *
 * The record is 24 bytes, which the stride of the destructor's teardown walk over mCards pins. Its
 * teardown releases exactly one buffer, at `+0x08`, which is the inlined HxStr destructor over an
 * HxStr at `+0x04`. The remaining twelve bytes are never touched by any routine of
 * MetMemCardLoadScreen, so they are recorded as a reserved span. The record emits no RTTI
 * descriptor and no literal identifies it, so the name here is inferred from its role.
 */
struct MetMemCardEntry {
    int mUnknown00;   /*!< +0x00 */
    HxStr mUnknown04; /*!< The one member the teardown releases. +0x04 */
    /** Never written by a recovered routine, and not recovered. +0x0c */
    unsigned char mUnknown0c[0xc];
};

/**
 * Screen that picks a memory card to load from.
 *
 * `20MetMemCardLoadScreen` in the RTTI descriptor at `0x00901f20`, with two public non-virtual
 * bases at fixed offsets, MetMemDetectScreen at `+0x00` and MetMemCardPickerUser at `+160`. The
 * object is at least 0xf0 bytes.
 *
 * The class emits **two** vtables, the 44-entry primary at `0x007fbde8` and the 21-entry
 * MemcardUser table at `0x007fbd38` that adjusts `this` by `-140`. It emits none for
 * MetMemCardPickerUser, and its constructor writes vptrs at `+0x00` and `+0x8c` and nothing at
 * `+0xa0`, which is a further confirmation that MetMemCardPickerUser declares no virtual function.
 *
 * The primary table is the same length as the MetMemDetectScreen table, so this class declares
 * **no** virtual of its own. Fifteen inherited slots differ, which makes it the most heavily
 * overriding class in the subsystem without extending the interface at all.
 *
 * The constructor at `0x002cb8c8` takes only the renderer and the load priority. It runs the
 * MetMemDetectScreen constructor with `mcl` for the screen name, `metagame/Shared` for the
 * directory, and `memcard_load` for the container, and pushes `mcl_card` into the container
 * object-name vector that MetScreen owns.
 *
 * The destructor at `0x002cbcc8` restores both vptrs, tears down mCards element by element and
 * then the two Rnd::Object vectors, runs the MetMemDetectScreen destructor, and releases the
 * object with the tag `MsgSink`. Every part of that teardown is compiler-generated member
 * destruction, so no destructor body is reconstructed.
 *
 * A diff of the primary table against the MetMemDetectScreen table at `0x007fccb8` reads the
 * fifteen overrides as slots 1, 5, 19, 20, 22, 23, 24, 30, 33, 36, 38, 39, 41, and 42 apart from
 * the type function. Slots 22 through 24 at `0x002d1d80`, `0x002d1eb0`, and `0x002d1ef8` are real
 * bodies rather than stubs.
 *
 * Four addresses that the memory-card band worklist assigned to this class are not in either of
 * its tables. `0x002d9e40`, `0x002dec10`, and `0x002db328` are MetMemDetectScreen primary slots 15,
 * 26, and 40, and `0x002deb98` is MetMemDetectScreen MemcardUser slot 13. All four were titled
 * `MetMemCardLoadScreen__Slot*` in the program and are now titled for MetMemDetectScreen, which
 * the same diff establishes.
 *
 * Slots 39, 41, and 42 override virtuals MetMemDetectScreen declares of its own and whose verbs are
 * unrecovered. That class records them rather than declaring them, so this class cannot declare an
 * override of any of the three, and all three are recorded here instead, at `0x002ce2c0`,
 * `0x002cd4e0`, and `0x002cdce0`. The base entry for slot 41 and the base entry for slot 42 are
 * both two-instruction stubs, and both overrides here do real work, so both are genuine overrides
 * rather than re-emitted empty bodies.
 *
 * Six bodies are not written. Every one of EnterAndShow(), HandleCommand(), OnUnknownSlot30(),
 * OnUnknownSlot33(), OnUnknownSlot36(), and ResolveContainerViews() is declared below with its
 * address, and its name comes from the base declaration through the table diff rather than from the
 * body.
 */
class MetMemCardLoadScreen : public MetMemDetectScreen, public MetMemCardPickerUser {
public:
    /**
     * Construct the screen.
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
     * Populate the card list and enter.
     *
     * Slot 5. The body is not written.
     *
     * @ghidraAddress 0x002cc9c0
     */
    virtual void EnterAndShow();

    /**
     * Act on one navigation command.
     *
     * Slot 19. The body is not written.
     *
     * @param pCommand The command the renderer translated from an input message.
     * @ghidraAddress 0x002cc328
     */
    virtual void HandleCommand(const MetScreenCommand *pCommand);

    /**
     * Respond to an alternation finishing.
     *
     * Slot 30. The body is not written.
     *
     * @param pObject The object slot 29 finished alternating.
     * @ghidraAddress 0x002ccfb8
     */
    virtual void OnUnknownSlot30(Rnd::Object *pObject);

    /**
     * Respond to the enter animation finishing.
     *
     * Slot 33. The body is not written.
     *
     * @ghidraAddress 0x002d2018
     */
    virtual void OnUnknownSlot33();

    /**
     * Respond to the exit animation finishing.
     *
     * Slot 36. The body is not written.
     *
     * @ghidraAddress 0x002cd0e8
     */
    virtual void OnUnknownSlot36();

    /**
     * Resolve the container views.
     *
     * Slot 38. The body is not written.
     *
     * @ghidraAddress 0x002cbeb8
     */
    virtual void ResolveContainerViews();

    /**
     * Play the slide sound while at least one card is listed.
     *
     * The override tests neither the selector nor any recorded selector of its own. It forwards to
     * MetScreen with the same selector whenever mCards is not empty.
     *
     * @param nSelector Passed through to MetScreen unchanged.
     * @ghidraAddress 0x002d1f40
     */
    virtual void PlaySlideSound(int nSelector);

    /**
     * Silence the high sound.
     *
     * A two-instruction stub, so it was written inline with an empty body.
     *
     * @ghidraAddress 0x002d1d80
     */
    virtual void PlayHighSound(int) {
    }

    /**
     * Play the cycle-left sound while more than one card is listed.
     *
     * @param nSelector Passed through to MetScreen unchanged.
     * @ghidraAddress 0x002d1eb0
     */
    virtual void PlayCycleLeftSound(int nSelector);

    /**
     * Play the cycle-right sound under the same condition as PlayCycleLeftSound().
     *
     * @param nSelector Passed through to MetScreen unchanged.
     * @ghidraAddress 0x002d1ef8
     */
    virtual void PlayCycleRightSound(int nSelector);

private:
    int mUnknowna4; // +0xa4
    int mUnknowna8; // +0xa8
    // Never written by the constructor and not recovered.
    unsigned char mUnknownac[0xc];         // +0xac
    std::vector<Rnd::Object *> mUnknownb8; // +0xb8
    std::vector<Rnd::Object *> mUnknownc4; // +0xc4
    int mUnknownd0;                        // +0xd0
    int mUnknownd4;                        // +0xd4
    // The listed cards. All three sound overrides read its size. +0xd8
    std::vector<MetMemCardEntry> mCards;
    int mUnknowne4; // +0xe4
};
