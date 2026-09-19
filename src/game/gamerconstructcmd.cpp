#include "game/gamerconstructcmd.h"

#include <iostream.h>

namespace {

constexpr char kDescription[] = "{Gamer}";

} // namespace

// 0x00116bc0
int GamerConstructCmd::CmdID() {
    return g_nGamerConstructCmdID;
}

// 0x00116bf0
void GamerConstructCmd::Print(ostream &stream) {
    stream << kDescription;
}
