#pragma once

#include "game/rawcontroller.h"

/**
 * Owner of the front-end world, outside a game session.
 *
 * `13MetaGameWorld` in the RTTI descriptor at `0x00901c90`, with RawController as its one public
 * base at offset 0. The object is 0xc bytes, which the allocation in GameManagerImpl::Start()
 * fixes, so the inherited vptr at `+0x00` is followed by two members of its own. Its vtable at
 * `0x00810f58` has three entries and a zero terminator at index 3, the type function at
 * `0x003d45e8`, the destructor at `0x003d4790`, and the RawController override below.
 *
 * Recovery of the bodies has barely started, and neither member below is defined. Each needs the
 * two owned objects declared, and neither of their classes is recovered.
 *
 * The constructor stores a null in `+0x04`, installs an 8-byte object in `+0x08` through
 * `0x001daaf8` and then overwrites that object's table pointer with `0x00810ee8`, and runs
 * `0x003d31c0` on itself. The destructor releases the object at `+0x08` through its own table and
 * then runs `0x003d4810`, which releases the object at `+0x04` and clears the field.
 *
 * Two further members are recorded by address rather than declared. `0x003d4858` returns the
 * pointer at `+0x04`, and `0x003d4890` runs slot 5 of that object. GameManagerImpl drives the
 * second one from three of its message handlers. The first is a single load, which the brief's own
 * rule places outside what a reconstruction may declare, and it currently carries the title
 * `RndMemStream__EofCopy1` in the program because a two-instruction accessor is byte-identical
 * across every class with a pointer at that offset.
 */
class MetaGameWorld : public RawController {
public:
    /**
     * @ghidraAddress 0x003d3110
     */
    MetaGameWorld();

    /**
     * @ghidraAddress 0x003d4790
     */
    virtual ~MetaGameWorld();

    /**
     * Report a controller reading. Slot 2.
     *
     * The body runs slot 2 of the object at `+0x08`, builds a RawControllerMsg on the stack whose
     * payload is the three words followed by the float in parameter order, and hands it to slot 2
     * of the object at `+0x04`. The temporary is destroyed on the way out, which the inlined
     * Message destructor at the end of the body records.
     *
     * @param nUnknown1 The first word of the reading.
     * @param nUnknown2 The second word of the reading.
     * @param nUnknown3 The third word of the reading.
     * @param flUnknown4 The float of the reading.
     * @ghidraAddress 0x003d3288
     */
    virtual void OnUnknownSlot2(int nUnknown1, int nUnknown2, int nUnknown3, float flUnknown4);
};
