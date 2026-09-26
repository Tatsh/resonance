#include "game/screenanim.h"
#include "game/tnlarena.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "rnd/animatable.h"
#include "rnd/manager.h"
#include "script/cxx/config.h"
#include "script/cxx/object.h"
#include "script/cxx/string.h"
#include "script/cxx/tuple.h"
#include "script/cxx/typeerror.h"

namespace {

// The tuple layouts the two parsers below accept, as their errors report them.
constexpr int kAnimatableArgCount = 2;
constexpr int kLoopRangeArgCount = 3;

// Split a (name, value) tuple into an animation target and a value.
//
// The target is the Rnd::Animatable the object registry holds under the name. A tuple of any
// other length, an unknown name, or a name held by an object of another type each throw
// Py::TypeError, after posting the reason to the interpreter. The tuple arrives by value, so
// the copy the caller builds is released here.
// 0x004071a0
void ScriptParseAnimatableArgs(Py::Tuple args, Rnd::Animatable **ppTarget, float *pflValue) {
    if (args.length() != kAnimatableArgCount) {
        throw Py::TypeError(
            HxStr(FormatString("incorrect # of args; expected 2, got %d", args.length())));
    }
    Py::Object element = args.getItem(0);
    Py::String text(element);
    HxStr name = text;
    Rnd::Animatable *pTarget = dynamic_cast<Rnd::Animatable *>(Rnd::g_manager.Find(name));
    *ppTarget = pTarget;
    if (pTarget == nullptr) {
        throw Py::TypeError(HxStr(FormatString("%s: not animatable", name.mStr)));
    }
    Py::Object number(PyNumber_Float(args.getItem(1).mPtr));
    *pflValue = static_cast<float>(PyFloat_AsDouble(number.mPtr));
}

// Split a (name, minimum, maximum) tuple into an animation target and a loop range.
//
// The target lookup and every error match ScriptParseAnimatableArgs(). The tuple arrives by
// value, so the copy the caller builds is released here.
// 0x00407990
void ScriptParseAnimLoopRange(Py::Tuple args,
                              Rnd::Animatable **ppTarget,
                              float *pflMin,
                              float *pflMax) {
    if (args.length() != kLoopRangeArgCount) {
        throw Py::TypeError(
            HxStr(FormatString("incorrect # of args; expected 3, got %d", args.length())));
    }
    Py::Object element = args.getItem(0);
    Py::String text(element);
    HxStr name = text;
    Rnd::Animatable *pTarget = dynamic_cast<Rnd::Animatable *>(Rnd::g_manager.Find(name));
    *ppTarget = pTarget;
    if (pTarget == nullptr) {
        throw Py::TypeError(HxStr(FormatString("%s: not animatable", name.mStr)));
    }
    Py::Object minNumber(PyNumber_Float(args.getItem(1).mPtr));
    *pflMin = static_cast<float>(PyFloat_AsDouble(minNumber.mPtr));
    Py::Object maxNumber(PyNumber_Float(args.getItem(2).mPtr));
    *pflMax = static_cast<float>(PyFloat_AsDouble(maxNumber.mPtr));
}

// Move a named animation to a frame.
// 0x004084a8
Py::Object ScriptAnimFrame(const Py::Tuple &args) {
    Rnd::Animatable *pTarget = nullptr;
    float flValue = 0.0f;
    ScriptParseAnimatableArgs(Py::Tuple(args), &pTarget, &flValue);
    pTarget->SetFrame(flValue);
    return Py::Object();
}

// Change the rate multiplier of a named animation.
// 0x004085c0
Py::Object ScriptAnimSpeed(const Py::Tuple &args) {
    Rnd::Animatable *pTarget = nullptr;
    float flValue = 0.0f;
    ScriptParseAnimatableArgs(Py::Tuple(args), &pTarget, &flValue);
    pTarget->SetRate(flValue);
    return Py::Object();
}

// Change the addend of a named animation.
// 0x004086d8
Py::Object ScriptAnimOffset(const Py::Tuple &args) {
    Rnd::Animatable *pTarget = nullptr;
    float flValue = 0.0f;
    ScriptParseAnimatableArgs(Py::Tuple(args), &pTarget, &flValue);
    pTarget->SetOffset(flValue);
    return Py::Object();
}

// Change both ends of the loop range of a named animation.
// 0x004087f0
Py::Object ScriptAnimMinmax(const Py::Tuple &args) {
    Rnd::Animatable *pTarget = nullptr;
    float flMin = 0.0f;
    float flMax = 0.0f;
    ScriptParseAnimLoopRange(Py::Tuple(args), &pTarget, &flMin, &flMax);
    pTarget->SetLoopRange(flMin, flMax);
    return Py::Object();
}

// Lock the arena screens at the neutral level.
//
// Sets the arena's juice-trip flag so HandleMessage() stops acting on juice, and passes level 1
// to the screen animation, which is what TnlArena::LockLevel() does. The arguments are unused;
// the tuple arrives by value and is released here.
// 0x00408910
Py::Object ScriptTestArena([[maybe_unused]] Py::Tuple args) {
    if (g_pTnlArena != nullptr) {
        g_pTnlArena->mUnknown24 = 1;
        g_pTnlArena->mScreenAnim->SetLevel(1);
    }
    return Py::Object();
}

// Run ScriptAnimFrame() on the interpreter's argument tuple.
//
// A C++ exception becomes a null return with the indicator the throw site posted, which is the
// shape every invoker below shares.
// 0x00408a38
PyObject *PyInvokeAnimFrame(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptAnimFrame(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    }
}

// Run ScriptAnimSpeed() on the interpreter's argument tuple.
// 0x00408be8
PyObject *PyInvokeAnimSpeed(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptAnimSpeed(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    }
}

// Run ScriptAnimOffset() on the interpreter's argument tuple.
// 0x00408d98
PyObject *PyInvokeAnimOffset(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptAnimOffset(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    }
}

// Run ScriptAnimMinmax() on the interpreter's argument tuple.
// 0x00408f48
PyObject *PyInvokeAnimMinmax(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptAnimMinmax(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    }
}

// Run ScriptTestArena() on the interpreter's argument tuple.
// 0x004090f8
PyObject *PyInvokeTestArena(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptTestArena(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    }
}

} // namespace
