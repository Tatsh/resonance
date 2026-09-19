#pragma once

#include "memcard/memcarduser.h"
#include "met/metkbuser.h"
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
 * None of the four slots the class declares is itself declared below, and the reason is worth
 * recording, because omitting them makes this declaration record a shorter table than the program
 * has. Slot 39 at `0x00372488` takes six arguments in a1 through t2, and two of them cannot be
 * typed. The first is a pointer to a 0x18-byte record whose five fields mirror this class's own
 * `+0x94` through `+0xa8`, and the fifth is a vector of the same 0x14-byte elements as mUnknownac.
 * Neither element class emits RTTI, and no allocation tag identifies either, so declaring slot 39
 * would require inventing two types. Declaring slots 40 through 42 without it would shift their
 * indices. The four are therefore recorded here and declared nowhere.
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
 * The constructor body is not written. Both vectors are recorded as reserved spans, because
 * neither element class is identified, and a reserved span cannot reproduce the zeroing that a
 * vector member's own constructor performs.
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
     * The body is not written, for the reason recorded in the class documentation.
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

private:
    int mUnknown94;   // +0x94, starts at -1
    HxStr mUnknown98; // +0x98, starts as a copy of the empty string
    int mUnknowna0;   // +0xa0, starts at -1
    int mUnknowna4;   // +0xa4, starts at -1
    int mUnknowna8;   // +0xa8
    // A vector of 0x14-byte elements by value. Each element is a class with no base, so its vptr
    // sits after its data members at `+0x10`, and slot 1 of its table is its destructor, which the
    // destructor of this class runs on every element with an `__in_chrg` of 2. The class is not
    // identified.
    unsigned char mUnknownac[0x0c]; // +0xac
    HxStr mUnknownb8;               // +0xb8
    HxStr mUnknownc0;               // +0xc0

protected:
    // The selector the MetSaveRemixScreen sound overrides compare their argument against, which is
    // why it is protected. The declarations below return to private, so that the recovered offset
    // order is preserved.
    int mUnknownc8; // +0xc8

private:
    // A vector of 0x38-byte records by value, destroyed through the routine at `0x00183fd0`. The
    // same record appears at `+0x114` of MetRemixManager. Four `HxStr` members sit at its `+0x00`,
    // `+0x08`, `+0x10`, and `+0x18`, a byte at `+0x20`, a word at `+0x24`, a vector of the same
    // 0x14-byte elements as mUnknownac at `+0x28`, and a word at `+0x34`. The record class is not
    // identified.
    unsigned char mUnknowncc[0x0c]; // +0xcc
    int mUnknownd8;                 // +0xd8
    int mUnknowndc;                 // +0xdc

protected:
    // Cleared by the MetSaveRemixScreen constructor, which is why it is protected. This class's own
    // constructor never writes it.
    int mUnknowne0; // +0xe0

private:
    int mUnknowne4; // +0xe4
};
