#include "script/scripttemplatemap.h"

#include <utility>

// 0x00773fb0
ScriptTemplateMap g_scriptTemplates;

// 0x005a4f20
void ScriptTemplateMap::Add(int nTemplate, const HxStr &text) {
    mTemplates.insert(std::make_pair(nTemplate, text));
}

// 0x005a5808
HxStr ScriptTemplateMap::Find(int nTemplate) {
    const auto it = mTemplates.find(nTemplate);
    if (it == mTemplates.end()) {
        return HxStr("");
    }
    return it->second;
}

// 0x00466470
void RegisterScriptTemplate(int nTemplate, const HxStr &text) {
    g_scriptTemplates.Add(nTemplate, text);
}

// 0x00466438
HxStr GetScriptTemplate(int nTemplate) {
    return g_scriptTemplates.Find(nTemplate);
}
