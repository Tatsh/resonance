#include "app/scriptsink.h"

#include "app/globals.h"
#include "msg/scriptmsg.h"
#include "script/scripthost.h"

ScriptSink::ScriptSink(Globals *pOwner) : mGlobals(pOwner) {
}

// The destructor slot of the table at `0x007cee50` holds the inherited `MsgSink` destructor
// at `0x00118a08`, which installs the base table at `0x007ccc40`; this class declares no
// destructor of its own.

// 0x00118c00
ScriptSink *ScriptSink::CreateInstance(Globals *pOwner) {
    return new ScriptSink(pOwner);
}

// 0x00118ad0
// The receiver is unused, which is why the body reads only the message.
void ScriptSink::RunMessageScript(Message *pMsg) {
    const char *pszScript = static_cast<ScriptMsg *>(pMsg)->mScript.mStr;
    if (pszScript == nullptr) {
        pszScript = g_szEmptyString;
    }
    RunScript(HxStr(pszScript));
}

// 0x00118b50
void ScriptSink::HandleMessage(Message *pMsg) {
    if (pMsg->Type() != g_nScriptMsgType) {
        return;
    }
    RunMessageScript(pMsg);
}
