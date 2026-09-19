#pragma once

#include <vector>

#include "game/freqappearance.h"
#include "memcard/memcarduser.h"
#include "met/metkbuser.h"
#include "met/metremixrecord.h"
#include "met/metremixselection.h"
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
 * two-instruction `jr ra` stubs. Slot 42 is not, and an earlier reading recorded all three as
 * stubs. Its body dispatches straight back through slot 40 of the primary table with no argument,
 * so slot 42 is a public alias for the empty slot 40. The 21-entry MemcardUser table at
 * `0x00809b50` adjusts `this` by `-140` and overrides its slots 2, 5, 8, and 11 at `0x00372c10`,
 * `0x00373808`, `0x00374b58`, and `0x00374208`. The three-entry MetKBUser table at `0x00809b30`
 * adjusts `this` by `-144` and overrides its slot 2 at `0x0037a650`.
 *
 * All four slots the class declares are declared below. Slot 39 takes six arguments in a1 through
 * t2, and the two that could not be typed before are now settled from other bands:
 * MetRemixSelection is the 0x18-byte record whose five fields mirror this class's own `+0x94`
 * through `+0xa8`, and FreqAppearance is the 0x14-byte element of the vector at `+0xac`, RTTI
 * name `14FreqAppearance`. The 0x38-byte element of the vector at `+0xcc` is MetRemixRecord, whose
 * name is inferred.
 *
 * The constructor at `0x00372120` takes the renderer, the load priority, and the three names, and
 * forwards all five to MetScreen. Everything it does after the three vptr writes is member
 * initialisation, in this order. mUnknown94 starts at -1. mUnknown98 is built from the empty
 * string at `0x00809840`, which is what makes the destructor free its buffer. mUnknowna0 and
 * mUnknowna4 start at -1 and mUnknowna8 at zero. Both vectors and the two strings at `+0xb8` and
 * `+0xc0` start empty, mUnknownc8, mUnknownd8, mUnknowndc, and mUnknowne4 start at zero, and
 * mUnknowne0 is never written. An earlier reading described `+0x94` and a nested object reached
 * through `+0x0c` as unresolved; the three words are `+0xa0`, `+0xa4`, and `+0xa8` reached through
 * a register set to `this + 0x94`, and all three are literals.
 *
 * Every one of those stores is a member initialiser, and the MetRemixSelection default
 * constructor produces the -1, empty-string, -1, -1, zero run at `+0x94` exactly, which is
 * independent confirmation of that record's layout.
 *
 * The destructor at `0x00372248` destroys the 0x38-byte elements of mUnknowncc and deallocates its
 * buffer, frees the two strings at `+0xc0` and `+0xb8`, destroys the 0x14-byte elements of
 * mUnknownac and deallocates that buffer, frees mUnknown98, restores the MetKBUser vptr to
 * `0x007f16a0` and the MemcardUser vptr to `0x007daf78`, runs the MetScreen destructor, and
 * releases the object with the tag `MsgSink`. Every one of those steps is compiler-generated
 * member destruction or a vptr restore, so the definition is empty. An earlier reading described
 * the first step as a call to a routine at `0x00183fd0`, which is the element destructor rather
 * than a statement of this destructor.
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
     * Unrecovered. Slot 39.
     *
     * Records the remix a save is about to write. The six parameters are what the register reads
     * prove: a1 is a MetRemixSelection whose five fields are copied into mUnknown94 through the
     * compiler-generated assignment, a2 becomes mUnknownc8, a3 and t0 are assigned to mUnknownb8
     * and mUnknownc0, t1 is assigned to mUnknownac, and t2 becomes mUnknowne4. It also clears
     * mUnknownd8.
     *
     * a3, t0, and t1 are by value rather than by reference, which the tail of the routine proves:
     * it frees the string buffer of each of the first two and destroys every element of the third
     * before returning. a1 is not destroyed there, so it is a reference.
     *
     * The body is not written. After the assignments the routine runs the routine at `0x001f61b8`
     * twice, writes its own MemcardUser subobject pointer into the result, and then runs
     * `0x001f2ae0` on it. Both routines belong to the memcard layer, neither is identified, and the
     * class the first returns is not recovered.
     *
     * @param selection The remix being saved.
     * @param selector The value the MetSaveRemixScreen sound overrides compare against.
     * @param first Assigned to mUnknownb8.
     * @param second Assigned to mUnknownc0.
     * @param appearances Assigned to mUnknownac.
     * @param last Assigned to mUnknowne4.
     * @ghidraAddress 0x00372488
     */
    virtual void OnUnknownSlot39(const MetRemixSelection &selection,
                                 int selector,
                                 HxStr first,
                                 HxStr second,
                                 std::vector<FreqAppearance> appearances,
                                 int last);

    /**
     * Unrecovered. Slot 40, empty.
     *
     * A two-instruction `jr ra` stub. Slot 42 is its one recovered caller.
     *
     * @ghidraAddress 0x0037a618
     */
    virtual void OnUnknownSlot40();

    /**
     * Unrecovered. Slot 41, empty.
     *
     * A two-instruction `jr ra` stub.
     *
     * @ghidraAddress 0x0037a620
     */
    virtual void OnUnknownSlot41();

    /**
     * Unrecovered. Slot 42.
     *
     * Dispatches through slot 40 of the primary table with no argument and does nothing else, so it
     * is an alias for the empty slot 40 rather than a stub of its own. An earlier reading recorded
     * it as a third stub.
     *
     * @ghidraAddress 0x0037a628
     */
    virtual void OnUnknownSlot42();

private:
    MetRemixSelection mUnknown94;           // +0x94
    std::vector<FreqAppearance> mUnknownac; // +0xac
    HxStr mUnknownb8;                       // +0xb8
    HxStr mUnknownc0;                       // +0xc0

protected:
    // The selector the MetSaveRemixScreen sound overrides compare their argument against, which is
    // why it is protected. The declarations below return to private, so that the recovered offset
    // order is preserved.
    int mUnknownc8; // +0xc8

private:
    std::vector<MetRemixRecord> mUnknowncc; // +0xcc
    int mUnknownd8;                         // +0xd8
    int mUnknowndc;                         // +0xdc

protected:
    // Cleared by the MetSaveRemixScreen constructor, which is why it is protected. This class's own
    // constructor never writes it.
    int mUnknowne0; // +0xe0

private:
    int mUnknowne4; // +0xe4
};
