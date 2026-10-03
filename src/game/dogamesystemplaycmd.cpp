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

// NTSC-U/C: 0x00105e40, PAL: 0x00105e40
Sch::Command *DoGameSystemPlayCmd::New() {
    return new DoGameSystemPlayCmd;
}

// NTSC-U/C: 0x0010bdb8, PAL: 0x0010bf50
int DoGameSystemPlayCmd::CmdID() {
    return sCmdID;
}

// NTSC-U/C: 0x0010bd80, PAL: 0x0010bf18
void DoGameSystemPlayCmd::Execute() {
    Application::shared()->GetGameManager()->StartPlay();
}

// NTSC-U/C: 0x0010bdd8, PAL: 0x0010bf70
void DoGameSystemPlayCmd::Print(std::ostream &stream) {
    stream << "{" << "DoGameSystemPlayCmd" << "}";
}

// NTSC-U/C: 0x0010bdc8, PAL: 0x0010bf60
void DoGameSystemPlayCmd::Save(OBStream &) {
}

// NTSC-U/C: 0x0010bdd0, PAL: 0x0010bf68
void DoGameSystemPlayCmd::Load(IBStream &) {
}
