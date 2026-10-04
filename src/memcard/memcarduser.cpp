#include "memcard/memcarduser.h"

MemcardUser::~MemcardUser() {
}

void MemcardUser::OnConnectState([[maybe_unused]] MemcardConnectState state,
                                 [[maybe_unused]] int nStatus) {
}

void MemcardUser::OnAllConnectStates() {
}

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

void MemcardUser::OnCardFormatted([[maybe_unused]] int nPortSlot, [[maybe_unused]] int nStatus) {
}

void MemcardUser::OnCardUnformatted([[maybe_unused]] int nPortSlot, [[maybe_unused]] int nStatus) {
}

#ifdef VIDEO_STANDARD_PAL
void MemcardUser::OnPersonasSaved([[maybe_unused]] int nPortSlot,
                                  [[maybe_unused]] int nStatus,
                                  [[maybe_unused]] int nKilobytes) {
}
#else
void MemcardUser::OnPersonasSaved([[maybe_unused]] int nPortSlot, [[maybe_unused]] int nStatus) {
}
#endif

#ifdef VIDEO_STANDARD_PAL
void MemcardUser::OnRemixSaved([[maybe_unused]] int nPortSlot,
                               [[maybe_unused]] int nStatus,
                               [[maybe_unused]] int nKilobytes) {
}
#else
void MemcardUser::OnRemixSaved([[maybe_unused]] int nPortSlot, [[maybe_unused]] int nStatus) {
}
#endif

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

void MemcardUser::OnRemixesListed([[maybe_unused]] int nPortSlot, [[maybe_unused]] int nStatus) {
}

void MemcardUser::OnRemixLoaded([[maybe_unused]] int nPortSlot, [[maybe_unused]] int nStatus) {
}

void MemcardUser::OnPersonasLoaded([[maybe_unused]] int nPortSlot, [[maybe_unused]] int nStatus) {
}

void MemcardUser::OnGlobalSettingsLoaded([[maybe_unused]] int nPortSlot,
                                         [[maybe_unused]] int nStatus) {
}

void MemcardUser::OnJukeboxPlayListLoaded([[maybe_unused]] int nPortSlot,
                                          [[maybe_unused]] int nStatus) {
}

void MemcardUser::OnRemixDeleted([[maybe_unused]] int nPortSlot, [[maybe_unused]] int nStatus) {
}

void MemcardUser::UnusedFirstReport([[maybe_unused]] int nPortSlot, [[maybe_unused]] int nStatus) {
}

void MemcardUser::UnusedSecondReport([[maybe_unused]] int nPortSlot, [[maybe_unused]] int nStatus) {
}

void MemcardUser::OnFileLoaded([[maybe_unused]] int nStatus) {
}

void MemcardUser::OnFileSaved([[maybe_unused]] int nStatus) {
}
