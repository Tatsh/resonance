#include "memcard/memcardcbhandler.h"

// 0x00183f30
MemcardCBHandler::~MemcardCBHandler() {
}

// 0x00183f60
void MemcardCBHandler::OnCheckInfo(CheckInfoOp *pOp) {
}

// 0x00183f68
void MemcardCBHandler::OnEntSpace(EntSpaceOp *pOp) {
}

// 0x00183f70
void MemcardCBHandler::OnFormat(FormatOp *pOp) {
}

// 0x00183f78
void MemcardCBHandler::OnUnformat(UnformatOp *pOp) {
}

// 0x00183f80
void MemcardCBHandler::OnCreateDir(CreateDirOp *pOp) {
}

// 0x00183f88
void MemcardCBHandler::OnListDir(ListDirOp *pOp) {
}

// 0x00183f90
void MemcardCBHandler::OnRead(ReadOp *pOp) {
}

// 0x00183f98
void MemcardCBHandler::OnWrite(WriteOp *pOp) {
}

// 0x00183fa0
void MemcardCBHandler::OnOpenWrite(OpenWriteOp *pOp) {
}

// 0x00183fa8
void MemcardCBHandler::OnOpenRead(OpenReadOp *pOp) {
}

// 0x00183fb0
void MemcardCBHandler::OnClose(CloseOp *pOp) {
}

// 0x00183fb8
void MemcardCBHandler::OnSeek(SeekOp *pOp) {
}

// 0x00183fc0
void MemcardCBHandler::OnDeleteFile(DeleteFileOp *pOp) {
}

// 0x00183fc8
void MemcardCBHandler::OnRenameFile(RenameFileOp *pOp) {
}
