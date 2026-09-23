#include "rnd/asyncloader.h"

#include <vector>

#include "os/async.h"
#include "os/hostmode.h"
#include "os/loadfile.h"
#include "os/log.h"
#include "os/mem.h"
#include "os/zone.h"
#include "rnd/bufstream.h"
#include "rnd/filepath.h"
#include "rnd/manager.h"
#include "rnd/tex.h"
#include "rnd/text.h"

namespace {

constexpr char kLoaderZoneName[] = "rndfile";

constexpr char kDuplicateRequestFormat[] = "INTERNAL ERROR: RNDASYNCLOAD SAME FILE TWICE (%s:%s)\n";

// Size of the buffer MemEndAccounting() writes its report into.
constexpr int kMemoryReportSize = 1024;

inline const char *NameText(const HxStr &name) {
    return name.mStr != nullptr ? name.mStr : g_szEmptyString;
}

} // namespace

// 0x006dba38
int g_nRndLoaderZone = kNoZone;

// 0x006dba40
std::vector<RndAsyncLoader *> g_pendingLoads;

// 0x006dba50
std::vector<RndActiveLoadEntry> g_activeLoads;

// 0x003f7c00
RndAsyncLoader::RndAsyncLoader(const HxStr &directory, const HxStr &file, int nZone)
    : mDirectory(directory), mFile(file), mPending(1), mFileRead(0), mFinished(0), mZone(nZone) {
}

// 0x003f7e50
RndAsyncLoader::RndAsyncLoader() : mPending(1), mFileRead(0), mFinished(0), mZone(kNoZone) {
}

// 0x003f8178
RndAsyncLoader::~RndAsyncLoader() {
    Unload();
}

// 0x003f8030
void RndAsyncLoader::Cancel() {
    if (mFileRead != 0) {
        return;
    }

    for (auto it = g_activeLoads.begin(); it != g_activeLoads.end(); ++it) {
        if (it->mRequest == this) {
            AsyncCancelRequest(it->mHandle);
            g_activeLoads.erase(it);
            break;
        }
    }

    for (auto it = g_pendingLoads.begin(); it != g_pendingLoads.end(); ++it) {
        if (*it == this) {
            g_pendingLoads.erase(it);
            return;
        }
    }
}

// 0x003f8240
void RndAsyncLoader::Unload() {
    Cancel();
    if (mPending != 0) {
        return;
    }

    for (auto it = mUnknown08.begin(); it != mUnknown08.end(); ++it) {
        delete *it;
    }
    mUnknown08.clear();
    mObjects.clear();
    mDrawables.clear();
    mFinished = 0;
    mPending = 1;
    mFileRead = 0;
}

// 0x003f8308
void RndAsyncLoader::Enqueue() {
    for (const auto &entry : g_activeLoads) {
        if (entry.mRequest == this) {
            LogPrintf(kDuplicateRequestFormat, NameText(mDirectory), NameText(mFile));
            return;
        }
    }
    for (RndAsyncLoader *pRequest : g_pendingLoads) {
        if (pRequest == this) {
            LogPrintf(kDuplicateRequestFormat, NameText(mDirectory), NameText(mFile));
            return;
        }
    }

    if (g_nRndLoaderZone == kNoZone) {
        g_nRndLoaderZone = FindZoneByName(kLoaderZoneName);
    }
    mPending = 0;
    mFileRead = 0;
    mFinished = 0;
    g_pendingLoads.push_back(this);
}

// 0x003f8460
void RndAsyncLoader::HarvestLoadedObjects() {
    mUnknown08 = Rnd::g_manager.mLoaded;

    for (auto it = Rnd::g_manager.mLoaded.begin(); it != Rnd::g_manager.mLoaded.end(); ++it) {
        if ((*it)->ClassName() == "Tex") {
            mObjects.push_back(dynamic_cast<Rnd::Tex *>(*it)); // The binary's cast helper.
        }
        // The binary calls ClassName() again rather than testing with an else.
        if ((*it)->ClassName() == "Text") {
            mDrawables.push_back(dynamic_cast<Rnd::Text *>(*it)); // The binary's cast helper.
        }
    }
    for (auto it = Rnd::g_manager.mMergeObjects.begin(); it != Rnd::g_manager.mMergeObjects.end();
         ++it) {
        if ((*it)->ClassName() == "Tex") {
            mObjects.push_back(dynamic_cast<Rnd::Tex *>(*it)); // The binary's cast helper.
        }
        if ((*it)->ClassName() == "Text") {
            mDrawables.push_back(dynamic_cast<Rnd::Text *>(*it)); // The binary's cast helper.
        }
    }
}

// 0x003f8930
void RndAsyncLoader::PollAsyncLoads() {
    const int nPreviousZone = ZoneGetCurrent();
    ZoneSetCurrent(g_nRndLoaderZone);
    if (g_pendingLoads.size() != 0 && g_activeLoads.size() == 0) {
        ZoneReset();
    }

    while (g_pendingLoads.size() != 0) {
        RndAsyncLoader *pRequest = g_pendingLoads.front();
        HxStr freqPath = MakeFreqPath(pRequest->mDirectory);
        HxStr path = freqPath + "gen/" + pRequest->mFile + ".gz";

        const int nLength = GetUncompressedFileLength(NameText(path));
        if (nLength == 0) {
            Fatal("RndAsyncLoader::Poll(): couldn't find: %s\n", NameText(path));
        }
        if (static_cast<unsigned>(ZoneGetAvail(nLength)) < static_cast<unsigned>(nLength)) {
            break;
        }

        void *pBuffer = ZoneAlloc(nLength);
        const int nHandle = AsyncLoadFileByPath(NameText(path), pBuffer, nLength, nullptr);
        g_activeLoads.push_back(RndActiveLoadEntry{pRequest, nHandle, pBuffer, nLength});
        g_pendingLoads.erase(g_pendingLoads.begin());
    }

    AsyncPumpCompletedRequests();

    while (g_activeLoads.size() != 0) {
        void *pData;
        int nSize;
        const auto it = g_activeLoads.begin();
        const int nStatus = AsyncPollComplete(it->mHandle, &pData, &nSize);
        RndAsyncLoader *pRequest = it->mRequest;
        if (nStatus == 0) {
            ZoneSetCurrent(pRequest->mZone);
            HxStr freqPath = MakeFreqPath(pRequest->mDirectory);
            Rnd::FilePath::SetRoot(freqPath);
            if (MemAccountingEnabled()) {
                MemBeginAccounting();
            }

            Rnd::BufStream stream(static_cast<char *>(pData), nSize);
            Rnd::g_manager.Read(stream);
            if (MemAccountingEnabled()) {
                char szReport[kMemoryReportSize];
                MemEndAccounting(szReport, sizeof(szReport));
                LogPrintf("MEMORY REPORT FOR: %s\n%s", NameText(pRequest->mFile), szReport);
            }

            pRequest->HarvestLoadedObjects();
            pRequest->mFileRead = 1;
            g_activeLoads.erase(it);
            break;
        }
        if (nStatus < 0) {
            break;
        }

        LogPrintf("ERROR reading RND file async: %s:%s!!\n",
                  NameText(pRequest->mDirectory),
                  NameText(pRequest->mFile));
        g_activeLoads.erase(it);
    }

    ZoneSetCurrent(nPreviousZone);
}

// 0x003f8fc0
int RndAsyncLoader::Poll(float *pfProgress) {
    if (mPending != 0) {
        *pfProgress = 0;
        return 0;
    }
    if (mFinished != 0) {
        *pfProgress = 1.0f;
        return 1;
    }
    if (mFileRead == 0) {
        *pfProgress = 0;
        return 0;
    }

    int nReady = 0;
    for (Rnd::Object *pObject : mObjects) {
        // HarvestLoadedObjects() appends only textures.
        if (static_cast<Rnd::Tex *>(pObject)->PollAsyncMips()) {
            ++nReady;
        }
    }
    if (static_cast<size_t>(nReady) == mObjects.size()) {
        mFinished = 1;
        *pfProgress = 1.0f;
        return 1;
    }
    *pfProgress = static_cast<float>(nReady) / static_cast<float>(mObjects.size());
    return 0;
}

// 0x003fc708
void RndAsyncLoader::Restart(const HxStr &directory, const HxStr &file) {
    Cancel();
    mDirectory = directory;
    mFile = file;
    mFinished = 0;
    mPending = 1;
    mFileRead = 0;
}
