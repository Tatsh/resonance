#include "game/jukeboxplaylist.h"

#include "met/metremixmanager.h"
#include "met/metremixrecord.h"

namespace {

// The record version save() writes.
constexpr int kPlayListVersion = 1;

} // namespace

JukeboxPlayList::~JukeboxPlayList() {
    clear();
}

void JukeboxPlayList::save(OBStream *pStream) {
    int version = kPlayListVersion;
    pStream->WriteLE(&version, sizeof(version));
    int count = entries.size();
    pStream->WriteLE(&count, sizeof(count));
    for (int i = 0; i < count; ++i) {
        const JukeboxPlayListEntry *pEntry = entries[i];
        unsigned length = pEntry->name.mLen;
        pStream->WriteLE(&length, sizeof(length));
        pStream->Write(pEntry->name.mStr != nullptr ? pEntry->name.mStr : g_szEmptyString, length);
        *pStream << entries[i]->factory;
    }
}

void JukeboxPlayList::load(IBStream *pStream) {
    int version;
    pStream->ReadLE(&version, sizeof(version));
    if (version <= 0) {
        return;
    }
    int count;
    pStream->ReadLE(&count, sizeof(count));
    // Yes, the entries the list held are dropped without being released.
    entries.resize(count, nullptr);
    for (int i = 0; i < count; ++i) {
        JukeboxPlayListEntry *pEntry = new JukeboxPlayListEntry;
        unsigned length;
        pStream->ReadLE(&length, sizeof(length));
        pEntry->name.Alloc(length);
        pStream->Read(pEntry->name.mStr != nullptr ? pEntry->name.mStr :
                                                     const_cast<char *>(g_szEmptyString),
                      length);
        *pStream >> pEntry->factory;
        entries[i] = pEntry;
    }
}

void JukeboxPlayList::AddEntry(const MetRemixRecord &record) {
    JukeboxPlayListEntry *pEntry = new JukeboxPlayListEntry;
    pEntry->factory = record.factory;
    pEntry->name = record.name;
    entries.push_back(pEntry);
}

void JukeboxPlayList::RemoveEntry(int nIndex) {
    if (entries.size() == 0) {
        return;
    }
    int i = 0;
    for (auto it = entries.begin(); it != entries.end(); ++it, ++i) {
        if (i == nIndex) {
            delete *it;
            entries.erase(it);
            return;
        }
    }
}

void JukeboxPlayList::SwapEntries(int nFirst, int nSecond) {
    JukeboxPlayListEntry *pFirst = entries[nFirst];
    entries[nFirst] = entries[nSecond];
    entries[nSecond] = pFirst;
}

void JukeboxPlayList::clear() {
    for (auto it = entries.begin(); it != entries.end(); ++it) {
        delete *it;
    }
    entries.clear();
}

void JukeboxPlayList::RemoveStaleEntries() {
    auto it = entries.begin();
    while (it != entries.end()) {
        if (MetRemixManager::shared()->FindRecord((*it)->name) != nullptr) {
            ++it;
        } else {
            entries.erase(it); // Yes, the binary drops the entry without releasing it.
        }
    }
}

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
