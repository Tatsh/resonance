#include "memcard/listremixesmct.h"

#include "memcard/checkinfoop.h"
#include "memcard/listdirop.h"
#include "memcard/memcard.h"
#include "memcard/memcardop.h"
#include "memcard/memcardsavepaths.h"
#include "memcard/remixindex.h"
#include "met/metremixrecord.h"
#include "os/hxstr.h"

namespace {

// Wildcard appended to the save directory prefix so the listing enumerates every remix directory.
static const char *const kAnyDirectory = "*";

// The sceMcGetDir() mode that starts a fresh listing rather than continuing the previous one.
constexpr unsigned kListDirModeFresh = 0;

// Prefix OnListDir() gives each listed directory name.
static const char *const kPathSeparator = "/";

// The index file inside each remix save directory.
static const char *const kIndexFileName = "/index";

// mStep once the index reads are under way.
constexpr int kStepReadIndex = 1;

} // namespace

// 0x0017e528
ListRemixesMCT::ListRemixesMCT(MemcardUser *pUser,
                               Memcard *pCard,
                               int nPortSlot,
                               int nCookie,
                               std::vector<MetRemixRecord> *pRecords)
    : MemcardTask(pUser, pCard, nPortSlot, nCookie), mLoadTask(nullptr), mRecords(pRecords),
      mStream(g_abRemixStagingBuffer, kRemixStagingBufferSize) {
    mBuffer = mStream.mBuffer;
}

ListRemixesMCT::~ListRemixesMCT() {
}

void ListRemixesMCT::ListRemixDir() {
    mStep = 0;
    HxStr pattern(g_saveDirBase);
    pattern += g_remixDirSuffix;
    pattern += kAnyDirectory;
    mCard->ListDir(this, mPortSlot, pattern, mCookie, kListDirModeFresh);
}

void ListRemixesMCT::OnCheckInfo(CheckInfoOp *pOp) {
    mStatus = pOp->mStatus;
    if (pOp->mStatus != kMemcardStatusUnknown && pOp->mStatus != kMemcardStatusNotFormatted) {
        ListRemixDir();
        return;
    }
    if (pOp->mStatus != kMemcardStatusOk) {
        mCard->Cancel(mCookie);
        Finish();
    }
}

// 0x0017e8b8
void ListRemixesMCT::OnListDir(ListDirOp *pOp) {
    mStatus = pOp->mStatus;
    if (mStatus != kMemcardStatusOk) {
        mCard->Cancel(mCookie);
        Finish();
        return;
    }

    const int nCount = pOp->mEntryCount;
    const sceMcTblGetDir *pEntries = pOp->mEntries;
    for (int i = 0; i < nCount; ++i) {
        const HxStr name(reinterpret_cast<const char *>(pEntries[i].EntryName));
        mDirNames.push_back(HxStr(kPathSeparator) + name);
    }
    if (mDirNames.empty()) {
        Finish();
        return;
    }

    mStep = kStepReadIndex;
    mCurrentDir = mDirNames.front();
    mDirNames.erase(mDirNames.begin());
    delete mLoadTask;
    mLoadTask = new LoadFileMCT(this, mCard, mPortSlot, mCookie);
    mLoadTask->Load(mCurrentDir + kIndexFileName, mBuffer, mStream.Capacity());
}

// 0x0017ece0
void ListRemixesMCT::OnFileLoaded(int nStatus) {
    mStatus = nStatus;
    if (nStatus != kMemcardStatusOk) {
        mCard->Cancel(mCookie);
        Finish();
        return;
    }

    mStream.SetSize(mLoadTask->mBytesRead);
    RemixIndex index;
    index.ReadFromStream(mStream);
    for (auto it = index.elements.begin(); it != index.elements.end(); ++it) {
        // Yes, the binary stores the GameOK byte as loaded, without normalising it to 0 or 1.
        mRecords->push_back(MetRemixRecord(HxStr(it->LevelName),
                                           HxStr(it->RemixName),
                                           HxStr(it->FileName),
                                           it->unknown54,
                                           it->GameOK,
                                           it->appearances,
                                           it->AlbumNum));
    }

    if (mDirNames.empty()) {
        Finish();
        return;
    }
    mStep = kStepReadIndex;
    mCurrentDir = mDirNames.front();
    mDirNames.erase(mDirNames.begin());
    mStream.Reset();
    delete mLoadTask;
    mLoadTask = new LoadFileMCT(this, mCard, mPortSlot, mCookie);
    mLoadTask->Load(mCurrentDir + kIndexFileName, mBuffer, mStream.Capacity());
}

void ListRemixesMCT::Finish() {
    mState = kMemcardTaskFinished;
    mUser->OnRemixesListed(mPortSlot, mStatus);
}

void ListRemixesMCT::Execute() {
    mState = kMemcardTaskRunning;
    mCard->CheckInfo(this, mPortSlot, mCookie);
}
