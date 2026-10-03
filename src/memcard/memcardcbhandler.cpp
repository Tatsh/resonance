#include "memcard/memcardcbhandler.h"

// NTSC-U/C: 0x00183f30, PAL: 0x001891a0
MemcardCBHandler::~MemcardCBHandler() {
}

// NTSC-U/C: 0x00183f60, PAL: 0x001891d0
void MemcardCBHandler::OnCheckInfo([[maybe_unused]] CheckInfoOp *pOp) {
}

// NTSC-U/C: 0x00183f68, PAL: 0x001891d8
void MemcardCBHandler::OnEntSpace([[maybe_unused]] EntSpaceOp *pOp) {
}

// NTSC-U/C: 0x00183f70, PAL: 0x001891e0
void MemcardCBHandler::OnFormat([[maybe_unused]] FormatOp *pOp) {
}

// NTSC-U/C: 0x00183f78, PAL: 0x001891e8
void MemcardCBHandler::OnUnformat([[maybe_unused]] UnformatOp *pOp) {
}

// NTSC-U/C: 0x00183f80, PAL: 0x001891f0
void MemcardCBHandler::OnCreateDir([[maybe_unused]] CreateDirOp *pOp) {
}

// NTSC-U/C: 0x00183f88, PAL: 0x001891f8
void MemcardCBHandler::OnListDir([[maybe_unused]] ListDirOp *pOp) {
}

// NTSC-U/C: 0x00183f90, PAL: 0x00189200
void MemcardCBHandler::OnRead([[maybe_unused]] ReadOp *pOp) {
}

// NTSC-U/C: 0x00183f98, PAL: 0x00189208
void MemcardCBHandler::OnWrite([[maybe_unused]] WriteOp *pOp) {
}

// NTSC-U/C: 0x00183fa0, PAL: 0x00189210
void MemcardCBHandler::OnOpenWrite([[maybe_unused]] OpenWriteOp *pOp) {
}

// NTSC-U/C: 0x00183fa8, PAL: 0x00189218
void MemcardCBHandler::OnOpenRead([[maybe_unused]] OpenReadOp *pOp) {
}

// NTSC-U/C: 0x00183fb0, PAL: 0x00189220
void MemcardCBHandler::OnClose([[maybe_unused]] CloseOp *pOp) {
}

// NTSC-U/C: 0x00183fb8, PAL: 0x00189228
void MemcardCBHandler::OnSeek([[maybe_unused]] SeekOp *pOp) {
}

// NTSC-U/C: 0x00183fc0, PAL: 0x00189230
void MemcardCBHandler::OnDeleteFile([[maybe_unused]] DeleteFileOp *pOp) {
}

// NTSC-U/C: 0x00183fc8, PAL: 0x00189238
void MemcardCBHandler::OnRenameFile([[maybe_unused]] RenameFileOp *pOp) {
}
