#include "memcard/deleteremixmct.h"

#include "memcard/checkinfoop.h"
#include "memcard/deletefileop.h"
#include "memcard/listdirop.h"
#include "memcard/memcard.h"
#include "memcard/memcardop.h"
#include "memcard/memcardsavepaths.h"
#include "memcard/remixdirinfo.h"
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
enum DeleteStep {
    kStepReadIndex = 1,
    kStepRewriteIndex = 2,
    kStepDeletePayload = 3,
    kStepReport = 4,
};

// The version DeleteNextFile() gives an index it did not parse.
constexpr int kUnparsedIndexVersion = 1;

// IOBPreallocMemStream::Seek() origin that measures from the start of the buffer.
constexpr int kSeekFromStart = 0;

// DeleteNextFile() saves the rewritten index together with its icon files.
constexpr int kWriteIconFiles = 0;

} // namespace

// 0x0017ce08
DeleteRemixMCT::DeleteRemixMCT(
    MemcardUser *pUser, Memcard *pCard, int nPortSlot, int nCookie, const HxStr &remixName)
    : MemcardTask(pUser, pCard, nPortSlot, nCookie), mRemixName(remixName), mLoadTask(nullptr),
      mSaveTask(nullptr), mStream(g_abRemixStagingBuffer, kRemixStagingBufferSize) {
    // The load at 0x0017cee4 reads the member rather than dispatching through
    // IOBPreallocMemStream::Buffer(), which no call site in the image reaches.
    mBuffer = mStream.mBuffer;
}

// 0x001854e0
DeleteRemixMCT::~DeleteRemixMCT() {
    delete mLoadTask;
    delete mSaveTask;
}

// 0x0017d098
void DeleteRemixMCT::ListRemixDir() {
    mStep = 0;
    HxStr pattern = g_saveDirBase + g_remixDirSuffix + kAnyDirectory;
    mCard->ListDir(this, mPortSlot, pattern, mCookie, kListDirModeFresh);
}

// 0x00186cc0
void DeleteRemixMCT::OnCheckInfo(CheckInfoOp *pOp) {
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

// 0x0017d248
void DeleteRemixMCT::OnListDir(ListDirOp *pOp) {
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
    mStream.Reset();
    delete mLoadTask;
    mLoadTask = new LoadFileMCT(this, mCard, mPortSlot, mCookie);
    mLoadTask->Load(mCurrentDir + kIndexFileName, mBuffer, mStream.Capacity());
}

// 0x0017d690
void DeleteRemixMCT::OnFileLoaded(int nStatus) {
    mStatus = nStatus;
    if (nStatus != kMemcardStatusOk) {
        mCard->Cancel(mCookie);
        Finish(); // Yes, the binary does not return here and parses the index all the same.
    }

    mStep = kStepReadIndex;
    mStream.SetSize(mLoadTask->mBytesRead);
    RemixIndex index;
    index.ReadFromStream(mStream);
    for (auto it = index.elements.begin(); it != index.elements.end(); ++it) {
        if (HxStr(it->RemixName) == mRemixName) {
            mFileName = HxStr(it->FileName);
            mStream.Seek(0, kSeekFromStart);
            mStep = kStepRewriteIndex;
            DeleteNextFile();
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

// 0x0017dc68
void DeleteRemixMCT::DeleteNextFile() {
    int nStep = mStep;
    if (mStatus != kMemcardStatusOk) {
        Finish();
        return;
    }

    RemixIndex index;
    if (mStep == kStepRewriteIndex) {
        // Yes, the else branch is unreachable, because the status was tested above.
        if (mStatus == kMemcardStatusOk) {
            index.ReadFromStream(mStream);
        } else {
            index.version = kUnparsedIndexVersion;
        }
        unsigned i = 0;
        while (i < index.elements.size() && !(HxStr(index.elements[i].FileName) == mFileName)) {
            ++i;
        }
        if (i == index.elements.size()) {
            mStatus = kMemcardStatusNoFile;
            Finish();
            return;
        }
        index.elements.erase(index.elements.begin() + i);
        mStream.Reset();
        index.WriteToStream(mStream);
        // The directory number, which follows the save prefix in the directory name.
        const HxStr dirNumber((mCurrentDir.mStr != nullptr ? mCurrentDir.mStr : g_szEmptyString) +
                              kRemixDirNumberOffset);
        delete mSaveTask;
        mSaveTask = new SaveFileMCT(this, mCard, mPortSlot, mCookie);
        mSaveTask->Save(mCurrentDir,
                        HxStr(kIndexFileName),
                        g_remixIconTitle + dirNumber,
                        mStream.mBuffer,
                        mStream.Size(),
                        kWriteIconFiles);
        nStep = kStepDeletePayload;
    } else if (mStep == kStepDeletePayload) {
        mCard->DeleteFile(this, mPortSlot, mCurrentDir + kPathSeparator + mFileName, mCookie);
        nStep = kStepReport;
    } else if (mStep == kStepReport) {
        Finish();
    }
    mStep = nStep;
}

// 0x00186d40
void DeleteRemixMCT::OnDeleteFile(DeleteFileOp *pOp) {
    mStatus = pOp->mStatus;
    if (pOp->mStatus != kMemcardStatusOk) {
        mCard->Cancel(mCookie);
        Finish();
    }
    // Yes, the next step runs even after the abandon path above has reported the task finished.
    DeleteNextFile();
}

// 0x00186d98
void DeleteRemixMCT::OnFileSaved(int nStatus) {
    mStatus = nStatus;
    DeleteNextFile();
}

// 0x00186db8
void DeleteRemixMCT::Finish() {
    mState = kMemcardTaskFinished;
    mUser->OnRemixDeleted(mPortSlot, mStatus);
}

// 0x00186c90
void DeleteRemixMCT::Execute() {
    mState = kMemcardTaskRunning;
    mCard->CheckInfo(this, mPortSlot, mCookie);
}
