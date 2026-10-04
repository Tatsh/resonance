#include "game/endrecordingcmd.h"

#include <iostream>

#include "game/gamerecorder.h"
#include "sch/commandfactory.h"

namespace {

constexpr int kEndRecordingCmdId = 6;

// NTSC-U/C: 0x006693f0, PAL: 0x006a9f80
const Sch::CommandFactory kEndRecordingCmdFactory(kEndRecordingCmdId, NewEndRecordingCmd);

constexpr char kDescription[] = "{EndRecordingCmd}";

} // namespace

// NTSC-U/C: 0x006693e8, PAL: 0x006a9f78
int EndRecordingCmd::sCmdID = kEndRecordingCmdId;

Sch::Command *NewEndRecordingCmd() {
    return new EndRecordingCmd;
}

int EndRecordingCmd::CmdID() {
    return sCmdID;
}

void EndRecordingCmd::Execute() {
    mRecorder->FinishUp();
}

void EndRecordingCmd::Print(std::ostream &stream) {
    stream << kDescription;
}

void EndRecordingCmd::saveGuts(OBStream &) const {
}

void EndRecordingCmd::UnusedHook() {
}
