#pragma once

#include "os/mem.h"
#include "sch/cmdid.h"
#include "sch/tick.h"

class FilterLover;

namespace Sch {
class Command;
class TickClock;
} // namespace Sch

/**
 * First-order smoothing filter that steps itself on a clock and reports each step to a
 * FilterLover.
 *
 * The class is not polymorphic and emits no RTTI. Its title comes from the allocation tag
 * `ActiveFilter` at `0x007cc628` and from the original file name `AppActiveFilter.cpp`, which the
 * RTTI of the unit's file-local command records. The object is 0x28 bytes. AxeFX embeds one at
 * `+0x20`.
 *
 * Each step moves mValue towards mTarget, keeping mRetention of the old value, and posts the
 * unit's command mInterval ahead to run the next step. The first SetTarget() creates that command
 * and runs the first step at once.
 */
class ActiveFilter {
public:
    /**
     * Prepare an idle filter.
     *
     * mRetention starts at 0.5, and mTarget, mValue, and mInterval start at zero.
     *
     * @param pClock The clock that runs the steps.
     * @param pLover The receiver of each step's value.
     * @ghidraAddress 0x00100080
     */
    ActiveFilter(Sch::TickClock *pClock, FilterLover *pLover);

    /**
     * Withdraw the pending step and release the command.
     *
     * The variant at `0x001000b8` takes the destructor flags, runs this body, and frees the object
     * under the `ActiveFilter` tag only when bit 0 of the flags is set. AxeFX::~AxeFX() calls it
     * at `0x0019b178` with flags 2 for its embedded filter.
     *
     * @ghidraAddress 0x00100100
     */
    ~ActiveFilter();

    /**
     * Release a filter under the tag `ActiveFilter`.
     *
     * @param pBlock The block.
     */
    static void operator delete(void *pBlock) {
        FreeTaggedMemory(pBlock, "ActiveFilter");
    }

    /**
     * Set the value to move towards, starting the steps on the first call.
     *
     * The first call creates the command, jumps mValue to the target, and runs Update() at once.
     * The title is inferred.
     *
     * @param flTarget The target value.
     * @ghidraAddress 0x00100150
     */
    void SetTarget(float flTarget);

    /**
     * Take one step, report it, and post the next.
     *
     * Sets mValue to `(1.0 - mRetention) * mTarget + mRetention * mValue` in double precision,
     * passes it to FilterLover::OnFilterValue(), and posts the command mInterval ahead under
     * mCommand. Public because the file-local command runs it. The title is inferred.
     *
     * @ghidraAddress 0x001001c8
     */
    void Update();

private:
    float mTarget;
    // The share of the previous value each step keeps.
    float mRetention;
    float mValue;
    FilterLover *mLover;
    Sch::TickClock *mClock;
    // The handle the command is queued under.
    CmdID mCommand;
    // The file-local command, created by the first SetTarget().
    Sch::Command *mStepCommand;

public:
    /**
     * Time between steps, in nanoseconds.
     *
     * Public because AxeFX's constructor writes 100 milliseconds into it directly after
     * construction, and the image has no setter.
     */
    Sch::Tick mInterval;
};
