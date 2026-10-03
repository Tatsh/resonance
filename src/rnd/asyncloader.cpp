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

// NTSC-U/C: 0x006dba38, PAL: 0x0071f228
int g_nRndLoaderZone = kNoZone;

namespace {

// Requests waiting for their file read to be issued, in queue order.
// NTSC-U/C: 0x006dba40, PAL: 0x0071f230
std::vector<RndAsyncLoader *> gPendingRndFiles;

// File reads issued and not yet collected, in issue order.
// NTSC-U/C: 0x006dba50, PAL: 0x0071f240
std::vector<RndActiveLoadEntry> gInProgressRndFiles;

} // namespace

// NTSC-U/C: 0x003f7c00, PAL: 0x00430428
RndAsyncLoader::RndAsyncLoader(const HxStr &directory, const HxStr &file, int nZone)
    : mDirectory(directory), mFile(file), mPending(1), mFileRead(0), mFinished(0), mZone(nZone) {
}

// NTSC-U/C: 0x003f7e50, PAL: 0x00430688
RndAsyncLoader::RndAsyncLoader() : mPending(1), mFileRead(0), mFinished(0), mZone(kNoZone) {
}

// NTSC-U/C: 0x003f8178, PAL: 0x004309b8
RndAsyncLoader::~RndAsyncLoader() {
    Unload();
}

// NTSC-U/C: 0x003f8030, PAL: 0x00430870
void RndAsyncLoader::Cancel() {
    if (mFileRead != 0) {
        return;
    }

    for (auto it = gInProgressRndFiles.begin(); it != gInProgressRndFiles.end(); ++it) {
        if (it->mRequest == this) {
            AsyncCancelRequest(it->mHandle);
            gInProgressRndFiles.erase(it);
            break;
        }
    }

    for (auto it = gPendingRndFiles.begin(); it != gPendingRndFiles.end(); ++it) {
        if (*it == this) {
            gPendingRndFiles.erase(it);
            return;
        }
    }
}

// NTSC-U/C: 0x003f8240, PAL: 0x00430aa0
void RndAsyncLoader::Unload() {
    Cancel();
    if (mPending != 0) {
        return;
    }

    for (auto it = mLoadedObjects.begin(); it != mLoadedObjects.end(); ++it) {
        delete *it;
    }
    mLoadedObjects.clear();
    mObjects.clear();
    mDrawables.clear();
    mFinished = 0;
    mPending = 1;
    mFileRead = 0;
}

// NTSC-U/C: 0x003f8308, PAL: 0x00430b68
void RndAsyncLoader::Enqueue() {
    for (const auto &entry : gInProgressRndFiles) {
        if (entry.mRequest == this) {
            LogPrintf(kDuplicateRequestFormat, NameText(mDirectory), NameText(mFile));
            return;
        }
    }
    for (RndAsyncLoader *pRequest : gPendingRndFiles) {
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
    gPendingRndFiles.push_back(this);
}

// NTSC-U/C: 0x003f8460, PAL: 0x00430cc0
void RndAsyncLoader::HarvestLoadedObjects() {
    mLoadedObjects = Rnd::TheManager.mLoaded;

    for (auto it = Rnd::TheManager.mLoaded.begin(); it != Rnd::TheManager.mLoaded.end(); ++it) {
        if ((*it)->ClassName() == "Tex") {
            mObjects.push_back(dynamic_cast<Rnd::Tex *>(*it)); // The binary's cast helper.
        }
        // The binary calls ClassName() again rather than testing with an else.
        if ((*it)->ClassName() == "Text") {
            mDrawables.push_back(dynamic_cast<Rnd::Text *>(*it)); // The binary's cast helper.
        }
    }
    for (auto it = Rnd::TheManager.mMergeObjects.begin(); it != Rnd::TheManager.mMergeObjects.end();
         ++it) {
        if ((*it)->ClassName() == "Tex") {
            mObjects.push_back(dynamic_cast<Rnd::Tex *>(*it)); // The binary's cast helper.
        }
        if ((*it)->ClassName() == "Text") {
            mDrawables.push_back(dynamic_cast<Rnd::Text *>(*it)); // The binary's cast helper.
        }
    }
}

// NTSC-U/C: 0x003f8930, PAL: 0x00431190
void RndAsyncLoader::PollAsyncLoads() {
    const int nPreviousZone = ZoneGetCurrent();
    ZoneSetCurrent(g_nRndLoaderZone);
    if (gPendingRndFiles.size() != 0 && gInProgressRndFiles.size() == 0) {
        ZoneReset();
    }

    while (gPendingRndFiles.size() != 0) {
        RndAsyncLoader *pRequest = gPendingRndFiles.front();
        HxStr freqPath = MakeFreqPath(pRequest->mDirectory);
        HxStr path = freqPath + "gen/" + pRequest->mFile + ".gz";

        const int nLength = GetUncompressedFileLength(NameText(path));
        if (nLength == 0) {
            Fatal("RndAsyncLoader::%s(): couldn't find: %s\n", __func__, NameText(path));
        }
        if (static_cast<unsigned>(ZoneGetAvail(nLength)) < static_cast<unsigned>(nLength)) {
            break;
        }

        void *pBuffer = ZoneAlloc(nLength);
        const int nHandle = AsyncLoadFileByPath(NameText(path), pBuffer, nLength, nullptr);
        gInProgressRndFiles.push_back(RndActiveLoadEntry{pRequest, nHandle, pBuffer, nLength});
        gPendingRndFiles.erase(gPendingRndFiles.begin());
    }

    AsyncPumpCompletedRequests();

    while (gInProgressRndFiles.size() != 0) {
        void *pData;
        int nSize;
        const auto it = gInProgressRndFiles.begin();
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
            Rnd::TheManager.Read(stream);
            if (MemAccountingEnabled()) {
                char szReport[kMemoryReportSize];
                MemEndAccounting(szReport, sizeof(szReport));
                LogPrintf("MEMORY REPORT FOR: %s\n%s", NameText(pRequest->mFile), szReport);
            }

            pRequest->HarvestLoadedObjects();
            pRequest->mFileRead = 1;
            gInProgressRndFiles.erase(it);
            break;
        }
        if (nStatus < 0) {
            break;
        }

        LogPrintf("ERROR reading RND file async: %s:%s!!\n",
                  NameText(pRequest->mDirectory),
                  NameText(pRequest->mFile));
        gInProgressRndFiles.erase(it);
    }

    ZoneSetCurrent(nPreviousZone);
}

// NTSC-U/C: 0x003f8fc0, PAL: 0x00431908
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

// NTSC-U/C: 0x003fc708, PAL: 0x00435108
void RndAsyncLoader::Restart(const HxStr &directory, const HxStr &file) {
    Cancel();
    mDirectory = directory;
    mFile = file;
    mFinished = 0;
    mPending = 1;
    mFileRead = 0;
}
