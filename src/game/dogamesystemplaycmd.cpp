#include "game/dogamesystemplaycmd.h"

#include <iostream>

#include "app/application.h"
#include "game/gamemanagerimpl.h"
#include "sch/commandfactory.h"

namespace {

constexpr int kDoGameSystemPlayCmdId = 4;

// 0x006682c0
const Sch::CommandFactory kDoGameSystemPlayCmdFactory(kDoGameSystemPlayCmdId,
                                                      DoGameSystemPlayCmd::New);

} // namespace

// 0x006682b8
int DoGameSystemPlayCmd::sCmdID = kDoGameSystemPlayCmdId;

// 0x00105e40
Sch::Command *DoGameSystemPlayCmd::New() {
    return new DoGameSystemPlayCmd;
}

// 0x0010bdb8
int DoGameSystemPlayCmd::CmdID() {
    return sCmdID;
}

// 0x0010bd80
void DoGameSystemPlayCmd::Execute() {
    Application::shared()->GetGameManager()->StartPlay();
}

// 0x0010bdd8
void DoGameSystemPlayCmd::Print(std::ostream &stream) {
    stream << "{" << "DoGameSystemPlayCmd" << "}";
}

// 0x0010bdc8
void DoGameSystemPlayCmd::Save(OBStream &) {
}

// 0x0010bdd0
void DoGameSystemPlayCmd::Load(IBStream &) {
}
