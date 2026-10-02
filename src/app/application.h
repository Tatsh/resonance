#pragma once

#include "app/globals.h"

/**
 * The FreQuency application.
 *
 * `11Application` in the RTTI descriptor at `0x008f0870`, deriving from Globals. Its vtable is at
 * `0x007dcf68` and it adds no data members, so the constructor is the base constructor alone and
 * the compiler inlined it into the static initialiser at `0x00198c58` that creates the instance
 * before main() runs.
 *
 * The instance has file-scope linkage. Its only three references in the image all sit in this
 * translation unit, at `0x00198c7c` and `0x00198c9c` in the static initialiser and at
 * `0x00198db0` in shared(). Nothing outside the unit refers to it, and shared() exists so that
 * nothing has to.
 *
 * The two overrides are the whole of the Globals interface. Their titles come from the two
 * exception templates the script layer registers,
 * `An exception was thrown by the function Application::Run().` and
 * `An exception was thrown by the function Application::ExitInstance().`
 */
class Application : public Globals {
public:
    /**
     * @ghidraAddress 0x00198cb0
     */
    virtual ~Application();

    /**
     * Bring the game up and run it until the frame loop stops.
     *
     * The routine registers the script call templates, creates the script module, creates every
     * service through Globals::Init(), runs `autoexec()`, loads the memory-card save icon, starts
     * the game manager, and then hands control to the frame loop.
     *
     * @return Always 1.
     * @ghidraAddress 0x00198d20
     */
    virtual int Run();

    /**
     * Tear the game down as it exits.
     *
     * The override has no effect. Nothing in the image invokes it.
     *
     * @return Always zero.
     * @ghidraAddress 0x00198da0
     */
    virtual int ExitInstance();

    /**
     * @return The single application instance.
     * @ghidraAddress 0x00198da8
     */
    static Application *shared();
};
