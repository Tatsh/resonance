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

// 0x001f6140
Memcard::~Memcard() {
}

// 0x001f61a8
void Memcard::Update() {
}

// 0x0047e370
void Memcard::CheckInfo(MemcardCBHandler *pHandler, int nPortSlot, int nCookie) {
    mOps.push_back(new CheckInfoOp(pHandler, nPortSlot, nCookie));
}

// 0x0047e498
void Memcard::EntSpace(MemcardCBHandler *pHandler, int nPortSlot, const HxStr &path, int nCookie) {
    mOps.push_back(new EntSpaceOp(pHandler, nPortSlot, path, nCookie));
}

// 0x0047e5d0
void Memcard::Format(MemcardCBHandler *pHandler, int nPortSlot, int nCookie) {
    mOps.push_back(new FormatOp(pHandler, nPortSlot, nCookie));
}

// 0x0047e6f8
void Memcard::Unformat(MemcardCBHandler *pHandler, int nPortSlot, int nCookie) {
    mOps.push_back(new UnformatOp(pHandler, nPortSlot, nCookie));
}

// 0x0047e820
void Memcard::CreateDir(MemcardCBHandler *pHandler, int nPortSlot, const HxStr &path, int nCookie) {
    mOps.push_back(new CreateDirOp(pHandler, nPortSlot, path, nCookie));
}

// 0x0047e990
void Memcard::ListDir(
    MemcardCBHandler *pHandler, int nPortSlot, const HxStr &path, int nCookie, unsigned nMode) {
    mOps.push_back(new ListDirOp(pHandler, nPortSlot, path, nCookie, nMode));
}

// 0x0047ead8
void Memcard::Read(
    MemcardCBHandler *pHandler, int nPortSlot, int nFile, void *pBuffer, int nLength, int nCookie) {
    mOps.push_back(new ReadOp(pHandler, nPortSlot, nFile, pBuffer, nLength, nCookie));
}

// 0x0047ec30
void Memcard::Write(MemcardCBHandler *pHandler,
                    int nPortSlot,
                    int nFile,
                    const void *pBuffer,
                    int nLength,
                    int nCookie) {
    mOps.push_back(new WriteOp(pHandler, nPortSlot, nFile, pBuffer, nLength, nCookie));
}

// 0x0047ed88
void Memcard::Seek(MemcardCBHandler *pHandler, int nFile, int nOffset, int nOrigin, int nCookie) {
    mOps.push_back(new SeekOp(pHandler, nFile, nOffset, nOrigin, nCookie));
}

// 0x0047eed0
void Memcard::OpenWrite(MemcardCBHandler *pHandler, int nPortSlot, const HxStr &path, int nCookie) {
    mOps.push_back(new OpenWriteOp(pHandler, nPortSlot, path, nCookie));
}

// 0x0047f008
void Memcard::OpenRead(MemcardCBHandler *pHandler, int nPortSlot, const HxStr &path, int nCookie) {
    mOps.push_back(new OpenReadOp(pHandler, nPortSlot, path, nCookie));
}

// 0x0047f140
void Memcard::Close(MemcardCBHandler *pHandler, int nFile, int nCookie) {
    mOps.push_back(new CloseOp(pHandler, nFile, nCookie));
}

// 0x0047f268
void Memcard::DeleteFile(MemcardCBHandler *pHandler,
                         int nPortSlot,
                         const HxStr &path,
                         int nCookie) {
    mOps.push_back(new DeleteFileOp(pHandler, nPortSlot, path, nCookie));
}

// 0x0047f3a0
void Memcard::RenameFile(MemcardCBHandler *pHandler,
                         int nPortSlot,
                         const HxStr &oldPath,
                         const HxStr &newPath,
                         int nCookie) {
    mOps.push_back(new RenameFileOp(pHandler, nPortSlot, oldPath, newPath, nCookie));
}

// 0x0047f4e8
void Memcard::Cancel(int nCookie) {
    std::list<MemcardOp *>::iterator it = mOps.begin();
    while (it != mOps.end()) {
        if ((*it)->mCookie == nCookie) {
            delete *it;
            it = mOps.erase(it);
        } else {
            ++it;
        }
    }
}
