#include "memcard/savefilemct.h"

#include <string.h>

#include "memcard/closeop.h"
#include "memcard/createdirop.h"
#include "memcard/memcard.h"
#include "memcard/memcarduser.h"
#include "memcard/openwriteop.h"
#include "memcard/saveicon.h"
#include "memcard/writeop.h"

namespace {

// Components of an iconIVECTOR and an iconFVECTOR.
enum IconVectorComponent { kIconX = 0, kIconY = 1, kIconZ = 2, kIconW = 3 };

// Half intensity, which every corner of the background gradient uses.
constexpr int kIconBackgroundLevel = 0x80;

// Character offset the browser breaks the title at.
constexpr int kIconTitleLineBreak = 0x18;

// Background transparency the browser applies.
constexpr int kIconTransparency = 0x60;

// Bytes the marker file holds, which is the terminated text below.
constexpr int kMarkerLength = 2;

const char kIconSysHeader[] = "PS2D";
const char kIconSysName[] = "/icon.sys";
const char kIconImageName[] = "/freq1.ico";
const char kIconImageLeafName[] = "freq1.ico";
const char kMarkerText[] = "0";

// 0x007da170
const iconFVECTOR kIconLightDir[] = {
    {0.5f, 0.5f, 0.5f, 0.0f}, {0.0f, -0.4f, -0.1f, 0.0f}, {-0.5f, -0.5f, 0.5f, 0.0f}};

// 0x007da1a0
const iconFVECTOR kIconLightCol[] = {
    {0.48f, 0.48f, 0.03f, 0.0f}, {0.5f, 0.33f, 0.2f, 0.0f}, {0.14f, 0.14f, 0.38f, 0.0f}};

// 0x007da1d0
const iconFVECTOR kIconLightAmbient = {0.5f, 0.5f, 0.5f, 0.0f};

} // namespace

SaveFileMCT::SaveFileMCT(MemcardUser *pUser, Memcard *pCard, int nPortSlot, void *pCookie)
    : MemcardTask(pUser, pCard, nPortSlot, pCookie), mSkipIconFiles(0) {
}

// 0x00184658
SaveFileMCT::~SaveFileMCT() {
}

// 0x001859e8
void SaveFileMCT::Save(const HxStr &dirName,
                       const HxStr &fileName,
                       const HxStr &iconTitle,
                       const void *pData,
                       int nLength,
                       int bSkipIconFiles) {
    mDirName = dirName;
    mFileName = fileName;
    mIconTitle = iconTitle;
    mData = pData;
    mDataLength = nLength;
    mSkipIconFiles = bSkipIconFiles;
    Execute();
}

// 0x001776e8
void SaveFileMCT::RunStep() {
    switch (mStep) {
    case kSaveFileStepCreateDir:
        mCard->CreateDir(this, mPortSlot, mDirName, mCookie);
        mStep = mSkipIconFiles != 0 ? kSaveFileStepOpenData : kSaveFileStepOpenIconSys;
        break;

    case kSaveFileStepOpenIconSys: {
        HxStr iconSysPath(mDirName);
        iconSysPath += kIconSysName;
        mCard->OpenWrite(this, mPortSlot, iconSysPath, mCookie);
        mStep = kSaveFileStepWriteIconSys;
        break;
    }

    case kSaveFileStepWriteIconSys:
        BuildIconSys(mIconTitle.mStr != nullptr ? mIconTitle.mStr : g_szEmptyString);
        mCard->Write(this, mPortSlot, mFile, &mIconSys, sizeof(mIconSys), mCookie);
        mCard->Close(this, mFile, mCookie);
        mStep = kSaveFileStepOpenIconImage;
        break;

    case kSaveFileStepOpenIconImage: {
        HxStr iconImagePath(mDirName);
        iconImagePath += kIconImageName;
        mCard->OpenWrite(this, mPortSlot, iconImagePath, mCookie);
        mStep = kSaveFileStepWriteIconImage;
        break;
    }

    case kSaveFileStepWriteIconImage:
        mCard->Write(this, mPortSlot, mFile, g_abSaveIcon, g_nSaveIconLength, mCookie);
        mCard->Close(this, mFile, mCookie);
        mStep = kSaveFileStepOpenMarker;
        break;

    case kSaveFileStepOpenMarker: {
        HxStr markerPath(mDirName);
        markerPath += mDirName;
        mCard->OpenWrite(this, mPortSlot, markerPath, mCookie);
        mStep = kSaveFileStepWriteMarker;
        break;
    }

    case kSaveFileStepWriteMarker:
        mCard->Write(this, mPortSlot, mFile, kMarkerText, kMarkerLength, mCookie);
        mCard->Close(this, mFile, mCookie);
        mStep = kSaveFileStepOpenData;
        break;

    case kSaveFileStepOpenData: {
        HxStr dataPath(mDirName);
        dataPath += mFileName;
        mCard->OpenWrite(this, mPortSlot, dataPath, mCookie);
        mStep = kSaveFileStepWriteData;
        break;
    }

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

// 0x00177ca0
void SaveFileMCT::BuildIconSys(const char *pszTitle) {
    mcIcon pattern;
    memset(pattern.bgCol, 0, sizeof(pattern.bgCol));
    pattern.bgCol[0][kIconX] = kIconBackgroundLevel;
    pattern.bgCol[1][kIconY] = kIconBackgroundLevel;
    pattern.bgCol[2][kIconZ] = kIconBackgroundLevel;
    pattern.bgCol[3][kIconX] = kIconBackgroundLevel;
    pattern.bgCol[3][kIconY] = kIconBackgroundLevel;
    pattern.bgCol[3][kIconZ] = kIconBackgroundLevel;
    memcpy(pattern.lightDir, kIconLightDir, sizeof(pattern.lightDir));
    memcpy(pattern.lightCol, kIconLightCol, sizeof(pattern.lightCol));
    memcpy(pattern.lightAmbient, kIconLightAmbient, sizeof(pattern.lightAmbient));

    HxStr iconName(kIconImageLeafName);

    memset(&mIconSys, 0, sizeof(mIconSys));
    memcpy(mIconSys.head, kIconSysHeader, sizeof(kIconSysHeader));

    char szShiftJisTitle[kIconTitleBufferSize];
    AsciiToShiftJis(pszTitle, szShiftJisTitle);
    // The field is typed as a code-unit array by the platform header and used as bytes here.
    strcpy(reinterpret_cast<char *>(mIconSys.title), szShiftJisTitle);

    mIconSys.nlOffset = kIconTitleLineBreak;
    mIconSys.trans = kIconTransparency;
    memcpy(mIconSys.bgCol, pattern.bgCol, sizeof(mIconSys.bgCol));
    memcpy(mIconSys.lightDir, pattern.lightDir, sizeof(mIconSys.lightDir));
    memcpy(mIconSys.lightCol, pattern.lightCol, sizeof(mIconSys.lightCol));
    memcpy(mIconSys.lightAmbient, pattern.lightAmbient, sizeof(mIconSys.lightAmbient));

    const char *pszIconName = iconName.mStr != nullptr ? iconName.mStr : g_szEmptyString;
    strcpy(reinterpret_cast<char *>(mIconSys.view), pszIconName);
    strcpy(reinterpret_cast<char *>(mIconSys.copy), pszIconName);
    strcpy(reinterpret_cast<char *>(mIconSys.del), pszIconName);
}

// 0x00185c20
void SaveFileMCT::OnCreateDir(CreateDirOp *pOp) {
    mStatus = pOp->mStatus;
    if (mStatus == kMemcardStatusOk || mStatus == kMemcardStatusNoEntry) {
        RunStep();
        return;
    }

    mCard->Cancel(mCookie);
    Finish();
}

// 0x00185b40
void SaveFileMCT::OnWrite(WriteOp *pOp) {
    mStatus = pOp->mStatus;
    if (mStatus != kMemcardStatusOk) {
        mCard->Cancel(mCookie);
        Finish();
        return;
    }

    // Unreachable in the shipped flow. Every step that writes also closes and advances the step in
    // the same body, so no write report arrives while mStep is still this value.
    if (mStep == kSaveFileStepWriteIconImage) {
        mCard->Close(this, mFile, mCookie);
        mStep = kSaveFileStepOpenMarker;
    }
}

// 0x00185ae0
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

// 0x00185bc0
void SaveFileMCT::OnClose(CloseOp *pOp) {
    mStatus = pOp->mStatus;
    if (mStatus == kMemcardStatusOk) {
        RunStep();
        return;
    }

    mCard->Cancel(mCookie);
    Finish();
}

// 0x00185a88
void SaveFileMCT::Finish() {
    mStep = kSaveFileStepDone;
    mUser->OnFileSaved(mStatus);
}

// 0x00185ac0
void SaveFileMCT::Execute() {
    mStep = kSaveFileStepCreateDir;
    RunStep();
}
