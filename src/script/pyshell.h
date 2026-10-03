#pragma once

#include "script/cxx/dict.h"
#include "script/cxx/object.h"
#include "script/pyinterpreter.h"

class HxStr;

/**
 * Host of the embedded Python interpreter, and the one object the game runs script through.
 *
 * The title comes from the diagnostic the constructor reports when start-up throws,
 * `while initializing PyShell`. The class is not polymorphic, has no RTTI descriptor, and has no
 * embedded `__FILE__`, so the title is an inference from that one string rather than a recovered
 * identifier.
 *
 * The object is twelve bytes, and GetPythonScriptHost() at `0x0050d588` reserves exactly that
 * much for the single instance. The four bytes at `+0x00` are the interpreter's lifetime, a member
 * with no data. No routine reads or writes them, and the member's constructor and destructor
 * bracket the dictionary at `+0x04`.
 */
class PyShell {
private:
    PyInterpreter mInterpreter; // +0x00

public:
    /**
     * Bring the interpreter up and prepare the script environment.
     *
     * The sequence is fixed. `Py_NoSiteFlag` is set, the interpreter starts, the namespace of
     * `__main__` becomes the dictionary below, `import sys` runs, two `sys.path.append` calls add
     * `gscripts` and `gscripts/hx` under the prefix GetFreqRoot() supplies, and the game's own
     * `hx` extension module registers last.
     *
     * A Py::Exception thrown anywhere after the dictionary exists is caught and reported through
     * ReportError() with the context `while initializing PyShell`, so a script failure at
     * start-up halts the machine with a message rather than propagating.
     *
     * @ghidraAddress NTSC-U/C: 0x005072e8
     * @ghidraAddress PAL: 0x005461e8
     */
    PyShell();

    /**
     * Release the namespace dictionary, and then shut the interpreter down through the
     * interpreter member.
     *
     * @ghidraAddress NTSC-U/C: 0x0050d480
     * @ghidraAddress PAL: 0x0054c938
     */
    ~PyShell();

    /**
     * Compile and run script text in the shared namespace.
     *
     * Globals and locals are the same dictionary. A null result is reported through ReportError()
     * with the source as the context, which halts the machine, so the returned handle is only
     * ever reached on success.
     *
     * @param source The text to run.
     * @param nStartSymbol The grammar start symbol, `Py_file_input` for a statement and
     *                     `Py_eval_input` for an expression.
     * @return A handle on the result.
     * @ghidraAddress NTSC-U/C: 0x00508ca8
     * @ghidraAddress PAL: 0x00547e08
     */
    Py::Object Eval(const HxStr &source, int nStartSymbol);

    /**
     * Run the master script.
     *
     * The path is `Global/GrvScript.py` under the prefix GetFreqRoot() supplies. The file is
     * opened with `fopen` in mode `r` and handed straight to `PyRun_File()`, so a script is read
     * through the game's own file layer rather than through any interpreter hook. The shipped
     * script imports `os`, `os.path`, and `hx`, calls `hx.get_freq_root()`, and runs
     * `global/defaults.py`.
     *
     * @ghidraAddress NTSC-U/C: 0x00508de8
     * @ghidraAddress PAL: 0x00547f48
     */
    void RunMasterInitScript();

    /**
     * Report a script failure and halt the machine.
     *
     * The Python error indicator is drained into a message, the message is decorated with the
     * context, and the machine stops. The routine never returns to its caller, because Fatal()
     * does not return.
     *
     * The receiver is unused. The routine reads only its two arguments, and it is a member rather
     * than a free function because the call sites pass the receiver.
     *
     * @param context Text describing what was being attempted.
     * @param bWithTraceback Non-zero to format the traceback through `hxutl.traceback_str`, which
     *                       also consumes the context. Zero appends the context directly instead.
     * @ghidraAddress NTSC-U/C: 0x00507f58
     * @ghidraAddress PAL: 0x00547018
     */
    void ReportError(const HxStr &context, int bWithTraceback);

    /**
     * Namespace every script runs in, which is the dictionary of `__main__`.
     *
     * Public because the free functions of the script layer read it directly off the singleton
     * and the image has no accessor for it. `0x0050a868` is one such reader.
     *
     * +0x04
     */
    Py::Dict mDict;
};
