#pragma once

#include "app/msgsink.h"
#include "app/msgsource.h"
#include "msg/message.h"

/**
 * Interface every gem catcher presents.
 *
 * `14GenericCatcher` in the RTTI descriptor at `0x008ef7c0`, over MsgSink at offset 0 and
 * MsgSource at offset 4. Its primary table is at `0x007e0c98` with seven entries and its MsgSource
 * subobject table at `0x007e1098` with four and a `-4` adjustment on every entry.
 *
 * The class is abstract twice over. Slot 3, MsgSink::HandleMessage(), still addresses the shared
 * pure-virtual stub at `0x005381a8`, and so does slot 6, the first virtual this class introduces
 * that it does not default. Catcher supplies both.
 *
 * The constructor writes the two table pointers and nothing else, so the class declares no data
 * member. The destructor's whole body is the implicit teardown of the MsgSource base's vector and
 * the tagged release behind the deleting flag, which is why the definition below is empty.
 *
 * Slots 4, 5, and 6 retain their table indices as their titles, because the index is part of the
 * layout and no literal attests a verb for any of the three. Slots 4 and 5 have empty default
 * bodies here and Catcher overrides all three.
 */
class GenericCatcher : public MsgSink, public MsgSource {
public:
    /**
     * @ghidraAddress 0x001b0a98
     */
    GenericCatcher();

    /**
     * @ghidraAddress 0x001b0b88
     */
    virtual ~GenericCatcher();

    /**
     * Slot 4. The default body is empty and Catcher overrides it with a body that schedules two
     * commands on the tick clock.
     *
     * @ghidraAddress 0x001b0c58
     */
    virtual void Slot4();

    /**
     * Slot 5. The default body is empty and Catcher overrides it with a body that withdraws the
     * two scheduled commands.
     *
     * @ghidraAddress 0x001b0c60
     */
    virtual void Slot5();

    /**
     * Slot 6, pure. Catcher reports whether its counter at `+0x60` is zero.
     *
     * @return Non-zero when the catcher has nothing outstanding.
     */
    virtual int Slot6() = 0;
};
