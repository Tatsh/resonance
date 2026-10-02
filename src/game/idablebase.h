#pragma once

/** Identifier a never-registered object stores, distinct from any real index. */
constexpr int kIDableUnregistered = -2;

/**
 * Base of an object that occupies a slot of a global identifier table.
 *
 * `IDableBase` in the RTTI descriptor at `0x008ef200`, a leaf with no base list. The object is 8
 * bytes: mId at `+0x00` and, following the g++ 2.x layout for a class with no base, the vptr at
 * `+0x04`. Only the destructor is declared beyond the compiler-generated slot 0, so the table has
 * two entries.
 *
 * The class itself does no registering. `IDable` supplies the table and clears the slot, which is
 * why this base has an identifier and no behaviour to go with it.
 */
class IDableBase {
    // IDablePtr's constructor from an object reads mId inline, and no accessor exists in the
    // image. PhraseMgr::SetPhraseOwner() at 0x001bafa8 is the recovered expansion.
    template <typename T>
    friend class IDablePtr;

public:
    /** @ghidraAddress 0x00121c68 */
    virtual ~IDableBase();

protected:
    int mId; // +0x00
};
