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

namespace {

// Loop the local player from the current song position.
//
// The tuple carries the looping flag. Nothing happens unless the first local player is a
// LocalPlayer. The image dispatches SetLooping through the secondary table; the static type
// carries the same slot.
// 0x0015ff98
Py::Object ScriptSetLoopMode(const Py::Tuple &args) {
    if (args.length() != 1) {
        throw Py::TypeError(HxStr(FormatString("wrong # args for set_loop_mode")));
    }
    const long nLooping = Py::Int(args.getItem(0));
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
    }
}

// Show or hide the local player's ghost.
//
// The tuple carries the ghost flag. Slot22 stores it and announces it; a player that is not a
// LocalPlayer keeps its display.
// 0x001602a8
Py::Object ScriptSetGhostMode(const Py::Tuple &args) {
    if (args.length() != 1) {
        throw Py::TypeError(HxStr(FormatString("wrong # args for set_ghost_mode")));
    }
    const long nGhost = Py::Int(args.getItem(0));
    GrooveWorld *pWorld = Application::shared()->GetWorld();
    if (pWorld != nullptr) {
        LocalPlayer *pPlayer = dynamic_cast<LocalPlayer *>(pWorld->mLocalPlayers[0]);
        if (pPlayer != nullptr) {
            pPlayer->Slot22(nGhost != 0 ? 1 : 0);
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
    }
}

} // namespace
