#include "memcard/loadremixmct.h"

#include "app/application.h"
#include "memcard/checkinfoop.h"
#include "memcard/listdirop.h"
#include "memcard/memcard.h"
#include "memcard/memcardop.h"
#include "memcard/memcardsavepaths.h"
#include "memcard/remixindex.h"
#include "os/hxstr.h"

namespace {

// Wildcard appended to the save directory prefix so the listing enumerates every remix directory.
static const char *const kAnyDirectory = "*";

// The sceMcGetDir() mode that starts a fresh listing rather than continuing the previous one.
constexpr unsigned kListDirModeFresh = 0;

// Prefix of each listed directory name, and separator before the payload file name.
static const char *const kPathSeparator = "/";

// The index file inside each remix save directory.
static const char *const kIndexFileName = "/index";

// Values of mStep.
constexpr int kStepReadIndex = 1;
constexpr int kStepReadPayload = 2;

} // namespace

// 0x0017be28
LoadRemixMCT::LoadRemixMCT(
    MemcardUser *pUser, Memcard *pCard, int nPortSlot, int nCookie, const HxStr &remixName)
    : MemcardTask(pUser, pCard, nPortSlot, nCookie), mRemixName(remixName), mLoadTask(nullptr),
      mStream(g_abRemixStagingBuffer, kRemixStagingBufferSize) {
    mBuffer = mStream.mBuffer;
    mPayload = Application::shared()->GetResetLog();
}

// 0x00185380
LoadRemixMCT::~LoadRemixMCT() {
}

// 0x0017c060
void LoadRemixMCT::ListRemixDir() {
    mStep = 0;
    HxStr pattern = g_saveDirBase + g_remixDirSuffix + kAnyDirectory;
    mCard->ListDir(this, mPortSlot, pattern, mCookie, kListDirModeFresh);
}

// 0x00186bd0
void LoadRemixMCT::OnCheckInfo(CheckInfoOp *pOp) {
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

// 0x0017c210
void LoadRemixMCT::OnListDir(ListDirOp *pOp) {
    mStatus = pOp->mStatus;
    if (mStatus != kMemcardStatusOk) {
        mCard->Cancel(mCookie);
        Finish(); // Yes, the binary does not return here and walks the entries all the same.
    }

    const int nCount = pOp->mEntryCount;
    const sceMcTblGetDir *pEntries = pOp->mEntries;
    for (int i = 0; i < nCount; ++i) {
        const HxStr name(reinterpret_cast<const char *>(pEntries[i].EntryName));
        mDirNames.push_back(HxStr(kPathSeparator) + name);
    }
    if (mDirNames.empty()) {
        mStatus = kMemcardStatusNoFile;
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

// 0x0017c650
void LoadRemixMCT::OnFileLoaded(int nStatus) {
    mStatus = nStatus;
    if (nStatus != kMemcardStatusOk) {
        mCard->Cancel(mCookie);
        Finish(); // Yes, the binary does not return here and runs the step all the same.
    }

    if (mStep != kStepReadIndex) {
        mPayload->SetSize(mLoadTask->mBytesRead);
        Finish();
        return;
    }

    mStream.SetSize(mLoadTask->mBytesRead);
    RemixIndex index;
    index.ReadFromStream(mStream);
    for (auto it = index.elements.begin(); it != index.elements.end(); ++it) {
        if (HxStr(it->RemixName) == mRemixName) {
            const HxStr path = mCurrentDir + kPathSeparator + it->FileName;
            mStep = kStepReadPayload;
            delete mLoadTask;
            mLoadTask = new LoadFileMCT(this, mCard, mPortSlot, mCookie);
            mPayload->Reset();
            char *pDest = mPayload->Buffer();
            mLoadTask->Load(path, pDest, mPayload->Capacity());
            return;
        }
    }

    if (mDirNames.empty()) {
        mStatus = kMemcardStatusNoFile;
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

// 0x00186c50
void LoadRemixMCT::Finish() {
    mState = kMemcardTaskFinished;
    mUser->OnRemixLoaded(mPortSlot, mStatus);
}

// 0x00186ba0
void LoadRemixMCT::Execute() {
    mState = kMemcardTaskRunning;
    mCard->CheckInfo(this, mPortSlot, mCookie);
}
