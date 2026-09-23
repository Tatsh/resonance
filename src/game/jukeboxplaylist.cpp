#include "game/jukeboxplaylist.h"

#include "met/metremixmanager.h"

JukeboxPlayList::~JukeboxPlayList() {
    clear();
}

void JukeboxPlayList::clear() {
    for (auto it = entries.begin(); it != entries.end(); ++it) {
        delete *it;
    }
    entries.clear();
}

// 0x001e5f10
void JukeboxPlayList::RemoveUnknownEntries() {
    auto it = entries.begin();
    while (it != entries.end()) {
        if (MetRemixManager::shared()->FindRecord((*it)->name) != nullptr) {
            ++it;
        } else {
            entries.erase(it); // Yes, the binary drops the entry without releasing it.
        }
    }
}

// 0x001e5ec0
JukeboxPlayListEntry *JukeboxPlayList::GetEntry(int nIndex) {
    if (entries.size() == 0) {
        return nullptr;
    }
    int i = 0;
    for (auto it = entries.begin(); it != entries.end(); ++it, ++i) {
        if (i == nIndex) {
            return *it;
        }
    }
    return nullptr;
}
