#include "game/gamercmd.h"

#include <iostream>

namespace {

constexpr char kDescription[] = "{Gamer}";

} // namespace

// 0x00116b48
GamerCmd::~GamerCmd() {
}

// 0x00116bc0
int GamerCmd::CmdID() {
    return g_nGamerCmdID;
}

// 0x00116bd0
void GamerCmd::Execute() {
    mGamer->OnBar(mBar);
}

// 0x00116bf0
void GamerCmd::Print(std::ostream &stream) {
    stream << kDescription;
}
