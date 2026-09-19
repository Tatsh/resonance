#pragma once

#include "gs/effector.h"

/**
 * Effect that reports its identity and does no work of its own.
 *
 * `18GhostNotesEffector` in the RTTI descriptor at `0x008f0110`, with Effector as its one base. The
 * class declares no data member, and the factory at `0x001a0df0` allocates 0x14 bytes for it. That
 * allocation is the size of the base alone, and it therefore fixes the size of the base as well.
 *
 * The table at `0x007de7a0` runs GetTypeInfo, the destructor, the two inherited MsgSource
 * registration routines, then Type() and Enable().
 *
 * Enable() has an empty body. The ghost-notes powerup therefore does not act through this object,
 * and the object exists only for Type() to report. An effect the synthesiser does not implement as
 * a control change still needs an Effector for the powerup that selects it.
 */
class GhostNotesEffector : public Effector {
public:
    /**
     * @ghidraAddress 0x001a1cf8
     */
    virtual ~GhostNotesEffector();

    /**
     * Report which effect this object applies.
     *
     * @return kEffectorTypeGhostNotes.
     * @ghidraAddress 0x001a1e30
     */
    virtual int Type();

    /**
     * Do nothing.
     *
     * @param bEnabled Ignored.
     * @ghidraAddress 0x001a1e38
     */
    virtual void Enable(int bEnabled);
};
