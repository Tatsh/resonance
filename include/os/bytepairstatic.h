#pragma once

/**
 * Two-byte object with one shared instance, reached through shared().
 *
 * The name and the purpose are both inferred and unrecovered. The class has no RTTI, no embedded
 * file path, and no string, and its routines sit among the compiler runtime between `__main` and
 * sceCdTrayReq. What the image does show is recorded here. shared() builds the instance at
 * `0x008de7c0` on first use, guarded by the word at `0x00725bf4`, and registers the destructor with
 * atexit. The constructor stores 0xff into both bytes, and the destructor is empty. The
 * InputPoller constructor is the only caller of shared(), and it stores the pointer in
 * InputPoller::mUnknown28, which nothing reads again. No routine loads either byte, so their
 * signedness is unknown.
 */
struct BytePairStatic {
    /**
     * Set both bytes to 0xff.
     *
     * The forwarder at `0x00558d68` runs this body, and shared() calls the forwarder.
     *
     * @ghidraAddress 0x00558d90
     */
    BytePairStatic();

    /**
     * Do nothing.
     *
     * shared() registers this destructor with atexit.
     *
     * @ghidraAddress 0x00558dd0
     */
    ~BytePairStatic();

    /**
     * Return the shared instance, constructing it on first use.
     *
     * @return The instance.
     * @ghidraAddress 0x00558d10
     */
    static BytePairStatic *shared();

    /** The two bytes, both 0xff after construction. +0x00 */
    unsigned char mBytes[2];
};
