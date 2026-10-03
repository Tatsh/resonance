#include "game/controllercmd.h"

#include <iostream>

#include "app/application.h"
#include "game/grooveworld.h"
#include "sch/commandfactory.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

namespace {

// saveGuts() writes these three bytes ahead of the reading and the closing three behind it.
constexpr char kTagC = 'C';
constexpr char kTagM = 'M';
constexpr char kTagOpen = '[';
constexpr char kTagClose = ']';

// restoreGuts() reads the six tag bytes into one buffer and never inspects it.
constexpr int kTagByteCount = 6;

constexpr int kControllerCmdId = 2;

// NTSC-U/C: 0x0067f240, PAL: 0x006c0470
const Sch::CommandFactory kControllerCmdFactory(kControllerCmdId, NewControllerCmd);

} // namespace

// NTSC-U/C: 0x0067f238, PAL: 0x006c0468
int ControllerCmd::sCmdID = kControllerCmdId;

// NTSC-U/C: 0x0018be70, PAL: 0x00191918
Sch::Command *NewControllerCmd() {
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
void ControllerCmd::saveGuts(OBStream &stream) const {
    const char cOpenC = kTagC;
    const char cOpenM = kTagM;
    const char cOpen = kTagOpen;
    OBStream &body = stream.Write(&cOpenC, sizeof(cOpenC))
                         .Write(&cOpenM, sizeof(cOpenM))
                         .Write(&cOpen, sizeof(cOpen));

    const char cClose = kTagClose;
    const char cCloseC = kTagC;
    const char cCloseM = kTagM;
    (body << mReading)
        .Write(&cClose, sizeof(cClose))
        .Write(&cCloseC, sizeof(cCloseC))
        .Write(&cCloseM, sizeof(cCloseM));
}

// NTSC-U/C: 0x001946b8, PAL: 0x0019a338
void ControllerCmd::restoreGuts(IBStream &stream) {
    char acTag[kTagByteCount];
    IBStream &body = stream.Read(&acTag[0], sizeof(acTag[0]))
                         .Read(&acTag[1], sizeof(acTag[1]))
                         .Read(&acTag[2], sizeof(acTag[2]));
    (body >> mReading)
        .Read(&acTag[3], sizeof(acTag[3]))
        .Read(&acTag[4], sizeof(acTag[4]))
        .Read(&acTag[5], sizeof(acTag[5]));
}
