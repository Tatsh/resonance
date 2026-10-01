#include <exception>

#include "app/application.h"
#include "app/globals.h"
#include "game/grooveworld.h"
#include "game/localplayer.h"
#include "mid/mbt.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "sch/tickclock.h"
#include "script/cxx/config.h"
#include "script/cxx/int.h"
#include "script/cxx/object.h"
#include "script/cxx/tuple.h"
#include "script/cxx/typeerror.h"
#include "script/scriptfunc.h"

namespace {

// Loop the local player from the current song position.
//
// The tuple carries the looping flag. Nothing happens unless the first local player is a
// LocalPlayer. The image dispatches SetLooping through the secondary table; the static type
// carries the same slot.
// 0x0015ff98
Py::Object ScriptSetLoopMode(const Py::Tuple &args) {
    if (args.length() != 1) {
        throw Py::TypeError(HxStr("wrong # args for set_loop_mode"));
    }
    const long long nLooping = Py::Int(args.getItem(0));
    GrooveWorld *pWorld = Application::shared()->GetWorld();
    if (pWorld != nullptr) {
        LocalPlayer *pPlayer = dynamic_cast<LocalPlayer *>(pWorld->mLocalPlayers[0]);
        if (pPlayer != nullptr) {
            const Mid::MBT position(Application::shared()->GetSongClock()->SongTick());
            pPlayer->SetLooping(nLooping != 0 ? 1 : 0, position);
        }
    }
    return Py::Object();
}

// Run ScriptSetLoopMode() on the interpreter's argument tuple.
// 0x00160590
PyObject *PyInvokeSetLoopMode(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptSetLoopMode(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    } catch (std::exception &error) {
        PyErr_SetString(PyExc_RuntimeError, const_cast<char *>(error.what()));
        return nullptr;
    }
}

// Show or hide the local player's ghost.
//
// The tuple includes the ghost flag. SetGhost() stores it and announces it; a player that is not a
// LocalPlayer retains its display.
// 0x001602a8
Py::Object ScriptSetGhostMode(const Py::Tuple &args) {
    if (args.length() != 1) {
        throw Py::TypeError(HxStr("wrong # args for set_ghost_mode"));
    }
    const long long nGhost = Py::Int(args.getItem(0));
    GrooveWorld *pWorld = Application::shared()->GetWorld();
    if (pWorld != nullptr) {
        LocalPlayer *pPlayer = dynamic_cast<LocalPlayer *>(pWorld->mLocalPlayers[0]);
        if (pPlayer != nullptr) {
            pPlayer->SetGhost(nGhost != 0 ? 1 : 0);
        }
    }
    return Py::Object();
}

// Run ScriptSetGhostMode() on the interpreter's argument tuple.
// 0x001607d8
PyObject *PyInvokeSetGhostMode(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptSetGhostMode(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    } catch (std::exception &error) {
        PyErr_SetString(PyExc_RuntimeError, const_cast<char *>(error.what()));
        return nullptr;
    }
}

// The script interface this file exports, registered in static initialisation.
// 0x00160fb8
const ScriptFunc kSetLoopModeFunc("set_loop_mode", PyInvokeSetLoopMode);
const ScriptFunc kSetGhostModeFunc("set_ghost_mode", PyInvokeSetGhostMode);

} // namespace
