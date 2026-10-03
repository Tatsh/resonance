#include "game/controllercmd.h"

#include <iostream>

#include "app/application.h"
#include "game/grooveworld.h"
#include "sch/commandfactory.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

namespace {

// Save() writes these three bytes ahead of the reading and the closing three behind it.
constexpr char kTagC = 'C';
constexpr char kTagM = 'M';
constexpr char kTagOpen = '[';
constexpr char kTagClose = ']';

// Load() reads the six tag bytes into one buffer and never inspects it.
constexpr int kTagByteCount = 6;

constexpr int kControllerCmdId = 2;

// NTSC-U/C: 0x0067f240, PAL: 0x006c0470
const Sch::CommandFactory kControllerCmdFactory(kControllerCmdId, ControllerCmd::NewCmd);

} // namespace

// NTSC-U/C: 0x0067f238, PAL: 0x006c0468
int ControllerCmd::sCmdID = kControllerCmdId;

// NTSC-U/C: 0x0018be70, PAL: 0x00191918
Sch::Command *ControllerCmd::NewCmd() {
    return new ControllerCmd;
}

// NTSC-U/C: 0x00194598, PAL: 0x0019a218
int ControllerCmd::CmdID() {
    return sCmdID;
}

// NTSC-U/C: 0x00194560, PAL: 0x0019a1e0
void ControllerCmd::Execute() {
    Application::shared()->GetWorld()->ReplayControllerReading(&mReading);
}

// NTSC-U/C: 0x00194790, PAL: 0x0019a410
void ControllerCmd::Print(std::ostream &stream) {
    stream << "{" << "ControllerCmd" << "}";
}

// NTSC-U/C: 0x001945a8, PAL: 0x0019a228
void ControllerCmd::Save(OBStream &stream) {
    const char cOpenC = kTagC;
    const char cOpenM = kTagM;
    const char cOpen = kTagOpen;
    OBStream &body = stream.WriteBytes(&cOpenC, sizeof(cOpenC))
                         .WriteBytes(&cOpenM, sizeof(cOpenM))
                         .WriteBytes(&cOpen, sizeof(cOpen));

    const char cClose = kTagClose;
    const char cCloseC = kTagC;
    const char cCloseM = kTagM;
    (body << mReading)
        .WriteBytes(&cClose, sizeof(cClose))
        .WriteBytes(&cCloseC, sizeof(cCloseC))
        .WriteBytes(&cCloseM, sizeof(cCloseM));
}

// NTSC-U/C: 0x001946b8, PAL: 0x0019a338
void ControllerCmd::Load(IBStream &stream) {
    char acTag[kTagByteCount];
    IBStream &body = stream.ReadBytes(&acTag[0], sizeof(acTag[0]))
                         .ReadBytes(&acTag[1], sizeof(acTag[1]))
                         .ReadBytes(&acTag[2], sizeof(acTag[2]));
    (body >> mReading)
        .ReadBytes(&acTag[3], sizeof(acTag[3]))
        .ReadBytes(&acTag[4], sizeof(acTag[4]))
        .ReadBytes(&acTag[5], sizeof(acTag[5]));
}
