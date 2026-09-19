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
 * The object at `+0x08` is an InputCheatDetectorMet. Walking the table the constructor installs
 * settles it: that table has four entries with a zero terminator at index 4, and its slot 0 guards
 * on the descriptor at `0x008efcb0`, built from the mangled name `21InputCheatDetectorMet` at
 * `0x00811010` over the InputCheatDetector descriptor at `0x008f2a40`. Slot 2 is inherited at
 * `0x001dc658` and slot 3 is its own at `0x003d4788`. InputCheatDetector derives from
 * RawController, which places its slot 2 at the same index as this class's own, and the cheat
 * detector family shares the InputPollerPS2.cpp string pool with InputPoller. The member is not
 * declared, because no header for that class exists yet and no body here is unblocked without the
 * type of `+0x04` as well.
 *
 * The object at `+0x04` is polymorphic and its class is unrecovered. No writer of the field is
 * recovered, so no table address is available to walk; the constructor only clears it and
 * `0x003d4810` only releases and clears it. Its table has at least six entries, because
 * `0x003d4890` dispatches slot 5 of it.
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
     * The body forwards all four arguments unchanged to slot 2 of the InputCheatDetectorMet at
     * `+0x08`, which is that class's own RawController slot, so a controller reading reaches the
     * cheat detector before the world handles it. No argument register is reloaded ahead of that
     * dispatch, which is what establishes the forwarding. It then builds a RawControllerMsg on the
     * stack whose
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
