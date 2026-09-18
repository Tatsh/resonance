#pragma once

#include "app/globals.h"

/**
 * The FreQuency application.
 *
 * `11Application` in the RTTI descriptor at `0x008f0870`, deriving from `Globals`. A single
 * instance is constructed by a static initialiser before `main` runs and is vended by shared().
 */
class Application : public Globals {
public:
    /**
     * @ghidraAddress 0x00198cb0
     */
    virtual ~Application();

    /**
     * Run the game until it exits.
     *
     * Brings up the script host and the main loop, then drives the main loop through its own
     * virtual entry point.
     *
     * @return Always 1.
     * @ghidraAddress 0x00198d20
     */
    virtual int Run();

    /**
     * Overridden with an empty body.
     *
     * @ghidraAddress 0x00198da0
     */
    virtual void Reset();

    /**
     * The single application instance.
     *
     * @ghidraAddress 0x00198da8
     */
    static Application *shared();
};
