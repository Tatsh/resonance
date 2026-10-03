#include "script/scripthost.h"

#include <exception>
#include <stdarg.h>

#include "os/log.h"
#include "script/cxx/config.h"
#include "script/pyshell.h"
#include "script/scripteval.h"
#include "script/scripttemplatemap.h"

PyShell *g_pPyShell;

// NTSC-U/C: 0x0050d588, PAL: 0x0054ca40
void GetPythonScriptHost() {
    if (g_pPyShell != nullptr) {
        return;
    }
    PyShell *pHost = new PyShell();
    pHost->RunMasterInitScript();
    g_pPyShell = pHost;
}

// NTSC-U/C: 0x0050d608, PAL: 0x0054cac0
// The destructor is inlined rather than called, which is why the interpreter teardown
// reads as part of this body in the disassembly.
void DestroyPythonScriptHost() {
    delete g_pPyShell;
    g_pPyShell = nullptr;
}

// NTSC-U/C: 0x0050d698, PAL: 0x0054cb50
void InvokeMasterInitScript() {
    g_pPyShell->RunMasterInitScript();
}

// NTSC-U/C: 0x00508f30, PAL: 0x005480c8
HxStr GetPythonErrorText() {
    try {
        g_pPyShell->ReportError(HxStr(""), 1);
    } catch (std::exception &error) {
        return HxStr("python error: ") + error.what();
    }
    return HxStr("no python exception found");
}

// NTSC-U/C: 0x00509b00, PAL: 0x00548e28
void RunScript(const HxStr &script) {
    g_pPyShell->Eval(script, Py_file_input);
}

// NTSC-U/C: 0x005099b0, PAL: 0x00548ca0
void CallScriptTemplate(int nTemplate, ...) {
    va_list args;
    va_start(args, nTemplate);
    const HxStr text = FormatMessage(GetScriptTemplate(nTemplate), args);
    va_end(args);
    g_pPyShell->Eval(text, Py_file_input);
}
