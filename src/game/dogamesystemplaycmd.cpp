#include "game/dogamesystemplaycmd.h"

#include <iostream>

#include "app/application.h"
#include "game/gamemanagerimpl.h"
#include "sch/commandfactory.h"

namespace {

constexpr int kDoGameSystemPlayCmdId = 4;

// NTSC-U/C: 0x006682c0, PAL: 0x006a8e40
const Sch::CommandFactory kDoGameSystemPlayCmdFactory(kDoGameSystemPlayCmdId,
                                                      DoGameSystemPlayCmd::New);

} // namespace

// NTSC-U/C: 0x006682b8, PAL: 0x006a8e38
int DoGameSystemPlayCmd::sCmdID = kDoGameSystemPlayCmdId;

Sch::Command *DoGameSystemPlayCmd::New() {
    return new DoGameSystemPlayCmd;
}

int DoGameSystemPlayCmd::CmdID() {
    return sCmdID;
}

void DoGameSystemPlayCmd::Execute() {
    Application::shared()->GetGameManager()->StartPlay();
}

void DoGameSystemPlayCmd::Print(std::ostream &stream) {
    stream << "{" << "DoGameSystemPlayCmd" << "}";
}

void DoGameSystemPlayCmd::saveGuts(OBStream &) const {
}

void DoGameSystemPlayCmd::restoreGuts(IBStream &) {
}
