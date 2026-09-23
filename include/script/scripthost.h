#pragma once

#include "os/hxstr.h"

/**
 * Identifier of the `autoexec()` call template, which Application::Run() invokes once the services
 * exist.
 */
constexpr int kScriptTemplateAutoexec = 0xc9;

/**
 * Register every script call template.
 *
 * The routine registers around two hundred templates against integer identifiers, among them
 * `autoexec()`, `get_screen_config()[0]`, and `level_list['%s'].stage`, plus the two exception
 * templates that title Application::Run() and Application::ExitInstance(). The registry itself is
 * not reconstructed.
 *
 * @ghidraAddress 0x004016c8
 */
void RegisterScriptCallTemplates();

/**
 * Create the interpreter host on first use and run the master script.
 *
 * The host is created with twelve bytes of storage, constructed, handed to
 * PyShell::RunMasterInitScript(), and only then stored in g_pPyShell, so the master script runs
 * against a host the global does not yet publish. Application::Run() invokes this for the
 * creation.
 *
 * The routine returns nothing. Its early exit and its creation path disagree about what the
 * result register holds, so the value is scratch rather than a return.
 *
 * @ghidraAddress 0x0050d588
 */
void GetPythonScriptHost();

/**
 * Destroy the interpreter host and clear the global.
 *
 * The destructor is inlined here rather than called, so the body repeats PyShell's teardown.
 *
 * @ghidraAddress 0x0050d608
 */
void DestroyPythonScriptHost();

/**
 * Run the master script against the existing host.
 *
 * A second entry point onto PyShell::RunMasterInitScript(), separate from the one
 * GetPythonScriptHost() takes during creation.
 *
 * @ghidraAddress 0x0050d698
 */
void InvokeMasterInitScript();

/**
 * Call one registered script template as a statement.
 *
 * The identifier selects a template, and the remaining arguments fill its format placeholders.
 * The formatted text runs with `Py_file_input`.
 *
 * The routine is variadic, and the register homing in its prologue is not the only evidence for
 * that. The homed block's address is passed on to the formatter as the argument list, which is
 * what proves the arguments are read rather than merely reserved.
 *
 * Not reconstructed. The body reads the template through GetScriptTemplate() and needs the HxStr
 * formatter at `0x005e4148`, which does not belong to this subsystem.
 *
 * @param nTemplate The template identifier.
 * @ghidraAddress 0x005099b0
 */
void CallScriptTemplate(int nTemplate, ...);

/**
 * Run one piece of script text as a statement.
 *
 * The result is discarded. ScriptSink uses this for the text a ScriptMsg supplies.
 *
 * @param script The text to run.
 * @ghidraAddress 0x00509b00
 */
void RunScript(const HxStr &script);

/**
 * Describe the pending Python error as text.
 *
 * The routine reports the error through PyShell::ReportError() with an empty context, which halts
 * the machine, so the text it returns is unreachable on the normal path. The plain return is
 * `no python exception found`, and a handler inside it prefixes `python error: ` to a message it
 * recovers from the caught object.
 *
 * Not reconstructed. The caught type has a virtual accessor at its third slot and the type is not
 * identified, so the handler cannot be written faithfully.
 *
 * @return The text.
 * @ghidraAddress 0x00508f30
 */
HxStr GetPythonErrorText();
