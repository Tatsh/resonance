#include "script/scripteval.h"

#include "script/cxx/config.h"

// 0x0050d6c0
Py::Object EvalScriptExpression(const HxStr &expression) {
    return g_pPyShell->Eval(expression, Py_eval_input);
}
