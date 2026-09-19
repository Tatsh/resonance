#include "memcard/memcarduser.h"

// 0x00184448
MemcardUser::~MemcardUser() {
}

// 0x00184478
void MemcardUser::OnConnectState(MemcardConnectState state, int nStatus) {
}

// 0x001844a0
void MemcardUser::OnAllConnectStates() {
}

// 0x001844a8
void MemcardUser::OnMinimumSaveSpace(int nPortSlot, int nSpace) {
}

// 0x001844b0
void MemcardUser::OnCardFormatted(int nPortSlot, int nStatus) {
}

// 0x001844b8
void MemcardUser::OnCardUnformatted(int nPortSlot, int nStatus) {
}

// 0x001844c0
void MemcardUser::OnPersonasSaved(int nPortSlot, int nStatus) {
}

// 0x001844c8
void MemcardUser::OnRemixSaved(int nPortSlot, int nStatus) {
}

// 0x001844d0
void MemcardUser::OnGlobalSettingsSaved(int nPortSlot, int nStatus) {
}

// 0x001844d8
void MemcardUser::OnJukeboxPlayListSaved(int nPortSlot, int nStatus) {
}

// 0x001844e0
void MemcardUser::OnRemixesListed(int nPortSlot, int nStatus) {
}

// 0x001844e8
void MemcardUser::OnRemixLoaded(int nPortSlot, int nStatus) {
}

// 0x001844f0
void MemcardUser::OnPersonasLoaded(int nPortSlot, int nStatus) {
}

// 0x001844f8
void MemcardUser::OnGlobalSettingsLoaded(int nPortSlot, int nStatus) {
}

// 0x00184500
void MemcardUser::OnJukeboxPlayListLoaded(int nPortSlot, int nStatus) {
}

// 0x00184508
void MemcardUser::OnRemixDeleted(int nPortSlot, int nStatus) {
}

// 0x00184510
void MemcardUser::OnUnknown17(int nPortSlot, int nStatus) {
}

// 0x00184518
void MemcardUser::OnUnknown18(int nPortSlot, int nStatus) {
}

// 0x00184520
void MemcardUser::OnFileLoaded(int nStatus) {
}

// 0x00184528
void MemcardUser::OnFileSaved(int nStatus) {
}
