#include "script/scripttemplatemap.h"

#include <utility>

// NTSC-U/C: 0x00773fb0, PAL: 0x007506a8
ScriptTemplateMap g_scriptTemplates;

// NTSC-U/C: 0x005a4f20, PAL: 0x0054fe40
void ScriptTemplateMap::Add(int nTemplate, const HxStr &text) {
    mTemplates.insert(std::make_pair(nTemplate, text));
}

// NTSC-U/C: 0x005a5808, PAL: 0x00550768
HxStr ScriptTemplateMap::Find(int nTemplate) {
    const auto it = mTemplates.find(nTemplate);
    if (it == mTemplates.end()) {
        return HxStr("");
    }
    return it->second;
}

// NTSC-U/C: 0x00466470, PAL: 0x004a3ea0
void RegisterScriptTemplate(int nTemplate, const HxStr &text) {
    g_scriptTemplates.Add(nTemplate, text);
}

// NTSC-U/C: 0x00466438, PAL: 0x004a3e68
HxStr GetScriptTemplate(int nTemplate) {
    return g_scriptTemplates.Find(nTemplate);
}
