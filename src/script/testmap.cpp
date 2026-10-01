#include <exception>

#include "game/playmaprepeatring.h"
#include "script/cxx/config.h"
#include "script/cxx/int.h"
#include "script/cxx/object.h"
#include "script/cxx/tuple.h"
#include "script/scriptfunc.h"

namespace {

// Probes per section the test map checks.
constexpr int kTestMapProbeCount = 20;

// Entries of the result tuple.
constexpr int kTestMapResultSize = 61;

// Build one probe pair: the bar and the section holding it.
// 0x0012c040
Py::Tuple BuildTestMapProbeList(PlayMapRepeatRing *pRing, long long nBar) {
    Py::Tuple probe(2);
    probe.setItem(0, Py::Int(nBar));
    probe.setItem(1, Py::Int(pRing->MapBar(static_cast<int>(nBar))));
    return probe;
}

// Build a synthetic map and probe it.
//
// Lays four sections over a repeat ring, probes three bars per section step, and returns the
// pairs. The tuple arrives by value and is released here.
// 0x0012c448
Py::Object ScriptTestMap([[maybe_unused]] Py::Tuple args) {
    PlayMapRepeatRing ring;
    ring.AddStep(5, HxStr(""));
    ring.AddStep(10, HxStr(""));
    ring.AddStep(20, HxStr(""));
    ring.AddStep(25, HxStr(""));
    ring.SetBarCount(0);
    ring.EndLoop(2);
    ring.EndLoop(0xC);
    ring.EndLoop(0x2F);
    ring.EndLoop(0x31);
    ring.EndLoop(0x46);
    ring.StartLoop(0x49);
    ring.StartLoop(0x4A);
    ring.EndLoop(0x4B);
    Py::Tuple result(kTestMapResultSize);
    for (int i = 0; i < kTestMapProbeCount; ++i) {
        result.setItem(i * 3, BuildTestMapProbeList(&ring, i * 5 + 4));
        result.setItem(i * 3 + 1, BuildTestMapProbeList(&ring, (i + 1) * 5));
        result.setItem(i * 3 + 2, BuildTestMapProbeList(&ring, (i + 1) * 5 + 1));
    }
    return result;
}

// Run ScriptTestMap() on the interpreter's argument tuple.
// 0x0012cbc8
PyObject *PyInvokeTestMap(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptTestMap(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    } catch (std::exception &error) {
        PyErr_SetString(PyExc_RuntimeError, const_cast<char *>(error.what()));
        return nullptr;
    }
}

// The script interface this file exports, registered in static initialisation.
// 0x0012d178
const ScriptFunc kTestMapFunc("test_map", PyInvokeTestMap);

} // namespace
