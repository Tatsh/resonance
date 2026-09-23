#include "game/jukeboxplaylist.h"

#include "met/metremixmanager.h"
#include "met/metremixrecord.h"

namespace {

// The record version save() writes.
constexpr int kPlayListVersion = 1;

} // namespace

// 0x001e5da8
JukeboxPlayList::~JukeboxPlayList() {
    clear();
}

// 0x001e1e38
void JukeboxPlayList::save(OBStream *pStream) {
    int version = kPlayListVersion;
    pStream->Write(&version, sizeof(version));
    int count = entries.size();
    pStream->Write(&count, sizeof(count));
    for (int i = 0; i < count; ++i) {
        const JukeboxPlayListEntry *pEntry = entries[i];
        unsigned length = pEntry->name.mLen;
        pStream->Write(&length, sizeof(length));
        pStream->WriteBytes(pEntry->name.mStr != nullptr ? pEntry->name.mStr : g_szEmptyString,
                            length);
        *pStream << entries[i]->factory;
    }
}

// 0x001e1f70
void JukeboxPlayList::load(IBStream *pStream) {
    int version;
    pStream->Read(&version, sizeof(version));
    if (version <= 0) {
        return;
    }
    int count;
    pStream->Read(&count, sizeof(count));
    // Yes, the entries the list held are dropped without being released.
    entries.resize(count, nullptr);
    for (int i = 0; i < count; ++i) {
        JukeboxPlayListEntry *pEntry = new JukeboxPlayListEntry;
        unsigned length;
        pStream->Read(&length, sizeof(length));
        pEntry->name.Alloc(length);
        pStream->ReadBytes(pEntry->name.mStr != nullptr ? pEntry->name.mStr :
                                                          const_cast<char *>(g_szEmptyString),
                           length);
        *pStream >> pEntry->factory;
        entries[i] = pEntry;
    }
}

// 0x001e21b8
void JukeboxPlayList::AddEntry(const MetRemixRecord &record) {
    JukeboxPlayListEntry *pEntry = new JukeboxPlayListEntry;
    pEntry->factory = record.factory;
    pEntry->name = record.name;
    entries.push_back(pEntry);
}

// 0x001e20f8
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

// 0x001e5fa0
void JukeboxPlayList::SwapEntries(int nFirst, int nSecond) {
    JukeboxPlayListEntry *pFirst = entries[nFirst];
    entries[nFirst] = entries[nSecond];
    entries[nSecond] = pFirst;
}

// 0x001e2248
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
