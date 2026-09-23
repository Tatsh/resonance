#include "met/metfreqloader.h"

#include "met/metfreqmakerassetmanager.h"
#include "met/metpersonadata.h"
#include "os/async.h"
#include "os/mem.h"
#include "os/zone.h"
#include "rnd/asyncloader.h"
#include "stream/iobmemstream.h"

namespace {

// The tag and line Done() bills the buffer release to.
static const char *const kSourceFile = "MetFreqLoader.cpp";
constexpr int kReleaseLine = 124;

// The value ParseIdentities() writes to each persona's word at +0x15c.
constexpr int kParsedIdentity = 1;

} // namespace

// 0x002a3498
MetFreqLoader::MetFreqLoader(const HxStr &path, std::vector<MetPersonaData *> *pIdentities)
    : mIdentities(pIdentities), mPath(path), mLoaded(0), mStarted(0) {
}

// 0x002a3430
MetFreqLoader::~MetFreqLoader() {
}

// 0x002a35d8
void MetFreqLoader::Done(int, int, void *pBuffer, int nLength, int nStatus) {
    if (nStatus < 0) {
        return;
    }
    ParseIdentities(pBuffer, nLength);
    MemFreeTagged(pBuffer, kSourceFile, kReleaseLine);
    mHandle = 0;
    mLoaded = 1;
}

// 0x002a3500
bool MetFreqLoader::PollAssets() {
    return MetFreqMakerAssetManager::shared()->PollLoad();
}

// 0x002a3528
void MetFreqLoader::Start() {
    mStarted = 1;
    MetFreqMakerAssetManager::shared()->WaitForLoad();
    int nZone = ZoneGetCurrent();
    ZoneSetCurrent(kNoZone);
    mHandle =
        AsyncLoadFileByPath(mPath.mStr != nullptr ? mPath.mStr : g_szEmptyString, nullptr, 0, this);
    ZoneSetCurrent(nZone);
}

// 0x002a35a8
int MetFreqLoader::IsLoaded() {
    RndAsyncLoader::PollAsyncLoads();
    AsyncPumpCompletedRequests();
    return mLoaded;
}

// 0x002a0e30
void MetFreqLoader::ParseIdentities(const void *pBuffer, int nLength) {
    IOBMemStream stream;
    stream.Load(pBuffer, nLength);
    int nCount;
    stream.Read(&nCount, sizeof(nCount));
    for (int i = 0; i < nCount; ++i) {
        MetPersonaData *pPersona = new MetPersonaData();
        pPersona->Load(&stream);
        pPersona->mUnknown15c = kParsedIdentity;
        pPersona->mStats.RebuildLevelList();
        mIdentities->push_back(pPersona);
    }
}
