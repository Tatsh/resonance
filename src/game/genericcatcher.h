#pragma once

#include "app/msgsink.h"
#include "app/msgsource.h"
#include "msg/message.h"

/**
 * Interface every gem catcher presents.
 *
 * Its RTTI descriptor is at `0x008ef7c0`. It is built over MsgSink at offset 0 and MsgSource at
 * offset 4. Its primary table is at `0x007e0c98` with seven entries and its MsgSource subobject
 * table at `0x007e1098` with four and a `-4` adjustment on every entry.
 *
 * The class is abstract twice over. Slot 3, MsgSink::HandleMessage(), still addresses the shared
 * pure-virtual stub at `0x005381a8`, and so does slot 6, the first virtual this class introduces
 * that it does not default. Catcher supplies both.
 *
 * The constructor writes the two table pointers and nothing else, so the class declares no data
 * member. The destructor's whole body is the implicit teardown of the MsgSource base's vector and
 * the tagged release behind the deleting flag, which is why the definition below is empty.
 *
 * No literal attests a verb for slots 4, 5, and 6. Their titles follow what Catcher's overrides
 * do. Slots 4 and 5 have empty default bodies here and Catcher overrides all three.
 */
class GenericCatcher : public MsgSink, public MsgSource {
public:
    /**
     * @ghidraAddress NTSC-U/C: 0x001b0a98
     * @ghidraAddress PAL: 0x001b6848
     */
    GenericCatcher();

    /**
     * @ghidraAddress NTSC-U/C: 0x001b0b88
     * @ghidraAddress PAL: 0x001b6938
     */
    virtual ~GenericCatcher();

    /**
     * Start catching. Slot 4. The default body is empty and Catcher overrides it with a body that
     * schedules two commands on the tick clock.
     *
     * @ghidraAddress NTSC-U/C: 0x001b0c58
     * @ghidraAddress PAL: 0x001b6a08
     */
    virtual void Start();

    /**
     * Stop catching. Slot 5. The default body is empty and Catcher overrides it with a body that
     * withdraws the two scheduled commands.
     *
     * @ghidraAddress NTSC-U/C: 0x001b0c60
     * @ghidraAddress PAL: 0x001b6a10
     */
    virtual void Stop();

    /**
     * Report whether no phrase run is in progress. Slot 6, pure. Catcher reports whether
     * Catcher::mPhraseRunBars is zero.
     *
     * @return Non-zero when the catcher has nothing outstanding.
     */
    virtual int IsPhraseRunEmpty() = 0;
};
