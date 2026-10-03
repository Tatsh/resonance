#include "app/application.h"

#include "game/gamemanagerimpl.h"
#include "memcard/saveicon.h"
#include "script/scripthost.h"

namespace {

// The compiler generated the initialiser and destructor pair at 0x00198c58 for this definition.
// NTSC-U/C: 0x00680f50, PAL: 0x006c21a0
Application g_app;

} // namespace

// NTSC-U/C: 0x00198cb0, PAL: 0x0019e9e0
Application::~Application() {
}

// NTSC-U/C: 0x00198d20, PAL: 0x0019ea50
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

// NTSC-U/C: 0x00198da0, PAL: 0x0019ead0
int Application::ExitInstance() {
#ifdef VIDEO_STANDARD_PAL
    Shutdown();
    DestroyPythonScriptHost();
#endif
    return 0;
}

// NTSC-U/C: 0x00198da8, PAL: 0x0019eaf8
Application *Application::shared() {
    return &g_app;
}
