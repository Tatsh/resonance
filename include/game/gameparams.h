#pragma once

#include "os/hxstr.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

/**
 * Settings a game session is started with.
 *
 * `10GameParams` in the RTTI descriptor at `0x0086f628`, with no base, so the compiler places the
 * vptr after the data at `+0x34` and the class is 0x38 bytes. Its vtable at `0x007db5a0` has four
 * entries and a zero terminator at index 4, so the destructor and the two transfer members are the
 * whole set of virtuals.
 *
 * The layout comes from the constructor, the destructor, and the copy constructor together. The
 * constructor zeroes every word from `+0x00` to `+0x30` in ascending order, the destructor releases
 * the buffers at `+0x18`, `+0x0c`, and `+0x04` in reverse declaration order, and the copy
 * constructor at `0x001fc480` copies all ten members in declaration order. The GameManagerImpl
 * destructor corroborates the three strings independently by releasing their buffers at `+0x6c`,
 * `+0x74`, and `+0x80` of a manager whose settings start at `+0x68`.
 *
 * That copy constructor is the compiler-generated one and is therefore not declared here. Two
 * measurements agree. It copies every member in exact declaration order with no logic of its own,
 * and it sits at `0x001fc480` while every hand-written member of the class sits in the `0x00187xxx`
 * run, which places it in the translation unit that copies a GameParams rather than in the one that
 * defines the class.
 *
 * The purpose of each member is not recovered. Three of the ten are public because GameManagerImpl
 * reads and writes them directly with no accessor in the image. A friend declaration on this class
 * would fit the image equally well as the promotion. The other seven have no traced reader and stay
 * private.
 *
 * mUnknown14 is the one member that neither Save(), Load(), nor operator=() touches, and the
 * compiler-generated copy constructor is the only routine in the image that copies it.
 *
 * This class is declared because several network packets embed one.
 */
class GameParams {
public:
    /**
     * Start with every member clear.
     *
     * @ghidraAddress 0x00187170
     */
    GameParams();

    /**
     * Copy every member in declaration order.
     *
     * Defaulted rather than written, because the routine at `0x001fc480` is the compiler-generated
     * copy constructor, as the class documentation records. Declaring it is what the destructor and
     * the assignment operator below make necessary: a user-declared destructor deprecates an
     * implicit copy constructor, and the three packets that embed a GameParams copy one.
     *
     * @ghidraAddress 0x001fc480
     */
    GameParams(const GameParams &other) = default;

    /**
     * Release the three strings.
     *
     * The declaration is first among the class's virtuals, which is what places it at vtable
     * slot 1. The body is empty, and the three releases are the compiler-generated member
     * destructor calls. The GameManagerImpl destructor inlines the whole sequence.
     *
     * @ghidraAddress 0x00187940
     */
    virtual ~GameParams();

    /**
     * Write the settings to a stream.
     *
     * Slot 2. GameManagerImpl::Save() runs it after writing its own three words.
     *
     * @param pStream The stream to write to.
     * @ghidraAddress 0x001871b8
     */
    virtual void Save(OBStream *pStream);

    /**
     * Read the settings back from a stream.
     *
     * Slot 3. GameManagerImpl::Load() runs it after reading its own three words. The fields come
     * back in the order Save() wrote them.
     *
     * @param pStream The stream to read from.
     * @ghidraAddress 0x00187390
     */
    virtual void Load(IBStream *pStream);

    /**
     * Copy every member except mUnknown14 from another instance.
     *
     * Self-assignment is not tested, and the routine returns this instance.
     *
     * @param other The settings to copy.
     * @return This instance.
     * @ghidraAddress 0x00187be8
     */
    GameParams &operator=(const GameParams &other);

private:
    HxStr mUnknown00; // +0x00
    HxStr mUnknown08; // +0x08
    int mUnknown10;   // +0x10
    HxStr mUnknown14; // +0x14

public:
    int mUnknown1c; /*!< Driven by GameManagerImpl slots 29 and 31, the play mode. +0x1c */
    int mUnknown20; /*!< Driven by GameManagerImpl slots 28 and 30. +0x20 */

private:
    // Load() normalises the transferred word to 0 or 1 before the store, which is what types this
    // member and the three below as truth values rather than as counts.
    bool mUnknown24; // +0x24

public:
    /** Written by GameManagerImpl::SetGameMode() as the test for `net`. +0x28 */
    bool mUnknown28;

private:
    bool mUnknown2c; // +0x2c
    bool mUnknown30; // +0x30
};
