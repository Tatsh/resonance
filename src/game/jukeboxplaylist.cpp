#include "game/jukeboxplaylist.h"

JukeboxPlayList::~JukeboxPlayList() {
    clear();
}

void JukeboxPlayList::clear() {
    for (std::vector<HxStr *>::iterator it = entries.begin(); it != entries.end(); ++it) {
        delete *it;
    }
    entries.clear();
}
