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

// NTSC-U/C: 0x0010c8d0, PAL: 0x0010caa0
Sch::Command *NewEndRecordingCmd() {
    return new EndRecordingCmd;
}

// NTSC-U/C: 0x0010efc8, PAL: 0x0010f428
int EndRecordingCmd::CmdID() {
    return sCmdID;
}

// NTSC-U/C: 0x0010efa8, PAL: 0x0010f408
void EndRecordingCmd::Execute() {
    mRecorder->FinishUp();
}

// NTSC-U/C: 0x0010efe8, PAL: 0x0010f448
void EndRecordingCmd::Print(std::ostream &stream) {
    stream << kDescription;
}

// NTSC-U/C: 0x0010efd8, PAL: 0x0010f438
void EndRecordingCmd::saveGuts(OBStream &) const {
}

// NTSC-U/C: 0x0010efe0, PAL: 0x0010f440
void EndRecordingCmd::UnusedHook() {
}
