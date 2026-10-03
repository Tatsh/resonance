#pragma once

#include "mid/mbt.h"
#include "msg/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * Its RTTI descriptor is at `0x008ef7a0`. It has Message as its one base. The object is 0xc bytes
 * and its vtable is at `0x007ce768`. The allocation in New() and the allocation in Clone() report
 * the same size.
 *
 * The payload layout comes from the run of field copies in Clone(). mAdvance is public because
 * Overlay::OnAdvanceSectionToggle() at `0x0041f440` reads it directly with no accessor in the
 * image, showing `ADVANCE TO NEXT SECTION` when it is non-zero and `REPEAT SECTION` otherwise. The
 * position at `+0x08` is the section tick Gamer's build at `0x00111754` clamps and passes through
 * the Mid::MBT constructor.
 *
 * The destructor at `0x00115ee0` is compiler-generated and has no declaration here. The routine
 * at `0x00115f18` is a further emission of the type-information accessor.
 */
class AdvanceSectionToggleMsg : public Message {
public:
    /**
     * Construct a message with the payload unset.
     *
     * Inline. New() expands it. A declaration is required because the class declares a second
     * constructor.
     */
    AdvanceSectionToggleMsg() {
    }

    /**
     * Report the section toggle.
     *
     * Inline, with no address of its own. Gamer's build at `0x00111754` expands it on its stack.
     *
     * @param nAdvance Non-zero to advance past the section, zero to repeat it.
     * @param position The section's position, already clamped to Mid::MBT's bounds.
     */
    AdvanceSectionToggleMsg(int nAdvance, Mid::MBT position)
        : mAdvance(nAdvance), mPosition(position) {
    }

    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory against identity 314.
     *
     * @return The message.
     * @ghidraAddress NTSC-U/C: 0x003d71d0
     * @ghidraAddress PAL: 0x0040f0d0
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress NTSC-U/C: 0x00115f90
     * @ghidraAddress PAL: 0x00116438
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nAdvanceSectionToggleMsgType.
     * @ghidraAddress NTSC-U/C: 0x00115fe0
     * @ghidraAddress PAL: 0x00116488
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `AdvanceSectionToggleMsg`.
     * @ghidraAddress NTSC-U/C: 0x00115ff0
     * @ghidraAddress PAL: 0x00116498
     */
    virtual const char *GetName() const;

    int mAdvance; /*!< Non-zero to advance past the section, zero to repeat it. +0x04 */

private:
    Mid::MBT mPosition; // +0x08
};

/**
 * Identity that AdvanceSectionToggleMsg::Type() reports.
 *
 * This word belongs to AdvanceSectionToggleMsg because AdvanceSectionToggleMsg::Type() at
 * `0x00115fe0` returns it, and the registration at `0x003d9818` passes the same value, 314, as the
 * identity of this class's factory.
 *
 * @ghidraAddress NTSC-U/C: 0x006d025c
 * @ghidraAddress PAL: 0x007139f4
 */
extern int g_nAdvanceSectionToggleMsgType;
