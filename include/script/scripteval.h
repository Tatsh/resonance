#pragma once

#include "script/cxx/object.h"

class HxStr;
class PyShell;
namespace Py {
class Tuple;
} // namespace Py

/**
 * @file
 *
 * Script entry points whose result is a Python handle.
 *
 * These sit apart from `script/scripthost.h` so that a caller wanting only RunScript() or one of
 * the void entry points does not pull the interpreter headers in through PyCXX. ScriptSink and
 * Application are both such callers.
 */

/**
 * Single interpreter host, null until GetPythonScriptHost() creates it.
 *
 * Every free entry point of the script layer reads this global directly rather than through the
 * accessor, so a call made before creation dereferences null. Application::Run() creates the host
 * early enough that no recovered caller does that.
 *
 * @ghidraAddress 0x0070b288
 */
extern PyShell *g_pPyShell;

/**
 * Call one registered script template as an expression and take its value.
 *
 * The same shape as CallScriptTemplate(), running with `Py_eval_input` and returning the result.
 *
 * Not reconstructed. The body needs the template registry reader at `0x00466438` and the HxStr
 * formatter at `0x005e4148`, and neither belongs to this subsystem.
 *
 * @param nTemplate The template identifier.
 * @return A handle on the result.
 * @ghidraAddress 0x0050a750
 */
Py::Object EvalScriptTemplate(int nTemplate, ...);

/**
 * Run one piece of script text as an expression and take its value.
 *
 * @param expression The text to evaluate.
 * @return A handle on the result.
 * @ghidraAddress 0x0050d6c0
 */
Py::Object EvalScriptExpression(const HxStr &expression);

/**
 * Call a function the scripts define at module level, by name.
 *
 * The name is looked up in PyShell::mDict. A name the dictionary does not have leaves the callee
 * `None`, and the call then raises inside the interpreter. The lookup and the item fetch are
 * released PyCXX's inline `hasKey()` and `getItem()`, expanded in place. No call site survives in
 * the shipped program, and the name is inferred.
 *
 * @param name The function name.
 * @param args The positional arguments, taken by value.
 * @return A handle on the call's result.
 * @ghidraAddress 0x0050a868
 */
Py::Object CallScriptFunction(const HxStr &name, Py::Tuple args);
