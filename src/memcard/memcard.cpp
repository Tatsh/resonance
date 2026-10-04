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

Memcard::~Memcard() {
}

void Memcard::Update() {
}

void Memcard::CheckInfo(MemcardCBHandler *pHandler, int nPortSlot, int nCookie) {
    mOps.push_back(new CheckInfoOp(pHandler, nPortSlot, nCookie));
}

void Memcard::EntSpace(MemcardCBHandler *pHandler, int nPortSlot, const HxStr &path, int nCookie) {
    mOps.push_back(new EntSpaceOp(pHandler, nPortSlot, path, nCookie));
}

void Memcard::Format(MemcardCBHandler *pHandler, int nPortSlot, int nCookie) {
    mOps.push_back(new FormatOp(pHandler, nPortSlot, nCookie));
}

void Memcard::Unformat(MemcardCBHandler *pHandler, int nPortSlot, int nCookie) {
    mOps.push_back(new UnformatOp(pHandler, nPortSlot, nCookie));
}

void Memcard::CreateDir(MemcardCBHandler *pHandler, int nPortSlot, HxStr path, int nCookie) {
    mOps.push_back(new CreateDirOp(pHandler, nPortSlot, path, nCookie));
}

void Memcard::ListDir(
    MemcardCBHandler *pHandler, int nPortSlot, const HxStr &path, int nCookie, unsigned nMode) {
    mOps.push_back(new ListDirOp(pHandler, nPortSlot, path, nCookie, nMode));
}

void Memcard::Read(
    MemcardCBHandler *pHandler, int nPortSlot, int nFile, void *pBuffer, int nLength, int nCookie) {
    mOps.push_back(new ReadOp(pHandler, nPortSlot, nFile, pBuffer, nLength, nCookie));
}

void Memcard::Write(MemcardCBHandler *pHandler,
                    int nPortSlot,
                    int nFile,
                    const void *pBuffer,
                    int nLength,
                    int nCookie) {
    mOps.push_back(new WriteOp(pHandler, nPortSlot, nFile, pBuffer, nLength, nCookie));
}

void Memcard::Seek(MemcardCBHandler *pHandler, int nFile, int nOffset, int nOrigin, int nCookie) {
    mOps.push_back(new SeekOp(pHandler, nFile, nOffset, nOrigin, nCookie));
}

void Memcard::OpenWrite(MemcardCBHandler *pHandler, int nPortSlot, const HxStr &path, int nCookie) {
    mOps.push_back(new OpenWriteOp(pHandler, nPortSlot, path, nCookie));
}

void Memcard::OpenRead(MemcardCBHandler *pHandler, int nPortSlot, const HxStr &path, int nCookie) {
    mOps.push_back(new OpenReadOp(pHandler, nPortSlot, path, nCookie));
}

void Memcard::Close(MemcardCBHandler *pHandler, int nFile, int nCookie) {
    mOps.push_back(new CloseOp(pHandler, nFile, nCookie));
}

void Memcard::DeleteFile(MemcardCBHandler *pHandler,
                         int nPortSlot,
                         const HxStr &path,
                         int nCookie) {
    mOps.push_back(new DeleteFileOp(pHandler, nPortSlot, path, nCookie));
}

void Memcard::RenameFile(MemcardCBHandler *pHandler,
                         int nPortSlot,
                         const HxStr &oldPath,
                         const HxStr &newPath,
                         int nCookie) {
    mOps.push_back(new RenameFileOp(pHandler, nPortSlot, oldPath, newPath, nCookie));
}

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
