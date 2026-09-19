#pragma once

#include "met/metremixsaver.h"
#include "met/metscreen.h"
#include "met/texturepairrecord.h"

/**
 * End-of-remix screen for a solo session.
 *
 * `21MetSoloEndRemixScreen` in the RTTI descriptor at `0x008f07f0`, with two public non-virtual
 * bases at fixed offsets, MetScreen at `+0x00` and MetRemixSaver at `+140`. The object is 0x11c
 * bytes, which the factory at `0x00399630` confirms by requesting exactly that many with the tag
 * `MsgSink`. The 39-entry primary vtable is at `0x0080c4f0`, the same length as the MetScreen
 * table, so the class declares no virtual of its own, and the five-entry MetRemixSaver table at
 * `0x0080c4c0` adjusts `this` by `-140` in every entry. That table is where this screen supplies
 * the three MetRemixSaver pure virtuals.
 *
 * The constructor at `0x003943f8` takes only the renderer and the load priority, and supplies
 * `erss` for the screen name, `metagame/_Solo` for the directory, and `end_remix` for the
 * container. It builds the two TexturePairRecord members from the texture pairs `gSongLogo1.tex`
 * with `gSongLogo2.tex` and `gSongLabel1.tex` with `gSongLabel2.tex`.
 *
 * The destructor at `0x003996b8` restores both vptrs, runs the TexturePairRecord destructor at
 * `0x001fc568` on each of the two records, restores the MetRemixSaver vptr to `0x007ffcd0`, runs
 * the MetScreen destructor, and releases the object with the tag `MsgSink`. Every step is
 * compiler-generated member destruction or a vptr restore, so the definition is empty.
 *
 * Seven entries of the primary table differ from the MetScreen table, which a diff of the two
 * tables settles rather than the title each routine carries. They are 0 `0x003995b0`, the
 * compiler-generated GetTypeInfo, 1 `0x003996b8` the destructor, 5 `0x00394e10`, 7 `0x003997d0`,
 * 26 `0x00399748`, 36 `0x00399870`, and 38 `0x00394728`. All five behaviour slots are declared
 * below. An earlier reading counted five entries by omitting the type function and the destructor.
 *
 * The three MetRemixSaver pure virtuals are supplied at `0x003998c8`, `0x00399898`, and
 * `0x00395918`, and all three are declared below with the spelling the base gives them.
 *
 * Slots 2, 3, 4, 7, and 36 all route through one private helper or one of the two navigation
 * virtuals, and mUnknownb8 is the flag that selects between them. Slot 3 sets it, slot 4 stores the
 * negation of its argument in it, and slots 2 and 36 branch on it.
 */
class MetSoloEndRemixScreen : public MetScreen, public MetRemixSaver {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x003943f8
     */
    MetSoloEndRemixScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x003996b8
     */
    virtual ~MetSoloEndRemixScreen();

    /**
     * Show the screen and start its enter animation. Slot 5.
     *
     * The body is not written. It runs for roughly 0x2c0 instructions, resolving the song title and
     * the score fields of the finished session and writing them into the container's Rnd::Text
     * objects, and it reads the two TexturePairRecord members. The routines it drives belong to the
     * game-session and data-array layers and none of them is identified.
     *
     * @ghidraAddress 0x00394e10
     */
    virtual void EnterAndShow();

    /**
     * Make the save screen the active panel. Slot 7.
     *
     * Activates the panel registered under `MetSaveRemixScreen` and does nothing else. MetScreen
     * slot 6 is its one caller.
     *
     * @ghidraAddress 0x003997d0
     */
    virtual void OnUnknownSlot7();

    /**
     * Advance both texture pairs and hand each one's current texture to its target. Slot 26.
     *
     * The body is not written. Each of the two records is advanced through the
     * TexturePairRecord method at `0x00249850` and then read through the one at `0x00249808`, and
     * the texture that read returns is handed to the routine at `0x004dd0a0` along with the field
     * at `+0x1c` of the object the screen stores at `+0xb0` for the first record and at `+0xb4` for
     * the second. Neither TexturePairRecord method is declared, `0x004dd0a0` sits in the
     * Rnd::MatAnim region and is not identified, and the class of the two stored objects is not
     * recovered. The float argument the slot receives is not read.
     *
     * @param flTime The current renderer time, which the body does not read.
     * @ghidraAddress 0x00399748
     */
    virtual void OnUnknownSlot26(float flTime);

    /**
     * Run the departure sequence once, unless a save is still pending. Slot 36.
     *
     * MetScreen slot 35 runs this slot once the exit animation has finished. A set mUnknownb8
     * records that slot 3 or slot 4 has already requested the departure, and the sequence then does
     * not run a second time.
     *
     * @ghidraAddress 0x00399870
     */
    virtual void OnUnknownSlot36();

    /**
     * Resolve the container objects this screen drives. Slot 38.
     *
     * The body is not written. It runs the MetScreen slot 38 body first and then resolves several
     * container objects by name through Rnd::Manager::Find(), narrowing each with dynamic_cast, and
     * fills the unrecovered span at `+0x90`.
     *
     * @ghidraAddress 0x00394728
     */
    virtual void ResolveContainerViews();

    /**
     * Depart the screen, or run the departure sequence when one is already pending.
     *
     * MetRemixSaver slot 2. A clear mUnknownb8 starts the exit animation and lets slot 36 run the
     * sequence once the animation has finished. A set mUnknownb8 runs the sequence at once.
     *
     * @ghidraAddress 0x003998c8
     */
    virtual void OnUnknownSlot2();

    /**
     * Record that the departure sequence has been requested and start the exit animation.
     *
     * MetRemixSaver slot 3.
     *
     * @ghidraAddress 0x00399898
     */
    virtual void OnUnknownSlot3();

    /**
     * Show or exit this screen and record which of the two happened.
     *
     * MetRemixSaver slot 4. The member takes the negation of the argument, which is faithful only
     * for an argument of 0 or 1, because the binary computes it with `xori` against 1 rather than
     * with a comparison.
     *
     * @param nFlag Non-zero to bring the screen back onto the stack, zero to exit it.
     * @ghidraAddress 0x00395918
     */
    virtual void OnUnknownSlot4(int nFlag);

private:
    // 0x00395a20. Hands the session result to the renderer and then returns to the title screen.
    // Slots 2 and 36 are its two callers. The body is not written: it runs the MetRenderer routine
    // at 0x0036a9e0 with a second argument of zero and then the two empty MetRenderer routines at
    // 0x00390088 and 0x00390090, none of which is identified, and only then pushes
    // `MetHelpScreen`, `MetScreenTitleScreen`, and `MetRemixTypeScreen` and activates
    // `MetRemixTypeScreen` as the panel.
    void ReturnToTitle();

    // +0x90 through +0xb7 are not recovered. Two of the words in the span are objects that slot 26
    // reaches through, at +0xb0 for the first texture pair and +0xb4 for the second, and neither
    // class is identified. The constructor writes none of the span.
    unsigned char mUnknown90[0x28]; // +0x90
    // Set once the departure has been requested. Slot 3 sets it, slot 4 stores the negation of its
    // argument in it, and slots 2 and 36 branch on it. The constructor never writes it, so a screen
    // whose slot 36 runs before either setter reads an indeterminate value.
    int mUnknownb8; // +0xb8
    // The two texture pairs the screen flips between, built from the song logo and the song label
    // pairs in that order.
    TexturePairRecord mUnknownbc; // +0xbc
    TexturePairRecord mUnknownec; // +0xec
};
