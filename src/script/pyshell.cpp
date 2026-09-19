#include "script/pyshell.h"

#include "os/hostmode.h"
#include "os/hxstr.h"
#include "os/log.h"
#include "script/cxx/callable.h"
#include "script/cxx/config.h"
#include "script/cxx/exception.h"
#include "script/cxx/fromapi.h"
#include "script/cxx/module.h"
#include "script/cxx/string.h"
#include "script/cxx/tuple.h"
#include "script/hxmodule.h"

// The two sys.path entries the interpreter needs before any game script runs.
static const char *const kScriptDirectory = "gscripts')";
static const char *const kHelperDirectory = "gscripts/hx')";

// Module and attribute the traceback formatter lives in. The formatter is Python rather than C,
// so the error path closes back into the interpreter it is reporting on.
static const char *const kTracebackModule = "hxutl";
static const char *const kTracebackFunction = "traceback_str";

// Master script, run relative to the prefix GetFreqRoot() supplies.
static const char *const kMasterScriptPath = "Global/GrvScript.py";

// Number of arguments hxutl.traceback_str takes.
static const int kTracebackArgCount = 2;

// 0x005072e8
PyShell::PyShell() {
    Py_NoSiteFlag = 1;
    Py_Initialize();
    try {
        Py::Module main(HxStr("__main__"));
        mDict = main.getDict();
        Eval(HxStr("import sys"), Py_file_input);

        HxStr appendCall("sys.path.append");
        HxStr root = GetFreqRoot();

        // The binary composes each of these with operator+, so it builds one temporary per step
        // and then materialises a further HxStr from the finished buffer for the call. The
        // appends below produce the same text in one string.
        HxStr scriptPath(appendCall);
        scriptPath += "('";
        scriptPath += root;
        scriptPath += kScriptDirectory;
        Eval(scriptPath, Py_file_input);

        HxStr helperPath(appendCall);
        helperPath += "('";
        helperPath += root;
        helperPath += kHelperDirectory;
        Eval(helperPath, Py_file_input);

        InitHxModule();
    } catch (Py::Exception &) {
        ReportError(HxStr("while initializing PyShell"), 0);
    }
}

// 0x0050d480. The binary runs Py_Finalize() after the dictionary is released rather than before,
// which is where a member or a base subobject at +0x00 would run and not where a destructor body
// runs. The four bytes at +0x00 are therefore probably a guard object holding the interpreter
// open, and the call is written here because nothing in the image establishes that class.
PyShell::~PyShell() {
    Py_Finalize();
}

// 0x00508ca8
Py::Object PyShell::Eval(const HxStr &source, int nStartSymbol) {
    char *pszSource = const_cast<char *>(source.mStr != nullptr ? source.mStr : g_szEmptyString);
    PyObject *pResult = PyRun_String(pszSource, nStartSymbol, mDict.mPtr, mDict.mPtr);
    if (pResult == nullptr) {
        ReportError(source, 1);
    }
    return Py::Object(Py::FromAPI(pResult).mPtr);
}

// 0x00508de8
void PyShell::RunMasterInitScript() {
    HxStr path = GetFreqRoot();
    path += kMasterScriptPath;
    char *pszPath = const_cast<char *>(path.mStr != nullptr ? path.mStr : g_szEmptyString);
    FILE *pFile = fopen(pszPath, "r");
    PyObject *pResult = PyRun_File(pFile, pszPath, Py_file_input, mDict.mPtr, mDict.mPtr);
    fclose(pFile);
    if (pResult == nullptr) {
        ReportError(path, 1);
    }
}

// 0x00507f58
void PyShell::ReportError(const HxStr &context, int bWithTraceback) {
    HxStr message;
    PyObject *pType = nullptr;
    PyObject *pValue = nullptr;
    PyObject *pTraceback = nullptr;
    PyErr_Fetch(&pType, &pValue, &pTraceback);

    if (pType != nullptr) {
        message = Py::Object(Py::FromAPI(pType).mPtr).as_string();
        if (pValue != nullptr) {
            message += '\n';
            message += Py::Object(Py::FromAPI(pValue).mPtr).as_string();
        }
    }

    if (bWithTraceback != 0 && pTraceback != nullptr) {
        Py::Object traceback(Py::FromAPI(pTraceback).mPtr);
        HxStr utilityName(kTracebackModule);
        Py::Module utility(utilityName);
        Py::Callable formatter(utility.getAttr(HxStr(kTracebackFunction)));
        Py::Tuple args(kTracebackArgCount);
        args.setItem(0, traceback);
        args.setItem(1, Py::String(context));
        message += '\n';
        message += Py::Object(Py::FromAPI(PyObject_CallObject(formatter.mPtr, args.mPtr)).mPtr)
                       .as_string();
    } else {
        message += '\n';
        message += context;
    }

    PyErr_Clear();
    Fatal(message.mStr != nullptr ? message.mStr : g_szEmptyString); // The message is the format.
}
