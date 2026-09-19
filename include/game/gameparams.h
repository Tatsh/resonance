#pragma once

#include "os/hxstr.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

/**
 * Settings a game session is started with.
 *
 * `10GameParams` in the RTTI descriptor at `0x0086f628`, with no base, so the compiler places the
 * vptr after the data at `+0x34` and the class is 0x38 bytes. Its vtable is at `0x007db5a0`.
 *
 * The whole layout comes from the copy constructor at `0x001fc480`, which copies every member in
 * order, so the offsets and widths are recovered and the three HxStr members are certain. The
 * GameManagerImpl destructor corroborates the three strings independently by releasing their
 * buffers at `+0x6c`, `+0x74`, and `+0x80` of a manager whose settings start at `+0x68`.
 *
 * The purpose of each member is not recovered. Three of the ten are public because GameManagerImpl
 * reads and writes them directly with no accessor in the image. A friend declaration on this class
 * would fit the image equally well as the promotion. The other seven have no traced reader and stay
 * private.
 *
 * This class is declared because several network packets embed one. It is the only one of the
 * three such member classes whose layout falls out of its copy constructor; PlayerInfo and
 * FreqAppearance both own heap state and need more work.
 */
class GameParams {
public:
    /**
     * @param other The settings to copy.
     * @ghidraAddress 0x001fc480
     */
    GameParams(const GameParams &other);

    /**
     * Start with every member clear.
     *
     * @ghidraAddress 0x00187170
     */
    GameParams();

    /**
     * Release the three strings.
     *
     * The declaration is first among the class's virtuals, which is what places it at vtable
     * slot 1. The GameManagerImpl destructor inlines the whole body, a vptr store followed by three
     * string releases.
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
     * Slot 3. GameManagerImpl::Load() runs it after reading its own three words.
     *
     * @param pStream The stream to read from.
     * @ghidraAddress 0x00187390
     */
    virtual void Load(IBStream *pStream);

    /**
     * Copy every member from another instance.
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
    int mUnknown24; // +0x24

public:
    int mUnknown28; /*!< Written by GameManagerImpl::SetGameMode() as the test for `net`. +0x28 */

private:
    int mUnknown2c; // +0x2c
    int mUnknown30; // +0x30
};
