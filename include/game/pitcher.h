#pragma once

#include "app/msgsink.h"
#include "app/msgsource.h"
#include "app/ticktask.h"
#include "msg/message.h"

/**
 * Base of the stages' pitch producers.
 *
 * `7Pitcher` in the RTTI descriptor at `0x008f1420`, over MsgSink at offset 0, MsgSource at offset
 * 4, and TickTask at offset 24. Its primary table is at `0x007e1520` with four entries, its
 * MsgSource subobject table at `0x007e14f8`, and its TickTask subobject table at `0x007e14c8` with
 * five entries and a `-24` adjustment. Slot 3 of the primary table still addresses the shared
 * pure-virtual stub at `0x005381a8`, so the class is abstract twice over: TickTask::Tick() is pure
 * as well, and each subclass supplies both.
 *
 * Three classes derive from it, NotePitcher, Scratcher, and Voxer, and each writes all three of
 * this class's tables in its own constructor, because the base constructor is inlined there.
 *
 * The class is not reconstructed. It is declared so that the three subclasses record the bases the
 * RTTI attests, and so that a stage class can reach the task through a pointer to it.
 */
class Pitcher : public MsgSink, public MsgSource, public TickTask {
public:
    /**
     * @ghidraAddress 0x001b3348
     */
    virtual ~Pitcher();
};
