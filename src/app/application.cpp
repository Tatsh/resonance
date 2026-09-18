#include "app/application.h"

#include "memcard/saveicon.h"
#include "script/scripthost.h"

namespace {

// 0x00680f50. The compiler generated the initialiser and destructor pair at 0x00198c58 for this
// definition.
Application g_app;

} // namespace

// 0x00198cb0
Application::~Application() {
}

// 0x00198d20
int Application::Run() {
    RegisterScriptCallTemplates();
    GetPythonScriptHost(); // Yes, the binary discards this call's result.
    Init();
    CallScriptTemplate(kScriptTemplateAutoexec);
    LoadSaveIcon();
    // The binary re-reads the singleton for each of these two calls rather than using this.
    Application::shared()->GetGameManager()->Start();
    Application::shared()->RunMainLoop();
    return 1;
}

// 0x00198da0
int Application::ExitInstance() {
    return 0;
}

// 0x00198da8
Application *Application::shared() {
    return &g_app;
}
