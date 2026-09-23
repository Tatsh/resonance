#include "memcard/saveremixmct.h"

#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app/application.h"
#include "memcard/checkinfoop.h"
#include "memcard/listdirop.h"
#include "memcard/memcard.h"
#include "memcard/memcardop.h"
#include "memcard/memcardsavepaths.h"
#include "memcard/remixindex.h"
#include "os/datetime.h"
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

// dateTime when the clock cannot be read.
static const char *const kDefaultDateTime = "FIXME: default date";

// Values of mStep.
enum SaveRemixStep {
    kStepReadIndex = 1,
    kStepReadTargetIndex = 2,
    kSaveRemixStepPayloadWritten = 3,
    kSaveRemixStepWriteIndex = 4,
};

// The version WriteIndex() gives an index it could not read.
constexpr int kFreshIndexVersion = 1;

// IOBPreallocMemStream::Seek() origin that measures from the start of the buffer.
constexpr int kSeekFromStart = 0;

// The payload is saved alone, and the rewritten index with its icon files.
constexpr int kSkipIconFiles = 1;
constexpr int kWriteIconFiles = 0;

// Where ReadTargetIndex() stamps the remix name into the payload, and the bytes it copies.
constexpr int kPayloadRemixNameOffset = 0x25;
constexpr int kPayloadRemixNameSize = 32;

// Bytes of the stack buffers the directory and file numbers are formatted into.
constexpr int kNumberTextSize = 16;

// The text of an HxStr, with the shared empty string standing in for a null buffer.
inline const char *TextOf(const HxStr &text) {
    return text.mStr != nullptr ? text.mStr : g_szEmptyString;
}

// Fills one index element from the task's remix. The binary expands it at both WriteIndex() sites.
inline void FillElement(RemixIndexElement &element,
                        const HxStr &levelName,
                        const HxStr &remixName,
                        const HxStr &fileName,
                        int nAlbumNum,
                        const std::vector<FreqAppearance> &appearances) {
    strncpy(element.LevelName, TextOf(levelName), kRemixIndexLevelNameSize - 1);
    strncpy(element.RemixName, TextOf(remixName), kRemixIndexRemixNameSize - 1);
    strncpy(element.FileName, TextOf(fileName), kRemixIndexFileNameSize - 1);
    element.GameOK = 1;
    element.AlbumNum = nAlbumNum;
    HxStr dateTime;
    if (!FormatCurrentDateTime(dateTime)) {
        dateTime = kDefaultDateTime;
    }
    element.dateTime = dateTime;
    element.appearances = appearances;
}

} // namespace

// 0x00179ec0
SaveRemixMCT::SaveRemixMCT(MemcardUser *pUser,
                           Memcard *pCard,
                           int nPortSlot,
                           int nCookie,
                           const HxStr &remixName,
                           const std::vector<FreqAppearance> &appearances,
                           const HxStr &levelName,
                           int nAlbumNum)
    : MemcardTask(pUser, pCard, nPortSlot, nCookie), mAlbumNum(nAlbumNum), mReplacing(0),
      mRemixName(remixName), mAppearances(appearances), mLevelName(levelName),
      mStream(g_abRemixStagingBuffer, kRemixStagingBufferSize), mLoadTask(nullptr),
      mSaveTask(nullptr) {
}

// 0x001850a8
SaveRemixMCT::~SaveRemixMCT() {
    delete mSaveTask;
    delete mLoadTask;
}

// 0x00179c28
void SaveRemixMCT::AppendDirInfo(std::vector<RemixDirInfo> &infos,
                                 const HxStr &name,
                                 int nEntryCount) {
    infos.push_back(RemixDirInfo(name, nEntryCount));
}

// 0x00179d60
HxStr SaveRemixMCT::ChooseTargetDir(const std::vector<RemixDirInfo> &infos) {
    unsigned int nHighest = 0;
    for (auto it = infos.begin(); it != infos.end(); ++it) {
        if (it->entryCount < kMaxRemixesPerDirectory) {
            return it->name;
        }
        nHighest = std::max(nHighest, it->dirNumber);
    }
    char szNumber[kNumberTextSize];
    sprintf(szNumber, "%02d", nHighest + 1);
    return g_remixDirBase + HxStr(szNumber);
}

// 0x0017a778
void SaveRemixMCT::ListRemixDir() {
    HxStr pattern = g_saveDirBase + g_remixDirSuffix + kAnyDirectory;
    mCard->ListDir(this, mPortSlot, pattern, mCookie, kListDirModeFresh);
}

// 0x00186a40
void SaveRemixMCT::OnCheckInfo(CheckInfoOp *pOp) {
    mStatus = pOp->mStatus;
    if (pOp->mStatus != kMemcardStatusUnknown && pOp->mStatus != kMemcardStatusNotFormatted) {
        if (pOp->mFree >= kRemixSaveMinimumFreeClusters) {
            ListRemixDir();
            return;
        }
        mStatus = kMemcardStatusCardFull;
    }
    if (mStatus != kMemcardStatusOk) {
        mCard->Cancel(mCookie);
        Finish();
    }
}

// 0x0017a430
void SaveRemixMCT::OnListDir(ListDirOp *pOp) {
    mStatus = pOp->mStatus;
    if (mStatus == kMemcardStatusOk) {
        const int nCount = pOp->mEntryCount;
        const sceMcTblGetDir *pEntries = pOp->mEntries;
        for (int i = 0; i < nCount; ++i) {
            const HxStr name(reinterpret_cast<const char *>(pEntries[i].EntryName));
            mDirNames.push_back(HxStr(kPathSeparator) + name);
        }
    }

    if (!mDirNames.empty()) {
        mStep = kStepReadIndex;
        ReadNextIndex();
        return;
    }
    mTargetDir = ChooseTargetDir(mDirInfos);
    mPayloadFileName = RemixDirInfo::NextPayloadFileName(mDirInfos);
    mStep = kStepReadTargetIndex;
    ReadTargetIndex();
}

// 0x0017a928
void SaveRemixMCT::ReadNextIndex() {
    mCurrentDir = mDirNames.front();
    mDirNames.erase(mDirNames.begin());
    mStream.Reset();
    delete mLoadTask;
    mLoadTask = new LoadFileMCT(this, mCard, mPortSlot, mCookie);
    mLoadTask->Load(mCurrentDir + kIndexFileName, mStream.mBuffer, mStream.Capacity());
}

// 0x0017ab50
void SaveRemixMCT::ReadTargetIndex() {
    IOBPreallocMemStream *pPayload = Application::shared()->GetLog();
    char szName[kPayloadRemixNameSize];
    strcpy(szName, TextOf(mRemixName));
    memcpy(pPayload->Buffer() + kPayloadRemixNameOffset, szName, sizeof(szName));
    mStream.Reset();
    mStep = kStepReadTargetIndex;
    delete mLoadTask;
    mLoadTask = new LoadFileMCT(this, mCard, mPortSlot, mCookie);
    mLoadTask->Load(mTargetDir + kIndexFileName, mStream.mBuffer, mStream.Capacity());
}

// 0x0017ad70
void SaveRemixMCT::OnFileLoaded(int nStatus) {
    mStatus = nStatus;
    if (nStatus != kMemcardStatusOk && mStep != kStepReadTargetIndex) {
        mCard->Cancel(mCookie);
        Finish();
        return;
    }

    if (mStep != kStepReadIndex) {
        if (nStatus == kMemcardStatusOk) {
            mStream.SetSize(mLoadTask->mBytesRead);
        }
        mTargetIndexStatus = nStatus;
        mStep = kSaveRemixStepPayloadWritten;
        WritePayload();
        return;
    }

    mStream.SetSize(mLoadTask->mBytesRead);
    RemixIndex index;
    index.ReadFromStream(mStream);
    AppendDirInfo(mDirInfos, mCurrentDir, index.elements.size());
    for (auto it = index.elements.begin(); it != index.elements.end(); ++it) {
        RemixDirInfo::RaiseHighestFileNumber(mDirInfos, mCurrentDir, atoi(it->FileName));
        if (HxStr(it->RemixName) == mRemixName) {
            mTargetDir = mCurrentDir;
            mPayloadFileName = it->FileName;
            mReplacing = 1;
            mStep = kStepReadTargetIndex;
            ReadTargetIndex();
            return;
        }
    }

    if (!mDirNames.empty()) {
        ReadNextIndex();
        return;
    }
    mTargetDir = ChooseTargetDir(mDirInfos);
    mPayloadFileName = RemixDirInfo::NextPayloadFileName(mDirInfos);
    mStep = kStepReadTargetIndex;
    ReadTargetIndex();
}

// 0x0017ba98
void SaveRemixMCT::WritePayload() {
    const HxStr dirNumber(TextOf(mTargetDir) + kRemixDirNumberOffset);
    const HxStr fileName = HxStr(kPathSeparator) + mPayloadFileName;
    delete mSaveTask;
    mSaveTask = new SaveFileMCT(this, mCard, mPortSlot, mCookie);
    // Yes, the binary rebuilds the file name from its text rather than copying the string.
    const HxStr fileText(TextOf(fileName));
    const HxStr title = g_remixIconTitle + dirNumber;
    char *pData = Application::shared()->GetLog()->Buffer();
    const int nLength = Application::shared()->GetLog()->Size();
    mSaveTask->Save(mTargetDir, fileText, title, pData, nLength, kSkipIconFiles);
}

// 0x0017b318
void SaveRemixMCT::WriteIndex() {
    RemixIndex index;
    mStream.Seek(0, kSeekFromStart);
    if (mTargetIndexStatus == kMemcardStatusOk) {
        index.ReadFromStream(mStream);
    } else {
        index.version = kFreshIndexVersion;
    }

    if (mReplacing != 0) {
        for (auto it = index.elements.begin(); it != index.elements.end(); ++it) {
            if (HxStr(it->RemixName) == mRemixName) {
                FillElement(*it, mLevelName, mRemixName, mPayloadFileName, mAlbumNum, mAppearances);
                break;
            }
        }
    } else {
        RemixIndexElement element;
        element.Reset();
        FillElement(element, mLevelName, mRemixName, mPayloadFileName, mAlbumNum, mAppearances);
        index.elements.push_back(element);
    }

    index.DumpElements();
    mStream.Reset();
    index.WriteToStream(mStream);
    mStep = kSaveRemixStepWriteIndex;
    const HxStr dirNumber(TextOf(mTargetDir) + kRemixDirNumberOffset);
    delete mSaveTask;
    mSaveTask = new SaveFileMCT(this, mCard, mPortSlot, mCookie);
    mSaveTask->Save(mTargetDir,
                    HxStr(kIndexFileName),
                    g_remixIconTitle + dirNumber,
                    mStream.mBuffer,
                    mStream.Size(),
                    kWriteIconFiles);
}

// 0x00186ad8
void SaveRemixMCT::OnFileSaved(int nStatus) {
    mStatus = nStatus;
    if (nStatus != kMemcardStatusOk) {
        mCard->Cancel(mCookie);
        Finish();
        return;
    }
    if (mStep == kSaveRemixStepPayloadWritten) {
        mStep = kSaveRemixStepWriteIndex;
        WriteIndex();
        return;
    }
    Finish();
}

// 0x00186b60
void SaveRemixMCT::Finish() {
    mState = kMemcardTaskFinished;
    mUser->OnRemixSaved(mPortSlot, mStatus);
}

// 0x00186a08
void SaveRemixMCT::Execute() {
    mState = kMemcardTaskRunning;
    mStep = 0;
    mCard->CheckInfo(this, mPortSlot, mCookie);
}
