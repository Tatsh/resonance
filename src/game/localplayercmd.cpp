#include "game/localplayercmd.h"

#include <iostream>

#include "game/localplayer.h"

namespace {

constexpr char kDescription[] = "{LocalPlayerCmd}";

} // namespace

int LocalPlayerCmd::sCmdID;

LocalPlayerCmd::~LocalPlayerCmd() {
}

int LocalPlayerCmd::CmdID() {
    return sCmdID;
}

void LocalPlayerCmd::Execute() {
    mPlayer->OnBarTick(mTick);
}

void LocalPlayerCmd::Print(std::ostream &stream) {
    stream << kDescription;
}

void LocalPlayerCmd::saveGuts(OBStream &) const {
}

void LocalPlayerCmd::restoreGuts(IBStream &) {
}
