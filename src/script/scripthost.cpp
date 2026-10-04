#include "script/scripthost.h"

#include <exception>
#include <stdarg.h>

#include "os/log.h"
#include "script/cxx/config.h"
#include "script/pyshell.h"
#include "script/scripteval.h"
#include "script/scripttemplatemap.h"

PyShell *g_pPyShell;

void GetPythonScriptHost() {
    if (g_pPyShell != nullptr) {
        return;
    }
    PyShell *pHost = new PyShell();
    pHost->RunMasterInitScript();
    g_pPyShell = pHost;
}

void DestroyPythonScriptHost() {
    delete g_pPyShell;
    g_pPyShell = nullptr;
}

void InvokeMasterInitScript() {
    g_pPyShell->RunMasterInitScript();
}

HxStr GetPythonErrorText() {
    try {
        g_pPyShell->ReportError(HxStr(""), 1);
    } catch (std::exception &error) {
        return HxStr("python error: ") + error.what();
    }
    return HxStr("no python exception found");
}

void RunScript(const HxStr &script) {
    g_pPyShell->Eval(script, Py_file_input);
}

void CallScriptTemplate(int nTemplate, ...) {
    va_list args;
    va_start(args, nTemplate);
    const HxStr text = FmtImp(Resid2Str(nTemplate), args);
    va_end(args);
    g_pPyShell->Eval(text, Py_file_input);
}
