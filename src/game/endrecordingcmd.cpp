#include "game/endrecordingcmd.h"

#include <iostream>

#include "game/gamerecorder.h"

namespace {

constexpr int kEndRecordingCmdId = 6;

constexpr char kDescription[] = "{EndRecordingCmd}";

} // namespace

// 0x006693e8
int EndRecordingCmd::sCmdID = kEndRecordingCmdId;

// 0x0010c8d0
Sch::Command *EndRecordingCmd::New() {
    return new EndRecordingCmd;
}

// 0x0010efc8
int EndRecordingCmd::CmdID() {
    return sCmdID;
}

// 0x0010efa8
void EndRecordingCmd::Execute() {
    mRecorder->EndRecording();
}

// 0x0010efe8
void EndRecordingCmd::Print(std::ostream &stream) {
    stream << kDescription;
}

// 0x0010efd8
void EndRecordingCmd::Save(OBStream &) {
}

// 0x0010efe0
void EndRecordingCmd::Slot8() {
}
