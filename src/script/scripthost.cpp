#include "script/scripthost.h"

#include <exception>
#include <stdarg.h>

#include "os/log.h"
#include "script/cxx/config.h"
#include "script/pyshell.h"
#include "script/scripteval.h"
#include "script/scripttemplatemap.h"

PyShell *g_pPyShell;

// 0x0050d588
void GetPythonScriptHost() {
    if (g_pPyShell != nullptr) {
        return;
    }
    PyShell *pHost = new PyShell();
    pHost->RunMasterInitScript();
    g_pPyShell = pHost;
}

// 0x0050d608
// The destructor is inlined rather than called, which is why the interpreter teardown
// reads as part of this body in the disassembly.
void DestroyPythonScriptHost() {
    delete g_pPyShell;
    g_pPyShell = nullptr;
}

// 0x0050d698
void InvokeMasterInitScript() {
    g_pPyShell->RunMasterInitScript();
}

// 0x00508f30
HxStr GetPythonErrorText() {
    try {
        g_pPyShell->ReportError(HxStr(""), 1);
    } catch (std::exception &error) {
        return HxStr("python error: ") + error.what();
    }
    return HxStr("no python exception found");
}

// 0x00509b00
void RunScript(const HxStr &script) {
    g_pPyShell->Eval(script, Py_file_input);
}

// 0x005099b0
void CallScriptTemplate(int nTemplate, ...) {
    va_list args;
    va_start(args, nTemplate);
    const HxStr text = FormatMessage(GetScriptTemplate(nTemplate), args);
    va_end(args);
    g_pPyShell->Eval(text, Py_file_input);
}
