#pragma once

#include "app/msgsink.h"
#include "game/rawcontroller.h"

/**
 * Owner of the world a game session runs in.
 *
 * `11GrooveWorld` in the RTTI descriptor at `0x008eff70`, with MsgSink as a public base at offset 0
 * and RawController as a public base at `+0x04`. The object is 0xbc bytes, which the allocation in
 * GameManagerImpl::CreateWorld() fixes. Two vtables belong to the class. The MsgSink subobject
 * addresses `0x007dc648` with no adjustment, and the RawController subobject addresses
 * `0x007dc628`, whose three entries each adjust `this` by `-4`.
 *
 * Recovery has barely started. This declaration exists so that GameManagerImpl can type the world
 * it creates and vends through vtable slot 15. The two pure virtuals the bases declare are left
 * unfilled here, so the class reads as abstract until the overrides are recovered.
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
};
