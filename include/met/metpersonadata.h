#pragma once

#include <cstddef>

#include "os/hxstr.h"

/**
 * Saved record of one player persona.
 *
 * `14MetPersonaData` in the RTTI descriptor at `0x0086f610`, a leaf class with no base. Following
 * the g++ 2.x layout for a class with no base, the vptr sits after the data members, here at
 * `+0x168`, so the object is 0x16c bytes. The four-entry vtable is at `0x00804bd8`.
 *
 * The class supplies both its own allocation and its own release function, and both tag the block
 * with the literal `MetPersonaData` at `0x00804a78`.
 *
 * Two of its members are objects with virtuals of their own. The first sits at offset 0, is 0x140
 * bytes, stores its vptr at `+0x13c`, and is built by `0x00140580` and torn down by `0x00140768`.
 * The second sits at `+0x140`, is 0x14 bytes, stores its vptr at `+0x10`, and is built by
 * `0x001745b8` and torn down by `0x00174730`. Neither class is identified, so both are recorded
 * here as a reserved span rather than typed.
 *
 * Vtable slots 2 and 3, at `0x0032b880` and `0x0032b968`, are a matched pair. Slot 2 hands the
 * constant 4-byte value 2 to its argument through the argument's vtable slot 4 and then forwards
 * the same argument to slot 2 of both member objects. Slot 3 takes a 4-byte value back through
 * slot 6 and forwards to slot 3 of both members. The pair is therefore the persist-and-restore
 * pair with a record version of 2, but the argument's class is not identified and neither virtual
 * has a recovered name, so the two are recorded rather than declared.
 */
class MetPersonaData {
public:
    /**
     * Allocate an instance from the tagged heap.
     *
     * @param nSize The object size, which the compiler supplies.
     * @return The block.
     * @ghidraAddress 0x0032e1e8
     */
    void *operator new(size_t nSize);

    /**
     * Release an instance to the tagged heap.
     *
     * @param pBlock The block.
     * @ghidraAddress 0x0032e208
     */
    void operator delete(void *pBlock);

    /**
     * Construct an empty record.
     *
     * @ghidraAddress 0x0032b760
     */
    MetPersonaData();

    /**
     * @ghidraAddress 0x0032e278
     */
    virtual ~MetPersonaData();

private:
    // The 0x140-byte member object whose vptr sits at +0x13c. Its class is not identified.
    unsigned char mUnknown00[0x140]; // +0x00
    // The 0x14-byte member object whose vptr sits at +0x10. Its class is not identified.
    unsigned char mUnknown140[0x14]; // +0x140
    HxStr mUnknown154;               // +0x154
    int mUnknown15c;                 // +0x15c
    HxStr mUnknown160;               // +0x160
};
