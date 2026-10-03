#include <exception>

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
#include "script/registercfunction.h"

namespace {

// Grant a powerup to one player.
//
// The tuple carries the player number and the powerup kind. The first player whose input slot
// matches gets a CaughtPowerbarMsg naming it and the kind. A number no slot matches leaves
// every player alone.
// NTSC-U/C: 0x0015a820, PAL: 0x0015c5a0
Py::Object ScriptAddPowerup(Py::Tuple args) {
    if (args.length() != 2) {
        throw Py::TypeError(HxStr("required args: int:player-num, int:powerup-type"));
    }
    const int nPlayer = Py::Int(args.getItem(0));
    const int nKind = Py::Int(args.getItem(1));
    GrooveWorld *pWorld = Application::shared()->GetWorld();
    if (pWorld != nullptr) {
        for (std::vector<Player *>::iterator it = pWorld->mPlayers.begin();
             it != pWorld->mPlayers.end();
             ++it) {
            if ((*it)->GetInputSlot() == static_cast<int>(nPlayer)) {
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
// NTSC-U/C: 0x0015aec8, PAL: 0x0015cc68
PyObject *PyInvokeAddPowerup(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptAddPowerup(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    } catch (std::exception &error) {
        PyErr_SetString(PyExc_RuntimeError, const_cast<char *>(error.what()));
        return nullptr;
    }
}

// The script interface this file exports, registered in static initialisation.
// NTSC-U/C: 0x0015b850, PAL: 0x0015d5f0
const RegisterCFunction kAddPowerupFunc("add_powerup", PyInvokeAddPowerup);

} // namespace
