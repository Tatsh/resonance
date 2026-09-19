#pragma once

#include "app/msgsink.h"
#include "game/rawcontroller.h"

/**
 * Owner of the world a game session runs in.
 *
 * `11GrooveWorld` in the RTTI descriptor at `0x008eff70`, with MsgSink as a public base at offset 0
 * and RawController as a public base at `+0x04`. The object is 0xbc bytes, which the allocation in
 * GameManagerImpl::CreateWorld() fixes.
 *
 * Two vtables belong to the class, and the secondary one precedes the primary one in memory. The
 * RawController subobject addresses `0x007dc628`, whose three entries each adjust `this` by `-4`,
 * and the MsgSink subobject addresses `0x007dc648` with no adjustment. The primary table has four
 * entries and a zero terminator at index 4, the type function at `0x001936f8`, the destructor at
 * `0x0018c368`, the inherited MsgSink::Handle() at `0x00105158`, and the HandleMessage() override
 * below. The secondary table has three entries and a zero terminator at index 3, the same type
 * function and destructor followed by the RawController override below.
 *
 * With both overrides present the class is concrete, which agrees with GameManagerImpl creating
 * one.
 *
 * Recovery of the bodies has barely started, and neither override is defined. Each needs members
 * this header does not declare.
 *
 * The constructor at `0x0018bef0` takes the application and the game manager's GameStats. It
 * records the first in `+0x08` and the second in `+0x30`, then clears the run from `+0x0c` to
 * `+0x2c`. The constructor is documented rather than declared. Declaring it would make this header
 * include the application header. That include closes a cycle through the application's own
 * reference to the game manager.
 *
 * GameManagerImpl reads `+0x0c` from three of its handlers, writes `+0x8c` from its load path and
 * its begin-game handler, and reads `+0x90` from its unpause handler. The accessor at `0x001952e0`
 * returns `&mUnknown28->mUnknown14` for a non-null `+0x28` and a null pointer otherwise, and
 * GameManagerImpl::DrawFrame() uses it to decide whether the world has anything to draw.
 *
 * Five further members are recorded by address instead of being declared. `0x00194ca0` reports
 * whether an asynchronous load has finished, `0x00194d00` completes it, `0x0018dc88` and
 * `0x0018de38` both run once the world is ready, and `0x00195170` advances the world during
 * playback.
 */
class GrooveWorld : public MsgSink, public RawController {
public:
    /**
     * @ghidraAddress 0x0018c368
     */
    virtual ~GrooveWorld();

    /**
     * Route a message to one of two owned sinks.
     *
     * Slot 3 of the primary table. The body reads the message's identity through Message::Type()
     * and compares it against the CripplePacket identity at `0x006d73fc` and then against the
     * BumpPacket identity at `0x006d7404`, forwarding to slot 2 of the object at `+0x24` on the
     * first match and to slot 2 of the object at `+0x10` on the second. Each identity is confirmed
     * by the Type() slot that returns it, CripplePacket::Type() at `0x003f0cd8` and
     * BumpPacket::Type() at `0x003f0ef8`, and by the class-name literal the adjacent Name() slot
     * returns. A message matching neither is discarded. Neither owned object's
     * class is recovered, and slot 2 of a MsgSink is Handle() rather than HandleMessage(), so
     * neither is a MsgSink.
     *
     * @param pMsg The message to route.
     * @ghidraAddress 0x00195388
     */
    virtual void HandleMessage(Message *pMsg);

    /**
     * Report a controller reading. Slot 2 of the secondary table.
     *
     * The body reads `+0x98` and returns at once unless it holds 4, which is what makes the world
     * respond to a controller only in one of its states.
     *
     * @param nUnknown1 The first word of the reading.
     * @param nUnknown2 The second word of the reading.
     * @param nUnknown3 The third word of the reading.
     * @param flUnknown4 The float of the reading.
     * @ghidraAddress 0x0018ed98
     */
    virtual void OnUnknownSlot2(int nUnknown1, int nUnknown2, int nUnknown3, float flUnknown4);
};
