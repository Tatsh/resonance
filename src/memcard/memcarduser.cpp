#include "memcard/memcarduser.h"

// 0x00184448
MemcardUser::~MemcardUser() {
}

// 0x00184478
void MemcardUser::OnConnectState([[maybe_unused]] MemcardConnectState state,
                                 [[maybe_unused]] int nStatus) {
}

// 0x001844a0
void MemcardUser::OnAllConnectStates() {
}

// 0x001844a8
void MemcardUser::OnMinimumSaveSpace([[maybe_unused]] int nPortSlot, [[maybe_unused]] int nSpace) {
}

// 0x001844b0
void MemcardUser::OnCardFormatted([[maybe_unused]] int nPortSlot, [[maybe_unused]] int nStatus) {
}

// 0x001844b8
void MemcardUser::OnCardUnformatted([[maybe_unused]] int nPortSlot, [[maybe_unused]] int nStatus) {
}

// 0x001844c0
void MemcardUser::OnPersonasSaved([[maybe_unused]] int nPortSlot, [[maybe_unused]] int nStatus) {
}

// 0x001844c8
void MemcardUser::OnRemixSaved([[maybe_unused]] int nPortSlot, [[maybe_unused]] int nStatus) {
}

// 0x001844d0
void MemcardUser::OnGlobalSettingsSaved([[maybe_unused]] int nPortSlot,
                                        [[maybe_unused]] int nStatus) {
}

// 0x001844d8
void MemcardUser::OnJukeboxPlayListSaved([[maybe_unused]] int nPortSlot,
                                         [[maybe_unused]] int nStatus) {
}

// 0x001844e0
void MemcardUser::OnRemixesListed([[maybe_unused]] int nPortSlot, [[maybe_unused]] int nStatus) {
}

// 0x001844e8
void MemcardUser::OnRemixLoaded([[maybe_unused]] int nPortSlot, [[maybe_unused]] int nStatus) {
}

// 0x001844f0
void MemcardUser::OnPersonasLoaded([[maybe_unused]] int nPortSlot, [[maybe_unused]] int nStatus) {
}

// 0x001844f8
void MemcardUser::OnGlobalSettingsLoaded([[maybe_unused]] int nPortSlot,
                                         [[maybe_unused]] int nStatus) {
}

// 0x00184500
void MemcardUser::OnJukeboxPlayListLoaded([[maybe_unused]] int nPortSlot,
                                          [[maybe_unused]] int nStatus) {
}

// 0x00184508
void MemcardUser::OnRemixDeleted([[maybe_unused]] int nPortSlot, [[maybe_unused]] int nStatus) {
}

// 0x00184510
void MemcardUser::OnUnknown17([[maybe_unused]] int nPortSlot, [[maybe_unused]] int nStatus) {
}

// 0x00184518
void MemcardUser::OnUnknown18([[maybe_unused]] int nPortSlot, [[maybe_unused]] int nStatus) {
}

// 0x00184520
void MemcardUser::OnFileLoaded([[maybe_unused]] int nStatus) {
}

// 0x00184528
void MemcardUser::OnFileSaved([[maybe_unused]] int nStatus) {
}
