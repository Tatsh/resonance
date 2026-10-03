#include "memcard/savespacecheckermct.h"

#include <iostream>

#include "memcard/listdirop.h"
#include "memcard/memcard.h"

namespace {

// Appended to the save directory to list every file in it.
const char kAnyFile[] = "/*";

// The `sceMcGetDir()` mode that starts a fresh listing.
constexpr unsigned kListDirModeFresh = 0;

// Bytes in one card cluster, the unit of the measurement.
constexpr int kKilobyte = 1024;

// Kilobytes a save directory that does not exist yet costs before its files.
constexpr int kNewDirectoryKilobytes = 2;

// New files whose directory entries share one cluster.
constexpr int kEntriesPerKilobyte = 2;

// Kilobytes a file of nLength bytes occupies.
inline int KilobytesOf(int nLength) {
    return (nLength + kKilobyte - 1) / kKilobyte;
}

} // namespace

// PAL: 0x0017ae70
SaveSpaceCheckerMCT::SaveSpaceCheckerMCT(SaveFileMCT *pOwner,
                                         const std::vector<SaveFileEntry> *pFiles,
                                         const HxStr &dirName,
                                         Memcard *pCard,
                                         int nPortSlot,
                                         int nCookie)
    : MemcardTask(nullptr, pCard, nPortSlot, nCookie), mOwner(pOwner), mFiles(pFiles),
      mDirName(dirName) {
    const HxStr pattern = mDirName + kAnyFile;
    mCard->ListDir(this, mPortSlot, pattern, mCookie, kListDirModeFresh);
}

// PAL: 0x00189ac0
SaveSpaceCheckerMCT::~SaveSpaceCheckerMCT() {
}

// PAL: 0x0017b050
void SaveSpaceCheckerMCT::OnListDir(ListDirOp *pOp) {
    int nNewFiles = 0;
    int nKilobytes = 0;
    mStatus = pOp->mStatus;
    if (mStatus == kMemcardStatusOk) {
        const int nEntryCount = pOp->mEntryCount;
        const sceMcTblGetDir *pEntries = pOp->mEntries;
        for (std::vector<SaveFileEntry>::size_type i = 0; i < mFiles->size(); ++i) {
            const SaveFileEntry &file = (*mFiles)[i];
            int nEntry = 0;
            for (; nEntry < nEntryCount; ++nEntry) {
                const HxStr entryName(reinterpret_cast<const char *>(pEntries[nEntry].EntryName));
                if (entryName == file.mName) {
                    const unsigned nStoredKilobytes =
                        (pEntries[nEntry].FileSizeByte + kKilobyte - 1) / kKilobyte;
                    nKilobytes += KilobytesOf(file.mLength) - static_cast<int>(nStoredKilobytes);
                    break;
                }
            }
            if (nEntry == nEntryCount) {
                ++nNewFiles;
                nKilobytes += KilobytesOf(file.mLength);
            }
        }
    } else if (mStatus == kMemcardStatusNoDirectory) {
        nKilobytes = kNewDirectoryKilobytes;
        nNewFiles = mFiles->size();
        for (std::vector<SaveFileEntry>::size_type i = 0; i < mFiles->size(); ++i) {
            nKilobytes += KilobytesOf((*mFiles)[i].mLength);
        }
    } else if (mStatus != kMemcardStatusOk) {
        mCard->Cancel(mCookie);
        Finish();
    }

    // Yes, the binary reports to the owner after abandoning a failed listing as well.
    nKilobytes += (nNewFiles + 1) / kEntriesPerKilobyte;
    mOwner->OnSpaceChecked(nKilobytes);
}

// PAL: 0x00189828
void SaveSpaceCheckerMCT::Finish() {
}

// PAL: 0x00189830
void SaveSpaceCheckerMCT::Execute() {
    std::cout << "SaveSpaceCheckerMCT::" << __func__ << "()\n\n";
}
