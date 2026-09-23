#pragma once

#include "app/msgsink.h"
#include "app/msgsource.h"

/**
 * Translator from controller readings to the players of one game world.
 *
 * `8InputMap` in the RTTI descriptor, with MsgSource at offset 0 and MsgSink at `+0x14`. Its two
 * tables are at `0x007cf6c0` and `0x007cf698`, the second adjusting `this` by `-20`. GrooveWorld
 * creates the one instance with a 0x60-byte allocation and the constructor at `0x00119160`, which
 * receives the application and the address of the world's player vector.
 *
 * The class is declared for the two routines GrooveWorld reaches. Both walk the `std::list` at
 * `+0x1c` and write one word of every entry, 1 to enable and 0 to disable. The member map is not
 * recovered, so no member is declared, and neither body is written.
 */
class InputMap : public MsgSource, public MsgSink {
public:
    /**
     * Clear the enable word of every entry.
     *
     * @ghidraAddress 0x0011db78
     */
    void DisableEntries();

    /**
     * Set the enable word of every entry.
     *
     * @ghidraAddress 0x0011dbc0
     */
    void EnableEntries();
};
