#include "met/metfreqloader.h"

#include "met/metfreqmakerassetmanager.h"
#include "met/metpersonadata.h"
#include "os/async.h"
#include "os/mem.h"
#include "os/zone.h"
#include "rnd/asyncloader.h"
#include "stream/iobmemstream.h"

namespace {

// The value ParseIdentities() writes to each persona's word at +0x15c.
constexpr int kParsedIdentity = 1;

} // namespace

// NTSC-U/C: 0x002a3498, PAL: 0x002c1288
MetFreqLoader::MetFreqLoader(const HxStr &path, std::vector<MetPersonaData *> *pIdentities)
    : mIdentities(pIdentities), mPath(path), mLoaded(0), mStarted(0) {
}

// NTSC-U/C: 0x002a3430, PAL: 0x002c1210
MetFreqLoader::~MetFreqLoader() {
}

// NTSC-U/C: 0x002a35d8, PAL: 0x002c13c8
void MetFreqLoader::Done(int, int, void *pBuffer, int nLength, int nStatus) {
    if (nStatus < 0) {
        return;
    }
    ParseIdentities(pBuffer, nLength);
    MemFreeTagged(pBuffer, __FILE__, __LINE__);
    mHandle = 0;
    mLoaded = 1;
}

// NTSC-U/C: 0x002a3500, PAL: 0x002c12f0
bool MetFreqLoader::PollAssets() {
    return MetFreqMakerAssetManager::shared()->PollLoad();
}

// NTSC-U/C: 0x002a3528, PAL: 0x002c1318
void MetFreqLoader::Start() {
    mStarted = 1;
    MetFreqMakerAssetManager::shared()->WaitForLoad();
    int nZone = ZoneGetCurrent();
    ZoneSetCurrent(kNoZone);
    mHandle =
        AsyncLoadFileByPath(mPath.mStr != nullptr ? mPath.mStr : g_szEmptyString, nullptr, 0, this);
    ZoneSetCurrent(nZone);
}

// NTSC-U/C: 0x002a35a8, PAL: 0x002c1398
int MetFreqLoader::IsLoaded() {
    RndAsyncLoader::PollAsyncLoads();
    AsyncPumpCompletedRequests();
    return mLoaded;
}

// NTSC-U/C: 0x002a0e30, PAL: 0x002bebe8
void MetFreqLoader::ParseIdentities(const void *pBuffer, int nLength) {
    IOBMemStream stream;
    stream.Fill(pBuffer, nLength);
    int nCount;
    stream.Read(&nCount, sizeof(nCount));
    for (int i = 0; i < nCount; ++i) {
        MetPersonaData *pPersona = new MetPersonaData();
        pPersona->Load(&stream);
        pPersona->mIsPrefab = kParsedIdentity;
        pPersona->mStats.RebuildLevelList();
        mIdentities->push_back(pPersona);
    }
}
