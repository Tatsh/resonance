#include "game/gamerconstructcmd.h"

#include <iostream>

namespace {

constexpr char kDescription[] = "{Gamer}";

} // namespace

// 0x00116bc0
int GamerConstructCmd::CmdID() {
    return g_nGamerConstructCmdID;
}

// 0x00116bf0
void GamerConstructCmd::Print(std::ostream &stream) {
    stream << kDescription;
}
