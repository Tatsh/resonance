#pragma once

/**
 * Abstract owner of the process-wide state.
 *
 * `7Globals` in the RTTI descriptor at `0x0086f5d0`. The class has no base, so the compiler places
 * its vptr after the data members at `+0x1c`. Both user virtuals are pure, and the vtable at
 * `0x007cee78` points them at the shared pure-virtual handler. The purpose of the seven data
 * words has not been recovered.
 */
class Globals {
public:
    /**
     * Construct the globals with every field cleared.
     *
     * @ghidraAddress 0x00118c40
     */
    Globals();

    /**
     * @ghidraAddress 0x00118c68
     */
    virtual ~Globals();

    /**
     * Run the application until it exits.
     *
     * @return The process exit status.
     */
    virtual int Run() = 0;

    /**
     * Second pure virtual, overridden by an empty body in Application.
     */
    virtual void Reset() = 0;

    int mUnknown00; // +0x00
    int mUnknown04; // +0x04
    int mUnknown08; // +0x08
    int mUnknown0c; // +0x0c
    int mUnknown10; // +0x10
    int mUnknown14; // +0x14
    int mUnknown18; // +0x18
};
