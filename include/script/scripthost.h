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
 * Create the embedded interpreter's host object on first use.
 *
 * The interpreter is out of scope, so only this accessor is declared. Application::Run() invokes
 * it for the creation and discards the result.
 *
 * @return The host object.
 * @ghidraAddress 0x0050d588
 */
void *GetPythonScriptHost();

/**
 * Call one registered script template.
 *
 * The identifier selects a template, and the remaining arguments fill its format placeholders.
 *
 * @param nTemplate The template identifier.
 * @ghidraAddress 0x005099b0
 */
void CallScriptTemplate(int nTemplate, ...);

/**
 * Run one line of script text.
 *
 * @param script The text to run.
 * @ghidraAddress 0x00509b00
 */
void RunScript(const HxStr &script);

/**
 * Text that stands in for a null string.
 *
 * ScriptSink falls back on this when a message supplies no script text. The save-icon loader at
 * `0x00177608` reads the same global for the same purpose, so it is shared rather than specific to
 * the script layer and may belong elsewhere.
 *
 * @ghidraAddress 0x006fbd10
 */
extern const char *g_pszDefaultText;
