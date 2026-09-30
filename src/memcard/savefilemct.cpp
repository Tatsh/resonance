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

namespace {

// Components of a background colour and a light vector.
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
const float kIconLightDir[][4] = {
    {0.5f, 0.5f, 0.5f, 0.0f}, {0.0f, -0.4f, -0.1f, 0.0f}, {-0.5f, -0.5f, 0.5f, 0.0f}};

// 0x007da1a0
const float kIconLightCol[][4] = {
    {0.48f, 0.48f, 0.03f, 0.0f}, {0.5f, 0.33f, 0.2f, 0.0f}, {0.14f, 0.14f, 0.38f, 0.0f}};

// 0x007da1d0
const float kIconLightAmbient[] = {0.5f, 0.5f, 0.5f, 0.0f};

} // namespace

SaveFileMCT::SaveFileMCT(MemcardUser *pUser, Memcard *pCard, int nPortSlot, int nCookie)
    : MemcardTask(pUser, pCard, nPortSlot, nCookie), mSkipIconFiles(0) {
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

// 0x00177ca0
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

    HxStr iconName(kIconImageLeafName);

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
