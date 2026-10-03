#include "game/localplayercmd.h"

#include <iostream>

#include "game/localplayer.h"

namespace {

constexpr char kDescription[] = "{LocalPlayerCmd}";

} // namespace

int LocalPlayerCmd::sCmdID;

// NTSC-U/C: 0x001227a8, PAL: 0x00122dc0
LocalPlayerCmd::~LocalPlayerCmd() {
}

// NTSC-U/C: 0x00122840, PAL: 0x00122e58
int LocalPlayerCmd::CmdID() {
    return sCmdID;
}

// NTSC-U/C: 0x00122820, PAL: 0x00122e38
void LocalPlayerCmd::Execute() {
    mPlayer->OnBarTick(mTick);
}

// NTSC-U/C: 0x00122860, PAL: 0x00122e78
void LocalPlayerCmd::Print(std::ostream &stream) {
    stream << kDescription;
}

// NTSC-U/C: 0x00122850, PAL: 0x00122e68
void LocalPlayerCmd::Save(OBStream &) {
}

// NTSC-U/C: 0x00122858, PAL: 0x00122e70
void LocalPlayerCmd::Load(IBStream &) {
}
