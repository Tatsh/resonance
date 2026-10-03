#include "memcard/savefilemct.h"

#include <string.h>

#include "memcard/closeop.h"
#include "memcard/createdirop.h"
#include "memcard/memcard.h"
#include "memcard/memcarduser.h"
#include "memcard/openwriteop.h"
#include "memcard/saveicon.h"
#include "memcard/shiftjis.h"
#include "memcard/writeop.h"
#ifdef VIDEO_STANDARD_PAL
#include "memcard/checkinfoop.h"
#include "memcard/memcardsavepaths.h"
#include "memcard/savespacecheckermct.h"
#endif

namespace {

// Components of a background colour and a light vector.
enum IconVectorComponent { kIconX = 0, kIconY = 1, kIconZ = 2, kIconW = 3 };

// Half intensity, used by every corner of the background gradient.
constexpr int kIconBackgroundLevel = 128;

// Character offset the browser breaks the title at.
constexpr int kIconTitleLineBreak = 24;

// Background transparency the browser applies.
constexpr int kIconTransparency = 96;

// Bytes the marker file stores. They are the terminated text below.
constexpr int kMarkerLength = 2;

const char kIconSysHeader[] = "PS2D";

#ifdef VIDEO_STANDARD_PAL
// The marker file is the save directory's name without its leading `/`.
constexpr unsigned kDirectoryPrefixLength = 1;

const char kPathSeparator[] = "/";
#else
const char kIconSysName[] = "/icon.sys";
const char kIconImageName[] = "/freq1.ico";
const char kIconImageLeafName[] = "freq1.ico";
const char kMarkerText[] = "0";
#endif

// 0x007da170
const float kIconLightDir[][4] = {
    {0.5f, 0.5f, 0.5f, 0.0f}, {0.0f, -0.4f, -0.1f, 0.0f}, {-0.5f, -0.5f, 0.5f, 0.0f}};

// 0x007da1a0
const float kIconLightCol[][4] = {
    {0.48f, 0.48f, 0.03f, 0.0f}, {0.5f, 0.33f, 0.2f, 0.0f}, {0.14f, 0.14f, 0.38f, 0.0f}};

// 0x007da1d0
const float kIconLightAmbient[] = {0.5f, 0.5f, 0.5f, 0.0f};

} // namespace

#ifdef VIDEO_STANDARD_PAL
// PAL: 0x00189900
SaveFileMCT::SaveFileMCT(MemcardUser *pUser, Memcard *pCard, int nPortSlot, int nCookie)
    : MemcardTask(pUser, pCard, nPortSlot, nCookie), mKilobytes(0), mSkipIconFiles(0), mExecuted(0),
      mFreeClusters(0) {
}
#else
SaveFileMCT::SaveFileMCT(MemcardUser *pUser, Memcard *pCard, int nPortSlot, int nCookie)
    : MemcardTask(pUser, pCard, nPortSlot, nCookie), mSkipIconFiles(0) {
}
#endif

// NTSC-U/C: 0x00184658, PAL: 0x00189968
SaveFileMCT::~SaveFileMCT() {
}

// NTSC-U/C: 0x001859e8, PAL: 0x0017a1c0
void SaveFileMCT::Save(const HxStr &dirName,
                       const HxStr &fileName,
                       const HxStr &iconTitle,
                       const void *pData,
                       int nLength,
                       int bSkipIconFiles) {
#ifdef VIDEO_STANDARD_PAL
    mDirName = dirName;
    mIconTitle = iconTitle;
    mFiles.clear();
    const SaveFileEntry file = {fileName, pData, nLength};
    mFiles.push_back(file);
#else
    mDirName = dirName;
    mFileName = fileName;
    mIconTitle = iconTitle;
    mData = pData;
    mDataLength = nLength;
#endif
    mSkipIconFiles = bSkipIconFiles;
    Execute();
}

#ifdef VIDEO_STANDARD_PAL
// PAL: 0x0017a6f0
void SaveFileMCT::RunStep() {
    HxStr path;
    switch (mStep) {
    case kSaveFileStepCheckSpace:
        StartSpaceCheck();
        break;

    case kSaveFileStepCreateDir:
        // Yes, the binary does not clear mSpaceChecker.
        delete mSpaceChecker;
        mCard->CreateDir(this, mPortSlot, mDirName, mCookie);
        mStep = kSaveFileStepOpenFile;
        break;

    case kSaveFileStepOpenFile:
        path = mDirName + kPathSeparator + mFiles[mFileIndex].mName;
        mCard->OpenWrite(this, mPortSlot, path, mCookie);
        mStep = kSaveFileStepWriteFile;
        break;

    case kSaveFileStepWriteFile: {
        const SaveFileEntry &file = mFiles[mFileIndex];
        mCard->Write(this, mPortSlot, mFile, file.mData, file.mLength, mCookie);
        mCard->Close(this, mFile, mCookie);
        if (mFileIndex == static_cast<int>(mFiles.size()) - 1) {
            mStep = kSaveFileStepReport;
        } else {
            ++mFileIndex;
            mStep = kSaveFileStepOpenFile;
        }
        break;
    }

    case kSaveFileStepReport:
        Finish();
        break;

    case kSaveFileStepDone:
        break;

    default:
        break;
    }
}
#else
// NTSC-U/C: 0x001776e8
void SaveFileMCT::RunStep() {
    HxStr path;
    switch (mStep) {
    case kSaveFileStepCreateDir:
        mCard->CreateDir(this, mPortSlot, mDirName, mCookie);
        mStep = mSkipIconFiles != 0 ? kSaveFileStepOpenData : kSaveFileStepOpenIconSys;
        break;

    case kSaveFileStepOpenIconSys:
        path = mDirName + kIconSysName;
        mCard->OpenWrite(this, mPortSlot, path, mCookie);
        mStep = kSaveFileStepWriteIconSys;
        break;

    case kSaveFileStepWriteIconSys:
        BuildIconSys(mIconTitle.mStr != nullptr ? mIconTitle.mStr : g_szEmptyString);
        mCard->Write(this, mPortSlot, mFile, &mIconSys, sizeof(mIconSys), mCookie);
        mCard->Close(this, mFile, mCookie);
        mStep = kSaveFileStepOpenIconImage;
        break;

    case kSaveFileStepOpenIconImage:
        path = mDirName + kIconImageName;
        mCard->OpenWrite(this, mPortSlot, path, mCookie);
        mStep = kSaveFileStepWriteIconImage;
        break;

    case kSaveFileStepWriteIconImage:
        mCard->Write(this, mPortSlot, mFile, g_abSaveIcon, g_nSaveIconLength, mCookie);
        mCard->Close(this, mFile, mCookie);
        mStep = kSaveFileStepOpenMarker;
        break;

    case kSaveFileStepOpenMarker:
        path = mDirName + mDirName;
        mCard->OpenWrite(this, mPortSlot, path, mCookie);
        mStep = kSaveFileStepWriteMarker;
        break;

    case kSaveFileStepWriteMarker:
        mCard->Write(this, mPortSlot, mFile, kMarkerText, kMarkerLength, mCookie);
        mCard->Close(this, mFile, mCookie);
        mStep = kSaveFileStepOpenData;
        break;

    case kSaveFileStepOpenData:
        path = mDirName + mFileName;
        mCard->OpenWrite(this, mPortSlot, path, mCookie);
        mStep = kSaveFileStepWriteData;
        break;

    case kSaveFileStepWriteData:
        mCard->Write(this, mPortSlot, mFile, mData, mDataLength, mCookie);
        mCard->Close(this, mFile, mCookie);
        mStep = kSaveFileStepReport;
        break;

    case kSaveFileStepReport:
        Finish();
        break;

    case kSaveFileStepDone:
        break;

    default:
        break;
    }
}
#endif

// NTSC-U/C: 0x00177ca0, PAL: 0x0017ab40
void SaveFileMCT::BuildIconSys(const char *pszTitle) {
    sceMcIconSys pattern;
    memset(pattern.BgColor, 0, sizeof(pattern.BgColor));
    pattern.BgColor[0][kIconX] = kIconBackgroundLevel;
    pattern.BgColor[1][kIconY] = kIconBackgroundLevel;
    pattern.BgColor[2][kIconZ] = kIconBackgroundLevel;
    pattern.BgColor[3][kIconX] = kIconBackgroundLevel;
    pattern.BgColor[3][kIconY] = kIconBackgroundLevel;
    pattern.BgColor[3][kIconZ] = kIconBackgroundLevel;
    memcpy(pattern.LightDir, kIconLightDir, sizeof(pattern.LightDir));
    memcpy(pattern.LightColor, kIconLightCol, sizeof(pattern.LightColor));
    memcpy(pattern.Ambient, kIconLightAmbient, sizeof(pattern.Ambient));

#ifdef VIDEO_STANDARD_PAL
    HxStr iconName(g_iconImageFileName);
#else
    HxStr iconName(kIconImageLeafName);
#endif

    memset(&mIconSys, 0, sizeof(mIconSys));
    memcpy(mIconSys.Head, kIconSysHeader, sizeof(kIconSysHeader));

    char szShiftJisTitle[kIconTitleBufferSize];
    AsciiToShiftJis(pszTitle, szShiftJisTitle);
    strcpy(reinterpret_cast<char *>(mIconSys.TitleName), szShiftJisTitle);

    mIconSys.OffsLF = kIconTitleLineBreak;
    mIconSys.TransRate = kIconTransparency;
    memcpy(mIconSys.BgColor, pattern.BgColor, sizeof(mIconSys.BgColor));
    memcpy(mIconSys.LightDir, pattern.LightDir, sizeof(mIconSys.LightDir));
    memcpy(mIconSys.LightColor, pattern.LightColor, sizeof(mIconSys.LightColor));
    memcpy(mIconSys.Ambient, pattern.Ambient, sizeof(mIconSys.Ambient));

    const char *pszIconName = iconName.mStr != nullptr ? iconName.mStr : g_szEmptyString;
    strcpy(reinterpret_cast<char *>(mIconSys.FnameView), pszIconName);
    strcpy(reinterpret_cast<char *>(mIconSys.FnameCopy), pszIconName);
    strcpy(reinterpret_cast<char *>(mIconSys.FnameDel), pszIconName);
}

#ifdef VIDEO_STANDARD_PAL
// PAL: 0x0018b5d8
void SaveFileMCT::OnCheckInfo(CheckInfoOp *pOp) {
    mStatus = pOp->mStatus;
    if (mStatus != kMemcardStatusUnknown && mStatus != kMemcardStatusNotFormatted) {
        mStep = kSaveFileStepCheckSpace;
        mFreeClusters = pOp->mFree;
        RunStep();
        return;
    }
    if (mStatus != kMemcardStatusOk) {
        mCard->Cancel(mCookie);
        Finish();
    }
}

// PAL: 0x0018b660
void SaveFileMCT::OnSpaceChecked(int nKilobytes) {
    mKilobytes = nKilobytes;
    if (mFreeClusters < nKilobytes) {
        mKilobytes = nKilobytes - mFreeClusters;
        mStatus = kMemcardStatusCardFull;
        mCard->Cancel(mCookie);
        Finish();
        return;
    }

    mStep = kSaveFileStepCreateDir;
    RunStep();
}

// PAL: 0x0018b6d8
void SaveFileMCT::StartSpaceCheck() {
    mSpaceChecker = new SaveSpaceCheckerMCT(this, &mFiles, mDirName, mCard, mPortSlot, mCookie);
}
#endif

// NTSC-U/C: 0x00185c20, PAL: 0x0018b558
void SaveFileMCT::OnCreateDir(CreateDirOp *pOp) {
    mStatus = pOp->mStatus;
    if (mStatus == kMemcardStatusOk || mStatus == kMemcardStatusNoEntry) {
        RunStep();
        return;
    }

    mCard->Cancel(mCookie);
    Finish();
}

// NTSC-U/C: 0x00185b40, PAL: 0x0018b4a8
void SaveFileMCT::OnWrite(WriteOp *pOp) {
    mStatus = pOp->mStatus;
    if (mStatus != kMemcardStatusOk) {
        mCard->Cancel(mCookie);
        Finish();
        return;
    }

#ifndef VIDEO_STANDARD_PAL
    // Unreachable in the shipped flow. Every step that writes also closes and advances the step in
    // the same body. No write report arrives while mStep is still this value.
    if (mStep == kSaveFileStepWriteIconImage) {
        mCard->Close(this, mFile, mCookie);
        mStep = kSaveFileStepOpenMarker;
    }
#endif
}

// NTSC-U/C: 0x00185ae0, PAL: 0x0018b448
void SaveFileMCT::OnOpenWrite(OpenWriteOp *pOp) {
    mStatus = pOp->mStatus;
    if (mStatus == kMemcardStatusOk) {
        mFile = pOp->mFile;
        RunStep();
        return;
    }

    mCard->Cancel(mCookie);
    Finish();
}

// NTSC-U/C: 0x00185bc0, PAL: 0x0018b4f8
void SaveFileMCT::OnClose(CloseOp *pOp) {
    mStatus = pOp->mStatus;
    if (mStatus == kMemcardStatusOk) {
        RunStep();
        return;
    }

    mCard->Cancel(mCookie);
    Finish();
}

// NTSC-U/C: 0x00185a88, PAL: 0x0018b410
void SaveFileMCT::Finish() {
    mStep = kSaveFileStepDone;
    mUser->OnFileSaved(mStatus);
}

#ifdef VIDEO_STANDARD_PAL
// PAL: 0x0017a390
void SaveFileMCT::Execute() {
    mExecuted = 1;
    mFileIndex = 0;
    BuildIconSys(mIconTitle.mStr != nullptr ? mIconTitle.mStr : g_szEmptyString);

    HxStr markerName(mDirName);
    // Yes, the binary writes the first two bytes of the icon mesh as the marker file.
    const SaveFileEntry marker = {
        markerName.Erase(0, kDirectoryPrefixLength), g_abSaveIcon, kMarkerLength};
    mFiles.push_back(marker);
    const SaveFileEntry iconImage = {g_iconImageFileName, g_abSaveIcon, g_nSaveIconLength};
    mFiles.push_back(iconImage);
    const SaveFileEntry iconSys = {g_iconSysFileName, &mIconSys, sizeof(mIconSys)};
    mFiles.push_back(iconSys);

    mCard->CheckInfo(this, mPortSlot, mCookie);
}
#else
// NTSC-U/C: 0x00185ac0
void SaveFileMCT::Execute() {
    mStep = kSaveFileStepCreateDir;
    RunStep();
}
#endif
