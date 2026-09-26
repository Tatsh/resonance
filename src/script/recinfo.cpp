#include "os/hxstr.h"
#include "script/cxx/config.h"
#include "script/cxx/object.h"
#include "script/cxx/string.h"
#include "script/cxx/tuple.h"
#include "stream/ibfilestream.h"

namespace {

// Read the two strings of a recording info file.
//
// The file holds two length-prefixed strings, and the result joins them with a newline. The
// tuple carries the path.
// 0x0010d1a8
Py::Object ScriptRecInfo(const Py::Tuple &args) {
    Py::Object element = args.getItem(0);
    Py::String text(element);
    HxStr path = text;
    IBFileStream stream(path);
    int nFirstLen = 0;
    int nSecondLen = 0;
    stream.Read(&nFirstLen, 4);
    stream.Read(&nSecondLen, 4);
    HxStr first;
    first.Alloc(nFirstLen);
    stream.Read(first.mStr, nFirstLen);
    HxStr second;
    second.Alloc(nSecondLen);
    stream.Read(second.mStr, nSecondLen);
    HxStr result(first);
    result += "\n";
    result += second;
    return Py::String(result);
}

// Run ScriptRecInfo() on the interpreter's argument tuple.
// 0x0010d848
PyObject *PyInvokeRecInfo(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptRecInfo(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    }
}

} // namespace
