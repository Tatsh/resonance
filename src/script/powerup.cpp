#include "app/application.h"
#include "app/globals.h"
#include "app/hudutil.h"
#include "app/msgsink.h"
#include "game/grooveworld.h"
#include "game/player.h"
#include "msg/caughtpowerbarmsg.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "script/cxx/config.h"
#include "script/cxx/int.h"
#include "script/cxx/object.h"
#include "script/cxx/tuple.h"
#include "script/cxx/typeerror.h"

namespace {

// Grant a powerup to one player.
//
// The tuple carries the player number and the powerup kind. The first player whose input slot
// matches gets a CaughtPowerbarMsg naming it and the kind. A number no slot matches leaves
// every player alone.
// 0x0015a820
Py::Object ScriptAddPowerup(const Py::Tuple &args) {
    if (args.length() != 2) {
        throw Py::TypeError(HxStr(FormatString("required args: int:player-num, int:powerup-type")));
    }
    const long nPlayer = Py::Int(args.getItem(0));
    const long nKind = Py::Int(args.getItem(1));
    GrooveWorld *pWorld = Application::shared()->GetWorld();
    if (pWorld != nullptr) {
        for (std::vector<Player *>::iterator it = pWorld->mPlayers.begin();
             it != pWorld->mPlayers.end();
             ++it) {
            if ((*it)->Slot2() == static_cast<int>(nPlayer)) {
                CaughtPowerbarMsg message;
                message.mKind = static_cast<HudItemKind>(nKind);
                message.mPlayer = *it;
                (*it)->Handle(&message);
                break;
            }
        }
    }
    return Py::Object();
}

// Run ScriptAddPowerup() on the interpreter's argument tuple.
// 0x0015aec8
PyObject *PyInvokeAddPowerup(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptAddPowerup(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    }
}

} // namespace
