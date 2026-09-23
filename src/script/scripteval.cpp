#include "script/scripteval.h"

#include <stdarg.h>

#include "os/hxstr.h"
#include "os/log.h"
#include "script/cxx/callable.h"
#include "script/cxx/config.h"
#include "script/cxx/dict.h"
#include "script/cxx/fromapi.h"
#include "script/cxx/string.h"
#include "script/cxx/tuple.h"
#include "script/pyshell.h"
#include "script/scripttemplatemap.h"

// 0x0050d6c0
Py::Object EvalScriptExpression(const HxStr &expression) {
    return g_pPyShell->Eval(expression, Py_eval_input);
}

// 0x0050a750
Py::Object EvalScriptTemplate(int nTemplate, ...) {
    va_list args;
    va_start(args, nTemplate);
    const HxStr text = FormatMessage(GetScriptTemplate(nTemplate), args);
    va_end(args);
    return g_pPyShell->Eval(text, Py_eval_input);
}

// 0x0050a868
Py::Object CallScriptFunction(const HxStr &name, Py::Tuple args) {
    Py::Object function;
    {
        const Py::Dict dict = g_pPyShell->mDict;
        Py::Object key;
        key = Py::String(name);
        if (PyMapping_HasKey(dict.mPtr, key.mPtr)) {
            function = Py::Object(Py::FromAPI(PyObject_GetItem(dict.mPtr, key.mPtr)).mPtr);
        }
    }
    const Py::Callable callable(function);
    return Py::Object(Py::FromAPI(PyObject_CallObject(callable.mPtr, args.mPtr)).mPtr);
}
