#pragma once

/**
 * Reader of the physical controllers.
 *
 * `11InputPoller` in the RTTI descriptor at `0x0086f6d8`, with no base, so the vptr lands after the
 * data at `+0x58` and the class is 0x5c bytes. Its vtable is at `0x007e69b8` and has two entries,
 * the type function and the destructor at `0x001df080`. The destructor is therefore the only
 * virtual the class declares. The size comes from the allocation in the GameManagerImpl
 * constructor, and the constructor itself is at `0x001ded98`.
 *
 * Recovery has barely started. This declaration exists so that GameManagerImpl can type the poller
 * it creates and vends through vtable slot 17. Five members GameManagerImpl drives are recorded
 * below by address instead of being declared. None of their signatures is settled.
 *
 *  - `0x001e1998` receives the poller and the MetaGameWorld, or a null pointer where the manager
 * has no world. GameManagerImpl::Start() and four of its message handlers run it.
 *  - `0x001e1a80` receives the poller and one flag. Start() passes 1 and the begin-game handler
 *    passes 0.
 *  - `0x001e1c18` receives the poller and one flag.
 *  - `0x001e1c20` receives the poller and one flag.
 *  - `0x001e1c28` receives the poller alone, from GameManagerImpl::PollPlayback().
 *
 * PollPlayback() reads the field at `+0x38` as the first of its three conditions.
 */
class InputPoller {
public:
    /**
     * @ghidraAddress 0x001ded98
     */
    InputPoller();

    /**
     * @ghidraAddress 0x001df080
     */
    virtual ~InputPoller();
};
