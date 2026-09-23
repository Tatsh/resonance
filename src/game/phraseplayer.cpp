#include "game/phraseplayer.h"

#include <algorithm>
#include <vector>

#include "game/player.h"
#include "game/playmap.h"
#include "game/riff.h"
#include "gs/multimuse.h"
#include "gs/museutil.h"
#include "gs/phrasemgr.h"
#include "msg/multimusemsg.h"

namespace {

// The bar mLastBar holds before the first bar is played.
constexpr int kNoBar = -1;

// PlayBar() plays a bar from before its start, with no time elapsed.
constexpr int kFromBarStart = -1;
constexpr int kNoTimeElapsed = 0;

// MultiMuse::Add() tries appending before searching.
constexpr int kCheckLast = 1;

// A computed position, clamped to the finite range as the inline Mid::MBT arithmetic does.
inline Mid::MBT MakePosition(int nTick) {
    return Mid::MBT(std::min(std::max(nTick, kMBTMinimum), kMBTMaximum));
}

// A position taken over from a caller's plain tick, without the finiteness check.
inline Mid::MBT RawPosition(int nTick) {
    Mid::MBT position;
    position.mTick = nTick;
    return position;
}

// Sends one sequence to the sinks, then gives up the player's reference to it once the message has
// released its own.
inline void SendAndRelease(PhrasePlayer *pPlayer, MultiMuse *pMuse) {
    {
        MultiMuseMsg msg(pMuse);
        pPlayer->Send(&msg);
    }
    if (pMuse != nullptr) {
        pMuse->Release();
    }
}

} // namespace

// 0x001c17d8
PhrasePlayer::PhrasePlayer(PhraseMgr *pPhraseMgr,
                           Quantizer *pQuantizer,
                           const TrackData *pTrackData)
    : mPhraseMgr(pPhraseMgr), mQuantizer(pQuantizer), mTrackData(pTrackData),
      mTrackKind(pTrackData->mKind), mJamEffects(nullptr), mLastBar(kNoBar) {
}

// 0x001c1860
void PhrasePlayer::PlayBar(int nBar) {
    if (mJamEffects != nullptr) {
        mJamEffects->ApplyStepMask(*mPhraseMgr->GetStepValue(nBar));
    }
    if (!(mLastBar < nBar)) {
        return;
    }

    Phrase *pPhrase = mPhraseMgr->GetPhraseAt(nBar);
    if (pPhrase == nullptr) {
        return;
    }
    switch (mTrackKind) {
    case kTrackModeAxe:
    case kTrackModeVocal:
        PlayPhraseMuse(pPhrase, nBar);
        break;
    case kTrackModeRiff:
    case kTrackModeScratch:
        PlayPhraseGems(pPhrase, nBar, Mid::MBT(kFromBarStart), Mid::MBT(kNoTimeElapsed));
        break;
    case kTrackModeCatch:
        PlayBarGems(pPhrase, nBar, Mid::MBT(kFromBarStart), Mid::MBT(kNoTimeElapsed));
        break;
    default:
        break;
    }
}

// 0x001c1978
void PhrasePlayer::PlayBarGems(Phrase *pPhrase, int nBar, Mid::MBT from, Mid::MBT elapsed) {
    if (pPhrase->mPlayer->IsNull()) {
        return;
    }

    MultiMuse *pMuse = new MultiMuse;
    const std::vector<TickObj<int> > *pGems = mTrackData->GetGems(nBar);
    const int nStep = mPhraseMgr->mMap->Slot5(nBar);
    for (std::vector<TickObj<int> >::const_iterator it = pGems->begin(); it != pGems->end(); ++it) {
        const int nTick = it->mPosition.mTick;
        if (nTick < from.mTick) {
            continue;
        }
        MultiMuseMsg msg(mTrackData->GetRiffInBar(nStep, nTick, it->mValue));
        pMuse->Add(&msg, MakePosition(nTick - elapsed.mTick).mTick, kCheckLast);
    }

    SendAndRelease(this, pMuse);
    mLastBar = nBar;
}

// 0x001c1ba8
void PhrasePlayer::PlayPhraseGems(Phrase *pPhrase, int nBar, Mid::MBT from, Mid::MBT elapsed) {
    MultiMuse *pMuse = new MultiMuse;
    for (std::vector<Phrase::Gem>::iterator it = pPhrase->mGems.begin(); it != pPhrase->mGems.end();
         ++it) {
        if (it->mPosition.mTick < from.mTick) {
            continue;
        }

        Riff *pRiff = mTrackData->GetRiffInMappedBar(nBar, it->mPosition.mTick, it->mGem);
        if (pRiff == nullptr) {
            if (pMuse != nullptr) {
                pMuse->Release();
            }
            return;
        }

        const Mid::MBT position = MakePosition(it->mPosition.mTick - elapsed.mTick);
        if (it->mTrans != 0) {
            MultiMuse *pTransposed = TransposeMuse(pRiff, it->mTrans);
            {
                MultiMuseMsg msg(pTransposed);
                pMuse->Add(&msg, position.mTick, kCheckLast);
            }
            if (pTransposed != nullptr) {
                pTransposed->Release();
            }
        } else {
            MultiMuseMsg msg(pRiff);
            pMuse->Add(&msg, position.mTick, kCheckLast);
        }
    }

    SendAndRelease(this, pMuse);
    mLastBar = nBar;
}

// 0x001c2810
void PhrasePlayer::SetJamEffectsMgr(JamEffectsMgr *pJamEffects) {
    mJamEffects = pJamEffects;
}

// 0x001c2818
void PhrasePlayer::PlayBarAt(int nBar, int nOffset, int nElapsed) {
    Phrase *pPhrase = mPhraseMgr->GetPhraseAt(nBar);
    if (mTrackKind == kTrackModeRiff) {
        PlayPhraseGems(pPhrase, nBar, RawPosition(nOffset), RawPosition(nElapsed));
    } else {
        PlayBarGems(pPhrase, nBar, RawPosition(nOffset), RawPosition(nElapsed));
    }
}

// 0x001c28a0
void PhrasePlayer::PlayPhraseMuse(Phrase *pPhrase, int nBar) {
    if (pPhrase->mMuse != nullptr) {
        MultiMuseMsg msg(pPhrase->mMuse);
        Send(&msg);
    }
    mLastBar = nBar;
}

// 0x001c2928
void PhrasePlayer::HandleMessage(Message *pMsg) {
    pMsg->Type(); // Yes, the binary discards this call's result.
}
