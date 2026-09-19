#include "memcard/memcardcbhandler.h"

// 0x00183f30
MemcardCBHandler::~MemcardCBHandler() {
}

// 0x00183f60
void MemcardCBHandler::OnCheckInfo([[maybe_unused]] CheckInfoOp *pOp) {
}

// 0x00183f68
void MemcardCBHandler::OnEntSpace([[maybe_unused]] EntSpaceOp *pOp) {
}

// 0x00183f70
void MemcardCBHandler::OnFormat([[maybe_unused]] FormatOp *pOp) {
}

// 0x00183f78
void MemcardCBHandler::OnUnformat([[maybe_unused]] UnformatOp *pOp) {
}

// 0x00183f80
void MemcardCBHandler::OnCreateDir([[maybe_unused]] CreateDirOp *pOp) {
}

// 0x00183f88
void MemcardCBHandler::OnListDir([[maybe_unused]] ListDirOp *pOp) {
}

// 0x00183f90
void MemcardCBHandler::OnRead([[maybe_unused]] ReadOp *pOp) {
}

// 0x00183f98
void MemcardCBHandler::OnWrite([[maybe_unused]] WriteOp *pOp) {
}

// 0x00183fa0
void MemcardCBHandler::OnOpenWrite([[maybe_unused]] OpenWriteOp *pOp) {
}

// 0x00183fa8
void MemcardCBHandler::OnOpenRead([[maybe_unused]] OpenReadOp *pOp) {
}

// 0x00183fb0
void MemcardCBHandler::OnClose([[maybe_unused]] CloseOp *pOp) {
}

// 0x00183fb8
void MemcardCBHandler::OnSeek([[maybe_unused]] SeekOp *pOp) {
}

// 0x00183fc0
void MemcardCBHandler::OnDeleteFile([[maybe_unused]] DeleteFileOp *pOp) {
}

// 0x00183fc8
void MemcardCBHandler::OnRenameFile([[maybe_unused]] RenameFileOp *pOp) {
}
