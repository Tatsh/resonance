#pragma once

#include "game/player.h"

/**
 * Stand-in for an absent player.
 *
 * `NullPlayer` in the RTTI descriptor at `0x008ef210`, with `Player` as its only base. Its three
 * vtables are at `0x007d24e0`, `0x007d24b8`, and `0x007d2490`, each walked to its terminator.
 *
 * The primary table has 21 entries, the same as the base, so this class adds no virtual. It
 * replaces one primary slot, IsNull at index 3, and `HandleMessage` in its `MsgSink` table, which
 * it empties so that the stand-in ignores every message. It inherits
 * everything else. Two members against the base's nineteen is what makes this the null-object
 * member of the family, and the base being an interface with inert defaults is what lets it be
 * that small.
 *
 * No destructor is declared here. The routine at `0x001324e8` occupies slot 1 of all three of this
 * class's tables, so it is this class's destructor rather than a stray copy, and it is
 * byte-identical to the base destructor because a trivial derived destructor stores its own table
 * pointer, the inlined base destructor overwrites it, and the dead first store is dropped. The
 * compiler therefore generates it from an implicit declaration and the source owes no definition.
 *
 * The two routines the database titles `Slot3` at `0x00133528` and `0x00133530` are the same index
 * in two different tables, the `MsgSink` one and the primary one, rather than a titling collision.
 */
class NullPlayer : public Player {
public:
    /**
     * Report that this player is a stand-in.
     *
     * Returns 1 where the base returns 0, which is the whole of what makes this the null member of
     * the family.
     *
     * @return Always non-zero.
     * @ghidraAddress 0x00133530
     */
    virtual int IsNull();

    /** @ghidraAddress 0x00133528 */
    virtual void HandleMessage(Message *message);
};
