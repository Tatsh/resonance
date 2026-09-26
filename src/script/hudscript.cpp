#include "app/hudanimramp.h"
#include "app/hudpanel.h"
#include "app/linearramp.h"
#include "app/overlay.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "rnd/manager.h"
#include "script/cxx/config.h"
#include "script/cxx/int.h"
#include "script/cxx/object.h"
#include "script/cxx/string.h"
#include "script/cxx/tuple.h"
#include "script/cxx/typeerror.h"

namespace {

// Move the activator label to a value.
// 0x00420d00
Py::Object ScriptActivatorLabel(const Py::Tuple &args) {
    if (g_pOverlay == nullptr) {
        return Py::Object();
    }
    if (args.length() != 1) {
        throw Py::TypeError(HxStr(FormatString("wrong # args for activator_label")));
    }
    Py::Object number(PyNumber_Float(args.getItem(0).mPtr));
    const float flValue = static_cast<float>(PyFloat_AsDouble(number.mPtr));
    g_pOverlay->mPanel->mLabelSwap.mRamp.SetTarget(flValue);
    return Py::Object();
}

// Put the highlight box on a rectangle at once.
// 0x00421238
Py::Object ScriptHighlightSnap(const Py::Tuple &args) {
    if (g_pOverlay == nullptr) {
        return Py::Object();
    }
    if (args.length() != 4) {
        throw Py::TypeError(HxStr(FormatString("wrong # args for highlight_snap")));
    }
    Py::Object leftNumber(PyNumber_Float(args.getItem(0).mPtr));
    const float flLeft = static_cast<float>(PyFloat_AsDouble(leftNumber.mPtr));
    Py::Object topNumber(PyNumber_Float(args.getItem(1).mPtr));
    const float flTop = static_cast<float>(PyFloat_AsDouble(topNumber.mPtr));
    Py::Object rightNumber(PyNumber_Float(args.getItem(2).mPtr));
    const float flRight = static_cast<float>(PyFloat_AsDouble(rightNumber.mPtr));
    Py::Object bottomNumber(PyNumber_Float(args.getItem(3).mPtr));
    const float flBottom = static_cast<float>(PyFloat_AsDouble(bottomNumber.mPtr));
    g_pOverlay->mPanel->mHighlight.JumpTo(flLeft, flTop, flRight, flBottom);
    return Py::Object();
}

// Glide the highlight box to a rectangle over a time.
// 0x004220f8
Py::Object ScriptHighlightSlide(const Py::Tuple &args) {
    if (g_pOverlay == nullptr) {
        return Py::Object();
    }
    if (args.length() != 5) {
        throw Py::TypeError(HxStr(FormatString("wrong # args for highlight_slide")));
    }
    Py::Object leftNumber(PyNumber_Float(args.getItem(0).mPtr));
    const float flLeft = static_cast<float>(PyFloat_AsDouble(leftNumber.mPtr));
    Py::Object topNumber(PyNumber_Float(args.getItem(1).mPtr));
    const float flTop = static_cast<float>(PyFloat_AsDouble(topNumber.mPtr));
    Py::Object rightNumber(PyNumber_Float(args.getItem(2).mPtr));
    const float flRight = static_cast<float>(PyFloat_AsDouble(rightNumber.mPtr));
    Py::Object bottomNumber(PyNumber_Float(args.getItem(3).mPtr));
    const float flBottom = static_cast<float>(PyFloat_AsDouble(bottomNumber.mPtr));
    Py::Object durationNumber(PyNumber_Float(args.getItem(4).mPtr));
    const float flDuration = static_cast<float>(PyFloat_AsDouble(durationNumber.mPtr));
    g_pOverlay->mPanel->mHighlight.MoveTo(flLeft, flTop, flRight, flBottom, flDuration);
    return Py::Object();
}

// Show or hide the highlight box.
//
// Only a value of 1 shows.
// 0x004232d8
Py::Object ScriptHighlightSetshow(const Py::Tuple &args) {
    if (g_pOverlay == nullptr) {
        return Py::Object();
    }
    if (args.length() != 1) {
        throw Py::TypeError(HxStr(FormatString("wrong # args for highlight_show")));
    }
    const long nShow = Py::Int(args.getItem(0));
    g_pOverlay->mPanel->mHighlight.SetShowing(nShow == 1 ? 1 : 0);
    return Py::Object();
}

// Show or hide the analog stick prompt.
//
// Only a value of 1 shows.
// 0x004236f8
Py::Object ScriptAnalogStickSetshow(const Py::Tuple &args) {
    if (g_pOverlay == nullptr) {
        return Py::Object();
    }
    if (args.length() != 1) {
        throw Py::TypeError(HxStr(FormatString("wrong # args for analog_stick_show")));
    }
    const long nShow = Py::Int(args.getItem(0));
    g_pOverlay->mPanel->mAnalogStick.SetShowing(nShow == 1 ? 1 : 0);
    return Py::Object();
}

// Show or hide the controller group.
//
// Only a value of 1 shows. The length error repeats the analog stick message, as the image
// does.
// 0x00423b18
Py::Object ScriptControllerSetshow(const Py::Tuple &args) {
    if (g_pOverlay == nullptr) {
        return Py::Object();
    }
    if (args.length() != 1) {
        throw Py::TypeError(HxStr(FormatString("wrong # args for analog_stick_show")));
    }
    const long nShow = Py::Int(args.getItem(0));
    g_pOverlay->mPanel->mTcGroup.SetShowing(nShow == 1 ? 1 : 0);
    return Py::Object();
}

// Put the material for one stick motion on the prompt.
//
// The binary expands the two branches of HudAnalogStick::SetMotion() inline; calling it repeats
// them without duplicating the body.
// 0x00423f40
Py::Object ScriptAnalogStickSetmat(const Py::Tuple &args) {
    if (g_pOverlay == nullptr) {
        return Py::Object();
    }
    if (args.length() != 1) {
        throw Py::TypeError(HxStr(FormatString("wrong # args for analog_stick_show")));
    }
    Py::Object element = args.getItem(0);
    Py::String text(element);
    HxStr motion = text;
    g_pOverlay->mPanel->mAnalogStick.SetMotion(motion);
    return Py::Object();
}

// Run ScriptActivatorLabel() on the interpreter's argument tuple.
// 0x004243c0
PyObject *PyInvokeActivatorLabel(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptActivatorLabel(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    }
}

// Run ScriptHighlightSnap() on the interpreter's argument tuple.
// 0x00424570
PyObject *PyInvokeHighlightSnap(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptHighlightSnap(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    }
}

// Run ScriptHighlightSlide() on the interpreter's argument tuple.
// 0x00424720
PyObject *PyInvokeHighlightSlide(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptHighlightSlide(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    }
}

// Run ScriptHighlightSetshow() on the interpreter's argument tuple.
// 0x004248d0
PyObject *PyInvokeHighlightSetshow(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptHighlightSetshow(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    }
}

// Run ScriptAnalogStickSetshow() on the interpreter's argument tuple.
// 0x00424a80
PyObject *PyInvokeAnalogStickSetshow(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptAnalogStickSetshow(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    }
}

// Run ScriptControllerSetshow() on the interpreter's argument tuple.
// 0x00424c30
PyObject *PyInvokeControllerSetshow(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptControllerSetshow(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    }
}

// Run ScriptAnalogStickSetmat() on the interpreter's argument tuple.
// 0x00424de0
PyObject *PyInvokeAnalogStickSetmat(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptAnalogStickSetmat(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    }
}

} // namespace
