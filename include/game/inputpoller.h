#pragma once

/**
 * Reader of the physical controllers.
 *
 * `11InputPoller` in the RTTI descriptor at `0x0086f6d8`, with no base, so the vptr lands after the
 * data at `+0x58` and the class is 0x5c bytes. Its vtable at `0x007e69b8` has two entries and a
 * zero terminator at index 2, the type function and the destructor at `0x001df080`, so the
 * destructor is the only virtual the class declares. The size comes from the allocation in the
 * GameManagerImpl constructor.
 *
 * The constructor at `0x001ded98` and the destructor together recover the whole member map, and it
 * accounts for every byte up to the vptr. No member is declared, because the element type of the
 * first container is not recovered and declaring the later members without it would state a wrong
 * set of offsets.
 *
 *  - `+0x00` a std::vector whose element is 0x4c bytes. The destructor divides the byte span by
 * 0x4c through the reciprocal `0x286bca1b`, and its element walk calls nothing, so the element
 * needs no destructor.
 *  - `+0x0c` a std::vector with a four-byte element.
 *  - `+0x18` one word, cleared on construction.
 *  - `+0x1c` a std::vector with a four-byte element.
 *  - `+0x28` one word, set from the routine at `0x00558d10`.
 *  - `+0x2c` a std::list, which is the single pointer to a 16-byte self-linked node.
 *  - `+0x30` through `+0x54` eleven words. Construction sets `+0x34`, `+0x3c`, and `+0x4c` to 1 and
 * clears the other eight.
 *
 * Recovery of the bodies has barely started, and neither member below is defined. The constructor
 * ends by running `0x001e19b8` on itself and the destructor opens by running `0x001df9d8`, and
 * neither of those two is recovered.
 *
 * Five members GameManagerImpl drives are recorded below by address instead of being declared. None
 * of their signatures is settled.
 *
 *  - `0x001e1998` receives the poller and the MetaGameWorld, or a null pointer where the manager
 * has no world. GameManagerImpl::Start() and four of its message handlers run it.
 *  - `0x001e1a80` receives the poller and one flag. Start() passes 1 and the begin-game handler
 *    passes 0.
 *  - `0x001e1c18` receives the poller and one flag.
 *  - `0x001e1c20` receives the poller and one flag.
 *  - `0x001e1c28` receives the poller alone, from GameManagerImpl::PollPlayback().
 *
 * PollPlayback() reads the field at `+0x38` as the first of its three conditions, which
 * construction clears.
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
