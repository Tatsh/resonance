#pragma once

#include "os/hxstr.h"
#include "rnd/object.h"

namespace Rnd {

/**
 * Registry of every loaded renderer object.
 *
 * `Q23Rnd7Manager` in the RTTI descriptor. The manager owns the class registry that maps a type
 * name in a `.rnd` file to a factory, and it resolves objects by name once a file is loaded. Its
 * members have not been recovered.
 */
class Manager {
public:
    /**
     * Register the manager's profile timers and bring up the object registry.
     *
     * The timers registered are "callback", "anim", "updateworldxfm", "draw", "swap", and
     * "frame".
     *
     * @ghidraAddress 0x00519bb8
     */
    void Init();

    /**
     * Resolve a loaded object by name.
     *
     * @param name The object name as written in the `.rnd` file.
     * @return The object, or null when no object has that name.
     * @ghidraAddress 0x00520498
     */
    Object *Find(const HxStr &name);
};

/**
 * The renderer's object registry.
 *
 * @ghidraAddress 0x00719868
 */
extern Manager g_manager;

} // namespace Rnd
