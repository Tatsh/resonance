#pragma once

/**
 * Two-byte object with one shared instance, reached through shared().
 *
 * The purpose is unrecovered. The class has no RTTI, no embedded file path, and no string, and its
 * routines sit among the compiler runtime between `__main` and sceCdTrayReq. Its name comes from
 * the debugging symbols of the North American demo release. The demo's constructor, Reset(), and
 * shared() have the same instructions as this class's routines. shared() builds the instance at
 * `0x008de7c0` on first use, guarded by the word at `0x00725bf4`, and registers the destructor with
 * atexit. The constructor stores 0xff into both bytes through Reset(), and the destructor is
 * empty. Three uncalled routines at `0x00558db8`, `0x00558dc0`, and `0x00558dc8` sit between
 * Reset() and the destructor and each return zero, and position alone does not tie them to this
 * class. The InputPoller constructor is the only caller of shared(), and it stores the pointer in
 * InputPoller::mBytePairs. No routine reads the member again. No routine loads either byte, and
 * their signedness is unknown.
 */
struct KeyboardMgr {
    /**
     * Construct the instance through Reset().
     *
     * shared() calls it. The body returns this in v0, which is what marks it as the constructor
     * rather than the routine it calls.
     *
     * @ghidraAddress NTSC-U/C: 0x00558d68
     * @ghidraAddress PAL: 0x00599ec0
     */
    KeyboardMgr();

    /**
     * Set both bytes to 0xff, the second byte first.
     *
     * The constructor is the one caller. The routine writes no return value.
     *
     * @ghidraAddress NTSC-U/C: 0x00558d90
     * @ghidraAddress PAL: 0x00599ee8
     */
    void InitPortMap();

    /**
     * Do nothing.
     *
     * shared() registers this destructor with atexit.
     *
     * @ghidraAddress NTSC-U/C: 0x00558dd0
     * @ghidraAddress PAL: 0x00599f28
     */
    ~KeyboardMgr();

    /**
     * Return the shared instance, constructing it on first use.
     *
     * @return The instance.
     * @ghidraAddress NTSC-U/C: 0x00558d10
     * @ghidraAddress PAL: 0x00599e68
     */
    static KeyboardMgr *shared();

    /** The two bytes, both 0xff after construction. +0x00 */
    unsigned char mBytes[2];
};
