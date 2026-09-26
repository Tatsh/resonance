#include "app/application.h"
#include "app/globals.h"
#include "app/scriptsink.h"
#include "msg/scriptmsg.h"
#include "os/hxstr.h"
#include "sch/command.h"
#include "script/scriptcmd.h"

namespace {

/**
 * Scheduler command that hands one line of script text to the script sink when it runs.
 *
 * `Q234_GLOBAL_$N$CmdPostScript.cppdKuhgb9ScriptCmd` in the RTTI, with Sch::Command as its one
 * base. Its vtable at `0x007d64f8` retains Sch::Command::Print(), Save(), and Load(). The
 * `hx.PostScript` binding at `0x001597b8` expands the constructor into its 0x14-byte allocation
 * and posts the command on the song clock.
 *
 * The destructor at `0x0015a350` is implicitly declared. It releases the string, stores the base
 * table pointer, and runs Attachment's destructor, which is what the compiler generates.
 */
class ScriptCmd : public Sch::Command {
public:
    explicit ScriptCmd(const HxStr &script) : mScript(script) {
    }

    // 0x0015a3f8
    virtual int CmdID() {
        return sCmdID;
    }

    // 0x0015a408
    virtual void Execute() {
        ScriptMsg msg(mScript);
        Application::shared()->GetScriptSink()->Handle(&msg);
    }

    // The word at 0x00676e68, which the image initialises to zero.
    static int sCmdID;

private:
    HxStr mScript; // +0x0c
};

int ScriptCmd::sCmdID;

} // namespace

Sch::Command *NewScriptCmd(const HxStr &script) {
    return new ScriptCmd(script);
}
