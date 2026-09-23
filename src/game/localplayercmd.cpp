#include "game/localplayercmd.h"

#include <iostream>

#include "game/localplayer.h"

namespace {

constexpr char kDescription[] = "{LocalPlayerCmd}";

} // namespace

int LocalPlayerCmd::sCmdID;

// 0x001227a8
LocalPlayerCmd::~LocalPlayerCmd() {
}

// 0x00122840
int LocalPlayerCmd::CmdID() {
    return sCmdID;
}

// 0x00122820
void LocalPlayerCmd::Execute() {
    mPlayer->OnBarTick(mTick);
}

// 0x00122860
void LocalPlayerCmd::Print(std::ostream &stream) {
    stream << kDescription;
}

// 0x00122850
void LocalPlayerCmd::Save(OBStream &) {
}

// 0x00122858
void LocalPlayerCmd::Load(IBStream &) {
}
