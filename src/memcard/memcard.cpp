#include "memcard/memcard.h"

#include "memcard/checkinfoop.h"
#include "memcard/closeop.h"
#include "memcard/createdirop.h"
#include "memcard/deletefileop.h"
#include "memcard/entspaceop.h"
#include "memcard/formatop.h"
#include "memcard/listdirop.h"
#include "memcard/openreadop.h"
#include "memcard/openwriteop.h"
#include "memcard/readop.h"
#include "memcard/renamefileop.h"
#include "memcard/seekop.h"
#include "memcard/unformatop.h"
#include "memcard/writeop.h"

Memcard::Memcard() {
}

// NTSC-U/C: 0x001f6140, PAL: 0x001fcb50
Memcard::~Memcard() {
}

// NTSC-U/C: 0x001f61a8, PAL: 0x001fcbb8
void Memcard::Update() {
}

// NTSC-U/C: 0x0047e370, PAL: 0x004bc048
void Memcard::CheckInfo(MemcardCBHandler *pHandler, int nPortSlot, int nCookie) {
    mOps.push_back(new CheckInfoOp(pHandler, nPortSlot, nCookie));
}

// NTSC-U/C: 0x0047e498, PAL: 0x004bc170
void Memcard::EntSpace(MemcardCBHandler *pHandler, int nPortSlot, const HxStr &path, int nCookie) {
    mOps.push_back(new EntSpaceOp(pHandler, nPortSlot, path, nCookie));
}

// NTSC-U/C: 0x0047e5d0, PAL: 0x004bc2a8
void Memcard::Format(MemcardCBHandler *pHandler, int nPortSlot, int nCookie) {
    mOps.push_back(new FormatOp(pHandler, nPortSlot, nCookie));
}

// NTSC-U/C: 0x0047e6f8, PAL: 0x004bc3d0
void Memcard::Unformat(MemcardCBHandler *pHandler, int nPortSlot, int nCookie) {
    mOps.push_back(new UnformatOp(pHandler, nPortSlot, nCookie));
}

// NTSC-U/C: 0x0047e820, PAL: 0x004bc4f8
void Memcard::CreateDir(MemcardCBHandler *pHandler, int nPortSlot, HxStr path, int nCookie) {
    mOps.push_back(new CreateDirOp(pHandler, nPortSlot, path, nCookie));
}

// NTSC-U/C: 0x0047e990, PAL: 0x004bc688
void Memcard::ListDir(
    MemcardCBHandler *pHandler, int nPortSlot, const HxStr &path, int nCookie, unsigned nMode) {
    mOps.push_back(new ListDirOp(pHandler, nPortSlot, path, nCookie, nMode));
}

// NTSC-U/C: 0x0047ead8, PAL: 0x004bc7d0
void Memcard::Read(
    MemcardCBHandler *pHandler, int nPortSlot, int nFile, void *pBuffer, int nLength, int nCookie) {
    mOps.push_back(new ReadOp(pHandler, nPortSlot, nFile, pBuffer, nLength, nCookie));
}

// NTSC-U/C: 0x0047ec30, PAL: 0x004bc928
void Memcard::Write(MemcardCBHandler *pHandler,
                    int nPortSlot,
                    int nFile,
                    const void *pBuffer,
                    int nLength,
                    int nCookie) {
    mOps.push_back(new WriteOp(pHandler, nPortSlot, nFile, pBuffer, nLength, nCookie));
}

// NTSC-U/C: 0x0047ed88, PAL: 0x004bca80
void Memcard::Seek(MemcardCBHandler *pHandler, int nFile, int nOffset, int nOrigin, int nCookie) {
    mOps.push_back(new SeekOp(pHandler, nFile, nOffset, nOrigin, nCookie));
}

// NTSC-U/C: 0x0047eed0, PAL: 0x004bcbc8
void Memcard::OpenWrite(MemcardCBHandler *pHandler, int nPortSlot, const HxStr &path, int nCookie) {
    mOps.push_back(new OpenWriteOp(pHandler, nPortSlot, path, nCookie));
}

// NTSC-U/C: 0x0047f008, PAL: 0x004bcd00
void Memcard::OpenRead(MemcardCBHandler *pHandler, int nPortSlot, const HxStr &path, int nCookie) {
    mOps.push_back(new OpenReadOp(pHandler, nPortSlot, path, nCookie));
}

// NTSC-U/C: 0x0047f140, PAL: 0x004bce38
void Memcard::Close(MemcardCBHandler *pHandler, int nFile, int nCookie) {
    mOps.push_back(new CloseOp(pHandler, nFile, nCookie));
}

// NTSC-U/C: 0x0047f268, PAL: 0x004bcf60
void Memcard::DeleteFile(MemcardCBHandler *pHandler,
                         int nPortSlot,
                         const HxStr &path,
                         int nCookie) {
    mOps.push_back(new DeleteFileOp(pHandler, nPortSlot, path, nCookie));
}

// NTSC-U/C: 0x0047f3a0, PAL: 0x004bd098
void Memcard::RenameFile(MemcardCBHandler *pHandler,
                         int nPortSlot,
                         const HxStr &oldPath,
                         const HxStr &newPath,
                         int nCookie) {
    mOps.push_back(new RenameFileOp(pHandler, nPortSlot, oldPath, newPath, nCookie));
}

// NTSC-U/C: 0x0047f4e8, PAL: 0x004bd1e0
void Memcard::Cancel(int nCookie) {
    // The front operation is spared. It is the one completing when a handler cancels, and
    // MemcardPS2::Update() deletes it once its handler returns.
    if (mOps.empty()) {
        return;
    }
    std::list<MemcardOp *>::iterator it = std::next(mOps.begin());
    while (it != mOps.end()) {
        if ((*it)->mCookie == nCookie) {
            delete *it;
            it = mOps.erase(it);
        } else {
            ++it;
        }
    }
}
