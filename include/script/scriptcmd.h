#pragma once

class HxStr;

namespace Sch {
class Command;
} // namespace Sch

/**
 * Build a command that hands script text to the script sink when it runs.
 *
 * The command is the file-local ScriptCmd of the post-script unit. The hx.PostScript binding
 * builds it inline; this factory shares the construction without duplicating the class.
 *
 * @param script The script text to run.
 * @return The new command.
 */
Sch::Command *NewScriptCmd(const HxStr &script);
