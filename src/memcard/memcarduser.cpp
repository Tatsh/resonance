#include "memcard/memcarduser.h"

// NTSC-U/C: 0x00184448, PAL: 0x00189730
MemcardUser::~MemcardUser() {
}

// NTSC-U/C: 0x00184478, PAL: 0x00189760
void MemcardUser::OnConnectState([[maybe_unused]] MemcardConnectState state,
                                 [[maybe_unused]] int nStatus) {
}

// NTSC-U/C: 0x001844a0, PAL: 0x00189798
void MemcardUser::OnAllConnectStates() {
}

// NTSC-U/C: 0x001844a8, PAL: 0x001897a0
#ifdef VIDEO_STANDARD_PAL
void MemcardUser::OnMinimumSaveSpace([[maybe_unused]] int nPortSlot,
                                     [[maybe_unused]] int nSpace,
                                     [[maybe_unused]] int nSkipWarning,
                                     [[maybe_unused]] int nCampaign) {
}
#else
void MemcardUser::OnMinimumSaveSpace([[maybe_unused]] int nPortSlot, [[maybe_unused]] int nSpace) {
}
#endif

// NTSC-U/C: 0x001844b0, PAL: 0x001897a8
void MemcardUser::OnCardFormatted([[maybe_unused]] int nPortSlot, [[maybe_unused]] int nStatus) {
}

// NTSC-U/C: 0x001844b8, PAL: 0x001897b0
void MemcardUser::OnCardUnformatted([[maybe_unused]] int nPortSlot, [[maybe_unused]] int nStatus) {
}

// NTSC-U/C: 0x001844c0, PAL: 0x001897b8
#ifdef VIDEO_STANDARD_PAL
void MemcardUser::OnPersonasSaved([[maybe_unused]] int nPortSlot,
                                  [[maybe_unused]] int nStatus,
                                  [[maybe_unused]] int nKilobytes) {
}
#else
void MemcardUser::OnPersonasSaved([[maybe_unused]] int nPortSlot, [[maybe_unused]] int nStatus) {
}
#endif

// NTSC-U/C: 0x001844c8, PAL: 0x001897c0
#ifdef VIDEO_STANDARD_PAL
void MemcardUser::OnRemixSaved([[maybe_unused]] int nPortSlot,
                               [[maybe_unused]] int nStatus,
                               [[maybe_unused]] int nKilobytes) {
}
#else
void MemcardUser::OnRemixSaved([[maybe_unused]] int nPortSlot, [[maybe_unused]] int nStatus) {
}
#endif

// NTSC-U/C: 0x001844d0, PAL: 0x001897c8
#ifdef VIDEO_STANDARD_PAL
void MemcardUser::OnGlobalSettingsSaved([[maybe_unused]] int nPortSlot,
                                        [[maybe_unused]] int nStatus,
                                        [[maybe_unused]] int nKilobytes) {
}
#else
void MemcardUser::OnGlobalSettingsSaved([[maybe_unused]] int nPortSlot,
                                        [[maybe_unused]] int nStatus) {
}
#endif

// NTSC-U/C: 0x001844d8, PAL: 0x001897d0
#ifdef VIDEO_STANDARD_PAL
void MemcardUser::OnJukeboxPlayListSaved([[maybe_unused]] int nPortSlot,
                                         [[maybe_unused]] int nStatus,
                                         [[maybe_unused]] int nKilobytes) {
}
#else
void MemcardUser::OnJukeboxPlayListSaved([[maybe_unused]] int nPortSlot,
                                         [[maybe_unused]] int nStatus) {
}
#endif

// NTSC-U/C: 0x001844e0, PAL: 0x001897d8
void MemcardUser::OnRemixesListed([[maybe_unused]] int nPortSlot, [[maybe_unused]] int nStatus) {
}

// NTSC-U/C: 0x001844e8, PAL: 0x001897e0
void MemcardUser::OnRemixLoaded([[maybe_unused]] int nPortSlot, [[maybe_unused]] int nStatus) {
}

// NTSC-U/C: 0x001844f0, PAL: 0x001897e8
void MemcardUser::OnPersonasLoaded([[maybe_unused]] int nPortSlot, [[maybe_unused]] int nStatus) {
}

// NTSC-U/C: 0x001844f8, PAL: 0x001897f0
void MemcardUser::OnGlobalSettingsLoaded([[maybe_unused]] int nPortSlot,
                                         [[maybe_unused]] int nStatus) {
}

// NTSC-U/C: 0x00184500, PAL: 0x001897f8
void MemcardUser::OnJukeboxPlayListLoaded([[maybe_unused]] int nPortSlot,
                                          [[maybe_unused]] int nStatus) {
}

// NTSC-U/C: 0x00184508, PAL: 0x00189800
void MemcardUser::OnRemixDeleted([[maybe_unused]] int nPortSlot, [[maybe_unused]] int nStatus) {
}

// NTSC-U/C: 0x00184510, PAL: 0x00189808
void MemcardUser::UnusedFirstReport([[maybe_unused]] int nPortSlot, [[maybe_unused]] int nStatus) {
}

// NTSC-U/C: 0x00184518, PAL: 0x00189810
void MemcardUser::UnusedSecondReport([[maybe_unused]] int nPortSlot, [[maybe_unused]] int nStatus) {
}

// NTSC-U/C: 0x00184520, PAL: 0x00189818
void MemcardUser::OnFileLoaded([[maybe_unused]] int nStatus) {
}

// NTSC-U/C: 0x00184528, PAL: 0x00189820
void MemcardUser::OnFileSaved([[maybe_unused]] int nStatus) {
}
