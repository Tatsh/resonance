#pragma once

#include <map>

#include "os/hxstr.h"

/**
 * Registry of script call templates by integer identifier.
 *
 * The class emits no RTTI and the image records no name for it, so the name is inferred from the
 * one instance, g_scriptTemplates, and what RegisterScriptCallTemplates() stores there. Its unit
 * (static initialiser `0x005a57c8`) holds the instance, the two members below, and the
 * `std::map<int, HxStr>` instantiations they reach, which bill their nodes to `stl_maptree`. The
 * member is the whole object: the constructor at `0x005a4e70` and the destructor at `0x005a4ff8`
 * build and tear down nothing but the map, and both are compiler-generated.
 */
class ScriptTemplateMap {
public:
    /**
     * Register a template, retaining any template already registered under the identifier.
     *
     * @param nTemplate The identifier.
     * @param text The template text.
     * @ghidraAddress 0x005a4f20
     */
    void Add(int nTemplate, const HxStr &text);

    /**
     * Look a template up.
     *
     * @param nTemplate The identifier.
     * @return A copy of the template, or an empty string when none is registered.
     * @ghidraAddress 0x005a5808
     */
    HxStr Find(int nTemplate);

private:
    std::map<int, HxStr> mTemplates; // +0x00
};

/**
 * Every script call template.
 *
 * @ghidraAddress 0x00773fb0
 */
extern ScriptTemplateMap g_scriptTemplates;

/**
 * Register a script call template in g_scriptTemplates.
 *
 * RegisterScriptCallTemplates() and the routine at `0x005e1210` are the callers, each passing a
 * temporary built from a literal.
 *
 * @param nTemplate The identifier.
 * @param text The template text.
 * @ghidraAddress 0x00466470
 */
void RegisterScriptTemplate(int nTemplate, const HxStr &text);

/**
 * Look a script call template up in g_scriptTemplates.
 *
 * CallScriptTemplate(), the script evaluators, and the configuration queries read templates through
 * this routine.
 *
 * @param nTemplate The identifier.
 * @return A copy of the template, or an empty string when none is registered.
 * @ghidraAddress 0x00466438
 */
HxStr GetScriptTemplate(int nTemplate);
