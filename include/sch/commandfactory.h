#pragma once

namespace Sch {

class Command;

/** Produces one command of a registered class. */
typedef Command *(*CommandFactoryProc)();

/**
 * Registrar that adds one command class to the factory list Command::NewCommand() searches.
 *
 * A registrar is a static object, and its constructor runs from the static initialisation of the
 * unit that defines the command class. The class is not polymorphic, emits no RTTI descriptor,
 * and writes no member, and the title is inferred from the diagnostic `Cannot find ID %ld in
 * Command Factory List`.
 *
 * Four of the twenty-six call sites pass a non-zero identifier (ControllerCmd 2,
 * DoGameSystemPlayCmd 4, EndRecordingCmd 6, and ExitCmd 7). The others pass zero, for which the
 * constructor returns before the list is touched.
 */
class CommandFactory {
public:
    /**
     * Insert a factory into the list, ordered by identifier.
     *
     * The insertion position is searched for before the zero test. A zero identifier therefore
     * still performs the search and then returns without an insertion.
     *
     * @param nCmdID The identifier the class streams itself under.
     * @param pfnCreate The factory for the class.
     * @ghidraAddress 0x00538208
     */
    CommandFactory(int nCmdID, CommandFactoryProc pfnCreate);
};

} // namespace Sch
