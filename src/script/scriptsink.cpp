#include "script/scriptsink.h"

#include "msg/scriptmsg.h"
#include "os/hxstr.h"
#include "script/scripthost.h"

// 0x00118a08
ScriptSink::~ScriptSink() {
}

// 0x00118b50
void ScriptSink::HandleMessage(Message *pMsg) {
    if (pMsg->Type() != g_nScriptMsgType) {
        return;
    }

    const ScriptMsg *pScriptMsg = static_cast<const ScriptMsg *>(pMsg);
    RunScript(HxStr(pScriptMsg->mScript.mStr != nullptr ? pScriptMsg->mScript.mStr : ""));
}
