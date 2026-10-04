#include "script/scripttemplatemap.h"

#include <utility>

// NTSC-U/C: 0x00773fb0, PAL: 0x007506a8
ScriptTemplateMap g_scriptTemplates;

void ScriptTemplateMap::Add(int nTemplate, const HxStr &text) {
    mTemplates.insert(std::make_pair(nTemplate, text));
}

HxStr ScriptTemplateMap::Find(int nTemplate) {
    const auto it = mTemplates.find(nTemplate);
    if (it == mTemplates.end()) {
        return HxStr("");
    }
    return it->second;
}

void RegisterScriptTemplate(int nTemplate, const HxStr &text) {
    g_scriptTemplates.Add(nTemplate, text);
}

HxStr Resid2Str(unsigned int nResid) {
    return g_scriptTemplates.Find(nResid);
}
